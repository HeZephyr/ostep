# Note/Translation
到目前为止，我们已经看到了操作系统执行的基本抽象的发展。我们已经了解了如何将单个物理 CPU 转变为多个虚拟 CPU，从而实现多个程序同时运行的错觉。我们还了解了如何为每个进程创建一个大的、私有的虚拟内存；当操作系统确实在物理内存（有时是磁盘）上秘密复用地址空间时，地址空间的这种抽象使每个程序的行为就好像它拥有自己的内存一样。

在本文中，我们为单个正在运行的进程引入了一种新的抽象：线程。我们通常认为程序中只有一个执行点（即从一台 PC 获取指令并执行），而多线程程序则有多个执行点（即多台 PC，每台 PC 获取指令并执行）。也许我们可以换个角度来理解，每个线程都很像一个独立的进程，<font color="red">但有一点不同：它们共享相同的地址空间，因此可以访问相同的数据</font>。

因此，单个线程的状态与进程的状态非常相似。它有一个程序计数器 (PC)，用于跟踪程序从何处获取指令。每个线程都有自己的一组用于计算的私有寄存器；因此，如果有两个线程在单个处理器上运行，则当从运行一个线程 (T1) 切换到运行另一个线程 (T2) 时，必须进行上下文切换。线程之间的上下文切换与进程之间的上下文切换非常相似，因为在运行T2之前必须保存T1的寄存器状态并恢复T2的寄存器状态。对于进程，我们将状态保存到进程控制块（PCB）；现在，我们需要一个或多个线程控制块（TCB）来存储进程的每个线程的状态。不过，与进程相比，我们在线程之间执行的上下文切换有一个主要区别：<font color="red">地址空间保持不变（即无需切换我们正在使用的页表）</font>。

线程与进程的另一个主要区别与栈有关。在我们对传统进程（我们现在可以称之为单线程进程）地址空间的简单模型中，只有一个栈，通常位于地址空间的底部，如下图所示（左）。

![image-20240406102345691](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/The-Stack-Of-The-Relevant-Thread.png)

但是，在多线程进程中，每个线程都是独立运行的，当然也会调用各种例程来完成自己的工作。地址空间中的堆栈不是单一的，而是每个线程都有一个。假设我们有一个多线程进程，其中有两个线程，那么产生的地址空间看起来就不一样了（上图右）。

从图中可以看到，在整个进程的地址空间中分布着两个栈。因此，任何栈分配的变量、参数、返回值以及其他我们放在栈上的东西都将被放在有时被称为**线程本地存储**的地方，即相关线程的栈中。

你可能还会注意到，这破坏了我们美丽的地址空间布局。以前，栈和堆可以独立增长，只有当地址空间空间不足时才会出现问题。在这里，我们不再有这样好的情况了。幸运的是，这通常没有问题，因为栈一般不需要很大（大量使用递归的程序除外）。

## 1 为什么要使用线程

在了解线程的细节和编写多线程程序时可能遇到的一些问题之前，让我们先回答一个更简单的问题。为什么要使用线程？

事实证明，使用线程至少有两个主要原因。第一个原因很简单：<font color="red">并行性</font>。想象一下，你正在编写一个对大型数组执行操作的程序，例如，将两个大型数组相加，或将数组中每个元素的值递增一定量。如果只在单个处理器上运行，任务就很简单：只需执行每个操作即可完成。但是，如果在多处理器系统上执行程序，则可以通过使用处理器分别执行部分操作来大大加快这一过程。将标准的单线程程序转换为能在多个 CPU 上完成此类工作的程序的任务称为并行化，而在每个 CPU 上使用一个线程来完成这项工作是使程序在现代硬件上运行得更快的一种自然而典型的方法。

第二个原因比较微妙：<font color="red">避免因 I/O 速度慢而导致程序进程受阻</font>。想象一下，你正在编写一个执行不同类型 I/O 的程序：等待发送或接收消息，等待显式磁盘 I/O 完成，甚至（隐式）等待页面故障完成。与其等待，你的程序可能希望做其他事情，包括利用 CPU 进行计算，甚至发出更多 I/O 请求。使用线程是避免卡死的一种自然方法；当程序中的一个线程在等待（即等待 I/O 时被阻塞）时，CPU 调度器可以切换到其他线程，这些线程已经准备好运行并做一些有用的事情。线程可以使 I/O 与单个程序中的其他活动**重叠**，就像跨程序进程的多编程一样；因此，许多基于服务器的现代应用程序（网络服务器、数据库管理系统等）在其实现中都使用了线程。

当然，在上述任何一种情况下，你都可以使用多进程来代替线程。不过，线程共享地址空间，因此很容易共享数据，因此是构建这类程序时的自然选择。而对于逻辑上独立的任务，几乎不需要共享内存中的数据结构，进程则是更合理的选择。

## 2 示例：线程创建

让我们来了解一些细节。假设我们想运行一个程序，创建两个线程，每个线程都做一些独立的工作，在本例中就是打印 "A "或 "B"，代码如下。

