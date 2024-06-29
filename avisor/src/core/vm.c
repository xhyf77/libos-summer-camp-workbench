#include "interrupts.h"
#include "vm.h"
#include "config.h"
#include "sysregs.h"
#include "fences.h"
#include "mem.h"
#include "string.h"
#include "cpu.h"
#include "list.h"
#include "sched.h"
// #include "rq.h"

struct vm_list vm_list;

static void vm_init_mem_regions(struct vm* vm, const struct vm_config* vm_config) { 
    vaddr_t va;
    paddr_t pa;

    va = mem_alloc_map(&vm->as, NULL, vm_config->base_addr, NUM_PAGES(vm_config->size + vm_config->dmem_size), PTE_VM_FLAGS);
    if (va != vm_config->base_addr) {
        ERROR("va != vm's base_addr");
    }
    mem_translate(&vm->as, va, &pa);
    memcpy((void*)pa, (void*)vm_config->load_addr, vm_config->size);
    INFO("Copy vm%d to 0x%x, size = 0x%x", vm->id, pa, vm_config->size);
        
    va = mem_alloc_map(&vm->as, NULL,
                (vaddr_t)config.dtb.base_addr, NUM_PAGES(config.dtb.size), PTE_VM_FLAGS);
    if (va != config.dtb.base_addr) {
        ERROR("va != config->vm.base_addr");
    }
    mem_translate(&vm->as, va, &pa);
    memcpy((void*)pa, (void*)config.dtb.load_addr, config.dtb.size);
    INFO("Copy dtb to 0x%x, size = 0x%x", pa, config.dtb.size);
}

static struct vm* vm_allocation_init(struct vm_allocation* vm_alloc) {
    struct vm *vm = vm_alloc->vm;
    vm->vcpus = vm_alloc->vcpus;
    return vm;
}

void vm_cpu_init(struct vm* vm) {
    spin_lock(&vm->lock);
    vm->cpus |= (1UL << cpu()->id);
    spin_unlock(&vm->lock);
}

static vcpuid_t vm_calc_vcpu_id(struct vm* vm) {
    vcpuid_t vcpu_id = 0;
    for(size_t i = 0; i < cpu()->id; i++) {
        if (!!bit_get(vm->cpus, i)) vcpu_id++;
    }
    return vcpu_id;
}

void vm_vcpu_init(struct vm* vm, const struct vm_config* vm_config) {
    vcpuid_t vcpu_id = vm_calc_vcpu_id(vm);
    struct vcpu* vcpu = vm_get_vcpu(vm, vcpu_id);

    vcpu->id = vcpu_id;
    vcpu->p_id = cpu()->id;
    vcpu->vm = vm;
    cpu()->vcpu = vcpu;

    vcpu_arch_init(vcpu, vm);
    vcpu_arch_reset(vcpu, vm_config->entry);
}

static void vm_master_init(struct vm* vm, const struct vm_config* vm_config, vmid_t vm_id) {
    vm->master = cpu()->id;
    vm->nr_cpus = vm_config->nr_cpus;
    vm->id = vm_id;
    vm->vm_config = vm_config;

    cpu_sync_init(&vm->sync, vm->nr_cpus);

    as_init(&vm->as, AS_VM, vm_id, NULL);

    INIT_LIST_HEAD(&vm->emul_mem_list);
    INIT_LIST_HEAD(&vm->emul_reg_list);
}

static void vm_init_dev(struct vm* vm, const struct vm_config* config) {
    for (size_t i = 0; i < config->nr_devs; i++) {
        struct vm_dev_region* dev = &config->devs[i];

        size_t n = ALIGN(dev->size, PAGE_SIZE) / PAGE_SIZE;

        if (dev->va != INVALID_VA) {
            mem_alloc_map_dev(&vm->as, (vaddr_t)dev->va, dev->pa, n);
        }

        for (size_t j = 0; j < dev->interrupt_num; j++) {
            interrupts_vm_assign(vm, dev->interrupts[j]);
        }
    }

    if (io_vm_init(vm, config)) {
        for (size_t i = 0; i < config->nr_devs; i++) {
            struct vm_dev_region* dev = &config->devs[i];
            if (dev->id) {
                if (!io_vm_add_device(vm, dev->id)){
                    ERROR("Failed to add device to iommu");
                }
            }
        }
    }
      
}

