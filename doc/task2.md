# 任务2：Hypercall
Hypercall是虚拟机与Hypervisor通信的一种手段，有点类似于syscall。系统调用是用户态程序发起，由内核处理的一种机制，而Hypercall是虚拟机发起，由Hypervisor处理的机制。

在本次任务中，你需要做到
1. 理解hypercall在avisor上怎么被处理的，以及hypercall是怎么发起的。
2. 补充完`arch/hypercall.c`和`arch/hypercall.h`相应的内容
3. 自行实现一个Hypercall handler，可以输出任意消息.

下面是你可能需要使用到的代码
```C
__asm__ __volatile__("hvc #0x00");
```

以及阅读相应的手册, 你可以尝试寻找armv8手册中和异常处理有关的寄存器

## 必答题
+ hypercall是怎么发起和被处理的?