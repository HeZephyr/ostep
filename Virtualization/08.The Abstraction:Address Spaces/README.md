# Note/Translation
* 什么是内存虚拟化？
	* 操作系统虚拟其物理内存。 
	* 操作系统为每个进程提供一个虚拟内存空间。
	* 看起来每个进程都使用整个内存。
* 内存虚拟化的好处
	* 易于编程
	* 在时间和空间方面提高内存效率
	* 保证进程和操作系统的隔离，防止其他进程的错误访问

## 1 早期操作系统

从内存的角度来看，早期的机器并没有为用户提供太多的抽象概念。基本上，机器的物理内存如下图所示。

<img src="https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/early-system-physical-memory.png" alt="image-20240330095457257" style="zoom:67%;" />

操作系统是一组位于内存（本例中从物理地址 0 开始）中的例程（实际上是一个库），而当前位于物理内存（本例中从物理地址 64k 开始）中的一个正在运行的程序（进程）则使用内存的其余部分。用户对操作系统的期望值并不高。

## 2 多道程序设计和分时

一段时间后，由于机器价格昂贵，人们开始更有效地共享机器。因此，<font color="red">多道程序设计</font>的时代诞生了，其中多个进程准备在给定时间运行，并且操作系统将在它们之间切换，例如当一个进程决定执行 I/O 时。这样做提高了 CPU 的有效利用率。在当时每台机器的成本高达数十万甚至数百万美元（而且您认为您的 Mac 很昂贵！），这种效率的提高尤其重要。

然而，很快，人们开始对机器提出更多要求，<font color="red">分时时代</font>诞生了。具体来说，许多人意识到批处理计算的局限性，特别是对程序员本身而言，他们厌倦了长的程序调试周期。交互性的概念变得很重要，因为许多用户可能同时使用一台机器，每个用户都等待（或希望）当前正在执行的任务及时响应。实现分时的一种方法是运行一个进程一小会儿，赋予它对所有内存的完全访问权限，然后停止它，将其所有状态保存到某种磁盘（包括所有物理内存），加载其他进程的状态，运行一段时间，从而实现某种机器的粗略共享。

但这种方法有一个大问题：它太慢了，尤其是当内存增长时。虽然保存和恢复寄存器级状态（PC、通用寄存器等）相对较快，但将内存的全部内容保存到磁盘的性能却非常低。因此，我们宁愿做的是将进程留在内存中，同时在它们之间切换，从而允许操作系统有效地实现时间共享，如下图所示。

<img src="https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Three-Processes%3ASharing-Memory.png" alt="image-20240330100100856" style="zoom: 67%;" />

图中，有三个进程（A、B 和 C），每个进程都有为其分配的 512KB 物理内存的一小部分。假设只有一个 CPU，操作系统选择运行其中一个进程（例如 A），而其他进程（B 和 C）则位于就绪队列中等待运行。但允许多个程序同时驻留在内存中使得保护成为一个重要问题；你不希望一个进程能够读取，或者更糟糕的是，写入其他进程的内存。

## 3 地址空间

这样就需要操作系统创建一个易于使用的物理内存抽象。我们将这种抽象称为<font color="red">地址空间</font>，它是正在运行的程序对系统中内存的视图。了解操作系统对内存的基本抽象是理解内存如何虚拟化的关键。

进程的地址空间包含正在运行的程序的所有内存状态。例如，程序的**代码**（指令）必须位于内存中的某个位置，因此它们位于地址空间中。程序在运行时使用栈来跟踪它在函数调用链中的位置，以及分配局部变量和向例程传递参数和返回值。最后，**堆**用于动态分配、用户管理的内存，例如您可能从 C 中调用 `malloc()` 或面向对象语言（例如 C++ 或 Java）中的 new 调用中接收到的内存。当然，其中还有其他东西（例如静态初始化变量），但现在我们只假设这三个组件：<font color="red">代码、栈和堆</font>。

<img src="https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/new-code-stack-heap.png" alt="image-20240330101636730" style="zoom:67%;" />

