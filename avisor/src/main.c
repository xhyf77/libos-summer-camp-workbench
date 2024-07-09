#include "util.h"
#include "vmm.h"
#include "vm.h"
#include "cpu.h"
#include "interrupts.h"

unsigned int get_current_el() {
    unsigned int el;
    // 使用内联汇编读取 CurrentEL 寄存器
    asm volatile("mrs %0, CurrentEL" : "=r" (el));
    return el >> 2; // 当前特权级别存储在位[3:2]
}

uint64_t read_sctlr_el1(void) {
    uint64_t value;
    asm volatile ("mrs %0, SCTLR_EL1" : "=r" (value));
    return value;
}

int main(cpuid_t id) {
     unsigned int el = get_current_el();
     INFO("Current Exception Level (EL): %u\n", el);
     cpu_init(id);

     el = get_current_el();
     if (id == CPU_MASTER) {
          INFO("------------avisor started------------");
          INFO("Exception level: %d", sysreg_CurrentEL_read() >> 2);
          mem_init();
     }
     
     cpu_sync_barrier(&cpu_glb_sync);

     interrupts_init();

     vmm_init();

     while(1);

     return 0;
}
