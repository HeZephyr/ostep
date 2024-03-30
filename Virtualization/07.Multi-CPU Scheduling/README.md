# Note/Translation

## 1 多处理器架构

要理解围绕多处理器调度的新问题，我们必须了解单 CPU 硬件与多 CPU 硬件之间的一个新的根本区别。这种区别主要围绕硬件缓存的使用（如下图所示），以及多个处理器之间共享数据的具体方式。

<img src="https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Single-CPU-With-Cache" alt="image-20240328214905850" style="zoom:50%;" />

在单个CPU系统中，<font color="red">硬件缓存（Cache）</font>的层次结构通常有助于处理器更快地运行程序。`Cache`是一种小型快速存储器，（一般来说）保存系统主存储器中常用数据的副本。相比之下，主存储器保存了所有数据，但访问这个较大的内存时速度较慢。通过将频繁访问的数据保存在高速缓存中，系统可以让又大又慢的内存看起来很快。

例如，一个程序要从内存中获取一个值，需要发出一条显式加载指令，而一个简单的系统只有一个 CPU；CPU 有一个小缓存（比如 64 KB）和一个大主存。程序首次发出加载指令时，数据位于主内存中，因此需要很长时间（可能是几十纳秒，甚至几百纳秒）才能获取。处理器预计数据可能会被重复使用，因此会将加载数据的副本放入 CPU 缓存。如果程序稍后再次提取相同的数据项，CPU 会首先检查缓存中是否有该数据项；如果在缓存中找到了该数据项，提取数据的速度就会快得多（比如只需几纳秒），从而加快程序的运行速度。

因此，缓存是以局部性概念为基础的，局部性有两种：<font color="red">时间局部性和空间局部性</font>。时间局部性背后的理念是，当一个数据被访问时，它很可能在不久的将来再次被访问；想象一下变量甚至指令本身在循环中被反复访问的情景。空间局部性背后的理念是，如果程序访问了地址为 x 的数据项，那么它很可能也会访问 x 附近的数据项；在这里，可以想象程序在数组中流水作业，或者指令被一条接一条地执行。由于许多程序都存在这些类型的局部性，因此硬件系统可以很好地猜测哪些数据应放入高速缓存，从而很好地工作。

但如果在一个系统中使用多个处理器和一个共享主存储器，如下图所示，会发生什么情况？

<img src="https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Two-CPUs-With-Caches.png" alt="image-20240328220638664" style="zoom:50%;" />

事实证明，多 CPU 缓存要复杂得多。如下图所示，假设在 CPU 1 上运行的程序读取并更新了地址 A 上的一个数据项（值为 D）；由于 CPU 1 的缓存中没有该数据，系统会从主存中获取该数据项，并得到值 D。然后假设操作系统决定停止运行程序，并将其移至 CPU 2。然后程序重新读取地址 A 处的值，发现缓存没有这样的数据 ，因此系统从主内存中获取值，并获取旧值 D 而不是正确值 D ′。这个普遍问题称为缓存一致性问题。

![image-20240329084350661](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/new-cache-coherence.png)

硬件提供了基本的解决方案：通过监控内存访问，硬件可以确保基本上 "正确的事情 "发生了，并且单个共享内存的视图得以保留。在基于总线的系统（如上所述）上实现这一点的一种方法是使用一种称为<font color="red">总线窥探（bus snooping）</font>的古老技术；每个高速缓存通过观察连接内存和主内存的总线来关注内存更新。当 CPU 看到其缓存中的数据项有更新时，就会注意到这一变化，要么使其副本失效（即从自己的缓存中删除），要么进行更新（即把新值也放入自己的缓存中）。如上文述，回写缓存会使这一过程变得更加复杂（因为写入主内存的内容要到稍后才能看到），但你可以想象一下基本方案是如何工作的。

## 2 不要忘记同步

既然缓存为提供一致性做了所有这些工作，那么程序（或操作系统本身）在访问共享数据时还需要担心什么吗？答案是肯定的，程序（或操作系统）在访问共享数据时仍然需要考虑一些问题：

1. **竞争条件（Race Conditions）**：即当多个进程或线程试图同时访问共享资源时可能出现的问题。
2. **原子性操作**：某些操作可能涉及多个步骤，需要确保这些步骤的执行是原子的。
3. **内存栅栏和同步**：在某些情况下，需要使用内存栅栏（memory barriers）或者同步机制来确保在不同处理器上的操作执行顺序。这是因为缓存一致性只保证了缓存之间和缓存与内存之间的一致性，而不是对所有指令的执行顺序做出保证。

在跨 CPU 访问（尤其是更新）共享数据项或结构时，应使用互斥原语（如锁）来保证正确性（其他方法，如构建无锁数据结构，比较复杂，只能偶尔使用）。例如，假设多个 CPU 同时访问一个共享队列。如果没有锁，即使使用了底层一致性协议，并发添加或删除队列中的元素也无法达到预期效果；我们需要用锁将数据结构<font color="red">原子更新</font>到新状态。

为了更具体地说明这一点，我们可以看下面这段用于从共享链表中删除一个元素的代码序列。想象一下，如果两个 CPU 上的线程同时进入这个例程。如果线程 1 执行了第一行，它的 tmp 变量中将存储 head 的当前值；如果线程 2 也执行了第一行，它的私有 tmp 变量中也将存储 head 的相同值（tmp 在栈中分配，因此每个线程都有自己的私有存储空间）。这样，每个线程就不会从列表头部删除一个元素了，而是尝试删除这相同的头元素，就会导致各种问题（例如第 4 行中试图对头部元素进行双重释放，以及可能两次返回相同的数据值）。

```c
typedef struct __Node_t {
    int value;
    struct __Node_t *next;
} Node_t;

int List_Pop() {
    Node_t *tmp = head;         // remember old head ...
    int value = head->value;    // ... and its value
    head = head->next;          // advance head to next pointer
    free(tmp);                  // free old head
    return value;               // return value at head
}

```

当然，解决方案是通过锁定来使此类例程正确。在这种情况下，分配一个简单的互斥体（例如，`pthread mutex t m;`），然后在例程的开头添加一个 `lock(&m)` 并在末尾添加一个 `unlock(&m)` 将解决问题，确保代码能够执行如预期的。不幸的是，正如我们将看到的，这种方法并非没有问题，特别是在性能方面。具体来说，随着CPU数量的增加，对同步共享数据结构的访问变得相当慢。

```c
pthread_mutex_t m;

typedef struct __Node_t {
    int value;
    struct __Node_t *next;
} Node_t;

int List_Pop() {
    lock(&m);
    Node_t *tmp = head;         // remember old head ...
    int value = head->value;    // ... and its value
    head = head->next;          // advance head to next pointer
    free(tmp);                  // free old head
    unlock(&m);
    return value;               // return value at head
}

```

## 3 最后一个问题：缓存亲和性

最后一个问题是在构建多处理器缓存调度程序时出现的，称为缓存亲和性。这个概念很简单：进程在特定 CPU 上运行时，会在 CPU 的缓存（和 TLB）中构建相当多的状态。下一次进程运行时，在同一个 CPU 上运行它通常是有利的，因为如果它的某些状态已经存在于该 CPU 上的缓存中，那么它会运行得更快。相反，如果每次在不同的 CPU 上运行一个进程，则该进程的性能会更差，因为每次运行时都必须重新加载状态（注意，由于硬件的缓存一致性协议，它可以在不同的 CPU 上正常运行）。因此，多处理器调度程序在做出调度决策时应考虑缓存关联性，如果可能的话，可能更愿意将进程保留在同一 CPU 上。

## 4 Single queue Multiprocessor Scheduling (SQMS)

有了这个背景，我们现在讨论如何为多处理器系统构建调度程序。最基本的方法是简单地重用单处理器调度的基本框架，将所有需要调度的作业放入单个队列中；我们将此称为<font color="red">单队列多处理器调度或简称为 SQMS</font>。这种方法的优点是简单；不需要太多工作就可以采用现有策略来选择接下来运行的最佳作业，并将其调整为在多个 CPU 上工作（例如，如果有两个 CPU，它可能会选择最好的两个作业来运行） 。

然而，SQMS 也有明显的缺点。

第一个问题是缺乏可扩展性。为了确保调度程序在多个 CPU 上正常工作，开发人员将在代码中插入某种形式的锁，如上所述。锁确保当 SQMS 代码访问单个队列（例如，查找下一个要运行的作业）时，会出现正确的结果。但锁会大大降低性能，尤其是当系统中的 CPU 数量增加。随着对单个锁的争夺增加，系统花在锁开销上的时间越来越多，而花在系统本应完成的工作上的时间却越来越少。

SQMS 的第二个主要问题是缓存亲和性。例如，假设我们有五个作业（A、B、C、D、E）和四个处理器。因此，我们的调度队列看起来是这样的：

![image-20240329092713923](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Single-queue-SQMS.png)

随着时间的推移，假设每个作业运行一个时间片，然后选择另一个作业，下面是一个可能的跨 CPU 作业时间表：

![image-20240329092836566](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/cross-cpu-sqms.png)

由于每个 CPU 只需从全局共享队列中选择下一个要运行的作业，因此每个作业最终都会在 CPU 之间来回跳转，这与缓存亲和性的观点恰恰相反。

为了解决这个问题，大多数 SQMS 调度器都包含某种亲和性机制，以尽可能使进程更有可能继续在同一 CPU 上运行。具体来说，调度器可能会为某些作业提供亲和性，但会移动其他作业以平衡负载。例如，设想同样的五个工作调度如下：

![image-20240329093033902](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/SQMS-cache-affinity.png)

在这种安排下，作业 A 到 D 不会跨处理器迁移，只有作业 E 会从 CPU 迁移到 CPU，从而保留了大部分的亲和性。然后，你可以决定在下一次迁移时迁移不同的作业，从而实现某种亲和性公平性。不过，实施这样的方案可能会很复杂。因此，我们可以看到 SQMS 方法有其优点和缺点。对于现有的单 CPU 调度器（顾名思义，它只有一个队列）来说，它可以直接实施。但是，它的扩展性不佳（由于同步开销），而且不能轻易保留高速缓存亲和性。

