#include "util.h"
#include "sched.h"
#include "list.h"
#include "bitmap.h"
#include "spinlock.h"
#include "cpu.h"
#include "vm.h"
#include "sysregs.h"

struct __task_struct {
    int status;
    int level;
    volatile int need_resched;
    int counter;

    struct task_struct* task;
    struct vcpu* vcpu;
    struct fifo fifo;
}__tasks[MAX_VCPU_NUM];

struct task_struct {
    int status;
    int nr_vcpu_ready;
    volatile int need_resched;
    int counter;
    
    bitmap_t bitmap;
    struct fifo rq[BMQ_LEVELS];

    struct vm* vm;
    struct list_head list;
}tasks[MAX_VM_NUM];

struct list_head runqueue = LIST_HEAD_INIT(runqueue);
spinlock_t rq_lock = SPINLOCK_INITVAL;

//TODO: set and get current in stack top, not use this shit
static inline struct __task_struct* __current() {
    return &__tasks[cpu()->vcpu->id];
}

static inline struct task_struct* current() {
    return &tasks[cpu()->vcpu->vm->id];
}

static inline int goodness(struct task_struct* p) {
    //TODO: real goodness in bfs
    return p->counter + 20;
}

static inline void prepare_to_switch(struct task_struct* next) {
    struct vm* vm = next->vm;
    sysreg_vttbr_el2_write((((uint64_t)vm->id << VTTBR_VMID_OFF) & VTTBR_VMID_MSK) |
                       ((paddr_t)vm->as.pt.root & ~VTTBR_VMID_MSK));
    ISB();
}

static inline void switch_to(struct __task_struct* __next) {
    cpu()->vcpu = __next->vcpu;
}

static struct __task_struct* __task_struct_init(struct task_struct* task, int vcpu_id) {
    struct vm* vm = task->vm;
    unsigned long id = vm->vcpus[vcpu_id].id;
    struct __task_struct *__p = &__tasks[id];

    __p->status = TASK_READY;
    __p->level = DEFAULT_LEVEL;
    __p->need_resched = 0;
    __p->counter = DEFAULT_COUNTER;

    INIT_FIFO(&__p->fifo);

    __p->vcpu = &vm->vcpus[vcpu_id];
    __p->task = task;

    return __p;
}

void task_struct_init(struct vm* vm) {
    unsigned long id = vm->id;
    struct task_struct *p = &tasks[id];
    struct __task_struct *__p;
    int i;

    p->nr_vcpu_ready = vm->nr_cpus;
    p->need_resched = 0;
    p->counter = DEFAULT_COUNTER;
    p->vm = vm;

    for (i = 0; i < BMQ_LEVELS; i++) {
        INIT_FIFO(&p->rq[DEFAULT_LEVEL]);
    }

    for (i = 0; i < vm->nr_cpus; i++) {
        __p = __task_struct_init(p, i);
        fifo_in(&__p->fifo, &p->rq[DEFAULT_LEVEL]);
    }
    bitmap_set(&p->bitmap, DEFAULT_LEVEL);
    
    list_add_tail(&p->list, &runqueue);
}

void try_reschedule() {
#ifdef SCHEDULE
    if (__current()->need_resched || current()->need_resched) {
        INFO("SCHEDULE");
        schedule();
    }
#endif
}

void update_task_times() {
    struct task_struct *p = current();
    struct __task_struct *__p = __current();

    INFO("update task times");
    INFO("vpcu[%d]: %d", __p->vcpu->id, __p->counter);
    INFO("vm[%d]: %d", p->vm->id, p->counter);

    if (--__p->counter <= 0) {
        __p->counter = 0;
        __p->need_resched = 1;
        INFO("vpcu[%d] need resched", __p->vcpu->id);
    }

    if (--p->counter <= 0) {
        p->counter = 0;
        p->need_resched = 1;
        INFO("vm[%d] need resched", p->vm->id);
    }

    INFO("vpcu[%d]: %d", __p->vcpu->id, __p->counter);
    INFO("vm[%d]: %d", p->vm->id, p->counter);
}

void schedule() {
    struct task_struct *prev, *next = NULL, *temp;
    struct __task_struct *__prev, *__next;
    int weight, max_weight = 20;
    int repeat = 0;
    int i;

    __prev = __current();
    prev = current();

//TODO: two locks instead of one big lock
    spin_lock(&rq_lock);

    prev->status = TASK_READY;
    __prev->status = TASK_READY;
    prev->nr_vcpu_ready++;

    /* select vm */
select_vm:
    list_for_each_entry(temp, &runqueue, list) {
        if (temp->status == TASK_READY && temp->nr_vcpu_ready > 0) {
            weight = goodness(temp);
            if (weight > max_weight) {
                max_weight = weight;
                next = temp;
            }
        }
    }
    
    if (next == NULL) {
        if (!repeat) {
            list_for_each_entry(temp, &runqueue, list) {
                temp->counter = DEFAULT_COUNTER;
            }
            repeat = 1;
            goto select_vm;
        } else {
            //TODO: run idle task
            cpu_idle();
            spin_unlock(&rq_lock);
            return;
        }
    }

    /* select vcpu */
    i = bit32_ffs(next->bitmap);
    while(1) {
        ASSERT(i != -1);
        __next = fifo_entry(fifo_out(&next->rq[i]), struct __task_struct, fifo);
        if (__next->status == TASK_READY) {
            fifo_in(&__next->fifo, &next->rq[i]);
            break;
        }
        if (fifo_empty(&next->rq[i])) {
            bitmap_clear(&next->bitmap, i);
            i = bit32_ffs(next->bitmap);
        }
    }

    __next->status = TASK_RUNNING;
    next->nr_vcpu_ready--;
    __prev->need_resched = 0;
    prev->need_resched = 0;

    spin_unlock(&rq_lock);

    prepare_to_switch(next);

    switch_to(__next);
}