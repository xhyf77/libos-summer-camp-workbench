#ifndef SCHED_H
#define SCHED_H

#include "vm.h"

#define DEFAULT_COUNTER 1
#define BMQ_LEVELS 8
#define DEFAULT_LEVEL 4

#define TASK_READY       0
#define TASK_RUNNING     1
#define TASK_PENDING     2

void task_struct_init(struct vm* vm);
void try_reschedule();
void update_task_times();
void schedule();
void first_run_task();

#endif