## 5 Multi-queue Multiprocessor Scheduling (MQMS)

由于单队列调度器所带来的问题，一些系统选择了多队列，例如每个 CPU 一个队列。我们称这种方法为多队列多处理器调度（或 MQMS）。

在 MQMS 中，我们的基本调度框架由多个调度队列组成。每个队列都可能遵循特定的调度规则，如循环调度（RR），当然也可以使用任何算法。当作业进入系统时，会根据某种启发式（如随机，或挑选作业数量少于其他队列的队列），将其恰好置于一个调度队列中。然后，它基本上被独立调度，从而避免了单队列方法中的信息共享和同步问题。

例如，假设系统中只有两个 CPU（CPU 0 和 CPU 1），有若干作业进入系统：例如 A、B、C 和 D。鉴于每个 CPU 现在都有一个调度队列，操作系统必须决定将每个作业放入哪个队列。操作系统可能会这样做：

![image-20240329093643154](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/MQMS-job-queue.png)

根据队列调度策略的不同，现在每个 CPU 在决定运行什么作业时都有两个作业可供选择。例如，如果采用循环调度，系统可能会产生如下调度：

![image-20240329093833644](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/MQMS-scheduling.png)

与 SQMS 相比，MQMS 有一个明显的优势，那就是它本质上更具可扩展性。随着 CPU 数量的增加，队列的数量也会增加，因此锁和缓存争用不会成为核心问题。此外，MQMS 本身还提供缓存亲和性，这些工作都在同一个 CPU 上运行，因此可以获得重复使用其中缓存内容的优势。

但是，你可能会发现我们遇到了一个新问题，这也是基于多队列方法的基本问题：负载不平衡。假设我们有与上述相同的设置（四个工作、两个 CPU），但其中一个工作（比如 C）完成了。现在我们有以下调度队列：

![image-20240329094212750](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/MQMS-job-queue-load.png)

如果我们在系统的每个队列上运行轮循策略，就会看到这样的调度表：

![image-20240329094516161](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/MQMS-scheduling-1.png)

从图中可以看出，A 获得的 CPU 资源是 B 和 D 的两倍，这并不是我们想要的结果。更糟的是，假设 A 和 C 都完成了工作，系统中只剩下工作 B 和 D。调度队列将如下所示：

![image-20240329094545341](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/MQMS-job--1)

因此，CPU 0 将处于闲置状态！

![image-20240329094608072](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/MQMS-scheduling-2.png)

那么，关键是怎么解决负载不均衡的问题呢？MQMS应如何处理负载不平衡问题，从而更好地实现预期调度目标？

对于这个问题，显而易见的答案就是移动作业，我们（再次）将这种技术称为迁移。通过将作业从一个 CPU 迁移到另一个 CPU，可以实现真正的负载平衡。让我们看几个例子来进一步说明。我们再一次遇到这样的情况：一个 CPU 空闲，另一个 CPU 有一些作业。

在这种情况下，所需的迁移很容易理解：操作系统只需将 B 或 D 中的一个迁移到 CPU 0。 这种单一作业迁移的结果是负载均衡，大家都很高兴。

<img src="https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/MQMS-migration-1.png" alt="image-20240329095010415" style="zoom:50%;" />

在我们之前的示例中出现了一个更棘手的情况，其中 A 单独留在 CPU 0 上，而 B 和 D 交替出现在 CPU 1 上：

![image-20240329094212750](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/MQMS-job-queue-load.png)

在这种情况下，一次迁移并不能解决问题。在这种情况下该怎么办呢？答案是连续迁移一个或多个工作。一种可能的解决方案是不断切换工作，如下图所示。在图中，首先 A 单独运行在 CPU 0 上，B 和 D 交替运行在 CPU 1 上。几个时间片后，B 被转移到 CPU 0 上与 A 竞争，而 D 则在 CPU 1 上单独运行几个时间片。这样，负载就平衡了：

![image-20240329095332147](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/MQMS)

当然，还存在许多其他可能的迁移模式。但现在是棘手的部分：系统应该如何决定实施这样的迁移？

一种基本方法是使用一种称为工作窃取（work stealing）的技术。通过工作窃取方法，作业量较低的（源）队列偶尔会查看另一个（目标）队列，看看它有多满。如果目标队列（特别是）比源队列更满，则源队列将从目标“窃取”一个或多个作业以帮助平衡负载。

当然，这种方法自然会产生矛盾。如果过于频繁地查看其他队列，就会造成高开销和扩展困难，而实施多队列调度的目的就是为了解决这个问题！！另一方面，如果不经常查看其他队列，就有可能出现严重的负载不平衡。在系统策略设计中，找到合适的阈值仍然是一门黑科技。

## 6 Linux 多处理器调度器

有趣的是，Linux 社区在构建多处理器调度程序方面没有通用的解决方案。随着时间的推移，出现了三种不同的调度器：

1. O(1)调度器

	* 基于优先级的调度程序
	* 使用多队列（类似于MLFQ）
	* 随着时间的推移改变流程的优先级 
	* 调度优先级高的进程
	* 交互性是关注的重点

2. 完全公平调度器（CFS）

	* 使用多队列

	* 确定性比例份额方法 
	* 基于阶梯截止日期（公平是重点）
	*  红黑树可扩展性

3. BF调度器（BFS）

	* 单队列调度方法
	* 使用比例份额方法
	* 基于最早符合条件的虚拟截止时间优先（EEVDF）
	* 侧重于交互式（不能很好地扩展内核）。已被 MuQSS 取代，以解决这一问题

## 7 总结

我们已经看到了多种多处理器调度方法。单队列方法 (SQMS) 构建起来相当简单，并且可以很好地平衡负载，但本质上难以扩展到许多处理器和缓存亲和力。多队列方法（MQMS）可扩展性更好，并且可以很好地处理缓存亲和性，但存在负载不平衡的问题，并且更复杂。无论您采用哪种方法，都没有简单的答案：构建通用调度器仍然是一项艰巨的任务，因为小的代码更改可能会导致大的行为差异。

# 2 Program Explanation

`multi.py`是一个模拟多CPU调度器的python脚本，用于模拟作业在多CPU系统中的调度和运行过程。

1. 导入必要的库和模块

	```py
	from __future__ import print_function
	from collections import *
	from optparse import OptionParser
	import random
	```

	这些导入语句用于引入脚本所需的库和模块，包括用于命令行选项解析的 `OptionParser`、`namedtuple` 用于定义 `Job` 结构、以及 `random` 用于生成随机数。

2. 定义功能函数

	```py
	# to make Python2 and Python3 act the same -- how dumb
	def random_seed(seed):
	    try:
	        random.seed(seed, version=1)
	    except:
	        random.seed(seed)
	    return
	
	# helper print function for columnar output
	def print_cpu(cpu, str):
	    print((' ' * cpu * 35) + str)
	    return
	```

	`random_seed`用于设置随机数生成器的种子，以便使得程序在不同的运行中产生相同的随机数序列。这是为了确保程序的可重现性；`print_cpu`用于打印CPU相关信息，以便在模拟中观察不同CPU的运行情况。

3. 定义`Job`结构体

	```py
	Job = namedtuple('Job', ['name', 'run_time', 'working_set_size', 'affinity', 'time_left'])
	```

	这个结构体用于表示作业的相关信息，包括作业名称、运行时间、工作集大小、关联的 CPU、以及剩余运行时间等。

4. 定义`cache`类

	```py
	class cache:
	    def __init__(self, cpu_id, jobs, cache_size, cache_rate_cold, cache_rate_warm, cache_warmup_time):
	        self.cpu_id = cpu_id                        # CPU ID where the cache is located
	        self.jobs = jobs                            # Dictionary containing job objects
	        self.cache_size = cache_size                # Maximum size of the cache
	        self.cache_rate_cold = cache_rate_cold      # Rate for cold cache accesses
	        self.cache_rate_warm = cache_rate_warm      # Rate for warm cache accesses
	        self.cache_warmup_time = cache_warmup_time  # Time required to warm up the cache
	
	        # cache_contents
	        # - should track whose working sets are in the cache
	        # - it's a list of job_names that 
	        #   * is len>=1, and the SUM of working sets fits into the cache
	        # OR
	        #   * len=1 and whose working set may indeed be too big
	        self.cache_contents = []
	
	        # cache_warming(cpu)
	        # - list of job_name that are trying to warm up this cache right now
	        # cache_warming_counter(cpu, job)
	        # - counter for each, showing how long until the cache is warm for that job
	        self.cache_warming = []
	        self.cache_warming_counter = {}
	        return
	
	    def new_job(self, job_name):
	        """
	        Add a new job to the cache or initiate cache warming process if cache warmup is needed.
	        """
	        if job_name not in self.cache_contents and job_name not in self.cache_warming:
	            # print_cpu(self.cpu_id, '*new cache*')
	            if self.cache_warmup_time == 0:
	                # special case (alas): no warmup, just right into cache
	                self.cache_contents.insert(0, job_name)
	                self.adjust_size()
	            else:
	                self.cache_warming.append(job_name)
	                self.cache_warming_counter[job_name] = cache_warmup_time
	        return
	
	    def total_working_set(self):
	        """
	        Calculate the total size of the working sets currently in the cache.
	        """
	        cache_sum = 0
	        for job_name in self.cache_contents:
	            cache_sum += self.jobs[job_name].working_set_size
	        return cache_sum
	
	    def adjust_size(self):
	        """
	        Adjust the cache size by removing jobs if the total working set exceeds the cache size.
	        """
	        working_set_total = self.total_working_set()
	        while working_set_total > self.cache_size:
	            # Get the index of the last job in the cache
	            last_entry = len(self.cache_contents) - 1
	            # Get the name of the job that will be removed from the cache
	            job_gone = self.cache_contents[last_entry]
	            # print_cpu(self.cpu_id, 'kicking out %s' % job_gone)
	            # Remove the job from the cache
	            del self.cache_contents[last_entry]
	            # Add the removed job to the cache warming list
	            self.cache_warming.append(job_gone)
	            # Set the cache warming counter for the removed job to the cache warmup time
	            self.cache_warming_counter[job_gone] = cache_warmup_time
	            working_set_total -= self.jobs[job_gone].working_set_size
	        return
	
	    def get_cache_state(self, job_name):
	        """
	        Get the cache state ('w' for warm, ' ' for cold) for a specific job.
	        """
	        if job_name in self.cache_contents:
	            return 'w'
	        else:
	            return ' '
	        
	    def get_rate(self, job_name):
	        """
	        Get the access rate for a specific job based on cache state.
	        """
	        if job_name in self.cache_contents:
	            return self.cache_rate_warm
	        else:
	            return self.cache_rate_cold
	
	    def update_warming(self, job_name):
	        """
	        Update cache warming status for a specific job.
	        """
	        if job_name in self.cache_warming:
	            self.cache_warming_counter[job_name] -= 1
	            if self.cache_warming_counter[job_name] <= 0:
	                self.cache_warming.remove(job_name)
	                self.cache_contents.insert(0, job_name)
	                self.adjust_size()
	                # print_cpu(self.cpu_id, '*warm cache*')
	        return
	```

	这个 `cache` 类的作用是模拟计算机系统中的缓存行为。缓存在计算机系统中被用来存储经常访问的数据，以提高数据访问速度。这个类中的缓存对象可以管理多个作业（jobs），根据作业的访问模式将其分为冷缓存和温缓存。

	冷缓存（Cold Cache）和温缓存（Warm Cache）是用来描述缓存中数据的访问状态的。冷缓存表示缓存中的数据很少被访问，因此可能会有较长的访问延迟；而温缓存表示缓存中的数据经常被访问，因此具有较短的访问延迟。这个 `cache` 类通过跟踪作业的访问模式，并根据需要将其放入冷缓存或温缓存中，来模拟实际计算机系统中的缓存行为。这样可以更好地理解缓存对系统性能的影响，并进行性能优化或者缓存算法的研究。

