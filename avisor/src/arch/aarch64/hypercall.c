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
    switch (iss) {
        case HYPERCALL_ISS_HALT:
            INFO("Hypercall: HALT");
            guest_halt_hanlder( iss , arg0 , arg1 , arg2 );
            break;
        case HYPERCALL_ISS_CHECKPOINT_SNAPSHOT:
            INFO("Hypercall: CHECKPOINT_SNAPSHOT");
            checkpoint_snapshot_hanlder( iss , arg0 , arg1 , arg2 );
            break;
        case HYPERCALL_ISS_RESTORE_SNAPSHOT:
            INFO("Hypercall: RESTORE_SNAPSHOT");
            restore_snapshot_hanlder( iss , arg0 , arg1 , arg2 );
            break;
        case HYPERCALL_ISS_RESTART:
            INFO("Hypercall: RESTART");
            restart_vm();
            break;
        case HYPERCALL_ISS_PRINT:
            INFO("Hypercall: MY_PRINT");
            print_handler( iss , arg0 );
            break;
        case HYPERCALL_ISS_RESTORE_FROM_ID:
            INFO("Hypercall: RESTORE_TO:%d" , arg0 );
            restore_snapshot_hanlder_by_ss( arg0 );
            break;
        case HYPERCALL_ISS_SCHED_YIELD:
            INFO("Hypercall: SCHED_YIELD");
            sched_yield();
            break;
        default:
            INFO("Unknown hypercall: %lu, arg0: %lu, arg1: %lu, arg2: %lu", iss, arg0, arg1, arg2);
            break;
    }
}