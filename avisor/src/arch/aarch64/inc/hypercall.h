#ifndef HYPERCALL_H
#define HYPERCALL_H

typedef enum {
    // Halt
    HYPERCALL_ISS_HALT = 0,
    // Snapshot
    HYPERCALL_ISS_CHECKPOINT_SNAPSHOT,
    HYPERCALL_ISS_RESTORE_SNAPSHOT,
    HYPERCALL_ISS_PRINT,
    HYPERCALL_ISS_RESTART,
    HYPERCALL_ISS_RESTORE_FROM_ID,
} HYPERCALL_TYPE;

typedef void (*hypercall_handler_t)(unsigned long, unsigned long, unsigned long, unsigned long);

void hypercall_handler(unsigned long iss, unsigned long arg0, unsigned long arg1, unsigned long arg2);

#endif