5. 定义`scheduler`类

	```py
	class scheduler:
	    def __init__(self, job_list, per_cpu_queues, affinity, peek_interval,
	                 job_num, max_run, max_wset,
	                 num_cpus, time_slice, random_order,
	                 cache_size, cache_rate_cold, cache_rate_warm, cache_warmup_time,
	                 solve, trace, trace_time_left, trace_cache, trace_sched):
	
	        # Initialize scheduler with given parameters and generate job list if not provided
	        # job_list: comma-separated list of job_name:run_time:working_set_size
	        if job_list == '':
	            # this means randomly generate jobs
	            for j in range(job_num):
	                run_time = int((random.random() * max_run)/10.0) * 10
	                working_set = int((random.random() * max_wset)/10.0) * 10
	                if job_list == '':
	                    job_list = '%s:%d:%d' % (str(j), run_time, working_set)
	                else:
	                    job_list += (',%s:%d:%d' % (str(j), run_time, working_set))
	                
	        # just the job names
	        self.job_name_list = []
	
	        # info about each job
	        self.jobs = {}
	        
	        for entry in job_list.split(','):
	            tmp = entry.split(':')
	            # check for correct format
	            if len(tmp) != 3:
	                print('bad job description [%s]: needs triple of name:runtime:working_set_size' % entry)
	                exit(1)
	            job_name, run_time, working_set_size = tmp[0], int(tmp[1]), int(tmp[2])
	            self.jobs[job_name] = Job(name=job_name, run_time=run_time, working_set_size=working_set_size, affinity=[], time_left=[run_time])
	            print('Job name:%s run_time:%d working_set_size:%d' % (job_name, run_time, working_set_size))
	            # self.sched_queue.append(job_name)
	            if job_name in self.job_name_list:
	                print('repeated job name %s' % job_name)
	                exit(1)
	            self.job_name_list.append(job_name)
	        print('')
	
	        # parse the affinity list
	        if affinity != '':
	            for entry in affinity.split(','):
	                # form is 'job_name:cpu.cpu.cpu'
	                # where job_name is the name of an existing job
	                # and cpu is an ID of a particular CPU (0 ... max_cpus-1)
	                tmp = entry.split(':')
	                if len(tmp) != 2:
	                    print('bad affinity spec %s' % affinity)
	                    exit(1)
	                job_name = tmp[0]
	                if job_name not in self.job_name_list:
	                    print('job name %s in affinity list does not exist' % job_name)
	                    exit(1)
	                for cpu in tmp[1].split('.'):
	                    self.jobs[job_name].affinity.append(int(cpu))
	                    if int(cpu) < 0 or int(cpu) >= num_cpus:
	                        print('bad cpu %d specified in affinity %s' % (int(cpu), affinity))
	                        exit(1)
	
	        # now, assign jobs to either ALL the one queue, or to each of the queues in RR style
	        # (as constrained by affinity specification)
	        self.per_cpu_queues = per_cpu_queues
	        self.per_cpu_sched_queue = {}
	
	        if self.per_cpu_queues:
	            # create a queue for each CPU
	            for cpu in range(num_cpus):
	                self.per_cpu_sched_queue[cpu] = []
	            # now assign jobs to these queues 
	            jobs_not_assigned = list(self.job_name_list)
	            while len(jobs_not_assigned) > 0:
	                # assign jobs to CPUs in RR fashion
	                for cpu in range(num_cpus):
	                    assigned = False
	                    for job_name in jobs_not_assigned:
	                        # if no affinity, or if affinity includes this CPU
	                        if len(self.jobs[job_name].affinity) == 0 or cpu in self.jobs[job_name].affinity:
	                            self.per_cpu_sched_queue[cpu].append(job_name)
	                            jobs_not_assigned.remove(job_name)
	                            assigned = True
	                        if assigned:
	                            break
	
	            for cpu in range(num_cpus):
	                print('Scheduler CPU %d queue: %s' % (cpu, self.per_cpu_sched_queue[cpu]))
	            print('')
	                            
	        else:
	            # SQMS: single queue multiprocessor scheduling
	            # assign them all to same single queue
	            self.single_sched_queue = []
	            for job_name in self.job_name_list:
	                self.single_sched_queue.append(job_name)
	            # all CPUs share the same queue
	            for cpu in range(num_cpus):
	                self.per_cpu_sched_queue[cpu] = self.single_sched_queue
	
	            print('Scheduler central queue: %s\n' % (self.single_sched_queue))
	
	        self.num_jobs = len(self.job_name_list)
	
	        self.peek_interval = peek_interval
	
	        self.num_cpus = num_cpus
	        self.time_slice = time_slice
	        self.random_order = random_order
	
	        self.solve = solve
	
	        self.trace = trace
	        self.trace_time_left = trace_time_left
	        self.trace_cache = trace_cache
	        self.trace_sched = trace_sched
	
	        # tracking each CPU: is it idle or running a job?
	        self.STATE_IDLE = 1
	        self.STATE_RUNNING = 2
	
	        # the scheduler state (RUNNING or IDLE) of each CPU
	        # Initially, all CPUs are IDLE
	        self.sched_state = {}
	        for cpu in range(self.num_cpus):
	            self.sched_state[cpu] = self.STATE_IDLE
	
	        # if a job is running on a CPU, which job is it
	        self.sched_current = {}
	        for cpu in range(self.num_cpus):
	            self.sched_current[cpu] = ''
	
	        # just some stats
	        self.stats_ran = {}
	        self.stats_ran_warm = {}
	        for cpu in range(self.num_cpus):
	            self.stats_ran[cpu] = 0
	            self.stats_ran_warm[cpu] = 0
	
	        # scheduler (because it runs the simulation) also instantiates and updates each cache
	        self.caches = {}
	        for cpu in range(self.num_cpus):
	            self.caches[cpu] = cache(cpu, self.jobs, cache_size, cache_rate_cold, cache_rate_warm, cache_warmup_time)
	
	        return
	
	    def handle_one_interrupt(self, interrupt, cpu):
	        # HANDLE: interrupts here, so jobs don't run an extra tick
	        if interrupt and self.sched_state[cpu] == self.STATE_RUNNING:
	            self.sched_state[cpu] = self.STATE_IDLE
	            job_name = self.sched_current[cpu]
	            self.sched_current[cpu] = ''
	            # print_cpu(cpu, 'tick done for job %s' % job_name)
	            self.per_cpu_sched_queue[cpu].append(job_name)
	        return
	
	    def handle_interrupts(self):
	        if self.system_time % self.time_slice == 0 and self.system_time > 0:
	            interrupt = True
	            # num_to_print = time + per-cpu info + cache status for each job - last set of space
	            num_to_print = 8 + (7 * self.num_cpus) - 5
	            if self.trace_time_left:
	                num_to_print += 6 * self.num_cpus
	            if self.trace_cache:
	                num_to_print += 8 * self.num_cpus + self.num_jobs * (self.num_cpus)
	            if self.trace:
	                print('-' * num_to_print)
	        else:
	            interrupt = False
	
	        if self.trace:
	            print(' %3d   ' % self.system_time, end='')
	
	        # INTERRUPTS first: this might deschedule a job, putting it into a runqueue
	        for cpu in range(self.num_cpus):
	            self.handle_one_interrupt(interrupt, cpu)
	        return
	
	    def get_job(self, cpu, sched_queue):
	        # get next job?
	        for job_index in range(len(sched_queue)):
	            job_name = sched_queue[job_index]
	            # len(affinity) == 0 is special case, which means ANY cpu is fine
	            if len(self.jobs[job_name].affinity) == 0 or cpu in self.jobs[job_name].affinity:
	                # extract job from runqueue, put in CPU local structures
	                sched_queue.pop(job_index)
	                self.sched_state[cpu] = self.STATE_RUNNING
	                self.sched_current[cpu] = job_name
	                self.caches[cpu].new_job(job_name)
	                # print('got job %s' % job_name)
	                return
	        return
	
	    def assign_jobs(self):
	        # random assignment of jobs to CPUs
	        if self.random_order:
	            cpu_list = list(range(self.num_cpus))
	            random.shuffle(cpu_list)
	        else:
	            cpu_list = range(self.num_cpus)
	        for cpu in cpu_list:
	            if self.sched_state[cpu] == self.STATE_IDLE:
	                self.get_job(cpu, self.per_cpu_sched_queue[cpu])
	
	    def print_sched_queues(self):
	        # PRINT queue information
	        if not self.trace_sched:
	            return
	        if self.per_cpu_queues:
	            for cpu in range(self.num_cpus):
	                print('Q%d: ' % cpu, end='')
	                for job_name in self.per_cpu_sched_queue[cpu]:
	                    print('%s ' % job_name, end='')
	                print('  ', end='')
	            print('    ', end='')
	        else:
	            print('Q: ', end='')
	            for job_name in self.single_sched_queue:
	                print('%s ' % job_name, end='')
	            print('    ', end='')
	        return
	        
	    def steal_jobs(self):
	        """
	        Steal jobs from other CPUs' queues if they are idle.
	        """
	        if not self.per_cpu_queues or self.peek_interval <= 0:
	            return
	
	        # if it is time to steal
	        if self.system_time > 0 and self.system_time % self.peek_interval == 0:
	            for cpu in range(self.num_cpus):
	                if len(self.per_cpu_sched_queue[cpu]) == 0:
	                    # find IDLE job in some other CPUs queue
	                    other_cpu_list = list(range(self.num_cpus))
	                    other_cpu_list.remove(cpu)
	                    # random choice of which CPU to look at
	                    other_cpu = random.choice(other_cpu_list)
	                    # print('cpu %d is idle' % cpu)
	                    # print('-> look at %d' % other_cpu)
	
	                    for job_name in self.per_cpu_sched_queue[other_cpu]:
	                        # print('---> examine job %s' % job_name)
	                        if len(self.jobs[job_name].affinity) == 0 or cpu in self.jobs[job_name]:
	                           self.per_cpu_sched_queue[other_cpu].remove(job_name)
	                           self.per_cpu_sched_queue[cpu].append(job_name)
	                           # print('stole job %s from %d to %d' % (job_name, other_cpu, cpu))
	                           break
	        return
	
	    def run_one_tick(self, cpu):
	        """
	        Run one tick of a job on a CPU.
	        """
	        job_name = self.sched_current[cpu]
	        job = self.jobs[job_name]
	
	        # Update the remaining time for the job and handle cache warming
	        # USE cache_contents to determine if cache is cold or warm
	        # (list usage w/ time_left field: a hack to deal with namedtuple and its lack of mutability)
	        current_rate = self.caches[cpu].get_rate(job_name)
	        self.stats_ran[cpu] += 1
	        if current_rate > 1:
	            self.stats_ran_warm[cpu] += 1
	        time_left = job.time_left.pop() - current_rate
	        if time_left < 0:
	            time_left = 0
	        job.time_left.append(time_left)
	
	        if self.trace:
	            print('%s ' % job.name, end='')
	            if self.trace_time_left:
	                print('[%3d] ' % job.time_left[0], end='')
	
	        # UPDATE: cache warming
	        self.caches[cpu].update_warming(job_name)
	
	        if time_left <= 0:
	            self.sched_state[cpu] = self.STATE_IDLE
	            job_name = self.sched_current[cpu]
	            self.sched_current[cpu] = ''
	            # remember: it is time X now, but job ran through this tick, so finished at X + 1
	            # print_cpu(cpu, 'finished %s at time %d' % (job_name, self.system_time + 1))
	            self.jobs_finished += 1
	        return
	
	    def run_jobs(self):
	        for cpu in range(self.num_cpus):
	            if self.sched_state[cpu] == self.STATE_RUNNING:
	                self.run_one_tick(cpu)
	            elif self.trace:
	                print('- ', end='')
	                if self.trace_time_left:
	                    print('[   ] ', end='')
	
	            # PRINT: cache state
	            cache_string = ''
	            for job_name in self.job_name_list:
	                # cache_string += '%s%s ' % (job_name, self.caches[cpu].get_cache_state(job_name))
	                cache_string += '%s' % self.caches[cpu].get_cache_state(job_name)
	            if self.trace:
	                if self.trace_cache:
	                    print('cache[%s]' % cache_string, end='')
	                print('     ', end='')
	        return
	
	    #
	    # MAIN SIMULATION
	    #
	    def run(self):
	        # things to track
	        self.system_time = 0
	        self.jobs_finished = 0
	
	        while self.jobs_finished < self.num_jobs:
	            # interrupts: may cause end of a tick, thus making job schedulable elsewhere
	            self.handle_interrupts()
	
	            # if it's time, do some job stealing
	            self.steal_jobs()
	                
	            # assign_jobsign news jobs to CPUs (this can happen every tick?)
	            self.assign_jobs()
	
	            # run each CPU for a time slice and handle POSSIBLE end of job
	            self.run_jobs()
	
	            self.print_sched_queues()
	
	            # to add a newline after all the job updates
	            if self.trace:
	                print('')
	
	            # the clock keeps ticking            
	            self.system_time += 1
	
	        if self.solve:
	            print('\nFinished time %d\n' % self.system_time)
	            print('Per-CPU stats')
	            for cpu in range(self.num_cpus):
	                print('  CPU %d  utilization %3.2f [ warm %3.2f ]' % (cpu, 100.0 * float(self.stats_ran[cpu])/float(self.system_time),
	                                                                      100.0 * float(self.stats_ran_warm[cpu])/float(self.system_time)))
	            print('')
	        return
	```

	这个 `scheduler` 类是一个调度器，用于模拟作业在计算机系统中的调度和执行过程。它接收一系列的参数，包括作业列表、每个 CPU 的队列情况、作业的亲和性（affinity）、窥探时间间隔（peek_interval）等等，然后根据这些参数进行模拟。

	具体来说，这个类的功能包括：

	- 根据给定的参数生成作业列表。
	- 根据作业的亲和性将作业分配到不同的 CPU 队列中。
	- 在每个时间片段内模拟作业的运行和调度过程，包括处理中断、获取下一个作业、执行作业等。
	- 根据模拟的结果输出统计信息，如各个 CPU 的利用率。

