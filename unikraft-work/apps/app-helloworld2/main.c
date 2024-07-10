#include <stdio.h>

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

const unsigned long shared_memory = 0x50000000;

int main(int argc, char *argv[])
{
#if CONFIG_APPHELLOWORLD_PRINTARGS || CONFIG_APPHELLOWORLD_SPINNER
	int i;
#endif
	printf("Hello,this is hello_world 2 !\n");
	strcpy((char *)shared_memory, "This is a message i write to the shared_memory\n");
	printf("I have write something to the share memory !\n");
	for(int i = 1 ; i <= 10000000000 ; i ++ ) {
		int x = 1;
		x = x + 1;
	}//not to exit

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