```c
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "common.h"
#include "common_threads.h"

void *mythread(void *arg) {
    printf("%s\n", (char *) arg);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p1, p2;
    printf("main: begin\n");
    Pthread_create(&p1, NULL, mythread, "A");
    Pthread_create(&p2, NULL, mythread, "B");
    Pthread_join(p1, NULL);
    Pthread_join(p2, NULL);
    printf("main: end\n");
    return 0;
}
```

主程序创建了两个线程，每个线程都将运行函数 `mythread()`，但使用不同的参数（字符串 A 或 B）。线程创建后，可能会立即开始运行（取决于调度程序的奇思妙想）；也可能处于 "就绪 "状态，但不是 "运行 "状态，因此尚未运行。当然，在多处理器上，线程甚至可以同时运行，但我们先不要担心这种可能性。

创建两个线程（我们暂且称其为 T1 和 T2）后，主线程调用 `pthread_join()`，等待特定线程完成。它会这样做两次，从而确保 T1 和 T2 运行并完成，最后才允许主线程再次运行；当主线程运行时，它会打印 `"main: end"`并退出。总的来说，这次运行共使用了三个线程：<font color="red">主线程、T1 和 T2</font>。

让我们来看看这个小程序可能的执行顺序。

![image-20240406105430536](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Possible-Outcomes-Example.png)

在执行图中，时间向下递增，每一列表示不同线程（主线程、线程 1 或线程 2）运行的时间。

正如你所看到的，一种理解线程创建的方法是，它有点像函数调用；不过，系统并不是首先执行函数，然后返回给调用者，而是为被调用的例程创建一个新的执行线程，它独立于调用者运行，可能在从创建返回之前运行，但也可能更晚。下一步运行什么由操作系统调度程序决定，虽然调度程序很可能实现了某种合理的算法，但很难知道在任何给定时间内会运行什么。

## 3 为什么情况会变得更糟：共享数据

我们上面展示的简单线程示例很有用，它展示了线程是如何创建的，以及它们是如何根据调度程序的决定以不同顺序运行的。但它没有向你展示线程在访问共享数据时是如何交互的。

让我们想象一个简单的例子：两个线程希望更新一个全局共享变量，代码如下。

```c
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "common.h"
#include "common_threads.h"

int max;
volatile int counter = 0; // shared global variable

void *mythread(void *arg) {
    char *letter = arg;
    int i; // stack (private per thread) 
    printf("%s: begin [addr of i: %p]\n", letter, &i);
    for (i = 0; i < max; i++) {
		counter = counter + 1; // shared: only one
    }
    printf("%s: done\n", letter);
    return NULL;
}
                                                                             
int main(int argc, char *argv[]) {                    
    if (argc != 2) {
		fprintf(stderr, "usage: main-first <loopcount>\n");
		exit(1);
    }
    max = atoi(argv[1]);

    pthread_t p1, p2;
    printf("main: begin [counter = %d] [%x]\n", counter, 
	   (unsigned int) &counter);
    Pthread_create(&p1, NULL, mythread, "A"); 
    Pthread_create(&p2, NULL, mythread, "B");
    // join waits for the threads to finish
    Pthread_join(p1, NULL); 
    Pthread_join(p2, NULL); 
    printf("main: done\n [counter: %d]\n [should: %d]\n", 
	   counter, max*2);
    return 0;
}

```

首先，`Pthread create()` 只需调用 `pthread create()`，并确保返回代码为 0；如果不是，`Pthread create()` 只需打印一条信息并退出。其次，我们不再为工作线程使用两个独立的函数体，而是只使用一段代码，并向线程传递一个参数（在本例中是一个字符串），这样我们就可以让每个线程在其消息前打印不同的字母。

最后，也是最重要的一点，我们现在可以看看每个工作线程要做什么：在共享变量计数器中添加一个数字，并在一个循环中执行 1000 万次（1e7）。因此，我们想要的最终结果是：20,000,000。

现在我们编译并运行程序，看看它的运行情况。不幸的是，当我们运行这段代码时，即使在单个处理器上，我们也不一定能得到期望的结果。

```shell
❯ make shared_data
gcc -o shared_data shared_data.c -Wall
❯ ./shared_data 10000000
main: begin [counter = 0] [2b2c000]
A: begin [addr of i: 0x16d362fac]
B: begin [addr of i: 0x16d3eefac]
A: done
B: done
main: done
 [counter: 10306439]
 [should: 20000000]
```

我们可以多运行几次，可以发现不仅每次运行都是错误的，而且得出的结果也不一样！还有一个大问题：为什么会出现这种情况？

## 4 问题的核心：不受控制的调度

要理解为什么会出现这种情况，我们必须了解编译器为更新`counter`而生成的代码序列。在本例中，我们希望简单地向`counter`添加一个数字 (1)。因此，这样做的代码序列可能是这样的（在 x86 中）；

```asm
mov 0x8049a1c, %eax
add $0x1, %eax
mov %eax, 0x8049a1c
```

本例假设变量计数器位于地址 0x8049a1c。在这个三条指令序列中，首先使用 x86 mov 指令获取地址处的内存值，并将其放入寄存器 eax。然后执行 add 指令，将 eax 寄存器的内容加上 1 (0x1)，最后将 eax 的内容存储回相同地址的内存中。