6. 模拟调度

	```py
	if __name__ == '__main__':
	    # MAIN PROGRAM
	    parser = OptionParser()
	    parser.add_option('-s', '--seed',        default=0,     help='the random seed',                        action='store', type='int', dest='seed')
	    parser.add_option('-j', '--job_num',     default=3,     help='number of jobs in the system',           action='store', type='int', dest='job_num')
	    parser.add_option('-R', '--max_run',     default=100,   help='max run time of random-gen jobs',        action='store', type='int', dest='max_run')
	    parser.add_option('-W', '--max_wset',    default=200,   help='max working set of random-gen jobs',     action='store', type='int', dest='max_wset')
	    parser.add_option('-L', '--job_list',    default='',    help='provide a comma-separated list of job_name:run_time:working_set_size (e.g., a:10:100,b:10:50 means 2 jobs with run-times of 10, the first (a) with working set size=100, second (b) with working set size=50)', action='store', type='string', dest='job_list')
	    parser.add_option('-p', '--per_cpu_queues', default=False, help='per-CPU scheduling queues (not one)', action='store_true',        dest='per_cpu_queues')
	    parser.add_option('-A', '--affinity',    default='',    help='a list of jobs and which CPUs they can run on (e.g., a:0.1.2,b:0.1 allows job a to run on CPUs 0,1,2 but b only on CPUs 0 and 1', action='store', type='string', dest='affinity')
	    parser.add_option('-n', '--num_cpus',    default=2,     help='number of CPUs',                         action='store', type='int', dest='num_cpus')
	    parser.add_option('-q', '--quantum',     default=10,    help='length of time slice',                   action='store', type='int', dest='time_slice')
	    parser.add_option('-P', '--peek_interval', default=30,  help='for per-cpu scheduling, how often to peek at other schedule queue; 0 turns this off', action='store', type='int', dest='peek_interval')
	    parser.add_option('-w', '--warmup_time', default=10,    help='time it takes to warm cache',            action='store', type='int', dest='warmup_time')
	    parser.add_option('-r', '--warm_rate', default=2,     help='how much faster to run with warm cache', action='store', type='int', dest='warm_rate')
	    parser.add_option('-M', '--cache_size',  default=100,   help='cache size',                             action='store', type='int', dest='cache_size')
	    parser.add_option('-o', '--rand_order',  default=False, help='has CPUs get jobs in random order',      action='store_true',        dest='random_order')
	    parser.add_option('-t', '--trace',       default=False, help='enable basic tracing (show which jobs got scheduled)',      action='store_true',        dest='trace')
	    parser.add_option('-T', '--trace_time_left', default=False, help='trace time left for each job',       action='store_true',        dest='trace_time_left')
	    parser.add_option('-C', '--trace_cache', default=False, help='trace cache status (warm/cold) too',     action='store_true',        dest='trace_cache')
	    parser.add_option('-S', '--trace_sched', default=False, help='trace scheduler state',                  action='store_true',        dest='trace_sched')
	    parser.add_option('-c', '--compute',     default=False, help='compute answers for me',                 action='store_true',        dest='solve')
	
	    (options, args) = parser.parse_args()
	
	    random_seed(options.seed)
	
	    print('ARG seed %s' % options.seed)
	    print('ARG job_num %s' % options.job_num)
	    print('ARG max_run %s' % options.max_run)
	    print('ARG max_wset %s' % options.max_wset)
	    print('ARG job_list %s' % options.job_list)
	    print('ARG affinity %s' % options.affinity)
	    print('ARG per_cpu_queues %s' % options.per_cpu_queues)
	    print('ARG num_cpus %s' % options.num_cpus)
	    print('ARG quantum %s' % options.time_slice)
	    print('ARG peek_interval %s' % options.peek_interval)
	    print('ARG warmup_time %s' % options.warmup_time)
	    print('ARG cache_size %s' % options.cache_size)
	    print('ARG random_order %s' % options.random_order)
	    print('ARG trace %s' % options.trace)
	    print('ARG trace_time %s' % options.trace_time_left)
	    print('ARG trace_cache %s' % options.trace_cache)
	    print('ARG trace_sched %s' % options.trace_sched)
	    print('ARG compute %s' % options.solve)
	    print('')
	
	    #
	    # JOBS
	    # 
	    job_list = options.job_list
	    job_num = int(options.job_num)
	    max_run = int(options.max_run)
	    max_wset = int(options.max_wset)
	
	    #
	    # MACHINE
	    #
	    num_cpus = int(options.num_cpus)
	    time_slice = int(options.time_slice)
	
	    #
	    # CACHES
	    #
	    cache_size = int(options.cache_size)
	    cache_rate_warm = int(options.warm_rate)
	    cache_warmup_time = int(options.warmup_time)
	
	    do_trace = options.trace
	    if options.trace_time_left or options.trace_cache or options.trace_sched:
	        do_trace = True
	
	    #
	    # SCHEDULER (and simulator)
	    #
	    S = scheduler(job_list=job_list, affinity=options.affinity, per_cpu_queues=options.per_cpu_queues, peek_interval=options.peek_interval,
	                job_num=job_num, max_run=max_run, max_wset=max_wset,
	                num_cpus=num_cpus, time_slice=time_slice, random_order=options.random_order,
	                cache_size=cache_size, cache_rate_cold=1, cache_rate_warm=cache_rate_warm,
	                cache_warmup_time=cache_warmup_time, solve=options.solve,
	                trace=do_trace, trace_time_left=options.trace_time_left, trace_cache=options.trace_cache,
	                trace_sched=options.trace_sched)
	
	    # Finally, ...
	    S.run()
	```

	首先使用 `OptionParser` 来解析命令行选项，这些选项用于配置作业调度器和模拟器。然后，我们根据命令行提供的参数初始化了一个作业调度器 `s`。最后，调用 `S.run()` 方法来启动模拟器的运行。在这个方法中，模拟器会不断循环执行以下步骤，直到所有作业都完成：

	1. 处理中断：检查是否需要中断当前作业的执行，以便将其重新放回到调度队列中。
	2. 偷取作业：如果配置了每个 CPU 定期偷取其他 CPU 的作业，就会进行偷取操作。
	3. 分配作业：根据调度算法，为每个 CPU 分配下一个要执行的作业。
	4. 运行作业：模拟器按照时间片的方式执行作业，并更新作业的剩余执行时间。
	5. 打印调度队列：如果配置了跟踪信息，会打印每个 CPU 的调度队列和其他相关信息。
	6. 更新系统时间：模拟器的系统时间不断增加，以模拟实际时间的流逝。

	通过以上步骤，模拟器可以模拟出作业调度的整个过程，并且可以根据配置的不同参数进行灵活的调度算法和性能分析。

