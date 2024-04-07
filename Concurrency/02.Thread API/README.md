# Note/Translation

## 1 线程创建

编写多线程程序的第一件事就是创建新线程，因此必须有某种线程创建接口。在 POSIX 中，这很容易：

```c
#include <pthread.h>
int pthread_create(     pthread_t*              thread, 
                  const pthread_attr_t*         attr, 
                        void*                   (*start_routine)(void*),
                        void*                   arg);
```

这个函数声明有四个参数：`thread`、`attr`、`start_routine` 和 `arg`。第一个参数 `thread` 是指向 `pthread_t` 类型结构的指针；我们将使用该结构与线程交互，因此需要将其传递给 `pthread_create()` 以对其进行初始化。

第二个参数 `attr` 用于指定该线程可能具有的任何属性。例如包括设置栈大小或可能有关线程的调度优先级的信息。通过单独调用 `pthread_attr_init()` 来初始化属性；有关详细信息，请参阅手册页：`man pthread_create`。然而，在大多数情况下，默认值就可以了，在这种情况下，我们将简单地传递 NULL 值。

第三个参数是最复杂的，但实际上只是询问：这个线程应该开始在哪个函数中运行？在 C 中，我们将其称为函数指针，这告诉我们预期的内容：函数名称（`start_routine`），它传递一个类型为 `void *` 的单个参数（如`start_routine`后面的括号中所示），以及它返回一个 `void *` 类型的值（即，一个 void 指针）。如果此例程需要整数参数而不是 void 指针，则声明将如下所示：

```c
int pthread_create(..., // first two args are the same
                    void *  (*start_routine)(int),
                    int     arg);
```

如果例程的参数是一个 void 指针，但返回值是一个整数，那么就会是这样：

```c
int pthread_create(..., // first two args are the same
                    int     (*start_routine)(void *),
                    void *  arg);
```

最后，第四个参数 `arg` 正是要传递给线程开始执行的函数的参数。你可能会问：为什么我们需要这些 void 指针？答案其实很简单：<font color="red">将 void 指针作为函数`start_routine`的参数，可以让我们传递任何类型的参数</font>；将它作为返回值，可以让线程返回任何类型的结果。

还有函数的返回值，如果运行正常，则返回 0（否则为错误代码：EAGAIN、EINVAL、EPERM）。

让我们看看下面这段代码。

```c
#include <assert.h>
#include <stdio.h>

#include "common.h"
#include "common_threads.h"

typedef struct {
    int a;
    int b;
} myarg_t;

void *mythread(void *arg) {
    myarg_t *args = (myarg_t *) arg;
    printf("%d %d\n", args->a, args->b);
    return NULL;
}

int main(int argc, char *argv[]) {
    pthread_t p;
    myarg_t args = { 10, 20 };

    int rc = pthread_create(&p, NULL, mythread, &args);
    assert(rc == 0);
    (void) pthread_join(p, NULL);
    printf("done\n");
    return 0;
}
```

在这里，我们只是创建了一个线程，它传递了两个参数，并打包成我们自己定义的单一类型（`myarg t`）。线程创建后，可以简单地将其参数转换为它所期望的类型，从而根据需要解包参数。就是这样！一旦创建了线程，你就真正拥有了另一个活生生的执行实体，它拥有自己的调用栈，与程序中当前存在的所有线程运行在同一地址空间。程序的运行结果如下：

```shell
❯ make thread_create
gcc -o thread_create thread_create.c -Wall -Werror -I../include -pthread
❯ ./thread_create
10 20
done
```

## 2 等待线程完成

上面的例子展示了如何创建一个线程。但是，如果您想等待线程完成，会发生什么情况？你需要做一些特别的事情才能等待完成；特别是，您必须调用例程 `pthread_join()`。

```c
int pthread_join(pthread_t thread, void **value_ptr);
```

此例程需要两个参数。第一个参数的类型是 `pthread_t`，用于指定等待哪个线程。该变量由线程创建例程初始化（将指针作为参数传递给 `pthread create()`）；如果保留该变量，就可以用它来等待该线程终止。

第二个参数是指向你期望返回值的指针。由于该例程可以返回任何值，因此它被定义为返回 void 的指针；由于 `pthread_join() `例程会改变传入参数的值，因此你需要传入指向该值的指针，而不仅仅是该值本身。

让我们看下面这段代码，在代码中，再次创建了一个单线程，并通过 `myarg_t` 结构传递了几个参数。返回值使用 `myret_t` 类型。一旦线程运行完毕，一直在 `pthread_join() `例程 中等待的主线程就会返回，我们就可以访问从线程返回的值，即 `myret_t` 中的值。