在上图的示例中，我们有一个很小的地址空间（只有 16KB）。程序代码位于地址空间的顶部（在本例中从 0 开始，并被打包到地址空间的前 1KB 中）。代码是静态的（因此很容易放置在内存中），因此我们可以将其放置在地址空间的顶部，并且知道程序运行时它不会需要更多空间。

接下来，我们有两个在程序运行时可能会增长（和收缩）的地址空间区域。这些是堆（在顶部）和栈（在底部）。我们这样放置它们是因为每个都希望能够增长，并且通过将它们放在地址空间的两端，我们可以允许这样的增长：它们只需要向相反的方向增长即可。因此，堆在代码之后开始（1KB）并向下增长（例如，当用户通过 `malloc()` 请求更多内存时）；堆栈从 16KB 开始并向上增长（例如当用户进行过程调用时）。然而，栈和堆的这种放置只是一种约定；如果您愿意，可以以不同的方式安排地址空间（正如我们稍后将看到的，当多个线程共存于一个地址空间中时，没有像这样划分地址空间的好方法了）。

当然，当我们描述地址空间时，我们描述的是操作系统为正在运行的程序提供的抽象。该程序实际上并不位于物理地址 0 到 16KB 的内存中；相反，它被加载到某个任意的物理地址。我们可以看到上图 中的进程 A、B 和 C，每个进程如何加载到不同地址的内存中。因此出现了问题：怎么虚拟化内存？操作系统如何在单个物理内存之上为多个正在运行的进程（所有共享内存）构建私有的、可能很大的地址空间的抽象？

当操作系统执行此操作时，我们说操作系统正在<font color="red">虚拟化内存</font>，因为正在运行的程序认为它已加载到内存中的特定地址（例如 0），并且具有潜在的非常大的地址空间（例如 32 位或 64 位） ，但现实却截然不同。

例如，当上图的进程 A 尝试在地址 0（我们将其称为虚拟地址）处执行加载时，操作系统与某些硬件支持相结合，必须确保加载实际上不是到物理地址 0，而是转到物理地址 320KB（A 被加载到内存中）。这是内存虚拟化的关键，内存虚拟化是世界上每个现代计算机系统的基础。

> <center>TIP 隔离原则</center>
>
> 隔离是构建可靠系统的关键原则。如果两个实体适当地相互隔离，这意味着其中一个实体发生故障时不会影响到另一个实体。操作系统努力将进程相互隔离，从而防止一个进程损害另一个进程。
>
> 通过使用内存隔离，操作系统进一步确保运行中的程序不会影响底层操作系统的运行。一些现代操作系统甚至更进一步，将操作系统的各个部分与操作系统的其他部分隔离开来。因此，这种微内核比典型的单片内核设计更可靠。

## 4 目标

因此，我们在这组笔记中找到了操作系统的工作：虚拟化内存。不过，操作系统不仅会虚拟化内存；它会很有风格地做到这一点。为了确保操作系统做到这一点，我们需要一些目标来指导我们。

虚拟内存 (VM) 系统的主要目标之一是**透明度**。操作系统应该以对正在运行的程序不可见的方式实现虚拟内存。因此，程序不应该意识到内存是虚拟化的；相反，程序的行为就好像它有自己的私有物理内存一样。操作系统（和硬件）在背后完成了在许多不同作业之间复用内存的所有工作，从而实现了这种错觉。 

VM的另一个目标是**效率**。操作系统应努力使虚拟化尽可能高效，无论是在时间（即不使程序运行得更慢）还是在空间（即不为支持虚拟化所需的结构使用太多内存）方面。在实现高效虚拟化时，操作系统必须依赖硬件支持，包括TLB等硬件功能。

最后，第三个VM目标是**保护**。操作系统应确保进程免受其他进程的影响以及操作系统本身免受进程的影响。当一个进程执行加载、存储或取指令时，它不应该能够以任何方式访问或影响任何其他进程或操作系统本身的内存内容（即其地址空间之外的任何内容）。因此，保护使我们能够提供进程之间的隔离特性；每个进程都应该在自己隔离的茧中运行，免受其他错误甚至恶意进程的破坏。

