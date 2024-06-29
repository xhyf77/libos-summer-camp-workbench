#include "hypercall.h"
#include "util.h"
#include "lcm.h"
#include "rq.h"

hypercall_handler_t hypercall_handlers[8] = {
                                                [HYPERCALL_ISS_HALT]                = guest_halt_hanlder,
                                                
                                        };

void hypercall_handler(unsigned long iss, unsigned long arg0, unsigned long arg1, unsigned long arg2) {
    // TODO: 根据hypercall id调用对应函数
}