要运行模拟器，您只需输入：

```sh
python multi.py
```



然后，这将运行模拟器并进行一些随机作业。在默认模式下，系统中有一个或多个 CPU（由 -n 标志指定）。因此，要在模拟中使用 4 个 CPU 运行，请输入：

```shell
python multi.py -n 4
```

每个CPU都有一个高速缓存，可以保存一个或多个正在运行的进程的重要数据。每个 CPU 高速缓存的大小由 -M 标志设置。因此，要使 4-CPU 系统上每个缓存的大小为“100”，请运行：

```shell
python multi.py -n 4 -M 100
```

要运行模拟，您需要安排一些作业。有两种方法可以做到这一点。第一个是让系统为你创建一些具有随机特征的作业（这是默认的，即如果你什么都不指定，你就会得到这个）；还有一些控件可以（在某种程度上）控制随机生成的作业的性质。二是指定作业列表，供系统精准调度。

每个工作都有两个特点。第一个是它的“运行时间”（它将运行多少个时间单位）。第二个是它的“工作集大小”（有效运行需要多少缓存空间）。如果随机生成作业，则可以使用 -R（最大运行时间标志）和 -W（最大工作集大小标志）来控制这些值的范围；然后随机生成器将生成不大于这些值的值。如果您手动指定作业，则可以使用 -L 标志显式设置每个作业。例如，如果您想运行两个作业，每个作业的运行时间为 100，但工作集大小分别为 50 和 150，您将执行以下操作：

```shell
python multi.py -n 4 -M 100 -L 1:100:50,2:100:150
```

请注意，您为每个作业分配了一个名称，第一个作业为“1”，第二个作业为“2”。自动生成作业时，会自动分配名称（仅使用数字）。

作业在特定 CPU 上运行的效率取决于该 CPU 的缓存当前是否保存给定作业的工作集。如果没有，作业运行缓慢，这意味着每个时钟周期仅从其剩余时间中减去其运行时间的 1 个周期。在这种模式下，该作业的缓存是“冷”的（即，它还不包含作业的工作集）。但是，如果作业之前已在 CPU 上运行“足够长的时间”，则 CPU 缓存现在已“热”，并且作业将执行得更快。你问要快多少？嗯，这取决于 -r 标志的值，即“预热率”。此处，默认情况下约为 2x，但您可以根据需要更改它。

您问缓存需要多长时间才能预热？好吧，这也是由一个标志设置的，在本例中是 -w 标志，它设置“预热时间”。默认情况下，大约是 10 个时间单位。因此，如果作业运行 10 个时间单位，该 CPU 上的缓存就会变热，然后作业开始运行得更快。当然，所有这些都是真实系统工作方式的粗略近似，但这就是模拟的美妙之处，对吧？

现在我们有了 CPU，每个 CPU 都有缓存，以及指定作业的方法。还剩下什么？啊哈，你说的是调度策略！你是对的。加油，勤奋做作业的人！

第一个（默认）策略很简单：集中式调度队列，将作业循环分配给空闲 CPU。第二种是多队列 CPU 调度方法（使用 -p 打开），其中作业被分配到 N 个调度队列之一（每个 CPU 一个）；在这种方法中，空闲的CPU（有时）会查看其他CPU的队列并窃取作业，以改善负载平衡。执行此操作的频率由“查看”间隔（由 -P 标志设置）设置。

有了这些基本的了解，您现在应该能够做好功课，并研究不同的日程安排方法。要查看该模拟器的完整选项列表，只需键入：

```shell
Usage: multi.py [options]

Options:
  -h, --help            show this help message and exit
  -s SEED, --seed=SEED  the random seed
  -j JOB_NUM, --job_num=JOB_NUM
                        number of jobs in the system
  -R MAX_RUN, --max_run=MAX_RUN
                        max run time of random-gen jobs
  -W MAX_WSET, --max_wset=MAX_WSET
                        max working set of random-gen jobs
  -L JOB_LIST, --job_list=JOB_LIST
                        provide a comma-separated list of
                        job_name:run_time:working_set_size (e.g.,
                        a:10:100,b:10:50 means 2 jobs with run-times of 10,
                        the first (a) with working set size=100, second (b)
                        with working set size=50)
  -p, --per_cpu_queues  per-CPU scheduling queues (not one)
  -A AFFINITY, --affinity=AFFINITY
                        a list of jobs and which CPUs they can run on (e.g.,
                        a:0.1.2,b:0.1 allows job a to run on CPUs 0,1,2 but b
                        only on CPUs 0 and 1
  -n NUM_CPUS, --num_cpus=NUM_CPUS
                        number of CPUs
  -q TIME_SLICE, --quantum=TIME_SLICE
                        length of time slice
  -P PEEK_INTERVAL, --peek_interval=PEEK_INTERVAL
                        for per-cpu scheduling, how often to peek at other
                        schedule queue; 0 turns this off
  -w WARMUP_TIME, --warmup_time=WARMUP_TIME
                        time it takes to warm cache
  -r WARM_RATE, --warm_rate=WARM_RATE
                        how much faster to run with warm cache
  -M CACHE_SIZE, --cache_size=CACHE_SIZE
                        cache size
  -o, --rand_order      has CPUs get jobs in random order
  -t, --trace           enable basic tracing (show which jobs got scheduled)
  -T, --trace_time_left
                        trace time left for each job
  -C, --trace_cache     trace cache status (warm/cold) too
  -S, --trace_sched     trace scheduler state
  -c, --compute         compute answers for me
```

最好了解的可能是一些跟踪选项（如 -t、-T、-C 和 -S）。尝试这些以更好地了解调度程序和系统正在做什么。
# Homework
1. 首先，让我们学习如何使用模拟器来研究如何建立一个有效的多处理器调度程序。第一次模拟只运行一个运行时间为 30、工作集大小为 200 的作业。在一个模拟 CPU 上运行该作业（此处称为作业 "a"）的步骤如下：`python multi.py -n 1 -L a:30:200`。需要多长时间才能完成？打开 -c 标志可查看最终答案，打开 -t 标志可查看作业的逐时跟踪和调度情况。

	需要30s

	```shell
	❯ python multi.py -n 1 -L a:30:200 -s 0 -c -t
	ARG seed 0
	ARG job_num 3
	ARG max_run 100
	ARG max_wset 200
	ARG job_list a:30:200
	ARG affinity 
	ARG per_cpu_queues False
	ARG num_cpus 1
	ARG quantum 10
	ARG peek_interval 30
	ARG warmup_time 10
	ARG cache_size 100
	ARG random_order False
	ARG trace True
	ARG trace_time False
	ARG trace_cache False
	ARG trace_sched False
	ARG compute True
	
	Job name:a run_time:30 working_set_size:200
	
	Scheduler central queue: ['a']
	
	   0   a      
	   1   a      
	   2   a      
	   3   a      
	   4   a      
	   5   a      
	   6   a      
	   7   a      
	   8   a      
	   9   a      
	----------
	  10   a      
	  11   a      
	  12   a      
	  13   a      
	  14   a      
	  15   a      
	  16   a      
	  17   a      
	  18   a      
	  19   a      
	----------
	  20   a      
	  21   a      
	  22   a      
	  23   a      
	  24   a      
	  25   a      
	  26   a      
	  27   a      
	  28   a      
	  29   a      
	
	Finished time 30
	
	Per-CPU stats
	  CPU 0  utilization 100.00 [ warm 0.00 ]
	```

