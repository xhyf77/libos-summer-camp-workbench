# Intro
本文档记录以下可能会有帮助的内容

## 项目目的

本次项目希望通过在现有框架的基础上，实现一个简单的快照功能。最终能够做到快照的checkpoint与restore。

也希望各位同学能够通过完成本项目，深刻理解到现代云计算架构下，操作系统是怎么与硬件、Hypervisor协同工作的。

本项目是第一次以题目的形式出现，可能在代码、文档、设计上存在疏漏，欢迎各位同学向我们指出。

## 项目介绍
本次任务会使用到下列两个项目
1. Unikraft
2. Avisor

其中Unikraft是一个开源的LibOS构建框架, LibOS是一种微库化的操作系统，也是智能操作系统课题组的主要研究方向。

Avisor是智能操作系统课题组研发的基于ARMv8架构，用于虚拟化教学用的嵌入式虚拟化平台。对于Avisor更多的介绍请阅读`avisor/README.md`

## 目录结构
目录结构如下
```
.
├── avisor
│   ├── image
│   ├── Makefile
│   ├── README.md
│   └── src
├── doc
├── README.md
└── unikraft-work
    ├── apps
    ├── libs
    └── unikraft
```
具体的说明如下

+ `avisor/image`： 存放相关uk-image和设备树文件
+ `avisor/src`: avisor-kernel
+ `unikraft-work/apps`: 存放unikraft app工程
+ `unikraft-work/libs`: 存放unikraft第三方微库
+ `unikraft/unikraft`: unikraft kernel
+ `doc`: 存放相关的文档

在Unikraft/apps下已经存在了一个HelloWorld案例`unikraft-work/apps/app-helloworld`。
在doc中除了intro.md、task*.md外，其他可能是你们需要使用到的材料。包括
+ Unikraft-Demo

你可能还需要参考ARMv8-A编程手册, 你可以在arm官网找到.

Unikraft论文: [EuroSys'21 Unikraft](https://dl.acm.org/doi/10.1145/3447786.3456248)


## 必答题
当出现了必答题时，需要在你的报告中写出答案，写出自己思考的过程和答案即可，并没有标准答案。