假设两个线程中的一个（线程 1）进入了代码的这一区域，并准备将`counter`递增 1。它将`counter`的值（假设一开始是 50）加载到寄存器 eax 中。因此，线程 1 的寄存器 eax=50。然后在寄存器中加一，因此 eax=51 。

现在，不幸的事情发生了：定时器中断触发，操作系统将当前运行线程的状态（PC、包括 eax 在内的寄存器等）保存到线程的 TCB 中。现在更糟的事情发生了：线程 2 被选中运行，它也进入了这段代码。它也执行了第一条指令，获取了`counter`的值并将其放入 eax（记住：<font color="red">每个线程在运行时都有自己的专用寄存器；寄存器由上下文切换代码虚拟化，用于保存和恢复寄存器</font>）。此时计数器的值仍然是 50，因此线程 2 的 eax=50。假设线程 2 执行了接下来的两条指令，将 eax 递增 1（因此 eax=51），然后将 eax 的内容保存到`counter`（地址 0x8049a1c）中。这样，全局变量计数器现在的值是 51。

最后，进行另一次上下文切换，线程 1 恢复运行。回想一下，线程 1 刚刚执行了 mov 和 add 指令，现在即将执行最后一条 mov 指令。还记得 eax=51。因此，执行了最后一条 mov 指令，并将值保存到内存中；计数器再次被设置为 51。

简单地说，发生的情况是这样的：`counter`递增的代码运行了两次，但`counter`从 50 开始，现在只等于 51。这个程序的 "正确 "版本应该是变量`counter`等于 52。

让我们看一下详细的执行跟踪，以便更好地理解这个问题。在这个示例中，假设上述代码加载到内存地址 100，就像下面的序列一样（注意：x86 有可变长度指令；mov 指令占用 5 字节内存，而 add 指令只占用 3 字节内存）：

```asm
100 mov 0x8049a1c, %eax
105 add $0x1, %eax
108 mov %eax, 0x8049a1c
```

如下图所示，假设计数器的起始值为 50，并跟踪此示例以确保了解发生了什么。

![image-20240406122802826](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/The-Problem-Up-Close-And-Personal.png)

我们在这里演示的称为竞争条件（或更具体地说，数据竞争）：结果取决于代码的执行时机。如果运气不好（即上下文切换发生在执行过程中的不适时点），我们就会得到错误的结果。事实上，我们每次都可能得到不同的结果；因此，我们将这种结果称为不确定结果，而不是我们习惯于计算机中的良好确定性计算，即不知道输出是什么，并且在不同的运行中它确实可能会有所不同。

由于执行此代码的多个线程可能会导致竞争条件，因此我们将此代码称为<font color="red">临界区</font>。临界区是访问共享变量（或更一般地说，共享资源）的一段代码，并且不能由多个线程同时执行。

对于这段代码，我们真正想要的是我们所说的**互斥**。这一属性保证如果一个线程在临界区内执行，其他线程将被阻止这样做。

## 5 对原子性的渴望

解决这个问题的一种方法是使用功能更强大的指令，只需一步就能完成我们需要完成的任何工作，从而消除不适时中断的可能性。例如，如果我们有这样一条超级指令：

```asm
memory-add 0x8049a1c, $0x1
```

假设该指令向内存位置添加一个值，并且硬件保证它以原子方式执行；当指令执行时，它将根据需要执行更新。它不能在指令中间被中断，因为这正是我们从硬件得到的保证：当中断发生时，要么指令根本没有运行，要么已经运行完成；没有中间状态。

在这种情况下，“原子”意味着“作为一个单元”，有时我们将其视为“全部或无”。我们想要的是原子地执行三个指令序列：

```asm
mov 0x8049a1c, %eax
add $0x1, %eax
mov %eax, 0x8049a1c
```

正如我们所说，如果我们有一条指令来执行此操作，我们只需发出该指令即可完成。但一般情况下，我们不会有这样的指令。想象一下我们正在构建一个并发 B 树，并希望更新它；我们真的希望硬件支持“B 树原子更新”指令吗？可能不是，至少在健全的指令集中是这样。

因此，我们要做的是向硬件请求一些有用的指令，根据这些指令我们可以构建一组我们称之为同步原语的通用指令。通过使用这种硬件支持，并结合操作系统的一些帮助，我们将能够构建以同步和受控方式访问临界区的多线程代码，从而可靠地产生正确的结果，尽管并发执行具有挑战性。

## 6 另一个问题：等待另一个线程 

本章在讨论并发问题时，将线程间的交互设定为只有一种类型，即访问共享变量和需要支持临界区的原子性。事实证明，还有另一种常见的交互，即一个线程必须等待另一个线程完成某些操作后才能继续。例如，当一个线程执行磁盘 I/O 并进入休眠状态时，就会出现这种交互；当 I/O 完成后，该线程需要从沉睡中被唤醒，以便继续运行。有时，多个线程的操作应该是同步的。许多线程在数值问题中并行执行迭代， 所有线程应立即开始下一次迭代（屏障） ，此睡眠/唤醒周期将由条件变量控制。