2. 现在增加缓存大小，使作业的工作集（大小=200）适合缓存（默认大小=100）；例如，运行 `python multi.py -n 1 -L a:30:200 -M 300`。你能预测作业进入缓存后的运行速度吗？ 提示：请记住暖速率这一关键参数，它由 -r 参数设置）请启用求解参数 (-c) 运行，检查答案。

	`warmup_time`默认为10s，所以等到10s后才开始warm，此时运行速度提升2倍。故只需要20s。

	```shell
	❯ python multi.py -n 1 -L a:30:200 -M 300 -t -c
	ARG seed 0
	ARG job_num 3
	ARG max_run 100
	ARG max_wset 200
	ARG job_list a:30:200
	ARG affinity 
	ARG per_cpu_queues False
	ARG num_cpus 1
	ARG quantum 10
	ARG peek_interval 30
	ARG warmup_time 10
	ARG cache_size 300
	ARG random_order False
	ARG trace True
	ARG trace_time False
	ARG trace_cache False
	ARG trace_sched False
	ARG compute True
	
	Job name:a run_time:30 working_set_size:200
	
	Scheduler central queue: ['a']
	
	   0   a      
	   1   a      
	   2   a      
	   3   a      
	   4   a      
	   5   a      
	   6   a      
	   7   a      
	   8   a      
	   9   a      
	----------
	  10   a      
	  11   a      
	  12   a      
	  13   a      
	  14   a      
	  15   a      
	  16   a      
	  17   a      
	  18   a      
	  19   a      
	
	Finished time 20
	
	Per-CPU stats
	  CPU 0  utilization 100.00 [ warm 50.00 ]
	```

3. multi.py 的一大亮点是，您可以通过不同的跟踪选项查看更多细节。运行与上面相同的模拟，但这次启用了剩余时间跟踪 (-T)。该选项既显示了每个时间步在 CPU 上调度的作业，也显示了该作业在每个刻度运行后的剩余运行时间。你注意到第二列时间是如何减少的吗？

	```shell
	❯ python multi.py -n 1 -L a:30:200 -M 300 -t -c -T
	ARG seed 0
	ARG job_num 3
	ARG max_run 100
	ARG max_wset 200
	ARG job_list a:30:200
	ARG affinity 
	ARG per_cpu_queues False
	ARG num_cpus 1
	ARG quantum 10
	ARG peek_interval 30
	ARG warmup_time 10
	ARG cache_size 300
	ARG random_order False
	ARG trace True
	ARG trace_time True
	ARG trace_cache False
	ARG trace_sched False
	ARG compute True
	
	Job name:a run_time:30 working_set_size:200
	
	Scheduler central queue: ['a']
	
	   0   a [ 29]      
	   1   a [ 28]      
	   2   a [ 27]      
	   3   a [ 26]      
	   4   a [ 25]      
	   5   a [ 24]      
	   6   a [ 23]      
	   7   a [ 22]      
	   8   a [ 21]      
	   9   a [ 20]      
	----------------
	  10   a [ 18]      
	  11   a [ 16]      
	  12   a [ 14]      
	  13   a [ 12]      
	  14   a [ 10]      
	  15   a [  8]      
	  16   a [  6]      
	  17   a [  4]      
	  18   a [  2]      
	  19   a [  0]      
	
	Finished time 20
	
	Per-CPU stats
	  CPU 0  utilization 100.00 [ warm 50.00 ]
	```

4. 现在再添加一位跟踪，以使用 -C 选项显示每个作业的每个 CPU 缓存的状态。对于每个作业，每个缓存将显示一个空格（如果该作业的缓存是冷的）或“w”（如果该作业的缓存是热的）。在这个简单的例子中，作业“a”的缓存在什么时候变热？将预热时间参数 (-w) 更改为低于或高于默认值时会发生什么？

	缓存在第9s的时候变热。如果将预热时间降低，则运行时间更早得到提升，反之则更晚。

	```shell
	❯ python multi.py -n 1 -L a:30:200 -M 300 -t -c -T -C
	ARG seed 0
	ARG job_num 3
	ARG max_run 100
	ARG max_wset 200
	ARG job_list a:30:200
	ARG affinity 
	ARG per_cpu_queues False
	ARG num_cpus 1
	ARG quantum 10
	ARG peek_interval 30
	ARG warmup_time 10
	ARG cache_size 300
	ARG random_order False
	ARG trace True
	ARG trace_time True
	ARG trace_cache True
	ARG trace_sched False
	ARG compute True
	
	Job name:a run_time:30 working_set_size:200
	
	Scheduler central queue: ['a']
	
	   0   a [ 29] cache[ ]     
	   1   a [ 28] cache[ ]     
	   2   a [ 27] cache[ ]     
	   3   a [ 26] cache[ ]     
	   4   a [ 25] cache[ ]     
	   5   a [ 24] cache[ ]     
	   6   a [ 23] cache[ ]     
	   7   a [ 22] cache[ ]     
	   8   a [ 21] cache[ ]     
	   9   a [ 20] cache[w]     
	-------------------------
	  10   a [ 18] cache[w]     
	  11   a [ 16] cache[w]     
	  12   a [ 14] cache[w]     
	  13   a [ 12] cache[w]     
	  14   a [ 10] cache[w]     
	  15   a [  8] cache[w]     
	  16   a [  6] cache[w]     
	  17   a [  4] cache[w]     
	  18   a [  2] cache[w]     
	  19   a [  0] cache[w]     
	
	Finished time 20
	
	Per-CPU stats
	  CPU 0  utilization 100.00 [ warm 50.00 ]
	```