## 5 虚拟地址

事实上，作为用户级程序的程序员，你能看到的任何地址都是虚拟地址。只有操作系统通过其巧妙的虚拟内存技术，才能知道这些指令和数据值在机器物理内存中的位置。因此，千万不要忘记：如果你在程序中打印出一个地址，那只是一个虚拟地址，是内存中事物布局的假象；只有操作系统（和硬件）才知道真正的真相。下面是一个小程序 (va.c)，它可以打印出 main() 例程（代码所在位置）的位置、全局变量的位置、malloc() 返回的堆分配值以及堆栈中整数的位置：

```c
#include <stdio.h>
#include <stdlib.h>

int global = 1;

int main(int argc, char *argv[]) {
    // Declaring a variable x on the stack
    int x = 3;

    // Printing the location of the code
    printf("location of code : %p\n", (void *) main);
    // Printing the location of global variable
    printf("location of data : %p\n", (void *) &global);
    // Printing the location of heap memory
    printf("location of heap : %p\n", (void *) malloc(1));
    // Printing the location of stack variable x
    printf("location of stack : %p\n", (void *) &x);
    
    return x;
}
```

当运行在64位的Linux，我们得到如下输出：

> location of code: 0x40057d 
>
> location of data : 0x401010 
>
> location of heap : 0xcf2010 
>
> location of stack : 0x7fff9ca45fcc

如下图所示，可以看到代码首先出现在地址空间中，其次是静态数据，然后是堆，而栈一直位于这个大虚拟空间的另一端。所有这些地址都是虚拟的，并且将由操作系统和硬件进行转换，以便从其真实的物理位置获取值。

<img src="https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/memory-max-linux" alt="image-20240330111928011" style="zoom: 50%;" />

## 6 总结

我们已经看到了一个主要操作系统子系统的引入：虚拟内存。 VM系统负责为程序提供一个大的、稀疏的、私有的地址空间的假象，其中保存了它们的所有指令和数据。在一些硬件的帮助下，操作系统将获取每个虚拟内存引用，并将它们转换为物理地址，可以将其呈现给物理内存以获取所需的信息。操作系统将同时对许多进程执行此操作，确保保护程序免受彼此的侵害，并保护操作系统。

VM的目标如下：

* 透明度

	 VM 对于正在运行的程序应该是不可见的

* 效率

	最小化时间和空间方面的开销

* 保护

	隔离进程（和操作系统本身）[但允许选择性“通信”]

# Homework