> <center font-color="red">关键并发术语：临界区、竞争条件、不确定性、互斥</center>
>
> 这四个术语对于并发代码非常重要
>
> * **临界区**是访问共享资源（通常是变量或数据结构）的一段代码。
> * 如果多个执行线程大致同时进入临界区，则会出现**竞争条件**（或数据竞争）。两者都尝试更新共享数据结构，导致不符合期望的结果。 
> * 不确定程序由一个或多个竞争条件组成；程序的输出每次运行都会有所不同，具体取决于运行的线程。因此，结果是不确定性的，这与我们通常从计算机系统中期望的情况不同。
> * 为了避免这些问题，线程应该使用某种互斥原语；这样做可以保证只有一个线程进入临界区，从而避免竞争，并产生确定性的程序输出。 

# Program Explanation
这模拟器 `x86.py` 让你通过观察线程如何交错来熟悉线程，来帮助您更好的理解线程。模拟器模仿多线程执行短汇编序列。请注意，未显示将运行的操作系统代码（例如，执行上下文切换）；因此，您看到的只是用户代码的交错。运行的汇编代码基于 x86，但有所简化。在这个指令集中，有四个通用寄存器`（%ax、%bx、%cx、%dx）`、一个程序计数器（PC）和一小组足以满足我们目的的指令。

这是我们可以运行的示例代码片段：

```asm
.main
mov 2000, %ax   # get the value at the address
add $1, %ax     # increment it
mov %ax, 2000   # store it back
halt
```

代码很容易理解。第一条指令是 x86“mov”，只是将 2000 指定的地址中的值加载到寄存器 %ax 中。在 x86 的这个子集中，地址可以采用以下一些形式：

* `2000` ：数字（2000）是地址
* `(%cx)` ：寄存器的内容（括号内）构成地址
* `1000(%dx)` ：数字+寄存器内容组成地址
* `10(%ax,%bx)` ：数字+reg1+reg2组成地址

为了存储值，使用相同的 `mov` 指令，但这次参数相反，例如：

```asm
mov %ax, 2000
```

从上面的序列来看， `add` 指令应该很清楚：它将立即值（由 `$1` 指定）添加到第二个参数（即 `%ax = %ax + 1`，因此，我们现在可以理解上面的代码序列：它加载地址 2000 处的值，加 1，然后将该值存储回地址 2000。

假的 `halt` 指令只是停止运行该线程。让我们运行模拟器，看看这一切是如何工作的！假设上述代码序列位于文件 `simple-race.s` 中。

```c
❯ python x86.py -p simple-race.s -t 1
ARG seed 0
ARG numthreads 1
ARG program simple-race.s
ARG interrupt frequency 50
ARG interrupt randomness False
ARG argv 
ARG load address 1000
ARG memsize 128
ARG memtrace 
ARG regtrace 
ARG cctrace False
ARG printstats False
ARG verbose False

       Thread 0         
1000 mov 2000(%bx), %ax
1001 add $1, %ax
1002 mov %ax, 2000(%bx)
1003 halt
```

这里使用的参数指定程序 ( `-p` )、线程数 ( `-t 1` ) 和中断间隔，即调度程序被唤醒并运行以切换到的频率不同的任务。因为本例中只有一个线程，所以这个间隔并不重要。

输出很容易阅读：模拟器打印程序计数器（此处显示从 1000 到 1003）和执行的指令。请注意，我们假设（不切实际地）所有指令仅占用内存中的一个字节；在 x86 中，指令的大小是可变的，并且会占用 1 到几个字节。

我们可以使用更详细的跟踪来更好地了解执行过程中机器状态如何变化：

```shell
 2000      ax    bx          Thread 0         
    0       0     0   
    0       0     0   1000 mov 2000(%bx), %ax
    0       1     0   1001 add $1, %ax
    1       1     0   1002 mov %ax, 2000(%bx)
    1       1     0   1003 halt
```

通过使用 `-M` 标志，我们可以跟踪内存位置（逗号分隔的列表可以让您跟踪多个位置，例如 2000,3000）；通过使用 `-R` 标志，我们可以跟踪特定寄存器内的值。

左侧的值显示右侧指令执行后的内存/寄存器内容。例如，在 `add` 指令之后，您可以看到%ax已增加到值1；在第二条 `mov` 指令（PC=1002）之后，您可以看到 2000 处的内存内容现在也增加了。

您还需要了解更多说明，所以我们现在就开始了解它们。这是循环的代码片段：

```asm
.main
.top
sub  $1,%dx
test $0,%dx     
jgte .top         
halt
```

这里已经介绍了一些事情。首先是 `test` 指令。该指令接受两个参数并对它们进行比较；然后，它设置隐式“条件代码”（类似于 1 位寄存器），后续指令可以对其进行操作。

在这种情况下，另一条新指令是 `jump` 指令（在这种情况下， `jgte` 代表“如果大于或等于则跳转”）。如果测试中第二个值大于或等于第一个值，则该指令跳转。最后一点：要真正使此代码正常工作， `dx` 必须初始化为 1 或更大。因此，我们像这样运行程序：