5. 让我们在双 CPU 系统上运行以下三个作业（即，输入 `python multi.py -n 2 -L a:100:100,b:100:50,c:100:50`），你能预测一下如何考虑到循环集中式调度程序，这需要多长时间？使用 -c 来查看您是否正确，然后使用 -t 深入了解详细信息以查看分步操作，然后使用 -C 来查看缓存是否针对这些作业有效地预热。你注意到了什么？

	```shell
	❯ python multi.py -n 2 -L a:100:100,b:100:50,c:100:50 -c -t -C
	ARG seed 0
	ARG job_num 3
	ARG max_run 100
	ARG max_wset 200
	ARG job_list a:100:100,b:100:50,c:100:50
	ARG affinity 
	ARG per_cpu_queues False
	ARG num_cpus 2
	ARG quantum 10
	ARG peek_interval 30
	ARG warmup_time 10
	ARG cache_size 100
	ARG random_order False
	ARG trace True
	ARG trace_time False
	ARG trace_cache True
	ARG trace_sched False
	ARG compute True
	
	Job name:a run_time:100 working_set_size:100
	Job name:b run_time:100 working_set_size:50
	Job name:c run_time:100 working_set_size:50
	
	Scheduler central queue: ['a', 'b', 'c']
	
	   0   a cache[   ]     b cache[   ]     
	   1   a cache[   ]     b cache[   ]     
	   2   a cache[   ]     b cache[   ]     
	   3   a cache[   ]     b cache[   ]     
	   4   a cache[   ]     b cache[   ]     
	   5   a cache[   ]     b cache[   ]     
	   6   a cache[   ]     b cache[   ]     
	   7   a cache[   ]     b cache[   ]     
	   8   a cache[   ]     b cache[   ]     
	   9   a cache[w  ]     b cache[ w ]     
	---------------------------------------
	  10   c cache[w  ]     a cache[ w ]     
	  11   c cache[w  ]     a cache[ w ]     
	  12   c cache[w  ]     a cache[ w ]     
	  13   c cache[w  ]     a cache[ w ]     
	  14   c cache[w  ]     a cache[ w ]     
	  15   c cache[w  ]     a cache[ w ]     
	  16   c cache[w  ]     a cache[ w ]     
	  17   c cache[w  ]     a cache[ w ]     
	  18   c cache[w  ]     a cache[ w ]     
	  19   c cache[  w]     a cache[w  ]     
	---------------------------------------
	  20   b cache[  w]     c cache[w  ]     
	  21   b cache[  w]     c cache[w  ]     
	  22   b cache[  w]     c cache[w  ]     
	  23   b cache[  w]     c cache[w  ]     
	  24   b cache[  w]     c cache[w  ]     
	  25   b cache[  w]     c cache[w  ]     
	  26   b cache[  w]     c cache[w  ]     
	  27   b cache[  w]     c cache[w  ]     
	  28   b cache[  w]     c cache[w  ]     
	  29   b cache[ ww]     c cache[  w]     
	---------------------------------------
	  30   a cache[ ww]     b cache[  w]     
	  31   a cache[ ww]     b cache[  w]     
	  32   a cache[ ww]     b cache[  w]     
	  33   a cache[ ww]     b cache[  w]     
	  34   a cache[ ww]     b cache[  w]     
	  35   a cache[ ww]     b cache[  w]     
	  36   a cache[ ww]     b cache[  w]     
	  37   a cache[ ww]     b cache[  w]     
	  38   a cache[ ww]     b cache[  w]     
	  39   a cache[w  ]     b cache[ ww]     
	---------------------------------------
	  40   c cache[w  ]     a cache[ ww]     
	  41   c cache[w  ]     a cache[ ww]     
	  42   c cache[w  ]     a cache[ ww]     
	  43   c cache[w  ]     a cache[ ww]     
	  44   c cache[w  ]     a cache[ ww]     
	  45   c cache[w  ]     a cache[ ww]     
	  46   c cache[w  ]     a cache[ ww]     
	  47   c cache[w  ]     a cache[ ww]     
	  48   c cache[w  ]     a cache[ ww]     
	  49   c cache[  w]     a cache[w  ]     
	---------------------------------------
	  50   b cache[  w]     c cache[w  ]     
	  51   b cache[  w]     c cache[w  ]     
	  52   b cache[  w]     c cache[w  ]     
	  53   b cache[  w]     c cache[w  ]     
	  54   b cache[  w]     c cache[w  ]     
	  55   b cache[  w]     c cache[w  ]     
	  56   b cache[  w]     c cache[w  ]     
	  57   b cache[  w]     c cache[w  ]     
	  58   b cache[  w]     c cache[w  ]     
	  59   b cache[ ww]     c cache[  w]     
	---------------------------------------
	  60   a cache[ ww]     b cache[  w]     
	  61   a cache[ ww]     b cache[  w]     
	  62   a cache[ ww]     b cache[  w]     
	  63   a cache[ ww]     b cache[  w]     
	  64   a cache[ ww]     b cache[  w]     
	  65   a cache[ ww]     b cache[  w]     
	  66   a cache[ ww]     b cache[  w]     
	  67   a cache[ ww]     b cache[  w]     
	  68   a cache[ ww]     b cache[  w]     
	  69   a cache[w  ]     b cache[ ww]     
	---------------------------------------
	  70   c cache[w  ]     a cache[ ww]     
	  71   c cache[w  ]     a cache[ ww]     
	  72   c cache[w  ]     a cache[ ww]     
	  73   c cache[w  ]     a cache[ ww]     
	  74   c cache[w  ]     a cache[ ww]     
	  75   c cache[w  ]     a cache[ ww]     
	  76   c cache[w  ]     a cache[ ww]     
	  77   c cache[w  ]     a cache[ ww]     
	  78   c cache[w  ]     a cache[ ww]     
	  79   c cache[  w]     a cache[w  ]     
	---------------------------------------
	  80   b cache[  w]     c cache[w  ]     
	  81   b cache[  w]     c cache[w  ]     
	  82   b cache[  w]     c cache[w  ]     
	  83   b cache[  w]     c cache[w  ]     
	  84   b cache[  w]     c cache[w  ]     
	  85   b cache[  w]     c cache[w  ]     
	  86   b cache[  w]     c cache[w  ]     
	  87   b cache[  w]     c cache[w  ]     
	  88   b cache[  w]     c cache[w  ]     
	  89   b cache[ ww]     c cache[  w]     
	---------------------------------------
	  90   a cache[ ww]     b cache[  w]     
	  91   a cache[ ww]     b cache[  w]     
	  92   a cache[ ww]     b cache[  w]     
	  93   a cache[ ww]     b cache[  w]     
	  94   a cache[ ww]     b cache[  w]     
	  95   a cache[ ww]     b cache[  w]     
	  96   a cache[ ww]     b cache[  w]     
	  97   a cache[ ww]     b cache[  w]     
	  98   a cache[ ww]     b cache[  w]     
	  99   a cache[w  ]     b cache[ ww]     
	---------------------------------------
	 100   c cache[w  ]     a cache[ ww]     
	 101   c cache[w  ]     a cache[ ww]     
	 102   c cache[w  ]     a cache[ ww]     
	 103   c cache[w  ]     a cache[ ww]     
	 104   c cache[w  ]     a cache[ ww]     
	 105   c cache[w  ]     a cache[ ww]     
	 106   c cache[w  ]     a cache[ ww]     
	 107   c cache[w  ]     a cache[ ww]     
	 108   c cache[w  ]     a cache[ ww]     
	 109   c cache[  w]     a cache[w  ]     
	---------------------------------------
	 110   b cache[  w]     c cache[w  ]     
	 111   b cache[  w]     c cache[w  ]     
	 112   b cache[  w]     c cache[w  ]     
	 113   b cache[  w]     c cache[w  ]     
	 114   b cache[  w]     c cache[w  ]     
	 115   b cache[  w]     c cache[w  ]     
	 116   b cache[  w]     c cache[w  ]     
	 117   b cache[  w]     c cache[w  ]     
	 118   b cache[  w]     c cache[w  ]     
	 119   b cache[ ww]     c cache[  w]     
	---------------------------------------
	 120   a cache[ ww]     b cache[  w]     
	 121   a cache[ ww]     b cache[  w]     
	 122   a cache[ ww]     b cache[  w]     
	 123   a cache[ ww]     b cache[  w]     
	 124   a cache[ ww]     b cache[  w]     
	 125   a cache[ ww]     b cache[  w]     
	 126   a cache[ ww]     b cache[  w]     
	 127   a cache[ ww]     b cache[  w]     
	 128   a cache[ ww]     b cache[  w]     
	 129   a cache[w  ]     b cache[ ww]     
	---------------------------------------
	 130   c cache[w  ]     a cache[ ww]     
	 131   c cache[w  ]     a cache[ ww]     
	 132   c cache[w  ]     a cache[ ww]     
	 133   c cache[w  ]     a cache[ ww]     
	 134   c cache[w  ]     a cache[ ww]     
	 135   c cache[w  ]     a cache[ ww]     
	 136   c cache[w  ]     a cache[ ww]     
	 137   c cache[w  ]     a cache[ ww]     
	 138   c cache[w  ]     a cache[ ww]     
	 139   c cache[  w]     a cache[w  ]     
	---------------------------------------
	 140   b cache[  w]     c cache[w  ]     
	 141   b cache[  w]     c cache[w  ]     
	 142   b cache[  w]     c cache[w  ]     
	 143   b cache[  w]     c cache[w  ]     
	 144   b cache[  w]     c cache[w  ]     
	 145   b cache[  w]     c cache[w  ]     
	 146   b cache[  w]     c cache[w  ]     
	 147   b cache[  w]     c cache[w  ]     
	 148   b cache[  w]     c cache[w  ]     
	 149   b cache[ ww]     c cache[  w]     
	
	Finished time 150
	
	Per-CPU stats
	  CPU 0  utilization 100.00 [ warm 0.00 ]
	  CPU 1  utilization 100.00 [ warm 0.00 ]
	```

	没有CPU缓存进程成功，因为进程a不断驱逐其他进程缓存，我们只利用两个核心并行运行程序：$(100 + 100 +100) / 2 = 150$