1. 你应该查看的第一个 Linux 工具是非常简单的 free 工具。首先，键入 man free 并阅读整个手册页面。~~RTFM~~

	```
	FREE(1)                       User Commands                      FREE(1)
	NAME
	       free - Display amount of free and used memory in the system
	SYNOPSIS
	       free [options]
	DESCRIPTION
	       free displays the total amount of free and used physical and swap
	       memory in the system, as well as the buffers and caches used by
	       the kernel. The information is gathered by parsing /proc/meminfo.
	       The displayed columns are:
	
	       total  Total usable memory (MemTotal and SwapTotal in
	              /proc/meminfo). This includes the physical and swap memory
	              minus a few reserved bits and kernel binary code.
	
	       used   Used or unavailable memory (calculated as total -
	              available)
	
	       free   Unused memory (MemFree and SwapFree in /proc/meminfo)
	
	       shared Memory used (mostly) by tmpfs (Shmem in /proc/meminfo)
	
	       buffers
	              Memory used by kernel buffers (Buffers in /proc/meminfo)
	
	       cache  Memory used by the page cache and slabs (Cached and
	              SReclaimable in /proc/meminfo)
	
	       buff/cache
	              Sum of buffers and cache
	
	       available
	              Estimation of how much memory is available for starting
	              new applications, without swapping. Unlike the data
	              provided by the cache or free fields, this field takes
	              into account page cache and also that not all reclaimable
	              memory slabs will be reclaimed due to items being in use
	              (MemAvailable in /proc/meminfo, available on kernels 3.14,
	              emulated on kernels 2.6.27+, otherwise the same as free)
	OPTIONS
	       -b, --bytes
	              Display the amount of memory in bytes.
	
	       -k, --kibi
	              Display the amount of memory in kibibytes.  This is the
	              default.
	
	       -m, --mebi
	              Display the amount of memory in mebibytes.
	
	       -g, --gibi
	              Display the amount of memory in gibibytes.
	
	       --tebi Display the amount of memory in tebibytes.
	
	       --pebi Display the amount of memory in pebibytes.
	
	       --kilo Display the amount of memory in kilobytes. Implies --si.
	
	       --mega Display the amount of memory in megabytes. Implies --si.
	
	       --giga Display the amount of memory in gigabytes. Implies --si.
	
	       --tera Display the amount of memory in terabytes. Implies --si.
	
	       --peta Display the amount of memory in petabytes. Implies --si.
	
	       -h, --human
	              Show all output fields automatically scaled to shortest
	              three digit unit and display the units of print out.
	              Following units are used.
	
	                B = bytes
	                Ki = kibibyte
	                Mi = mebibyte
	                Gi = gibibyte
	                Ti = tebibyte
	                Pi = pebibyte
	
	              If unit is missing, and you have exbibyte of RAM or swap,
	              the number is in tebibytes and columns might not be
	              aligned with header.
	
	       -w, --wide
	              Switch to the wide mode. The wide mode produces lines
	              longer than 80 characters. In this mode buffers and cache
	              are reported in two separate columns.
	
	       -c, --count count
	              Display the result count times.  Requires the -s option.
	
	       -l, --lohi
	              Show detailed low and high memory statistics.
	
	       -s, --seconds delay
	              Continuously display the result delay  seconds apart.  You
	              may actually specify any floating point number for delay
	              using either . or , for decimal point.  usleep(3) is used
	              for microsecond resolution delay times.
	
	       --si   Use kilo, mega, giga etc (power of 1000) instead of kibi,
	              mebi, gibi (power of 1024).
	
	       -t, --total
	              Display a line showing the column totals.
	
	       --help Print help.
	
	       -V, --version
	              Display version information.
	FILES
	       /proc/meminfo
	              memory information
	```

2. 现在，自由运行，或许可以使用一些可能有用的参数（例如，-m，以兆字节为单位显示内存总量）。系统中有多少内存？有多少是空闲的？这些数字符合你的直觉吗？

	```shell
	> free -m
	               total        used        free      shared  buff/cache   available
	Mem:            1690         346         233           2        1110        1156
	Swap:
	```

	```shell
	> free -m -s 1 -c 3
	               total        used        free      shared  buff/cache   available
	Mem:            1690         344         236           2        1110        1159
	Swap:              0           0           0
	
	               total        used        free      shared  buff/cache   available
	Mem:            1690         344         236           2        1110        1159
	Swap:              0           0           0
	
	               total        used        free      shared  buff/cache   available
	Mem:            1690         344         236           2        1110        1159
	Swap:              0           0           0
	```

3. 接下来，创建一个使用一定内存量的小程序，称为 memory-user.c。该程序应采用一个命令行参数：它将使用的内存兆字节数。运行时，它应该分配一个数组，并不断地流过该数组，接触每个条目。程序应该无限期地执行此操作，或者可能在命令行中指定的一定时间内执行此操作。

	```c
	#include <stdio.h>
	#include <stdlib.h>
	#include <time.h>
	#include <unistd.h>
	
	int main(int argc, char *argv[]) {
	    if (argc != 3) {
	        fprintf(stderr, "Usage: %s <memory> <time>\n", argv[0]);
	        return 1;
	    }
	
	    printf("Current Process ID = %d\n", getpid());
	
	    long long int size = ((long long int)atoi(argv[1])) * 1024 * 1024;
	    int* buffer = (int*)malloc(size);
	
	    // Run the while loop for given amount of time.
	    time_t endwait, seconds, start;
	    seconds = atoi(argv[2]);
	    start = time(NULL);
	    // print format start time
	    printf("Start Time: %s", ctime(&start));
	    endwait = start + seconds;
	
	    while (start < endwait) {
	        for (long long int i = 0; i < size / sizeof(int); i++) {
	            buffer[i] = i;
	        }
	        start = time(NULL);
	    }
	    printf("End Time: %s", ctime(&start));
	    free(buffer);
	    return 0;
	}
	```

	编译该程序后，只需使用两个参数运行即可。第一个参数是要保留的 MB 数，第二个参数是运行程序的最小秒数。

