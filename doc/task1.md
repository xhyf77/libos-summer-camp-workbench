# 任务1：熟悉Unikraft和Avisor
## 任务说明
1. 在Avisor上执行构建的Unikraft-HelloWorld镜像，打印输出`Hello, LibOS!`
## Unikraft

请参考下列步骤学习Unikraft，学习到uk_test前就好，x86和arm的版本都可以尝试一遍。

[Baby Steps - Unikraft Internals - Unikraft](https://unikraft.org/guides/internals#enabling-tests)

现在Unikraft社区已经更新到了0.17.0版本，全部采用kraftkit工具替换掉make和Kconfig工具。

我们在这还是采用make + Kconfig的方式去构建LibOS。当然也可以跟着目前Unikraft主线给的demo去学，但是在做本项目时，请采用`0.12.0`版本。

学习Unikraft时，你可以重新创建一个目录学习，也可以在本项目中的目录学习。

### 需要注意的事项
1. 上述博客中可能讲的不是很详细，你可能要参考`doc/Demos_00.-Manual-kraft-Installation.md`和`doc/Demos_01.-Building-and-Running-the-Helloworld-Application.md`, 这两个文件是unikraft远古版本的资料，现在已经淘汰了，可能图片加载不太正确，作为上述博客的补充材料。
2. 你可能需要安装交叉编译链以及qemu-system-aarch64
3. 在构建platform为avisor时，请选择unikraft的platform为kvm, 一般来说能在qemu-system-aarch64上跑起来就没什么问题。

## Avisor
Avisor的代码量过大，对于和本次任务无关的部分，可以不做阅读，例如物理内存管理。

在阅读Avisor相应的源码之前，你可能要了解ARMv8异常等级相关知识以及相关虚拟化的知识，这部分需要你自行查阅资料。可以尝试阅读avisor的Makefile。

下面的提示可能对你有所帮助：
1. 作为一个1型Hypervisor, 在缺少文件系统的情况下，Avisor是如何读取镜像文件的？
2. 在其他配置不更改的情况下，想要正确执行一个程序，第一步是什么？换句话说，第一个要知道的信息是什么？这一步在Avisor中是怎么体现的？
3. 在构建unikraft for avisor的时候，unikraft内核的消息可以不用开启。

## 必答题
+ 为了完成这个任务,需要做哪些事情?
+ bin与elf的区别是什么?