struct vm* vm_init(struct vm_allocation* vm_alloc, const struct vm_config* vm_config, bool master, vmid_t vm_id) {
    struct vm *vm = vm_allocation_init(vm_alloc);
    
    if (master) {
        vm_master_init(vm, vm_config, vm_id);
    }

    vm_cpu_init(vm);

    cpu_sync_barrier(&vm->sync);

    vm_vcpu_init(vm, vm_config);
    
    cpu_sync_barrier(&vm->sync);

    vm_arch_init(vm, vm_config);

    if (master) {
        vm_init_mem_regions(vm, vm_config);
        vm_init_dev(vm, vm_config);
        // init address space first
        vm_rq_init(vm, vm_config);
    }

#ifdef SCHEDULE
    task_struct_init(vm);
#endif

    cpu_sync_barrier(&vm->sync);

    INIT_LIST_HEAD(&vm->list);
    spin_lock(&vm_list.lock);
    list_add(&vm->list, &vm_list.list);
    spin_unlock(&vm_list.lock);

    return vm;
}

void vcpu_run(struct vcpu* vcpu) {
    vcpu_arch_run(vcpu);
}

void vm_msg_broadcast(struct vm* vm, struct cpu_msg* msg) {
    for (size_t i = 0, n = 0; n < vm->nr_cpus - 1; i++) {
        if (((1U << i) & vm->cpus) && (i != cpu()->id)) {
            n++;
            cpu_send_msg(i, msg);
        }
    }
}

__attribute__((weak)) cpumap_t vm_translate_to_pcpu_mask(struct vm* vm,
                                                         cpumap_t mask,
                                                         size_t len) {
    cpumap_t pmask = 0;
    cpuid_t shift;
    for (size_t i = 0; i < len; i++) {
        if ((mask & (1ULL << i)) &&
            ((shift = vm_translate_to_pcpuid(vm, i)) != INVALID_CPUID)) {
            pmask |= (1ULL << shift);
        }
    }
    return pmask;
}

__attribute__((weak)) cpumap_t vm_translate_to_vcpu_mask(struct vm* vm,
                                                         cpumap_t mask,
                                                         size_t len) {
    cpumap_t pmask = 0;
    vcpuid_t shift;
    for (size_t i = 0; i < len; i++) {
        if ((mask & (1ULL << i)) &&
            ((shift = vm_translate_to_vcpuid(vm, i)) != INVALID_CPUID)) {
            pmask |= (1ULL << shift);
        }
    }
    return pmask;
}

void vm_emul_add_mem(struct vm* vm, struct emul_mem* emu) {
    list_add_tail(&emu->list, &vm->emul_mem_list);
}

void vm_emul_add_reg(struct vm* vm, struct emul_reg* emu) {
    list_add_tail(&emu->list, &vm->emul_reg_list);
}    

emul_handler_t vm_emul_get_mem(struct vm* vm, vaddr_t addr) {
    struct emul_mem* emu = NULL;
    emul_handler_t handler = NULL;

    list_for_each_entry(emu, &vm->emul_mem_list, list) {
        if (addr >= emu->va_base && (addr < (emu->va_base + emu->size))) {
            handler = emu->handler;
            break;
        }
    }
    return handler;
}

emul_handler_t vm_emul_get_reg(struct vm* vm, vaddr_t addr) {
    struct emul_reg* emu = NULL;
    emul_handler_t handler = NULL;

    // list_foreach(vm->emul_reg_list, struct emul_reg, emu) {
    //     if(emu->addr == addr) {
    //         handler = emu->handler;
    //         break; 
    //     }
    // }
    list_for_each_entry(emu, &vm->emul_reg_list, list) {
        if (emu->addr == addr) {
            handler = emu->handler;
            break;
        }
    }

    return handler;
}

struct vm* get_vm_by_id(vmid_t id) {
    struct vm* res_vm = NULL;

    list_for_each_entry(res_vm, &vm_list.list, list) {
        if (res_vm->id == id) {
            return res_vm;
        }
    }
    return NULL;
}