4. 现在，在运行内存用户程序的同时，也（在不同的终端窗口，但在同一台机器上）运行`free`工具。当你的程序运行时，内存使用总量有什么变化？杀死内存用户程序时又如何？这些数字符合你的预期吗？针对不同的内存使用量进行尝试。如果内存使用量非常大，会发生什么情况？

	```shell
	> free -k -s 1 -c 10
	               total        used        free      shared  buff/cache   available
	Mem:         1731172      376744     1092732        2640      261696     1169784
	Swap:              0           0           0
	
	               total        used        free      shared  buff/cache   available
	Mem:         1731172      376996     1092480        2640      261696     1169540
	Swap:              0           0           0
	
	               total        used        free      shared  buff/cache   available
	Mem:         1731172      376868     1092480        2640      261824     1169668
	Swap:              0           0           0
	
	               total        used        free      shared  buff/cache   available
	Mem:         1731172      653312      816036        2640      261824      893224
	Swap:              0           0           0
	
	               total        used        free      shared  buff/cache   available
	Mem:         1731172     1203960      265164        2640      262048      342568
	Swap:              0           0           0
	
	               total        used        free      shared  buff/cache   available
	Mem:         1731172     1399788       75604        2640      255780      146792
	Swap:              0           0           0
	
	               total        used        free      shared  buff/cache   available
	Mem:         1731172     1399788       75604        2640      255780      146792
	Swap:              0           0           0
	
	               total        used        free      shared  buff/cache   available
	Mem:         1731172     1399724       75604        2640      255844      146800
	Swap:              0           0           0
	
	               total        used        free      shared  buff/cache   available
	Mem:         1731172     1399724       75604        2640      255844      146800
	Swap:              0           0           0
	
	               total        used        free      shared  buff/cache   available
	Mem:         1731172      374780     1100388        2640      256004     1171832
	Swap:              0           0           0
	```

	如`free`输出，

	1. **内存使用增加**：因为程序动态分配了大量的内存，因此系统的内存使用量会增加。这些内存可能会被标记为“已分配”，但实际上并没有被物理分配，而是在需要时才会真正分配。
	2. **缓慢增长**：内存使用量可能会以缓慢的速度增长，因为你的程序在循环中不断对分配的内存进行写操作。这种增长的速度取决于循环的迭代次数以及每次迭代中对内存的写入量。

	当你杀死内存用户程序时或者程序结束后，系统的内存使用总量会减少。杀死程序和主动释放内存都会导致系统回收程序所分配的内存，并将其标记为可用内存。系统会尽可能快地释放这些内存，并将其用于其他进程或系统操作。

	如果使用大量内存，会出现段错误：

	```shell
	❯ ./a.out 1000000000000 6
	Current Process ID = 65564
	Start Time: Sat Mar 30 13:51:31 2024
	[1]    65564 segmentation fault  ./a.out 1000000000000 6
	```

	但如果不遍历数组，即只分配不使用，正如上面所述，只有需要时才会真正分配。这个时候就不会发生段错误。