```sh
❯ python x86.py -p loop.s -t 1 -a dx=3 -R dx -C -c
ARG seed 0
ARG numthreads 1
ARG program loop.s
ARG interrupt frequency 50
ARG interrupt randomness False
ARG argv dx=3
ARG load address 1000
ARG memsize 128
ARG memtrace 
ARG regtrace dx
ARG cctrace True
ARG printstats False
ARG verbose False

   dx   >= >  <= <  != ==        Thread 0         
    3   0  0  0  0  0  0  
    2   0  0  0  0  0  0  1000 sub  $1,%dx
    2   1  1  0  0  1  0  1001 test $0,%dx
    2   1  1  0  0  1  0  1002 jgte .top
    1   1  1  0  0  1  0  1000 sub  $1,%dx
    1   1  1  0  0  1  0  1001 test $0,%dx
    1   1  1  0  0  1  0  1002 jgte .top
    0   1  1  0  0  1  0  1000 sub  $1,%dx
    0   1  0  1  0  0  1  1001 test $0,%dx
    0   1  0  1  0  0  1  1002 jgte .top
   -1   1  0  1  0  0  1  1000 sub  $1,%dx
   -1   0  0  1  1  1  0  1001 test $0,%dx
   -1   0  0  1  1  1  0  1002 jgte .top
   -1   0  0  1  1  1  0  1003 halt
```

`-R dx` 标志跟踪 %dx 的值； `-C` 标志跟踪由测试指令设置的条件代码的值。最后， `-a dx=3` 标志将 `%dx` 寄存器设置为值 3 以开始。

从跟踪中可以看出， `sub` 指令慢慢降低了%dx的值。前几次调用 `test` 时，仅设置 ">="、">" 和 "!= 条件。然而，跟踪中的最后一个 `test` 发现 %dx 和 0 相等，因此后续跳转不会发生，程序最终停止。

现在，最后，我们遇到一个更有趣的情况，即具有多个线程的竞争条件。我们先看代码：

```c
# assumes %bx has loop count in it

.main
.top	
# critical section
mov 2000, %ax  # get 'value' at address 2000
add $1, %ax    # increment it
mov %ax, 2000  # store it back

# see if we're still looping
sub  $1, %bx
test $0, %bx
jgt .top	

halt
```

该代码有一个临界区，它加载变量的值（地址 2000），然后将该值加 1，然后将其存储回来。之后的代码只是递减循环计数器（以 %bx 为单位），测试它是否大于或等于零，如果是，则再次跳回顶部到临界区。

```sh
❯ python x86.py -p looping-race-nolock.s -t 2 -a bx=1 -M 2000 -c
ARG seed 0
ARG numthreads 2
ARG program looping-race-nolock.s
ARG interrupt frequency 50
ARG interrupt randomness False
ARG argv bx=1
ARG load address 1000
ARG memsize 128
ARG memtrace 2000
ARG regtrace 
ARG cctrace False
ARG printstats False
ARG verbose False

 2000          Thread 0                Thread 1         
    0   
    0   1000 mov 2000, %ax
    0   1001 add $1, %ax
    1   1002 mov %ax, 2000
    1   1003 sub  $1, %bx
    1   1004 test $0, %bx
    1   1005 jgt .top
    1   1006 halt
    1   ----- Halt;Switch -----  ----- Halt;Switch -----  
    1                            1000 mov 2000, %ax
    1                            1001 add $1, %ax
    2                            1002 mov %ax, 2000
    2                            1003 sub  $1, %bx
    2                            1004 test $0, %bx
    2                            1005 jgt .top
    2                            1006 halt
```

在这里，您可以看到每个线程运行一次，并且每个线程更新一次地址 2000 处的共享变量，从而导致该处的计数为 2。每当一个线程停止并且必须运行另一个线程时，就会插入 `Halt;Switch` 行。

最后一个示例：运行与上面相同的操作，但中断频率较小。看起来是这样的：

```sh
❯ python x86.py -p looping-race-nolock.s -t 2 -a bx=1 -M 2000 -c -i 2
ARG seed 0
ARG numthreads 2
ARG program looping-race-nolock.s
ARG interrupt frequency 2
ARG interrupt randomness False
ARG argv bx=1
ARG load address 1000
ARG memsize 128
ARG memtrace 2000
ARG regtrace 
ARG cctrace False
ARG printstats False
ARG verbose False

 2000          Thread 0                Thread 1         
    0   
    0   1000 mov 2000, %ax
    0   1001 add $1, %ax
    0   ------ Interrupt ------  ------ Interrupt ------  
    0                            1000 mov 2000, %ax
    0                            1001 add $1, %ax
    0   ------ Interrupt ------  ------ Interrupt ------  
    1   1002 mov %ax, 2000
    1   1003 sub  $1, %bx
    1   ------ Interrupt ------  ------ Interrupt ------  
    1                            1002 mov %ax, 2000
    1                            1003 sub  $1, %bx
    1   ------ Interrupt ------  ------ Interrupt ------  
    1   1004 test $0, %bx
    1   1005 jgt .top
    1   ------ Interrupt ------  ------ Interrupt ------  
    1                            1004 test $0, %bx
    1                            1005 jgt .top
    1   ------ Interrupt ------  ------ Interrupt ------  
    1   1006 halt
    1   ----- Halt;Switch -----  ----- Halt;Switch -----  
    1                            1006 halt
```

