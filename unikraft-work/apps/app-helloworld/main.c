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


void print_message(const char *message) {
    unsigned long msg_addr = (unsigned long)message;

    asm volatile (
        "mov x0, %0\n"  // 将 hypercall_id 加载到 x0
        "hvc #3\n"      // 发起 Hypercall
        :
        :  "r" (msg_addr)
        : "x0"
    );
}

long long restore_from_id( unsigned long long id) {
    long long result;
    asm volatile (
        "mov x0, %1\n"  // 将 id 加载到 x0
        "hvc #5\n"      // 发起 Hypercall
        "mov %0, x0\n"  // 将 x0 寄存器的值移动到 result 变量
        : "=r" (result) // 输出操作数，告诉编译器将 x0 的值存储在 result 中
        : "r" (id)      // 输入操作数，将 id 的值加载到 x0
        : "x0"          // clobber list，表明 x0 寄存器被修改
    );
    return result;
}

long long checkpoint() {
    long long result;
    asm volatile (
        "hvc #1\n"      // 发起 Hypercall
        "mov %0, x0\n"  // 将 x0 寄存器的值移动到 result 变量
        : "=r" (result) // 输出操作数，告诉编译器将 x0 的值存储在 result 中
        :               // 无输入操作数
        : "x0"          // clobber list，表明 x0 寄存器被修改
    );
    return result;
}

long long restore() {
    long long result;
    asm volatile (
        "hvc #2\n"      // 发起 Hypercall
        "mov %0, x0\n"  // 将 x0 寄存器的值移动到 result 变量
        : "=r" (result) // 输出操作数，告诉编译器将 x0 的值存储在 result 中
        :               // 无输入操作数
        : "x0"          // clobber list，表明 x0 寄存器被修改
    );
    return result;
}

void restart() {
    asm volatile (
        "hvc #4\n"      // 发起 Hypercall
    );
}

unsigned int get_current_el(void) {
    unsigned int el;
    __asm__ volatile ("mrs %0, CurrentEL" : "=r" (el));
    return (el >> 2) & 3;
}

unsigned int read_sctlr_el1(void) {
    unsigned int value;
    asm volatile ("mrs %0, SCTLR_EL1" : "=r" (value));
    return value;
}

unsigned int read_ttbr0_el1(void) {
    unsigned int value;
    asm volatile ("mrs %0, ttbr0_el1" : "=r" (value));
    return value;
}

void hvc_sched_yield(){
    asm volatile(
        "hvc #6\n"
    );
}


void print_sp_address() {
    unsigned long sp;
    asm volatile ("mov %0, sp" : "=r" (sp));
    printf("Current SP address: 0x%lx\n", sp);
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
	int x = 0;
	printf("Hello,this is hello_world 1 !\n");
	printf("Hello world!\n");
	if( checkpoint() == -1 ){
        printf("This snapshot has not create\n");
        return 0;
    }//ssid = 0
	printf("x is :%d\n",x); // x = 0
	x = x + 1;
	if( checkpoint() == -1 ){
        printf("This snapshot has not create\n");
        return 0;
    }//ssid = 1
	printf("x is :%d\n",x); //x = 1
	x = x + 1;
	if( checkpoint() == -1 ){
        printf("This snapshot has not create\n");
        return 0;
    }//ssid = 2
	printf("x is :%d\n",x); // x = 2
    restart();

	
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
