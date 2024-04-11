# Overview

这个作业让你尝试多种方法在 C 中实现一个小的、无死锁的向量对象。向量对象非常有限（例如，它只有 `add()` 和 `init()` 函数），但仅用于说明避免死锁的不同方法。

您应该注意的一些文件如下。特别是，本作业中的所有变体都使用了它们。

* `common_threads.h` ：许多不同的 pthread（和其他）库调用的常用包装器，以确保它们不会默默地失败；
* `vector-header.h` ：向量例程的简单头文件，主要定义固定向量大小，然后定义每个向量使用的结构体 (`vector_t`)；
* `main-header.h` ：每个不同程序共有的一些全局变量；
* `main-common.c` ：包含 `main()` 例程（带有 `arg` 解析），该例程初始化两个向量，启动一些线程来访问它们（通过 `worker()` 例程），然后等待许多 `vector_add()`去完成。

本作业的变体可以在以下文件中找到。每个都采用不同的方法来处理名为 `vector_add()` 的“向量加法”例程内的并发性；检查这些文件中的代码以了解发生了什么。他们都是利用上面的文件来制作一个完整的可运行的程序。

相关文件：

* `vector-deadlock.c` ：这个版本愉快地按特定顺序获取锁（dst 然后 src）。通过这样做，它会创建一个“死锁邀请”，因为一个线程可能调用 `vector_add(v1, v2)` 而另一个线程同时调用 `vector_add(v2, v1)` 。
* `vector-global-order.c` ：此版本的 `vector_add()` 根据向量的地址以全序方式获取锁。
* `vector-try-wait.c` ：此版本的 `vector_add()` 使用 `pthread_mutex_trylock()` 尝试获取锁；当尝试失败时，代码会释放它可能持有的所有锁，并返回到顶部并再次尝试。
* `vector-avoid-hold-and-wait.c` ：此版本通过在锁获取周围使用单个锁来确保它不会陷入保持和等待模式。
* `vector-nolock.c` ：这个版本甚至不使用锁；相反，它使用原子获取和添加来实现 `vector_add()` 例程。它的语义（结果）略有不同。

键入 `make` （并读取 `Makefile` ）以构建五个可执行文件中的每一个。然后您只需输入程序名称即可运行该程序：

```shell
> make
./vector-deadlock
```

每个程序都采用相同的参数集（有关详细信息，请参阅 main-common.c）：

* `-d` ：该标志开启线程死锁的能力。当您将 `-d` 传递给程序时，每个其他线程都会以不同的顺序调用带有向量的 `vector_add()` ，例如，使用两个线程，并且启用 `-d` ，Thread 0 调用 `vector_add(v1, v2)` ，线程 1 调用 `vector_add(v2, v1)`。
* `-p` ：此标志为每个线程提供一组不同的向量来调用 add，而不仅仅是两个向量。使用它来查看当不存在对同一组向量的争用时事情的执行情况。
* `-n num_threads` ：创建一定数量的线程；你需要不止一个才能陷入死锁。
* `-l loops` ：每个线程应该调用add多少次？
* `-v` ：详细标志：打印出更多有关正在发生的事情的信息。
* `-t` ：打开计时并显示一切花费的时间。

# Homework

