#include "hypercall.h"
#include "util.h"
#include "lcm.h"
#include "rq.h"

hypercall_handler_t hypercall_handlers[8] = {
                                                [HYPERCALL_ISS_HALT]                = guest_halt_hanlder,
                                                [HYPERCALL_ISS_CHECKPOINT_SNAPSHOT] = checkpoint_snapshot_hanlder,
                                                [HYPERCALL_ISS_RESTORE_SNAPSHOT] = restore_snapshot_hanlder,
                                                [HYPERCALL_ISS_PRINT] = print_handler,
                                                [HYPERCALL_ISS_RESTART] = restart_vm,
                                                [HYPERCALL_ISS_RESTORE_FROM_ID] = restore_snapshot_hanlder_by_ss,
                                                [HYPERCALL_ISS_SCHED_YIELD] = sched_yield,
                                        };

void hypercall_handler(unsigned long iss, unsigned long arg0, unsigned long arg1, unsigned long arg2) {
    INFO("hypercall: %lu, arg0: %lu, arg1: %lu, arg2: %lu", iss, arg0, arg1, arg2);
    unsigned long hypercall_number = iss;
    if (hypercall_number < sizeof(hypercall_handlers) / sizeof(hypercall_handlers[0])) {
        hypercall_handler_t handler = hypercall_handlers[hypercall_number];
        handler( iss , arg0, arg1, arg2 );
    } else {
        // 处理无效的 hypercall 编号
        INFO("Invalid hypercall number: %lu", hypercall_number);
    }
}