6. 现在我们将应用一些显式控制来研究缓存亲和性，如本章中所述。为此，您需要 -A 标志。该标志可用于限制调度程序可以在哪些 CPU 上放置特定作业。在本例中，让我们用它在 CPU 1 上放置作业“b”和“c”，同时将“a”限制在 CPU 0 上。这个魔法是通过输入这个 `python multi.py -n 2 -L a:100:100,b:100:50,c:100:50 -A a:0,b:1,c:1` 来完成的，不要忘记转使用各种跟踪选项来查看到底发生了什么！你能预测这个版本的运行速度吗？为什么它做得更好？两个处理器上的“a”、“b”和“c”的其他组合运行速度会更快还是更慢？

	```shell
	❯ python multi.py -n 2 -L a:100:100,b:100:50,c:100:50 -A a:0,b:1,c:1 -c -C -t
	ARG seed 0
	ARG job_num 3
	ARG max_run 100
	ARG max_wset 200
	ARG job_list a:100:100,b:100:50,c:100:50
	ARG affinity a:0,b:1,c:1
	ARG per_cpu_queues False
	ARG num_cpus 2
	ARG quantum 10
	ARG peek_interval 30
	ARG warmup_time 10
	ARG cache_size 100
	ARG random_order False
	ARG trace True
	ARG trace_time False
	ARG trace_cache True
	ARG trace_sched False
	ARG compute True
	
	Job name:a run_time:100 working_set_size:100
	Job name:b run_time:100 working_set_size:50
	Job name:c run_time:100 working_set_size:50
	
	Scheduler central queue: ['a', 'b', 'c']
	
	   0   a cache[   ]     b cache[   ]     
	   1   a cache[   ]     b cache[   ]     
	   2   a cache[   ]     b cache[   ]     
	   3   a cache[   ]     b cache[   ]     
	   4   a cache[   ]     b cache[   ]     
	   5   a cache[   ]     b cache[   ]     
	   6   a cache[   ]     b cache[   ]     
	   7   a cache[   ]     b cache[   ]     
	   8   a cache[   ]     b cache[   ]     
	   9   a cache[w  ]     b cache[ w ]     
	---------------------------------------
	  10   a cache[w  ]     c cache[ w ]     
	  11   a cache[w  ]     c cache[ w ]     
	  12   a cache[w  ]     c cache[ w ]     
	  13   a cache[w  ]     c cache[ w ]     
	  14   a cache[w  ]     c cache[ w ]     
	  15   a cache[w  ]     c cache[ w ]     
	  16   a cache[w  ]     c cache[ w ]     
	  17   a cache[w  ]     c cache[ w ]     
	  18   a cache[w  ]     c cache[ w ]     
	  19   a cache[w  ]     c cache[ ww]     
	---------------------------------------
	  20   a cache[w  ]     b cache[ ww]     
	  21   a cache[w  ]     b cache[ ww]     
	  22   a cache[w  ]     b cache[ ww]     
	  23   a cache[w  ]     b cache[ ww]     
	  24   a cache[w  ]     b cache[ ww]     
	  25   a cache[w  ]     b cache[ ww]     
	  26   a cache[w  ]     b cache[ ww]     
	  27   a cache[w  ]     b cache[ ww]     
	  28   a cache[w  ]     b cache[ ww]     
	  29   a cache[w  ]     b cache[ ww]     
	---------------------------------------
	  30   a cache[w  ]     c cache[ ww]     
	  31   a cache[w  ]     c cache[ ww]     
	  32   a cache[w  ]     c cache[ ww]     
	  33   a cache[w  ]     c cache[ ww]     
	  34   a cache[w  ]     c cache[ ww]     
	  35   a cache[w  ]     c cache[ ww]     
	  36   a cache[w  ]     c cache[ ww]     
	  37   a cache[w  ]     c cache[ ww]     
	  38   a cache[w  ]     c cache[ ww]     
	  39   a cache[w  ]     c cache[ ww]     
	---------------------------------------
	  40   a cache[w  ]     b cache[ ww]     
	  41   a cache[w  ]     b cache[ ww]     
	  42   a cache[w  ]     b cache[ ww]     
	  43   a cache[w  ]     b cache[ ww]     
	  44   a cache[w  ]     b cache[ ww]     
	  45   a cache[w  ]     b cache[ ww]     
	  46   a cache[w  ]     b cache[ ww]     
	  47   a cache[w  ]     b cache[ ww]     
	  48   a cache[w  ]     b cache[ ww]     
	  49   a cache[w  ]     b cache[ ww]     
	---------------------------------------
	  50   a cache[w  ]     c cache[ ww]     
	  51   a cache[w  ]     c cache[ ww]     
	  52   a cache[w  ]     c cache[ ww]     
	  53   a cache[w  ]     c cache[ ww]     
	  54   a cache[w  ]     c cache[ ww]     
	  55   - cache[w  ]     c cache[ ww]     
	  56   - cache[w  ]     c cache[ ww]     
	  57   - cache[w  ]     c cache[ ww]     
	  58   - cache[w  ]     c cache[ ww]     
	  59   - cache[w  ]     c cache[ ww]     
	---------------------------------------
	  60   - cache[w  ]     b cache[ ww]     
	  61   - cache[w  ]     b cache[ ww]     
	  62   - cache[w  ]     b cache[ ww]     
	  63   - cache[w  ]     b cache[ ww]     
	  64   - cache[w  ]     b cache[ ww]     
	  65   - cache[w  ]     b cache[ ww]     
	  66   - cache[w  ]     b cache[ ww]     
	  67   - cache[w  ]     b cache[ ww]     
	  68   - cache[w  ]     b cache[ ww]     
	  69   - cache[w  ]     b cache[ ww]     
	---------------------------------------
	  70   - cache[w  ]     c cache[ ww]     
	  71   - cache[w  ]     c cache[ ww]     
	  72   - cache[w  ]     c cache[ ww]     
	  73   - cache[w  ]     c cache[ ww]     
	  74   - cache[w  ]     c cache[ ww]     
	  75   - cache[w  ]     c cache[ ww]     
	  76   - cache[w  ]     c cache[ ww]     
	  77   - cache[w  ]     c cache[ ww]     
	  78   - cache[w  ]     c cache[ ww]     
	  79   - cache[w  ]     c cache[ ww]     
	---------------------------------------
	  80   - cache[w  ]     b cache[ ww]     
	  81   - cache[w  ]     b cache[ ww]     
	  82   - cache[w  ]     b cache[ ww]     
	  83   - cache[w  ]     b cache[ ww]     
	  84   - cache[w  ]     b cache[ ww]     
	  85   - cache[w  ]     b cache[ ww]     
	  86   - cache[w  ]     b cache[ ww]     
	  87   - cache[w  ]     b cache[ ww]     
	  88   - cache[w  ]     b cache[ ww]     
	  89   - cache[w  ]     b cache[ ww]     
	---------------------------------------
	  90   - cache[w  ]     c cache[ ww]     
	  91   - cache[w  ]     c cache[ ww]     
	  92   - cache[w  ]     c cache[ ww]     
	  93   - cache[w  ]     c cache[ ww]     
	  94   - cache[w  ]     c cache[ ww]     
	  95   - cache[w  ]     c cache[ ww]     
	  96   - cache[w  ]     c cache[ ww]     
	  97   - cache[w  ]     c cache[ ww]     
	  98   - cache[w  ]     c cache[ ww]     
	  99   - cache[w  ]     c cache[ ww]     
	---------------------------------------
	 100   - cache[w  ]     b cache[ ww]     
	 101   - cache[w  ]     b cache[ ww]     
	 102   - cache[w  ]     b cache[ ww]     
	 103   - cache[w  ]     b cache[ ww]     
	 104   - cache[w  ]     b cache[ ww]     
	 105   - cache[w  ]     c cache[ ww]     
	 106   - cache[w  ]     c cache[ ww]     
	 107   - cache[w  ]     c cache[ ww]     
	 108   - cache[w  ]     c cache[ ww]     
	 109   - cache[w  ]     c cache[ ww]     
	
	Finished time 110
	
	Per-CPU stats
	  CPU 0  utilization 50.00 [ warm 40.91 ]
	  CPU 1  utilization 100.00 [ warm 81.82 ]
	```

	进程 a 的完成时间为10（预热a）+90/2（a的剩余作业）=55，先于 b 和 c 完成其作业，因此总运行时间为 10(预热 b) + 10(预热 c) + (90 + 90)(b 和 c 的剩余作业) / 2 = 110。其他组合将导致更长的运行时间，因为“a”不断逐出其他进程的缓存（因此，其他进程一旦占用 CPU 就会逐出“a”缓存内容）

7. 缓存多处理器的一个有趣的方面是，与在单个处理器上运行作业相比，使用多个 CPU（及其缓存）时作业的速度有可能超出预期。具体来说，当你在 N 个 CPU 上运行时，有时你可以加速超过 N 倍，这种情况称为超线性加速。要对此进行实验，请使用此处的作业描述 ( `-L a:100:100,b:100:100,c:100:100` ) 和一个小型缓存 ( `-M 50` ) 来创建三个作业。在具有 1、2 和 3 个 CPU ( `-n 1, -n 2, -n 3` ) 的系统上运行此程序。现在，执行相同的操作，但每个 CPU 缓存的大小为 100。随着 CPU 数量的增加，您对性能有何看法？使用 `-c` 确认您的猜测，并使用其他跟踪标志进行更深入的研究。

	```shell
	python multi.py -n 1 -L a:100:100,b:100:100,c:100:100 -M 50 -c -C -t
	python multi.py -n 2 -L a:100:100,b:100:100,c:100:100 -M 50 -c -C -t
	python multi.py -n 3 -L a:100:100,b:100:100,c:100:100 -M 50 -c -C -t
	```

	完成时间分别为300、200、100。

	```shell
	python multi.py -n 1 -L a:100:100,b:100:100,c:100:100 -M 100 -c -C -t
	python multi.py -n 2 -L a:100:100,b:100:100,c:100:100 -M 100 -c -C -t
	python multi.py -n 3 -L a:100:100,b:100:100,c:100:100 -M 100 -c -C -t
	```

	完成时间为300、200、55。这是因为当每个进程独占一个CPU时缓存就不会被驱逐，而当CPU数小于作业数，且`cache`无法满足多个作业共同的缓存，则每次切换上下文就会清空缓存，无法得到运行加速。

8. 模拟器的另一个值得研究的方面是每个 CPU 队列调度选项（MQMS），即 -p 参数。再次使用两个 CPU 运行，以及这三个作业配置 ( `-L a:100:100,b:100:50,c:100:50` )。与上面设置的手动控制的亲和力限制相比，此选项的效果如何？当您将“窥探间隔”(-P) 更改为更低或更高的值时，性能会如何变化？随着 CPU 数量的增加，这种基于 CPU 的方法如何发挥作用？

	```shell
	python multi.py -n 2 -L a:100:100,b:100:50,c:100:50 -T -C -p -t -c
	```

	完成时间为100s。

	```shell
	python multi.py -n 2 -L a:100:100,b:100:50,c:100:50 -T -C -t -c
	```

	完成时间为150s，此选项效果能这么好，是因为MQMS使每个CPU独占一个调度队列，有的CPU缓存就能得到使用而不会被切换，所以运行速度能得到提升。

	```
	python multi.py -n 2 -L a:100:100,b:100:50,c:100:50 -T -C -p -t -c -P 9
	```

	如果将窥探间隔更改为更低或更高的值，性能会得到提升，这是因为窥探将CPU0的调度队列中c作业给CPU1偷走了，而b，c一起工作也能满足CPU缓存，故运行速度能得到加速。而为什么CPU0没偷走CPU1的作业呢？我们可以看下面代码

	```py
	# if it is time to steal
	if self.system_time > 0 and self.system_time % self.peek_interval == 0:
	    for cpu in range(self.num_cpus):
	        if len(self.per_cpu_sched_queue[cpu]) == 0:
	            # find IDLE job in some other CPUs queue
	            other_cpu_list = list(range(self.num_cpus))
	            other_cpu_list.remove(cpu)
	            # random choice of which CPU to look at
	            other_cpu = random.choice(other_cpu_list)
	            # print('cpu %d is idle' % cpu)
	            # print('-> look at %d' % other_cpu)
	
	            for job_name in self.per_cpu_sched_queue[other_cpu]:
	                # print('---> examine job %s' % job_name)
	                if len(self.jobs[job_name].affinity) == 0 or cpu in self.jobs[job_name]:
	                    self.per_cpu_sched_queue[other_cpu].remove(job_name)
	                    self.per_cpu_sched_queue[cpu].append(job_name)
	                    # print('stole job %s from %d to %d' % (job_name, other_cpu, cpu))
	                    break
	```

	根据遍历规则，因为CPU0先偷，CPU1后偷，所以最后CPU1还是有两个作业，且一定是b，c。

	特殊的，当P为9时，性能反而会下降，你可以运行一下代码：

	```
	python multi.py -n 2 -L a:100:100,b:100:50,c:100:50 -T -C -p -t -c -P 9
	```

	当CPU数量提升，例如为3，那么运行速度都能得到提升，完成时间为55s。

	```shell
	python multi.py -n 3 -L a:100:100,b:100:50,c:100:50 -T -C -p -t -c
	```