```c
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "common_threads.h"

typedef struct {
    int a;
    int b;
} myarg_t;

typedef struct {
    int x;
    int y;
} myret_t;

void *mythread(void *arg) {
    myarg_t *args = (myarg_t *) arg;
    printf("args %d %d\n", args->a, args->b);
    myret_t *rvals = malloc(sizeof(myret_t));
    assert(rvals != NULL);
    rvals->x = 1;
    rvals->y = 2;
    return (void *) rvals;
}

int main(int argc, char *argv[]) {
    pthread_t p;
    myret_t *rvals;
    myarg_t args = { 10, 20 };
    Pthread_create(&p, NULL, mythread, &args);
    Pthread_join(p, (void **) &rvals);
    printf("returned %d %d\n", rvals->x, rvals->y);
    free(rvals);
    return 0;
}
```

关于这个例子，有几点需要注意。首先，很多时候我们不必对参数进行这些痛苦的打包和拆包。例如，如果我们只是创建一个不带参数的线程，我们可以在创建线程时将 NULL 作为参数传递进去。同样，如果我们不关心返回值，也可以将 NULL 传递给 `pthread_join()`。

其次，如果我们只传递一个值（如 int），就不必将其打包为参数。

如下面这段代码所示，在这种情况下，我们不必将参数和返回值打包到结构内部。

```c
#include <stdio.h>

#include "common.h"
#include "common_threads.h"

void *mythread(void *arg) {
    long long int value = (long long int) arg;
    printf("%lld\n", value);
    return (void *) (value + 1);
}

int main(int argc, char *argv[]) {
    pthread_t p;
    long long int rvalue;
    Pthread_create(&p, NULL, mythread, (void *) 100);
    Pthread_join(p, (void **) &rvalue);
    printf("returned %lld\n", rvalue);
    return 0;
}
```

