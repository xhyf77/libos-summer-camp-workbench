#include <stdio.h>
#include <string.h>
/* Import user configuration: */
#ifdef __Unikraft__
#include <uk/config.h>
#endif /* __Unikraft__ */

#if CONFIG_APPHELLOWORLD_SPINNER
#include <time.h>
#include <errno.h>
#include "monkey.h"

static void millisleep(unsigned int millisec)
{
	struct timespec ts;
	int ret;

	ts.tv_sec = millisec / 1000;
	ts.tv_nsec = (millisec % 1000) * 1000000;
	do
		ret = nanosleep(&ts, &ts);
	while (ret && errno == EINTR);
}
#endif /* CONFIG_APPHELLOWORLD_SPINNER */


void print_sp_address() {
    unsigned long sp;
    asm volatile ("mov %0, sp" : "=r" (sp));
    printf("Current SP address: 0x%lx\n", sp);
}

void perform_hypercall(const char *message) {
    unsigned long msg_addr = (unsigned long)message;

    asm volatile (
        "mov x0, %0\n"  // 将 hypercall_id 加载到 x0
        "hvc #3\n"      // 发起 Hypercall
        :
        :  "r" (msg_addr)
        : "x0"
    );
}

void restore_from_id( unsigned long long id) {
    asm volatile (
        "mov x0, %0\n"  // 将 hypercall_id 加载到 x0
        "hvc #5\n"      // 发起 Hypercall
        :
        :  "r" (id)
        : "x0"
    );
}

void checkpoint() {
    asm volatile (
        "hvc #1\n"      // 发起 Hypercall
    );
}

void restore() {
    asm volatile (
        "hvc #2\n"      // 发起 Hypercall
    );
}

void restart() {
    asm volatile (
        "hvc #4\n"      // 发起 Hypercall
    );
}


void hvc_sched_yield(){
    asm volatile(
        "hvc #6\n"
    );
}

#define SHARED_MEM_VIRT_ADDR 0x50000000
#define SHARED_MEM_SIZE (4096 * 10)
#define MESSAGE_SIZE 256
#define MAX_VMS (SHARED_MEM_SIZE / sizeof(vm_shared_mem_t))
#define FLAG_EMPTY 0
#define FLAG_FULL 1

typedef struct {
    volatile int locked;
} spinlock_t;

void spinlock_init(spinlock_t *lock) {
    lock->locked = 0;
}

void spinlock_lock(spinlock_t *lock) {
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
        while (lock->locked) {
            hvc_sched_yield();
        }
    }
}

void spinlock_unlock(spinlock_t *lock) {
    __sync_lock_release(&lock->locked);
}

typedef struct {
    spinlock_t lock;
    volatile unsigned char flag;
    char data[MESSAGE_SIZE];
} vm_shared_mem_t;

typedef struct {
    vm_shared_mem_t vms[MAX_VMS];
} shared_mem_t;

volatile shared_mem_t *shared_memory = (volatile shared_mem_t *)SHARED_MEM_VIRT_ADDR;


void init_shared_memory() {
    for (int i = 0; i < MAX_VMS; i++) {
        shared_memory->vms[i].flag = FLAG_EMPTY;
        spinlock_init(&shared_memory->vms[i].lock);
    }
}

void send_message_to(const char *data, int vm_id) {
    if (vm_id < 0 || vm_id >= MAX_VMS) {
        printf("Invalid VM ID\n");
        return;
    }

    vm_shared_mem_t *vm_mem = &shared_memory->vms[vm_id];
    spinlock_lock(&vm_mem->lock);
    while (vm_mem->flag != FLAG_EMPTY) {
        spinlock_unlock(&vm_mem->lock);
        hvc_sched_yield();
        spinlock_lock(&vm_mem->lock);
    }

    strncpy((char *)(vm_mem->data), data, MESSAGE_SIZE - 1);
    vm_mem->data[MESSAGE_SIZE - 1] = '\0';
    vm_mem->flag = FLAG_FULL;
    spinlock_unlock(&vm_mem->lock);
}

void get_message(char *data, int vm_id) {
    if (vm_id < 0 || vm_id >= MAX_VMS) {
        printf("Invalid VM ID\n");
        return;
    }
    vm_shared_mem_t *vm_mem = &shared_memory->vms[vm_id];
    spinlock_lock(&vm_mem->lock);
    while (vm_mem->flag != FLAG_FULL) {
        spinlock_unlock(&vm_mem->lock);
        hvc_sched_yield();
        spinlock_lock(&vm_mem->lock);
    }
    strncpy(data, (char *)(vm_mem->data), MESSAGE_SIZE);
    vm_mem->flag = FLAG_EMPTY;
    spinlock_unlock(&vm_mem->lock);
}


int main(int argc, char *argv[])
{
#if CONFIG_APPHELLOWORLD_PRINTARGS || CONFIG_APPHELLOWORLD_SPINNER
	int i;
#endif
	printf("Hello,this is hello_world 2 !\n");
    char buffer[SHARED_MEM_SIZE];
	for(int i = 1 ; i <= 100000000 ; i ++ ) {
        printf("VM_2: %d/%d\n", i , 100000000);
        //get_message(buffer, 1 );
        //printf("Get the message: %s\n", buffer);
    }

#if CONFIG_APPHELLOWORLD_SPINNER
	i = 0;
	printf("\n\n\n");
	for (;;) {
		i %= (monkey3_frame_count * 3);
		printf("\r\033[2A %s \n", monkey3[i++]);
		printf(" %s \n",          monkey3[i++]);
		printf(" %s ",            monkey3[i++]);
		fflush(stdout);
		millisleep(250);
	}
#endif /* CONFIG_APPHELLOWORLD_SPINNER */

	return 0;
}