正如您所看到的，每个线程每 2 个指令就会中断一次，正如我们通过 `-i 2` 标志指定的那样。

现在让我们提供更多关于该程序可以模拟什么的信息。全套寄存器：%ax、%bx、%cx、%dx 和 PC。在此版本中，不支持“栈”，也没有调用和返回指令。

```asm
mov immediate, register     # moves immediate value to register
mov memory, register        # loads from memory into register
mov register, register      # moves value from one register to other
mov register, memory        # stores register contents in memory
mov immediate, memory       # stores immediate value in memory

add immediate, register     # register  = register  + immediate
add register1, register2    # register2 = register2 + register1
sub immediate, register     # register  = register  - immediate
sub register1, register2    # register2 = register2 - register1

test immediate, register    # compare immediate and register (set condition codes)
test register, immediate    # same but register and immediate
test register, register     # same but register and register

jne                         # jump if test'd values are not equal
je                          #                       ... equal
jlt                         #     ... second is less than first
jlte                        #               ... less than or equal
jgt                         #            ... is greater than
jgte                        #               ... greater than or equal

xchg register, memory       # atomic exchange: 
                            #   put value of register into memory
                            #   return old contents of memory into reg
                            # do both things atomically

nop                         # no op
```

Notes: 笔记：

- 'immediate' 的形式为 $number
- 'memory' 的形式为 'number' 或 '(reg)' 或 'number(reg)' 或 'number(reg,reg)' （如上所述）
- 'register' 是 %ax、%bx、%cx、%dx 之一

运行以下代码获取选项帮助：

```sh
❯ python x86.py -h
Usage: x86.py [options]

Options:
  -h, --help            show this help message and exit
  -s SEED, --seed=SEED  the random seed
  -t NUMTHREADS, --threads=NUMTHREADS
                        number of threads
  -p PROGFILE, --program=PROGFILE
                        source program (in .s)
  -i INTFREQ, --interrupt=INTFREQ
                        interrupt frequency
  -r, --randints        if interrupts are random
  -a ARGV, --argv=ARGV  comma-separated per-thread args (e.g., ax=1,ax=2 sets
                        thread 0 ax reg to 1 and thread 1 ax reg to 2);
                        specify multiple regs per thread via colon-separated
                        list (e.g., ax=1:bx=2,cx=3 sets thread 0 ax and bx and
                        just cx for thread 1)
  -L LOADADDR, --loadaddr=LOADADDR
                        address where to load code
  -m MEMSIZE, --memsize=MEMSIZE
                        size of address space (KB)
  -M MEMTRACE, --memtrace=MEMTRACE
                        comma-separated list of addrs to trace (e.g.,
                        20000,20001)
  -R REGTRACE, --regtrace=REGTRACE
                        comma-separated list of regs to trace (e.g.,
                        ax,bx,cx,dx)
  -C, --cctrace         should we trace condition codes
  -S, --printstats      print some extra stats
  -v, --verbose         print some extra info
  -c, --compute         compute answers for me
```
# Homework
1. 让我们看一个简单的程序“loop.s”。首先，阅读并理解它。然后，使用这些参数运行它（ `python x86.py -p loop.s -t 1 -i 100 -R dx` ）。这指定一个线程、每 100 条指令一个中断，并跟踪寄存器 `%dx` 。运行期间 `%dx` 会发生什么？使用 `-c` 标记检查您的答案；左侧的答案显示右侧指令运行后寄存器的值（或内存值）。

	dx初始值为0，经过sub操作后不满足`jgte`条件，所以，dx最后值为-1，只循环了一次。

	```shell
	❯ python x86.py -p loop.s -t 1 -i 100 -R dx -c
	ARG seed 0
	ARG numthreads 1
	ARG program loop.s
	ARG interrupt frequency 100
	ARG interrupt randomness False
	ARG argv 
	ARG load address 1000
	ARG memsize 128
	ARG memtrace 
	ARG regtrace dx
	ARG cctrace False
	ARG printstats False
	ARG verbose False
	
	   dx          Thread 0         
	    0   
	   -1   1000 sub  $1,%dx
	   -1   1001 test $0,%dx
	   -1   1002 jgte .top
	   -1   1003 halt
	```