第三，我们应该注意，从线程返回值的方式必须非常谨慎。尤其是，千万不要返回指向线程调用栈中分配的指针。如果这样做，你觉得会发生什么？(想想吧！）下面是一段危险代码的示例，它是根据上面的示例修改的。

```c
void *mythread(void *arg) {
    myarg_t *m = (myarg_t *)arg;
    printf("%d %d\n", m->a, m->b);
    myret_t r; // ALLOCATED ON STACK: BAD!
    r.x = 1;
    r.y = 2;
    return (void *)&r;
}
```

在这种情况下，变量 r 被分配到 `mythread` 的栈中。然而，当它返回时，该值会被自动解除分配（毕竟这就是栈如此易于使用的原因！），因此，将指向已解除分配的变量的指针传回会导致各种糟糕的结果。

最后，你可能会注意到，使用 `pthread_create()` 创建线程，然后立即调用 `pthread_join()` 是一种非常奇怪的创建线程的方法。事实上，有一种更简单的方法可以完成这一任务，那就是<font color="red">过程调用</font>。显然，我们通常要创建不止一个线程并等待它完成，否则使用线程就没有什么意义了。

我们应该注意，并非所有多线程代码都使用`join`例程。例如，多线程网络服务器可能会创建许多工作线程，然后使用主线程接受请求并将请求无限期地传递给工作线程。因此，这种长寿命程序可能不需要`join`。然而，创建线程执行特定任务（并行）的并行程序可能会使用 `join` 来确保在退出或进入下一阶段计算之前，所有这些工作都已完成。

## 3 锁

除了线程创建和等待线程完成之外，POSIX 线程库提供的下一组最有用的函数可能就是那些通过**锁**为临界区提供互斥的函数了。为此目的使用的最基本的一对例程如下：

```c
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
```

例程应该易于理解和使用。当您的代码区域是临界区，因此需要受到保护以确保正确操作时，锁非常有用。你大概可以想象代码的样子：

```c
pthread_mutex_t lock;
pthread_mutex_lock(&lock);
x = x + 1; // 或者不管你的临界区是什么
pthread_mutex_unlock(&lock);
```

代码的意图如下：如果调用 `pthread_mutex_lock()` 时没有其他线程持有锁，则该线程将获取锁并进入临界区。如果另一个线程确实持有锁，则尝试获取锁的线程将不会从调用中返回，直到它获得锁（这意味着持有锁的线程已通过unlock调用释放了锁）。当然，在给定时间，许多线程可能会卡在锁获取函数内等待；然而，只有获得锁的线程才应该调用`unlock`。

不幸的是，这段代码在两个重要方面被破坏了。第一个问题是<font color="red">缺乏正确的初始化</font>。所有锁都必须正确初始化，以保证它们具有正确的值，从而在调用lock和unlock时按需要工作。

对于 POSIX 线程，有两种初始化锁的方法。一种方法是使用 `PTHREAD_MUTEX_INITIALIZER`，如下所示：

```c
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
```

这样做会将锁设置为默认值，从而使锁可用。动态方法（即在运行时）是调用 `pthread_mutex_init()`，如下所示：

```c
int rc = pthread_mutex_init(&lock, NULL);
assert(rc == 0 && “Error in mutex init”);
```

该例程的第一个参数是锁本身的地址，第二个参数是一组可选属性。传递 NULL 即只需使用默认值即可。两种方法都可以，但我们通常使用动态（后一种）方法。需要注意的是，在使用完锁后，还需要调用 `pthread_mutex_destroy()`。

上述代码的第二个问题是，它在调用lock和unlock时没有检查错误代码。就像你在 UNIX 系统中调用的几乎所有库例程一样，这些例程也可能失败！如果你的代码没有正确检查错误代码，失败就会无声无息地发生，在这种情况下，可能会允许多个线程进入临界区段。在最低限度上，应使用包装器来断言例程成功（如下面这段代码所示）；更复杂的（非玩具）程序在出错时不能简单地退出，而应检查失败，并在lock或unlock不成功时采取适当的措施。

```c
// Use this to keep your code clean but check for failures
// Only use if exiting program is OK upon failure
void Pthread_mutex_lock(pthread_mutex_t *mutex) {
    int rc = pthread_mutex_lock(mutex);
    assert(rc == 0 && “Error in acquire”);
}
```

lock和unlock例程并不是 `pthreads` 库中与锁交互的唯一例程。这里还有两个例程可能值得关注：

```c
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_timedlock(pthread_mutex_t *mutex,
                            struct timespec *abs_timeout);
```

这两个调用用于获取锁。如果锁已被持有，`trylock` 版本会返回失败；获取锁的 `timedlock` 版本会在超时或获取锁后返回，以先发生者为准。因此，超时后的 `timedlock` 会退化为 `trylock`。一般来说，这两种情况都应该避免；不过，在某些情况下，避免卡在（也许是无限期地）锁获取例程中是有用的。

## 4 条件变量

任何线程库的另一个主要组成部分，当然也包括 POSIX 线程，就是**条件变量**的存在。<font color="red">当线程之间必须进行某种信号传递时，如果一个线程正在等待另一个线程做某事，然后才能继续，那么条件变量就非常有用</font>。希望以这种方式进行交互的程序主要使用两个例程：

```c
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_signal(pthread_cond_t *cond);
```

要使用条件变量，还必须拥有与该条件关联的锁。当调用上述任一例程时，应保持此锁。

第一个例程 `pthread_cond_wait()` 使调用线程进入睡眠状态，从而等待其他线程向其发出信号，通常是在程序中的某些内容发生更改而现在正在睡眠的线程可能关心的情况下。典型的用法如下：

```c
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

Pthread_mutex_lock(&lock);
while (ready == 0)
    Pthread_cond_wait(&cond, &lock);
Pthread_mutex_unlock(&lock);
```

在此代码中，在初始化相关锁和条件之后，线程检查变量`ready`是否已设置为非零的值。如果没有，该线程只需调用等待例程即可休眠，直到其他线程将其唤醒。唤醒一个线程的代码如下所示，该代码将在其他线程中运行：

```c
Pthread_mutex_lock(&lock);
ready = 1;
Pthread_cond_signal(&cond);
Pthread_mutex_unlock(&lock);
```

关于这个代码序列，有几点需要注意。首先，在发送信号时（以及修改全局变量 `ready` 时），我们始终要确保`lock`。这样可以确保我们的代码不会意外引入竞争条件。

其次，你可能会注意到，`wait` 调用的第二个参数是锁，而 `signal` 调用只需要一个条件。造成这种差异的原因是，wait 调用除了让调用线程休眠外，还会在让调用者休眠时释放锁。试想一下，如果不这样做，其他线程怎么可能获得锁并发出信号唤醒它呢？不过，在被唤醒后返回之前，`pthread_cond_wait()` 会重新获取锁，从而确保在等待序列开始时获取锁和结束时释放锁之间的任何时间，等待线程都持有锁。

最后一个奇怪的现象：等待线程在 while 循环中重新检查条件，而不是简单的 if 语句。因为使用 while 循环是简单安全的做法。虽然它会重新检查条件（可能会增加一点开销），但有些 pthread 实现可能会错误地唤醒等待线程；在这种情况下，如果不重新检查，等待线程就会继续认为条件改变，即使它并没有改变。例如，如果有多个线程在等待，而只有一个线程应该抓取数据（生产者-消费者）。因此，更安全的做法是将唤醒视为可能已发生变化的提示，而不是绝对的事实。

需要注意的是，有时在两个线程之间使用一个简单的标志来发出信号，而不是使用条件变量和相关的锁，这很有诱惑力。例如，我们可以重写上面的等待代码，在等待代码中看起来更像这样：

```c
while (ready == 0)
    ; // spin
```

相关的信号代码如下：

```c
ready = 1;
```

永远不要这样做，原因如下。首先，它在很多情况下表现不佳（长时间自旋，即持续检查某个条件是否满足，这只会浪费 CPU 周期）。其次，容易出错。使用标志（如上所述）在线程之间进行同步时非常容易出错。

## 5 线程API指南

当你使用POSIX线程库（或者实际上，任何线程库）构建一个多线程程序时，有一些小但重要的事情需要记住。它们包括：

- **保持简单。**最重要的是，任何涉及线程之间的锁定或信号的代码都应尽可能简单。复杂的线程交互会导致错误。
- **最小化线程交互。**尽量减少线程之间交互的方式。每个交互都应该经过深思熟虑，并用经过验证的方法构建。
- **初始化锁和条件变量。**未初始化将导致代码有时能够正常工作，有时会以非常奇怪的方式失败。
- **检查返回码。**当然，在你所做的任何C和UNIX编程中，你都应该检查每一个返回码，这在这里也是正确的。不这样做将导致奇怪且难以理解的行为。
- **在传递参数给线程和从线程返回值时要小心。**特别是，任何时候你传递指向栈上分配的变量的引用时，你可能在做一些错误的事情。
- **每个线程都有自己的栈。**与上面的观点相关，请记住每个线程都有自己的栈。因此，如果你在某个线程执行的函数中有一个在本地分配的变量，它基本上是私有的，其他线程无法（轻易）访问它。要在线程之间共享数据，这些值必须在堆上或者以其他全局可访问的位置。
- **总是使用条件变量来在线程之间进行信号传递。**虽然使用简单的标志往往很诱人，但不要这样做。
- **使用手册页面。**特别是在Linux上，pthread手册页面非常有信息量。

# Program Explanation

工具：`helgrind`，网址：[https://valgrind.org/docs/manual/manual.html](https://valgrind.org/docs/manual/manual.html)

首先要做的事情是：下载并安装 `valgrind` 和相关的 `helgrind` 工具。

然后，您将查看许多多线程 C 程序，了解如何使用该工具来调试有问题的线程代码。输入 `make` 来构建所有不同的程序。检查 `Makefile` 以获取有关其工作原理的更多详细信息。

然后，您可以查看一些不同的 C 程序：

* `thread_create.c`：简单的线程创建示例
* `thread_create_with_return_args.c`：获取创建线程的返回值示例
* `thread_create_simple_args.c`：使用简单类型作为线程参数

- `main-race.c` ：一个简单的竞争条件
- `main-deadlock.c` ：一个简单的死锁
- `main-deadlock-global.c` ：死锁问题的解决方案
- `main-signal.c` ：一个简单的子/父信号示例
- `main-signal-cv.c` ：通过条件变量发出更有效的信号
- `../include/common_threads.h` ：带有包装器的头文件，使代码检查错误并更具可读性

# Homework

1. 首先构建 `main-race.c`。检查代码，以便您可以看到代码中的（希望是明显的）数据竞争。现在运行 `helgrind`（通过输入 `valgrind --tool=helgrind main-race`）来查看它如何报告竞争。它是否指向正确的代码行？它还为您提供了哪些其他信息？

	```shell
	> valgrind --tool=helgrind ./main-race
	==160518== Helgrind, a thread error detector
	==160518== Copyright (C) 2007-2017, and GNU GPL'd, by OpenWorks LLP et al.
	==160518== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
	==160518== Command: ./main-race
	==160518==
	==160518== ---Thread-Announcement------------------------------------------
	==160518==
	==160518== Thread #1 is the program's root thread
	==160518==
	==160518== ---Thread-Announcement------------------------------------------
	==160518==
	==160518== Thread #2 was created
	==160518==    at 0x49929F3: clone (clone.S:76)
	==160518==    by 0x49938EE: __clone_internal (clone-internal.c:83)
	==160518==    by 0x49016D8: create_thread (pthread_create.c:295)
	==160518==    by 0x49021FF: pthread_create@@GLIBC_2.34 (pthread_create.c:828)
	==160518==    by 0x4853767: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==160518==    by 0x109209: main (in /home/zfhe/ostep/Concurrency/02.Thread API/main-race)
	==160518==
	==160518== ----------------------------------------------------------------
	==160518==
	==160518== Possible data race during read of size 4 at 0x10C014 by thread #1
	==160518== Locks held: none
	==160518==    at 0x109236: main (in /home/zfhe/ostep/Concurrency/02.Thread API/main-race)
	==160518==
	==160518== This conflicts with a previous write of size 4 by thread #2
	==160518== Locks held: none
	==160518==    at 0x1091BE: worker (in /home/zfhe/ostep/Concurrency/02.Thread API/main-race)
	==160518==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==160518==    by 0x4901AC2: start_thread (pthread_create.c:442)
	==160518==    by 0x4992A03: clone (clone.S:100)
	==160518==  Address 0x10c014 is 0 bytes inside data symbol "balance"
	==160518==
	==160518== ----------------------------------------------------------------
	==160518==
	==160518== Possible data race during write of size 4 at 0x10C014 by thread #1
	==160518== Locks held: none
	==160518==    at 0x10923F: main (in /home/zfhe/ostep/Concurrency/02.Thread API/main-race)
	==160518==
	==160518== This conflicts with a previous write of size 4 by thread #2
	==160518== Locks held: none
	==160518==    at 0x1091BE: worker (in /home/zfhe/ostep/Concurrency/02.Thread API/main-race)
	==160518==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==160518==    by 0x4901AC2: start_thread (pthread_create.c:442)
	==160518==    by 0x4992A03: clone (clone.S:100)
	==160518==  Address 0x10c014 is 0 bytes inside data symbol "balance"
	==160518==
	==160518==
	==160518== Use --history-level=approx or =none to gain increased speed, at
	==160518== the cost of reduced accuracy of conflicting-access information
	==160518== For lists of detected and suppressed errors, rerun with: -s
	==160518== ERROR SUMMARY: 2 errors from 2 contexts (suppressed: 0 from 0)
	```

	是的，它指出了以下情况：

	1. 在线程 #1 中，对地址 0x10c014 的大小为 4 的数据进行了写操作。
	2. 在线程 #2 中，对地址 0x10c014 的大小为 4 的数据进行了写操作。

	这可能会导致数据竞争。同时，它还提供锁的持有情况等信息。

2. 当您删除其中一行有问题的代码时会发生什么？现在，在共享变量的其中一个更新周围添加一个锁，然后在两个更新周围添加一个锁。 `helgrind` 在每个案例中报告了什么？

	1. 删除一行有问题的代码，则不会出现数据竞争问题，没有错误。

	2. 只添加一个锁仍然会导致数据竞争，报告如下：

		```shell
		> valgrind --tool=helgrind ./main-race
		==161849== Helgrind, a thread error detector
		==161849== Copyright (C) 2007-2017, and GNU GPL'd, by OpenWorks LLP et al.
		==161849== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
		==161849== Command: ./main-race
		==161849==
		==161849== ---Thread-Announcement------------------------------------------
		==161849==
		==161849== Thread #1 is the program's root thread
		==161849==
		==161849== ---Thread-Announcement------------------------------------------
		==161849==
		==161849== Thread #2 was created
		==161849==    at 0x49929F3: clone (clone.S:76)
		==161849==    by 0x49938EE: __clone_internal (clone-internal.c:83)
		==161849==    by 0x49016D8: create_thread (pthread_create.c:295)
		==161849==    by 0x49021FF: pthread_create@@GLIBC_2.34 (pthread_create.c:828)
		==161849==    by 0x4853767: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
		==161849==    by 0x10926B: main (in /home/zfhe/ostep/Concurrency/02.Thread API/main-race)
		==161849==
		==161849== ----------------------------------------------------------------
		==161849==
		==161849==  Lock at 0x10C060 was first observed
		==161849==    at 0x4850CCF: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
		==161849==    by 0x109207: worker (in /home/zfhe/ostep/Concurrency/02.Thread API/main-race)
		==161849==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
		==161849==    by 0x4901AC2: start_thread (pthread_create.c:442)
		==161849==    by 0x4992A03: clone (clone.S:100)
		==161849==  Address 0x10c060 is 0 bytes inside data symbol "mutex"
		==161849==
		==161849== Possible data race during read of size 4 at 0x10C040 by thread #1
		==161849== Locks held: none
		==161849==    at 0x109298: main (in /home/zfhe/ostep/Concurrency/02.Thread API/main-race)
		==161849==
		==161849== This conflicts with a previous write of size 4 by thread #2
		==161849== Locks held: 1, at address 0x10C060
		==161849==    at 0x109211: worker (in /home/zfhe/ostep/Concurrency/02.Thread API/main-race)
		==161849==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
		==161849==    by 0x4901AC2: start_thread (pthread_create.c:442)
		==161849==    by 0x4992A03: clone (clone.S:100)
		==161849==  Address 0x10c040 is 0 bytes inside data symbol "balance"
		==161849==
		==161849== ----------------------------------------------------------------
		==161849==
		==161849==  Lock at 0x10C060 was first observed
		==161849==    at 0x4850CCF: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
		==161849==    by 0x109207: worker (in /home/zfhe/ostep/Concurrency/02.Thread API/main-race)
		==161849==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
		==161849==    by 0x4901AC2: start_thread (pthread_create.c:442)
		==161849==    by 0x4992A03: clone (clone.S:100)
		==161849==  Address 0x10c060 is 0 bytes inside data symbol "mutex"
		==161849==
		==161849== Possible data race during write of size 4 at 0x10C040 by thread #1
		==161849== Locks held: none
		==161849==    at 0x1092A1: main (in /home/zfhe/ostep/Concurrency/02.Thread API/main-race)
		==161849==
		==161849== This conflicts with a previous write of size 4 by thread #2
		==161849== Locks held: 1, at address 0x10C060
		==161849==    at 0x109211: worker (in /home/zfhe/ostep/Concurrency/02.Thread API/main-race)
		==161849==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
		==161849==    by 0x4901AC2: start_thread (pthread_create.c:442)
		==161849==    by 0x4992A03: clone (clone.S:100)
		==161849==  Address 0x10c040 is 0 bytes inside data symbol "balance"
		==161849==
		==161849==
		==161849== Use --history-level=approx or =none to gain increased speed, at
		==161849== the cost of reduced accuracy of conflicting-access information
		==161849== For lists of detected and suppressed errors, rerun with: -s
		==161849== ERROR SUMMARY: 2 errors from 2 contexts (suppressed: 0 from 0)
		```

		- 在地址 0x10c060 处检测到了一个锁的使用。
		- 在线程 #1 中，对地址 0x10c040 处的数据进行了写操作，但没有持有锁。
		- 在线程 #2 中，对地址 0x10c040 处的数据进行了写操作，并且持有了相同的锁（地址 0x10c060 处）。

		这表明你的程序在对共享数据 `balance` 进行读写时使用了锁，但是在读写数据时没有正确地持有锁。

	3. 两个都使用了锁，避免了数据竞争，则没有错误。

3. 现在让我们看看 main-deadlock.c。检查代码。这段代码存在一个称为**死锁**的问题。你能看出它可能有什么问题吗？

	当两个线程都拿到了另一个锁，则导致一直在尝试获取另一个锁。导致死锁。

4. 现在在此代码上运行 `helgrind`。 `helgrind` 报告什么？

	```shell
	> valgrind --tool=helgrind ./main-deadlock
	==163534== Helgrind, a thread error detector
	==163534== Copyright (C) 2007-2017, and GNU GPL'd, by OpenWorks LLP et al.
	==163534== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
	==163534== Command: ./main-deadlock
	==163534==
	==163534== ---Thread-Announcement------------------------------------------
	==163534==
	==163534== Thread #3 was created
	==163534==    at 0x49929F3: clone (clone.S:76)
	==163534==    by 0x49938EE: __clone_internal (clone-internal.c:83)
	==163534==    by 0x49016D8: create_thread (pthread_create.c:295)
	==163534==    by 0x49021FF: pthread_create@@GLIBC_2.34 (pthread_create.c:828)
	==163534==    by 0x4853767: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==163534==    by 0x1093F4: main (in /home/zfhe/main-deadlock)
	==163534==
	==163534== ----------------------------------------------------------------
	==163534==
	==163534== Thread #3: lock order "0x10C040 before 0x10C080" violated
	==163534==
	==163534== Observed (incorrect) order is: acquisition of lock at 0x10C080
	==163534==    at 0x4850CCF: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==163534==    by 0x109288: worker (in /home/zfhe/main-deadlock)
	==163534==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==163534==    by 0x4901AC2: start_thread (pthread_create.c:442)
	==163534==    by 0x4992A03: clone (clone.S:100)
	==163534==
	==163534==  followed by a later acquisition of lock at 0x10C040
	==163534==    at 0x4850CCF: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==163534==    by 0x1092C3: worker (in /home/zfhe/main-deadlock)
	==163534==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==163534==    by 0x4901AC2: start_thread (pthread_create.c:442)
	==163534==    by 0x4992A03: clone (clone.S:100)
	==163534==
	==163534== Required order was established by acquisition of lock at 0x10C040
	==163534==    at 0x4850CCF: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==163534==    by 0x10920E: worker (in /home/zfhe/main-deadlock)
	==163534==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==163534==    by 0x4901AC2: start_thread (pthread_create.c:442)
	==163534==    by 0x4992A03: clone (clone.S:100)
	==163534==
	==163534==  followed by a later acquisition of lock at 0x10C080
	==163534==    at 0x4850CCF: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==163534==    by 0x109249: worker (in /home/zfhe/main-deadlock)
	==163534==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==163534==    by 0x4901AC2: start_thread (pthread_create.c:442)
	==163534==    by 0x4992A03: clone (clone.S:100)
	==163534==
	==163534==  Lock at 0x10C040 was first observed
	==163534==    at 0x4850CCF: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==163534==    by 0x10920E: worker (in /home/zfhe/main-deadlock)
	==163534==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==163534==    by 0x4901AC2: start_thread (pthread_create.c:442)
	==163534==    by 0x4992A03: clone (clone.S:100)
	==163534==  Address 0x10c040 is 0 bytes inside data symbol "m1"
	==163534==
	==163534==  Lock at 0x10C080 was first observed
	==163534==    at 0x4850CCF: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==163534==    by 0x109249: worker (in /home/zfhe/main-deadlock)
	==163534==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==163534==    by 0x4901AC2: start_thread (pthread_create.c:442)
	==163534==    by 0x4992A03: clone (clone.S:100)
	==163534==  Address 0x10c080 is 0 bytes inside data symbol "m2"
	==163534==
	==163534==
	==163534==
	==163534== Use --history-level=approx or =none to gain increased speed, at
	==163534== the cost of reduced accuracy of conflicting-access information
	==163534== For lists of detected and suppressed errors, rerun with: -s
	==163534== ERROR SUMMARY: 1 errors from 1 contexts (suppressed: 7 from 7)
	```

	1. Helgrind 检测到了一个锁顺序违规，即在线程 #3 中发生了锁顺序违规，先后获取了两个锁。
	2. 锁顺序违规的情况如下：
		- 线程 #3 首先获取了地址为 0x10C080 的锁。
		- 然后，线程 #3 又获取了地址为 0x10C040 的锁。
	3. 需要的正确的锁顺序应该是：
		- 首先获取地址为 0x10C040 的锁。
		- 然后，获取地址为 0x10C080 的锁。

	修改锁的获取顺序后则没有错误。

5. 现在在 `main-deadlock-global.c` 上运行 `helgrind`。检查代码；它有与 `main-deadlock.c` 相同的问题吗？ `helgrind` 应该报告同样的错误吗？对于像 helgrind 这样的工具，这告诉您什么？

	没有相同的问题，因为`g`锁的存在，只有一个线程能够进入`worker`函数，并且在任何情况下都无法同时持有`m1`和`m2`锁，因此不会发生死锁。而`helgrind`会报告同样的错误，这告诉我们即使相对简单的案例，`helgrind`也会误报，这并不完美。

6. 接下来让我们看看 `main-signal.c`。这段代码使用一个变量（done）来发出信号，表示子线程已经完成，主线程现在可以继续。为什么这段代码效率很低？(特别是如果子线程需要很长时间才能完成，父线程最终要花时间做什么？）

	因为主线程陷入了循环并且什么也不做。

7. 现在在此程序上运行 `helgrind`。它报告什么？代码正确吗？

	```shell
	> valgrind --tool=helgrind ./main-signal
	==168132== Helgrind, a thread error detector
	==168132== Copyright (C) 2007-2017, and GNU GPL'd, by OpenWorks LLP et al.
	==168132== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
	==168132== Command: ./main-signal
	==168132==
	this should print first
	==168132== ---Thread-Announcement------------------------------------------
	==168132==
	==168132== Thread #2 was created
	==168132==    at 0x49929F3: clone (clone.S:76)
	==168132==    by 0x49938EE: __clone_internal (clone-internal.c:83)
	==168132==    by 0x49016D8: create_thread (pthread_create.c:295)
	==168132==    by 0x49021FF: pthread_create@@GLIBC_2.34 (pthread_create.c:828)
	==168132==    by 0x4853767: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==168132==    by 0x109217: main (in /home/zfhe/main-signal)
	==168132==
	==168132== ---Thread-Announcement------------------------------------------
	==168132==
	==168132== Thread #1 is the program's root thread
	==168132==
	==168132== ----------------------------------------------------------------
	==168132==
	==168132== Possible data race during write of size 4 at 0x10C014 by thread #2
	==168132== Locks held: none
	==168132==    at 0x1091C8: worker (in /home/zfhe/main-signal)
	==168132==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==168132==    by 0x4901AC2: start_thread (pthread_create.c:442)
	==168132==    by 0x4992A03: clone (clone.S:100)
	==168132==
	==168132== This conflicts with a previous read of size 4 by thread #1
	==168132== Locks held: none
	==168132==    at 0x109245: main (in /home/zfhe/main-signal)
	==168132==  Address 0x10c014 is 0 bytes inside data symbol "done"
	==168132==
	==168132== ----------------------------------------------------------------
	==168132==
	==168132== Possible data race during read of size 4 at 0x10C014 by thread #1
	==168132== Locks held: none
	==168132==    at 0x109245: main (in /home/zfhe/main-signal)
	==168132==
	==168132== This conflicts with a previous write of size 4 by thread #2
	==168132== Locks held: none
	==168132==    at 0x1091C8: worker (in /home/zfhe/main-signal)
	==168132==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==168132==    by 0x4901AC2: start_thread (pthread_create.c:442)
	==168132==    by 0x4992A03: clone (clone.S:100)
	==168132==  Address 0x10c014 is 0 bytes inside data symbol "done"
	==168132==
	==168132== ----------------------------------------------------------------
	==168132==
	==168132== Possible data race during write of size 1 at 0x529A1A5 by thread #1
	==168132== Locks held: none
	==168132==    at 0x4859796: mempcpy (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==168132==    by 0x48F8664: _IO_new_file_xsputn (fileops.c:1235)
	==168132==    by 0x48F8664: _IO_file_xsputn@@GLIBC_2.2.5 (fileops.c:1196)
	==168132==    by 0x48EDF1B: puts (ioputs.c:40)
	==168132==    by 0x10925D: main (in /home/zfhe/main-signal)
	==168132==  Address 0x529a1a5 is 21 bytes inside a block of size 1,024 alloc'd
	==168132==    at 0x484A919: malloc (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==168132==    by 0x48EBBA3: _IO_file_doallocate (filedoalloc.c:101)
	==168132==    by 0x48FACDF: _IO_doallocbuf (genops.c:347)
	==168132==    by 0x48F9F5F: _IO_file_overflow@@GLIBC_2.2.5 (fileops.c:744)
	==168132==    by 0x48F86D4: _IO_new_file_xsputn (fileops.c:1243)
	==168132==    by 0x48F86D4: _IO_file_xsputn@@GLIBC_2.2.5 (fileops.c:1196)
	==168132==    by 0x48EDF1B: puts (ioputs.c:40)
	==168132==    by 0x1091C7: worker (in /home/zfhe/main-signal)
	==168132==    by 0x485396A: ??? (in /usr/libexec/valgrind/vgpreload_helgrind-amd64-linux.so)
	==168132==    by 0x4901AC2: start_thread (pthread_create.c:442)
	==168132==    by 0x4992A03: clone (clone.S:100)
	==168132==  Block was alloc'd by thread #2
	==168132==
	this should print last
	==168132==
	==168132== Use --history-level=approx or =none to gain increased speed, at
	==168132== the cost of reduced accuracy of conflicting-access information
	==168132== For lists of detected and suppressed errors, rerun with: -s
	==168132== ERROR SUMMARY: 24 errors from 3 contexts (suppressed: 40 from 34)
	```

	代码不正确。报告数据争用和线程泄漏（主线程不调用 `Pthread_join()` ）。 `Helgrind` 是正确的（存在数据竞争），但它不会影响程序的正确性，假设指令没有重新排序，那么能在打印到标准输出之前设置 `done` 。

8. 现在看一下代码的稍微修改版本，该版本位于 `main-signal-cv.c` 中。此版本使用条件变量来执行信号发送（以及关联的锁定）。为什么此代码优于以前的版本？是正确性，还是性能，还是两者兼而有之？

	```shell
	> valgrind --tool=helgrind ./main-signal-cv
	==168655== Helgrind, a thread error detector
	==168655== Copyright (C) 2007-2017, and GNU GPL'd, by OpenWorks LLP et al.
	==168655== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
	==168655== Command: ./main-signal-cv
	==168655==
	this should print first
	this should print last
	==168655==
	==168655== Use --history-level=approx or =none to gain increased speed, at
	==168655== the cost of reduced accuracy of conflicting-access information
	==168655== For lists of detected and suppressed errors, rerun with: -s
	==168655== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 12 from 12)
	```

	因为它在使用条件变量进行线程同步时更加标准和可靠。

	1. **正确性问题**：
		- 在 `signal_done` 函数中，使用了条件变量 `pthread_cond_signal` 来通知等待线程，确保等待线程在条件满足时被唤醒，而不是简单地使用忙等待来轮询条件。
		- 在 `signal_wait` 函数中，使用了条件变量 `pthread_cond_wait` 来等待条件满足，这样等待线程可以在条件变量上原子地释放锁并等待唤醒，从而避免了死锁和忙等待。
	2. **性能问题**：
		- 相比之前版本的简单忙等待，使用条件变量进行线程同步可以减少CPU资源的浪费。在忙等待的情况下，线程会持续地占用CPU时间片来检查条件是否满足，而在条件变量中，线程在条件不满足时会进入阻塞状态，不会占用CPU资源，直到被唤醒。
		- 使用条件变量可以更有效地利用系统资源，因为在条件不满足时，线程会自动释放锁并进入睡眠状态，直到条件满足时才会被唤醒重新获取锁继续执行。

9. 再次在 `main-signal-cv` 上运行 `helgrind`。它会报告任何错误吗？

	不会。

	```shell
	> gcc common_threads.h main-signal-cv.c -lpthread -o main-signal-cv
	> valgrind --tool=helgrind ./main-signal-cv
	==168655== Helgrind, a thread error detector
	==168655== Copyright (C) 2007-2017, and GNU GPL'd, by OpenWorks LLP et al.
	==168655== Using Valgrind-3.18.1 and LibVEX; rerun with -h for copyright info
	==168655== Command: ./main-signal-cv
	==168655==
	this should print first
	this should print last
	==168655==
	==168655== Use --history-level=approx or =none to gain increased speed, at
	==168655== the cost of reduced accuracy of conflicting-access information
	==168655== For lists of detected and suppressed errors, rerun with: -s
	==168655== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 12 from 12)
	```

	