1. 首先，让我们确保您了解这些程序的一般工作原理以及一些关键选项。研究 `vector-deadlock.c` 以及 `main-common.c` 和相关文件中的代码。

	现在，运行 `./vector-deadlock -n 2 -l 1 -v` ，它实例化两个线程 ( `-n 2` )，每个线程执行一次向量加法 ( `-l 1` )，并以详细模式执行此操作 ( `-v`），确保您理解输出。每次运行时输出如何变化？

	线程的顺序可能会改变。

2. 现在添加 `-d` 标志，并将循环数 ( `-l` ) 从 1 更改为更高的数字。会发生什么？代码（总是）死锁吗？

	```shell
	$ ./vector-deadlock -n 2 -l 100000 -v -d
	```

	并非总是如此，但有时如此。

3. 更改线程数 ( `-n` ) 如何改变程序的结果？是否有任何 `-n` 值可以确保不会发生死锁？

	```c
	$ ./vector-deadlock -n 1 -l 100000 -v -d
	$ ./vector-deadlock -n 0 -l 100000 -v -d
	```

4. 现在检查 `vector-global-order.c` 中的代码。首先，确保你理解代码想要做什么；你明白为什么代码可以避免死锁吗？另外，当源向量和目标向量相同时，为什么此 `vector_add()` 例程中存在特殊情况？

	因为破坏了循环等待的条件。但当源向量和目标向量相同时，`v_src->lock`被锁了两次。

5. 现在使用以下标志运行代码： `-t -n 2 -l 100000 -d` 。代码需要多长时间才能完成？当增加循环数或线程数时，总时间有何变化？

	```shell
	❯ ./vector-global-order -t -n 2 -l 100000 -d
	Time: 0.06 seconds
	❯ ./vector-global-order -t -n 2 -l 200000 -d
	Time: 0.09 seconds
	❯ ./vector-global-order -t -n 4 -l 100000 -d
	Time: 0.25 seconds
	❯ ./vector-global-order -t -n 4 -l 200000 -d
	Time: 0.56 seconds
	```

6. 如果打开并行标志 ( `-p` ) 会发生什么？当每个线程添加不同的向量（这是 `-p` 所支持的）与处理相同的向量时，您预计性能会有多大变化？

	现在线程不需要等待相同的两个锁。

	```shell
	❯ ./vector-global-order -t -n 2 -l 100000 -d -p
	Time: 0.02 seconds
	❯ ./vector-global-order -t -n 2 -l 200000 -d -p
	Time: 0.04 seconds
	❯ ./vector-global-order -t -n 4 -l 100000 -d -p
	Time: 0.03 seconds
	❯ ./vector-global-order -t -n 4 -l 200000 -d -p
	Time: 0.04 seconds
	```

7. 现在让我们研究一下 `vector-try-wait.c` 。首先确保您理解代码。真的需要第一次调用 `pthread_mutex_trylock()` 吗？现在运行代码。与全局顺序方法相比，它的运行速度有多快？代码计算的重试次数如何随着线程数量的增加而变化？

	不需要。

	```shell
	❯ ./vector-try-wait -t -n 2 -l 100000 -d
	Retries: 626096
	Time: 0.09 seconds
	❯ ./vector-try-wait -t -n 2 -l 200000 -d
	Retries: 324592
	Time: 0.10 seconds
	❯ ./vector-try-wait -t -n 4 -l 100000 -d
	Retries: 54243984
	Time: 4.23 seconds
	❯ ./vector-try-wait -t -n 4 -l 200000 -d
	Retries: 124152627
	Time: 9.37 seconds
	```

	比全局顺序方法慢。重试次数随着线程数量的增加而变化。

8. 现在让我们看看 `vector-avoid-hold-and-wait.c` 。这种方法的主要问题是什么？在使用 -p 和不使用 -p 的情况下运行时，其性能与其他版本相比如何？

	一次只允许一个线程添加。

	```shell
	❯ ./vector-avoid-hold-and-wait -t -n 2 -l 100000 -d
	Time: 0.08 seconds
	❯ ./vector-avoid-hold-and-wait -t -n 2 -l 200000 -d
	Time: 0.17 seconds
	❯ ./vector-avoid-hold-and-wait -t -n 4 -l 100000 -d
	Time: 0.30 seconds
	❯ ./vector-avoid-hold-and-wait -t -n 4 -l 200000 -d
	Time: 0.81 seconds
	❯ ./vector-avoid-hold-and-wait -t -n 2 -l 100000 -d -p
	Time: 0.04 seconds
	❯ ./vector-avoid-hold-and-wait -t -n 2 -l 200000 -d -p
	Time: 0.06 seconds
	❯ ./vector-avoid-hold-and-wait -t -n 4 -l 100000 -d -p
	Time: 0.07 seconds
	❯ ./vector-avoid-hold-and-wait -t -n 4 -l 200000 -d -p
	Time: 0.12 seconds
	```

9. 最后，让我们看一下 `vector-nolock.c` 。这个版本根本不使用锁；它提供与其他版本完全相同的语义吗？为什么或者为什么不？

	它使用的是`fetch-and-add`而不是锁。

10. 现在将其性能与其他版本进行比较，无论是当线程处理相同的两个向量（无 `-p` ）时还是每个线程处理单独的向量（ `-p` ）时。这个无锁版本表现如何？

	```shell
	$ ./vector-nolock -t -n 2 -l 100000 -d
	Time: 0.39 seconds
	
	$ ./vector-nolock -t -n 2 -l 200000 -d
	Time: 0.83 seconds
	
	$ ./vector-nolock -t -n 4 -l 50000 -d
	Time: 0.34 seconds
	
	$ ./vector-nolock -t -n 2 -l 100000 -d -p
	Time: 0.06 seconds
	
	$ ./vector-nolock -t -n 2 -l 200000 -d -p
	Time: 0.11 seconds
	
	$ ./vector-nolock -t -n 4 -l 50000 -d -p
	Time: 0.04 seconds
	```

	