2. 相同的代码，不同的标志：( `python x86.py -p loop.s -t 2 -i 100 -a dx=3,dx=3 -R dx` )这指定了两个线程，并将每个 `%dx` 初始化为 3。 `%dx` 将看到什么值？使用 `-c` 运行进行检查。多线程的存在会影响您的计算吗？这段代码中有竞争吗？

	```shell
	❯ python x86.py -p loop.s -t 2 -i 100 -a dx=3,dx=3 -R dx -c
	ARG seed 0
	ARG numthreads 2
	ARG program loop.s
	ARG interrupt frequency 100
	ARG interrupt randomness False
	ARG argv dx=3,dx=3
	ARG load address 1000
	ARG memsize 128
	ARG memtrace 
	ARG regtrace dx
	ARG cctrace False
	ARG printstats False
	ARG verbose False
	
	   dx          Thread 0                Thread 1         
	    3   
	    2   1000 sub  $1,%dx
	    2   1001 test $0,%dx
	    2   1002 jgte .top
	    1   1000 sub  $1,%dx
	    1   1001 test $0,%dx
	    1   1002 jgte .top
	    0   1000 sub  $1,%dx
	    0   1001 test $0,%dx
	    0   1002 jgte .top
	   -1   1000 sub  $1,%dx
	   -1   1001 test $0,%dx
	   -1   1002 jgte .top
	   -1   1003 halt
	    3   ----- Halt;Switch -----  ----- Halt;Switch -----  
	    2                            1000 sub  $1,%dx
	    2                            1001 test $0,%dx
	    2                            1002 jgte .top
	    1                            1000 sub  $1,%dx
	    1                            1001 test $0,%dx
	    1                            1002 jgte .top
	    0                            1000 sub  $1,%dx
	    0                            1001 test $0,%dx
	    0                            1002 jgte .top
	   -1                            1000 sub  $1,%dx
	   -1                            1001 test $0,%dx
	   -1                            1002 jgte .top
	   -1                            1003 halt
	```

	不会，没有竞争，因为没有临界区，都是从各自的dx中存取数据。

3. 运行这个： `python x86.py -p loop.s -t 2 -i 3 -r -a dx=3,dx=3 -R dx` 这使得中断间隔变小/随机；使用不同的种子 ( `-s` ) 来查看不同的中断间隔。中断频率会改变什么吗？

	不会，没有竞争条件。

	```shell
	❯ python x86.py -p loop.s -t 2 -i 3 -r -a dx=3,dx=3 -R dx -c
	ARG seed 0
	ARG numthreads 2
	ARG program loop.s
	ARG interrupt frequency 3
	ARG interrupt randomness True
	ARG argv dx=3,dx=3
	ARG load address 1000
	ARG memsize 128
	ARG memtrace 
	ARG regtrace dx
	ARG cctrace False
	ARG printstats False
	ARG verbose False
	
	   dx          Thread 0                Thread 1         
	    3   
	    2   1000 sub  $1,%dx
	    2   1001 test $0,%dx
	    2   1002 jgte .top
	    3   ------ Interrupt ------  ------ Interrupt ------  
	    2                            1000 sub  $1,%dx
	    2                            1001 test $0,%dx
	    2                            1002 jgte .top
	    2   ------ Interrupt ------  ------ Interrupt ------  
	    1   1000 sub  $1,%dx
	    1   1001 test $0,%dx
	    2   ------ Interrupt ------  ------ Interrupt ------  
	    1                            1000 sub  $1,%dx
	    1   ------ Interrupt ------  ------ Interrupt ------  
	    1   1002 jgte .top
	    0   1000 sub  $1,%dx
	    1   ------ Interrupt ------  ------ Interrupt ------  
	    1                            1001 test $0,%dx
	    1                            1002 jgte .top
	    0   ------ Interrupt ------  ------ Interrupt ------  
	    0   1001 test $0,%dx
	    0   1002 jgte .top
	   -1   1000 sub  $1,%dx
	    1   ------ Interrupt ------  ------ Interrupt ------  
	    0                            1000 sub  $1,%dx
	   -1   ------ Interrupt ------  ------ Interrupt ------  
	   -1   1001 test $0,%dx
	   -1   1002 jgte .top
	    0   ------ Interrupt ------  ------ Interrupt ------  
	    0                            1001 test $0,%dx
	    0                            1002 jgte .top
	   -1   ------ Interrupt ------  ------ Interrupt ------  
	   -1   1003 halt
	    0   ----- Halt;Switch -----  ----- Halt;Switch -----  
	   -1                            1000 sub  $1,%dx
	   -1                            1001 test $0,%dx
	   -1   ------ Interrupt ------  ------ Interrupt ------  
	   -1                            1002 jgte .top
	   -1                            1003 halt
	```

4. 现在，另一个程序 `looping-race-nolock.s` 访问位于地址 2000 的共享变量；我们将这个变量称为 `value` 。使用单线程运行它以确认您的理解： `python x86.py -p looping-race-nolock.s -t 1 -M 2000` 在整个运行过程中 `value` （即内存地址 2000）是什么？使用 `-c` 进行检查。

	```shell
	❯ python x86.py -p looping-race-nolock.s -t 1 -M 2000 -c
	ARG seed 0
	ARG numthreads 1
	ARG program looping-race-nolock.s
	ARG interrupt frequency 50
	ARG interrupt randomness False
	ARG argv 
	ARG load address 1000
	ARG memsize 128
	ARG memtrace 2000
	ARG regtrace 
	ARG cctrace False
	ARG printstats False
	ARG verbose False
	
	 2000          Thread 0         
	    0   
	    0   1000 mov 2000, %ax
	    0   1001 add $1, %ax
	    1   1002 mov %ax, 2000
	    1   1003 sub  $1, %bx
	    1   1004 test $0, %bx
	    1   1005 jgt .top
	    1   1006 halt
	```

	为1，bx为0，则只进行一次循环，对地址2000的共享变量进行add操作，则最后`value`值为1。