5. 让我们再试试 pmap 工具。花点时间，详细阅读 pmap 手册页面

	```shell
	NAME
	       pmap - report memory map of a process
	SYNOPSIS
	       pmap [options] pid [...]
	DESCRIPTION
	       The pmap command reports the memory map of a process or processes.
	OPTIONS
	       -x, --extended
	              Show the extended format.
	       -d, --device
	              Show the device format.
	       -q, --quiet
	              Do not display some header or footer lines.
	       -A, --range low,high
	              Limit results to the given range to low and high address range.  Notice that the low and high arguments are single string separated with comma.
	       -X     Show even more details than the -x option. WARNING: format changes according to /proc/PID/smaps
	       -XX    Show everything the kernel provides
	       -p, --show-path
	              Show full path to files in the mapping column
	       -c, --read-rc
	              Read the default configuration
	       -C, --read-rc-from file
	              Read the configuration from file
	       -n, --create-rc
	              Create new default configuration
	       -N, --create-rc-to file
	              Create new configuration to file
	       -h, --help
	              Display help text and exit.
	       -V, --version
	              Display version information and exit.
	EXIT STATUS
	              0      Success.
	              1      Failure.
	              42     Did not find all processes asked for.
	```

6. 要使用 pmap，您必须知道您感兴趣的进程的PID。因此，首先运行 ps auxw 查看所有进程的列表；然后，选择一个有趣的，例如浏览器。在这种情况下，您还可以使用`memory-user.c`程序（实际上，您甚至可以让该程序调用 getpid() 并打印出其 PID 以方便您使用）。

	```
	> ps auxw | grep 1289471
	zfhe     1289471 93.1 60.6 1051356 1050244 pts/5 R+   13:32   0:14 ./a.out 1 100
	> pmap 1289471
	1289471:   ./a.out 1 100
	000055ce030a9000      4K r---- a.out
	000055ce030aa000      4K r-x-- a.out
	000055ce030ab000      4K r---- a.out
	000055ce030ac000      4K r---- a.out
	000055ce030ad000      4K rw--- a.out
	000055ce0502b000    132K rw---   [ anon ]
	00007f4e75d4f000 1048592K rw---   [ anon ]
	00007f4eb5d53000    160K r---- libc.so.6
	00007f4eb5d7b000   1620K r-x-- libc.so.6
	00007f4eb5f10000    352K r---- libc.so.6
	00007f4eb5f68000      4K ----- libc.so.6
	00007f4eb5f69000     16K r---- libc.so.6
	00007f4eb5f6d000      8K rw--- libc.so.6
	00007f4eb5f6f000     52K rw---   [ anon ]
	00007f4eb5f87000      8K rw---   [ anon ]
	00007f4eb5f89000      8K r---- ld-linux-x86-64.so.2
	00007f4eb5f8b000    168K r-x-- ld-linux-x86-64.so.2
	00007f4eb5fb5000     44K r---- ld-linux-x86-64.so.2
	00007f4eb5fc1000      8K r---- ld-linux-x86-64.so.2
	00007f4eb5fc3000      8K rw--- ld-linux-x86-64.so.2
	00007ffe9e81a000    132K rw---   [ stack ]
	00007ffe9e855000     16K r----   [ anon ]
	00007ffe9e859000      8K r-x--   [ anon ]
	ffffffffff600000      4K --x--   [ anon ]
	 total          1051360K
	```

7. 现在在其中一些进程上运行 pmap，使用各种标志（如 -X）来显示有关该进程的许多详细信息。你看到了什么？与我们简单的代码/堆栈/堆概念相反，有多少个不同的实体构成了现代地址空间？

	除了栈和堆之外，我们还观察到 `Anonymous memory` ：与文件系统中的任何命名对象或文件无关的内存。您还可能会在输出中观察到 `vDSO` 和 `vsyscall` ，具体取决于您所使用的程序和特定系统。它们都是加速系统调用的方法。您可以在[此处](https://stackoverflow.com/questions/19938324/what-are-vdso-and-vsyscall)阅读有关这些内容的更多信息。

8. 最后，让我们在您的 `memory-user` 程序上运行 pmap，并使用不同数量的已用内存。你在这里看到什么？ pmap 的输出符合您的期望吗？

	在执行 `memory-user` 时，当我们增加堆分配时，在 `pmap` 的输出中可以看到同样的情况。