5. 使用多个迭代/线程运行： `python x86.py -p looping-race-nolock.s -t 2 -a bx=3 -M 2000` 为什么每个线程循环三次？ `value` 的最终值是多少？

	因为bx值为3，所以每个线程循环3次。`value`值最终为6，虽然这段代码有竞争条件（临界区、写入共享资源、多个例程），但上下文切换时间合适（中断间隔默认为50）。

	```shell
	❯ python x86.py -p looping-race-nolock.s -t 2 -a bx=3  -M 2000 -c
	ARG seed 0
	ARG numthreads 2
	ARG program looping-race-nolock.s
	ARG interrupt frequency 50
	ARG interrupt randomness False
	ARG argv bx=3
	ARG load address 1000
	ARG memsize 128
	ARG memtrace 2000
	ARG regtrace 
	ARG cctrace False
	ARG printstats False
	ARG verbose False
	
	 2000          Thread 0                Thread 1         
	    0   
	    0   1000 mov 2000, %ax
	    0   1001 add $1, %ax
	    1   1002 mov %ax, 2000
	    1   1003 sub  $1, %bx
	    1   1004 test $0, %bx
	    1   1005 jgt .top
	    1   1000 mov 2000, %ax
	    1   1001 add $1, %ax
	    2   1002 mov %ax, 2000
	    2   1003 sub  $1, %bx
	    2   1004 test $0, %bx
	    2   1005 jgt .top
	    2   1000 mov 2000, %ax
	    2   1001 add $1, %ax
	    3   1002 mov %ax, 2000
	    3   1003 sub  $1, %bx
	    3   1004 test $0, %bx
	    3   1005 jgt .top
	    3   1006 halt
	    3   ----- Halt;Switch -----  ----- Halt;Switch -----  
	    3                            1000 mov 2000, %ax
	    3                            1001 add $1, %ax
	    4                            1002 mov %ax, 2000
	    4                            1003 sub  $1, %bx
	    4                            1004 test $0, %bx
	    4                            1005 jgt .top
	    4                            1000 mov 2000, %ax
	    4                            1001 add $1, %ax
	    5                            1002 mov %ax, 2000
	    5                            1003 sub  $1, %bx
	    5                            1004 test $0, %bx
	    5                            1005 jgt .top
	    5                            1000 mov 2000, %ax
	    5                            1001 add $1, %ax
	    6                            1002 mov %ax, 2000
	    6                            1003 sub  $1, %bx
	    6                            1004 test $0, %bx
	    6                            1005 jgt .top
	    6                            1006 halt
	```

6. 以随机中断间隔运行： `python x86.py -p looping-race-nolock.s -t 2 -M 2000 -i 4 -r -s 0` 使用不同的种子（ `-s 1` 、 `-s 2` 等），您可以通过查看线程交错来判断 `value`的最终值是什么？中断的时间重要吗？哪里可以安全地发生？哪里不是？换句话说，临界区到底在哪里？

	`value`的最终值是不确定的，中断的时间很重要，关键是临界区的互斥是否被中断的时机破坏了。临界区如下：

	```shell
	1000 mov 2000, %ax 1001 add $1, %ax 1002 mov %ax, 2000
	```

	当中断的时机可以保留临界区的互斥性，则可以安全发生，得到确定性结果。

7. 现在检查固定中断间隔： `python x86.py -p looping-race-nolock.s -a bx=1 -t 2 -M 2000 -i 1` 共享变量 `value` 的最终值是多少？当您更改 `-i 2` 、 `-i 3` 等时会怎样？程序对于哪些中断间隔给出“正确”答案？

	当中断间隔$\geq 3$ 时，程序将给出“正确”答案，为2。

8. 对更多循环运行相同的操作（例如，设置 ` -a bx=100` ）。什么中断间隔 ( `-i` ) 会导致正确的结果？哪些间隔令人惊讶？

	$i\geq 597 \text{ or }i \text{ is Multiples of } 3 $。第一个条件是让线程1执行完（总共600条指令，还剩3条不是临界区代码）。第二个条件是因为每个循环6条指令，3条是临界区代码，所以中断间隔是3的倍数就不会影响到临界区。

9. 最后一个程序： `wait-for-me.s` 。运行： `python x86.py -p wait-for-me.s -a ax=1,ax=0 -R ax -M 2000` 这将线程 0 的 `%ax` 寄存器设置为 1，线程 1 的寄存器设置为 0，并监视 `%ax` 和内存位置 2000。代码应该如何运行？线程如何使用位置 2000 处的值？`value`的最终值是多少？

	线程 0 将内存 2000 设置为 1，然后停止。线程1一直循环运行，直到内存2000为1。

10. 现在切换输入： `python x86.py -p wait-for-me.s -a ax=0,ax=1 -R ax -M 2000` 线程的行为如何？线程0在做什么？更改中断间隔（例如 `-i 1000` ，或者可能使用随机间隔）将如何改变跟踪结果？程序是否有效地使用CPU？

	线程交换了。现在线程 0 继续循环运行，等待线程 1 将内存 2000 更改为 1。无论您如何更改中断间隔，都不会改变跟踪结果。程序是有效使用CPU。