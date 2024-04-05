# Note/Translation
在虚拟内存管理器中，当您拥有大量可用内存时，生活会很容易。发生页面错误时，您在空闲页面列表中找到一个空闲页面，并将其分配给发生错误的页面。嘿，操作系统，恭喜！你又这么做了。

不幸的是，当空闲内存很少时，事情就会变得更有趣。在这种情况下，这种**内存压力**会迫使操作系统开始调出页面，为主动使用的页面腾出空间。决定调出哪个页面（或哪些页面）封装在操作系统的替换策略中；从历史上看，这是早期虚拟内存系统做出的最重要的决定之一，因为较旧的系统几乎没有物理内存。至少，这是一套值得更多了解的有趣政策。因此我们的问题关键是：操作系统如何决定从内存中驱逐哪一页（或多页）？这一决定是由系统的替换策略做出的，该策略通常遵循一些一般原则（如下所述），但也包括某些调整以避免极端情况行为。

## 1  缓存管理 

在深入讨论策略之前，我们首先更详细地描述我们试图解决的问题。鉴于主内存保存系统中所有页面的某些子集，它可以正确地被视为系统中虚拟内存页面的**缓存**。因此，我们为此缓存选择替换策略的目标是最小化缓存未命中的次数，即最小化我们必须从磁盘获取页面的次数。或者，我们的目标可以视为最大化缓存命中数，即在内存中找到被访问页面的次数。

了解缓存命中和未命中的次数后，我们可以计算程序的平均内存访问时间 (average memory access time, AMAT)（计算机架构师计算硬件缓存的度量标准]）。具体来说，给定这些值，我们可以计算程序的 AMAT，如下所示： 
$$
\text{AMAT}=\text{T}_M+\text{P}_{Miss}\cdot\text{T}_\text{D}
$$
其中$\text{T}_M$表示访问内存的成本，$\text{T}_D$表示访问磁盘的成本，$\text{T}_\text{Miss}$是在缓存中找不到数据的概率（未命中）； $\text{T}_\text{Miss}$ 从 0.0 到 1.0 不等，有时我们指的是丢失率百分比而不是概率（例如，10% 的丢失率意味着 $\text{T}_\text{Miss}=0.10$）。请注意，您总是要为访问内存中的数据付出代价；然而，当您未命中时，您必须额外支付从磁盘获取数据的成本。

例如，让我们想象一台具有（微小）地址空间的机器：4KB，具有 256 字节页面。因此，虚拟地址有两个组成部分：4 位 VPN（最高有效位）和 8 位偏移量（最低有效位）。因此，该示例中的进程总共可以访问 $2^4$ 或 16 个虚拟页。在此示例中，进程生成以下内存引用（即虚拟地址）：0x000、0x100、0x200、0x300、0x400、0x500、0x600、0x700、0x800、0x900。这些虚拟地址指的是地址空间前十页中每一页的第一个字节（页号是每个虚拟地址的第一个十六进制数字）。

让我们进一步假设除虚拟页 3 之外的所有页都已在内存中。因此，我们的内存引用序列将遇到以下行为：命中，命中，命中，未命中，命中，命中，命中，命中，命中，命中。我们可以计算命中率（在内存中找到的引用的百分比）：90%，因为十分之九的引用在内存中。因此，为命中率率为 10%（$\text{P}_\text{Miss}=0.10$）。一般情况下，$\text{P}_\text{Hit}+\text{P}_\text{Miss}=0.10=1.0$；命中率加上未命中率之和为 100%。

为了计算AMAT，我们需要知道访问内存的成本和访问磁盘的成本。假设访问内存的成本 (TM ) 约为 100 纳秒，访问磁盘的成本 (TD ) 约为 10 毫秒，则我们有以下 AMAT：100ns + 0.1 · 10ms，即 100ns + 1ms，即 1.0001 ms，或大约1毫秒。如果我们的命中率为 99.9%（Pmiss = 0.001），结果就完全不同：AMAT 为 10.1 微秒，或者大约快 100 倍。当命中率接近 100% 时，AMAT 接近 100 纳秒。

不幸的是，正如您在此示例中所看到的，现代系统中磁盘访问的成本非常高，即使很小的未命中率也会很快主导正在运行的程序的整体 AMAT。显然，我们需要尽可能避免未命中或以磁盘速率缓慢运行。解决这个问题的一种方法是仔细制定明智的策略，就像我们现在所做的那样。

## 2 最佳替换策略

为了更好地理解特定替换策略的工作原理，最好将其与最佳替换策略进行比较。事实证明，Belady多年前就提出了这样一种最佳策略（他最初称之为 MIN）。最佳替换策略会导致总体未命中次数最少。Belady 证明了一种简单（但不幸的是，很难实现！）的方法，即<font color="red">替换未来访问距离最远的页面是最佳策略</font>，它能带来尽可能少的缓存缺失。

> <center>TIP：与最优算法进行比较是有用的 
>     
> </center>
>
> 虽然最优算法作为实际策略并不实用，但在模拟或其他研究中作为比较点却非常有用。如果说你的新算法有 80% 的命中率，这并没有任何意义；如果说最优算法有 82% 的命中率（因此你的新方法非常接近最优算法），这将使结果更有意义，并赋予其背景。因此，在你进行的任何研究中，知道最佳值是什么可以让你进行更好的比较，显示还有多少改进的可能，以及什么时候你可以停止改进你的策略，因为它已经足够接近理想值了。

希望最优策略背后的直觉是有道理的。你可以这样想：如果你必须扔掉某个页面，为什么不扔掉离现在最远的那个页面呢？这样做实质上是在说，缓存中的所有其他页面都比最远的那一页更重要。原因很简单：在访问最远的那一页之前，你会先访问其他页面。

让我们通过一个简单的例子来了解最优策略的决策。假设一个程序访问以下虚拟页面流：0, 1, 2, 0, 1, 3, 0, 3, 1, 2, 1。下图显示了最优策略的行为，假设缓存可以容纳三个页面。

![image-20240404223004837](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Tracing-The-Optimal-Policy-Example.png)

从图中可以看到以下操作。毫不奇怪，前三次访问都是未命中，因为高速缓存一开始是空的；这种未命中有时被称为<font color="red">冷启动未命中（或强制未命中）</font>。然后我们再次访问第 0 页和第 1 页，它们都在缓存中被命中。最后，我们又发生了一次缺失（第 3 页），但这次缓存已满，必须进行替换！这就引出了一个问题：我们应该替换哪一页？根据最优策略，我们可以检查当前缓存中每个页面（0、1 和 2）的未来访问情况，发现 0 页面几乎会立即被访问，1 页面会稍后被访问，而 2 页面在未来会被访问得最远。因此，最优策略很容易做出选择：删除第 2 页，这样缓存中就有了第 0、1 和 3 页。接下来的三个引用都是命中的，但我们到了 2 页时，却又遭遇了一次未命中。在这里，最优策略再次检查了缓存中每个页面（0、1 和 3）的未来访问情况，发现只要不驱逐第 1 页（即将被访问），我们就不会有问题。示例显示第 3 页被驱逐，尽管第 0 页也可以被驱逐。最后，我们命中了第 1 页，跟踪完成。

我们还可以计算缓存的命中率：在 6 次命中和 5 次未命中的情况下，命中率为$\frac{\text{Hits}}{\text{Hits+Misses}}$命中+未命中，即 $\frac{6}{6+5}$或 54.5%。我们还可以计算强制未命中的命中率（即忽略对给定页面的第一次未命中），结果是命中率为 85.7%。

遗憾的是，正如我们之前在开发调度策略时所看到的，未来并不普遍为人所知；你不可能为通用操作系统构建最优策略。因此，在开发真正的、可部署的策略时，我们将重点关注那些能找到其他方法来决定驱逐哪个页面的方法。因此，最佳策略只能作为一个比较点，以了解我们离 "完美 "还有多远。

> <center>缓存未命中的类型
>     
> </center>
>
>  在计算机体系结构领域，架构师有时发现按类型将未命中特征分为以下三类之一很有用：<font color="red">强制、容量和冲突未命中</font>，有时称为 3 C。发生强制未命中（或冷启动未命中）是因为缓存一开始就是空的，并且这是对该项目的第一次引用；相反，由于缓存空间不足，必须逐出一个项目才能将新项目放入缓存，因此会发生容量未命中。第三种类型的未命中（冲突未命中）出现在硬件中，因为由于所谓的“组关联性”（setassociativity），项目可以放置在硬件缓存中的位置受到限制。它不会出现在操作系统页面缓存中，因为此类缓存始终是完全关联的，即页面可以放置在内存中的位置没有限制。

## 3 FIFO（先进先出策略）

许多早期系统避免了尝试接近最优的复杂性，而是采用了非常简单的替换策略。例如，一些系统使用 FIFO（先进先出）替换，其中页面在进入系统时被简单地放置在队列中；当发生替换时，队列尾部的页面（“先入”页面）将被逐出。 FIFO 有一个很大的优点：实现起来非常简单。

如下图所示，为 FIFO 在我们的示例引用流上的表现。我们再次以对页面 0、1 和 2 的三个强制未命中开始跟踪，然后命中 0 和 1。接下来，引用第 3 页，导致未命中；使用 FIFO 进行替换决策很容易：选择第一个页面（图中的缓存状态按 FIFO 顺序保存，第一个页面位于左侧），即第 0 页。不幸的是，我们的下一个访问是页 0，导致另一次丢失和替换（页 1）。然后我们命中了第 3 页，但第 1 和第 2 页未命中，最后命中了第 1 页。

将 FIFO 与最佳值相比，FIFO 的命中率明显更差：36.4% 的命中率（或 57.1%，不包括强制命中率）。 FIFO 根本无法确定块的重要性：即使页 0 已被访问多次，FIFO 仍然会将其踢出，仅仅因为它是第一个进入内存的块。

![image-20240404224745502](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Tracing-The-FIFO-Policy-Example.png)

> <center>BELADY 的异常</center>
>
>  Belady（最优策略提出者）和同事发现了一个有趣的引用流，其行为有点出乎意料。内存引用流：1,2,3,4,1,2,5,1,2,3,4,5。他们研究的替换策略是先进先出。有趣的部分是：当缓存大小从 3 页移动到 4 页时，缓存命中率如何变化。一般来说，当缓存变大时，您会期望缓存命中率会增加（变得更好）。但在这种情况下，如果使用 FIFO，情况会变得更糟！这种奇怪的行为通常被称为BELADY 的异常。其他一些策略，例如 LRU，不会遇到这个问题。事实证明，LRU 具有所谓的堆栈属性 。对于具有此属性的算法，大小为 N + 1 的缓存自然包含大小为 N 的缓存的内容。因此，当增加缓存大小时，命中率将保持不变或提高。 FIFO 和 Random（以及其他）显然不遵守堆栈属性，因此容易受到异常行为的影响。
>
> ![image-20240404225836336](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/belady's_anomaly.png)

## 4 随机策略

另一种类似的替换策略是随机策略，它只是在内存压力下选择随机页面进行替换。 Random 具有与 FIFO 类似的属性；它实现起来很简单，但它并没有真正尝试太智能地选择要驱逐的块。如下图所示，让我们看看 Random 在我们著名的示例引用流上的表现如何。

![image-20240404230206775](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Tracing-The-Random-Policy-Example.png)

当然，Random 的表现完全取决于 Random 在其选择中的幸运（或不幸）程度。在上面的例子中，Random 的表现比 FIFO 好一点，比最优的差一点。事实上，我们可以运行随机实验数千次并确定它的总体表现。

下图显示了 Random 在 10,000 多次试验中实现了多少次命中，每次试验都有不同的随机种子。正如您所看到的，有时（仅超过 40% 的时间），Random 与最佳效果一样好，在示例跟踪上实现了 6 个命中；有时效果更糟，达到 2 次或更少。随机抽奖的效果如何取决于抽奖的运气。

![image-20240404230533898](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Random-Performance-Over-10000-Trials.png)

## 5 使用历史记录：LRU

不幸的是，任何像 FIFO 或 Random 这样简单的策略都可能存在一个常见问题：它可能会踢出一个重要的页面，而该页面将被再次引用。 FIFO踢出最先带入的页面；如果这恰好是一个带有重要代码或数据结构的页面，那么它无论如何都会被丢弃，即使它很快就会被分页回来。因此，FIFO、Random和类似的策略不太可能达到最佳效果；需要更智能的东西。

正如我们对调度策略所做的那样，为了改善我们对未来的猜测，我们再次依靠过去并以历史为指导。例如，如果一个程序在不久的过去访问过一个页面，那么它很可能在不久的将来再次访问它。

页面替换策略可以使用的一类历史信息是**频率**。如果一个页面已被访问多次，也许它不应该被替换，因为它显然具有某些价值。页面的一个更常用的属性是访问时间；页面被访问的时间越近，也许就越有可能被再次访问。

这一系列策略基于人们所说的**局部性原则** ，这基本上只是对程序及其行为的观察。这个原则的意思很简单，就是程序倾向于非常频繁地访问某些代码序列（例如，在循环中）和数据结构（例如，循环访问的数组）；因此，我们应该尝试使用历史记录来找出哪些页面是重要的，并在驱逐时将这些页面保留在内存中。

因此，一系列简单的基于历史的算法诞生了。当必须进行逐出时，**最不常用 (LFU) 策略**会替换最不常用的页面。类似地，<font color="red">最近最少使用（LRU）策略</font>替换最近最少使用的页面。这些算法很容易记住：一旦您知道名称，您就确切地知道它的作用，这是名称的一个极好的属性。

为了更好地理解 LRU，我们来看看 LRU 在示例引用流上的表现，如下图所示。

![image-20240404231309931](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Trace-The-LRU.png)

从图中，您可以看到 LRU 如何利用历史记录比 Random 或 FIFO 等无状态策略做得更好。在示例中，LRU 在第一次必须替换页面时会逐出页面 2，因为最近访问过页面 0 和 1。然后它会替换第 0 页，因为最近访问了第 1 页和第 3 页。在这两种情况下，LRU 基于历史的决定都被证明是正确的，因此下一个引用会被命中。因此，在我们的示例中，LRU 表现得尽可能好，其性能达到最佳水平。

我们还应该注意到，这些算法存在相反的情况：**最常用（MFU）和最近使用（MRU）**。在大多数情况下（不是全部！），这些策略效果不佳，因为它们忽略了大多数程序所展示的局部性，而不是利用它。

> <center>局部性的类型
>     
> </center>
>
>  程序往往会表现出两种局部性。第一种是<font color="red">空间局部性</font>，即如果一个页面 P 被访问，那么它周围的页面（如 P - 1 或 P + 1）也很可能被访问。第二种是<font color="red">时间局部性</font>，即在不久的将来，在过去被访问过的页面很可能会再次被访问。存在这些类型的局部性的假设在硬件系统的缓存层次结构中发挥着重要作用，硬件系统部署多个级别的指令、数据和地址转换缓存，以帮助程序在存在此类局部性时快速运行。
>
> 当然，通常所说的局部性原则并不是所有程序都必须遵守的硬性规定。事实上，有些程序以相当随机的方式访问内存（或磁盘），在其访问流中并不表现出太多或任何局部性。因此，在设计任何类型的缓存（硬件或软件）时，虽然局部性是一个值得牢记的好东西，但它并不能保证成功。相反，它是一种启发式方法，在计算机系统设计中经常被证明是有用的。

## 6 工作负载实例

让我们再看几个例子，以便更好地理解其中一些策略的行为方式。在这里，我们将检查更复杂的工作负载而不是小跟踪。然而，即使是这些工作负载也被大大简化了；更好的研究将包括应用程序跟踪。

我们的第一个工作负载没有局部性，这意味着每个引用都是对访问页面集中的随机页面。在这个简单的示例中，工作负载随着时间的推移访问 100 个唯一页面，随机选择下一个页面进行引用；总共访问了 10,000 个页面。在实验中，我们将缓存大小从非常小（1 页）更改为足以容纳所有唯一页面（100 页），以便了解每个策略在缓存大小范围内的行为方式。

下图绘制了最优、LRU、随机和 FIFO 的实验结果。图中的y轴表示每个策略达到的命中率； x 轴改变缓存大小。

![image-20240405092645055](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/The-No-Locality-Workload.png)

我们可以从图中得出许多结论。首先，当工作负载不存在局部性时，您使用哪种实际策略并不重要； LRU、FIFO 和 Random 的执行方式都是相同的，命中率完全由缓存的大小决定。其次，当缓存足够大以适应整个工作负载时，使用哪种策略也并不重要；当所有引用的块都适合缓存时，所有策略（甚至随机）都会收敛到 100% 命中率。最后，您可以看到最优策略的性能明显优于现实策略；如果可能的话，展望未来可以更好地进行替代。

我们研究的下一个工作负载称为“80-20”工作负载，它表现出局部性：80% 的引用是针对 20% 的页面（“热门”页面）；其余 20% 的引用是针对其余 80% 的页面（“冷”页面）。在我们的工作负载中，总共有 100 个不同的页面；因此，大部分时间都引用“热”页面，其余时间引用“冷”页面。下图显示了策略如何在此工作负载下执行。

![image-20240405094547089](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/The-80-20-Workload.png)

从图中可以看出，虽然随机和 FIFO 都表现得相当好，但 LRU 表现更好，因为它更有可能保留热点页；由于这些页面过去曾被频繁引用，因此很可能在不久的将来再次被引用。 Optimal 再次表现更好，说明 LRU 的历史信息并不完美。

您现在可能想知道：LRU 相对于 Random 和 FIFO 的改进真的有那么大吗？答案是“视情况而定”。如果每次未命中的成本都非常高（并不罕见），那么即使命中率小幅增加（为命中率降低）也会对性能产生巨大影响。如果未命中的代价不那么高，那么 LRU 可能带来的好处当然就不那么重要了。

让我们看一下最终的工作负载。我们将其称为“循环顺序”工作负载，因为在其中，我们按顺序引用 50 个页面，从 0 开始，然后是 1，...，直到第 49 页，然后我们循环，重复这些访问，以获得对 50 个唯一页面的总共 10,000 次访问。下图显示了该工作负载下策略的行为。

![image-20240405095255830](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/The-Looping-Workload.png)

这种工作负载在许多应用程序（包括重要的商业应用程序，例如数据库）中很常见，代表了 LRU 和 FIFO 的最坏情况。这些算法在循环顺序工作负载下会踢出旧页面；不幸的是，由于工作负载的循环性质，这些较旧的页面将比策略希望保留在缓存中的页面更早被访问。事实上，即使缓存大小为 49，50 页的循环顺序工作负载也会导致 0% 的命中率。有趣的是，随机的效果明显更好，虽然还没有完全接近最佳，但至少实现了非零命中率。事实证明随机有一些很好的特性；其中一个属性就是没有奇怪的极端行为。

## 7 实现历史算法

正如您所看到的，LRU 等算法通常比 FIFO 或随机等简单策略做得更好，后者可能会丢弃重要的页面。不幸的是，历史策略给我们带来了新的挑战：我们如何实现它们？

我们以 LRU 为例。为了完美地实现它，我们需要做大量的工作。具体来说，在每次页面访问（即每次内存访问，无论是指令提取还是加载或存储）时，我们必须更新某些数据结构以将此页面移动到列表的前面（即 MRU 侧）。与 FIFO 相比，只有当页面被逐出（通过删除最先进入的页面）或将新页面添加到列表（最后进入的一侧）时，才会访问 FIFO 页面列表。为了跟踪哪些页面被最少和最近使用，系统必须在每次内存引用上进行一些统计工作。显然，如果不小心处理，这种统计工作可能会大大降低性能。

一种有助于加快速度的方法是添加一点硬件支持。例如，机器可以在每次页面访问时更新内存中的时间字段（例如，这可能位于每个进程的页表中，或者仅位于内存中的某个单独的数组中，系统中的每个物理页有一个条目）。因此，当访问页面时，时间字段将由硬件设置为当前时间。然后，当替换页面时，操作系统可以简单地扫描系统中的所有时间字段以找到最近最少使用的页面。

不幸的是，随着系统中页面数量的增长，扫描大量的时间只是为了找到绝对最近最少使用的页面是非常昂贵的。想象一下一台具有 4GB 内存的现代机器，被分成 4KB 页面。这台机器有 100 万个页面，因此即使在现代 CPU 速度下，找到 LRU 页面也需要很长时间。这就引出了一个问题：我们真的需要找到绝对最旧的页面来替换吗？我们可以通过近似来代替吗？

## 8 近似LRU

事实证明，答案是肯定的：从计算开销的角度来看，近似 LRU 更可行，而且实际上许多现代系统都是这么做的。这个想法需要一些硬件支持，以**使用位**（有时称为**引用位**）的形式，其中第一个是在第一个具有分页的系统，Atlas one-level store中实现的。系统的每一页都有一个使用位，并且使用位存在于内存中的某个位置（例如，它们可能位于每个进程的页表中，或者只是位于某个数组中）。每当引用页面（即读取或写入）时，硬件都会将使用位设置为 1。不过，硬件永远不会清除该位（即将其设置为 0）；这是操作系统的责任。

操作系统如何利用使用位位来近似LRU？嗯，可能有很多方法，但时钟算法提出了一种简单的方法。想象一下系统的所有页面排列在一个循环列表中。时钟指针首先指向某个特定的页面（哪一页并不重要）。当必须进行替换时，操作系统会检查当前指向的页面 P 的使用位是否为 1 或 0。如果为 1，则意味着页面 P 最近被使用过，因此不是替换的良好候选者。因此，P 的使用位设置为 0（清除），并且时钟指针递增到下一页 (P + 1)。该算法继续进行，直到找到一个设置为 0 的使用位，这意味着该页面最近没有被使用过（或者，在最坏的情况下，所有页面都已被使用过，并且我们现在已经搜索了整个页面集，从而清除了所有位）。

![image-20240405102346573](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/The-Clock-Page-Replacement-Algorithm.png)

请注意，此方法并不是使用使用位来近似 LRU 的唯一方法。事实上，任何定期清除使用位，然后区分哪些页面使用 1 与 0 的位来决定替换哪个的方法都可以。 Corbato 的时钟算法只是一种早期方法，它取得了一些成功，并且具有不重复扫描所有内存寻找未使用页面的良好特性。

时钟算法变体的行为如下图所示。

![image-20240405102131971](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/The-80-20-Workload-With-Clock.png)

该变体在进行替换时随机扫描页面；当它遇到引用位设置为 1 的页面时，它会清除该位（即将其设置为 0）；当它发现引用位设置为 0 的页面时，它会选择它作为受害者。正如您所看到的，虽然它的表现不如完美的 LRU，但它比根本不考虑历史的方法要好。

## 9 考虑脏位（在时钟算法中）

对时钟算法（最初也是由 Corbato提出的）的一个小修改是，在内存中额外考虑页面是否被修改。这样做的原因是：如果一个页面被修改过，因而是脏的，就必须将其写回磁盘以将其剔除，而这是很昂贵的。如果页面未被修改（因此是干净的），则剔除是免费的；物理页可以简单地重新用于其他目的，而无需额外的 I/O。因此，一些虚拟机系统更倾向于驱逐干净的页面，而不是脏页面。

为支持这种行为，硬件应包含一个修改位（又称脏位）。该位在任何页面被写入时都会被设置，因此可被纳入页面替换算法中。例如，时钟算法可以修改为首先扫描未使用且干净的页面，然后再扫描未使用且脏的页面，以此类推。

## 10 其他VM策略

页面替换并不是VM子系统采用的唯一策略（尽管它可能是最重要的）。例如，操作系统还必须决定何时将页面放入内存。该策略有时称为<font color="red">页面选择策略</font>，为操作系统提供了一些不同的选项。

对于大多数页面，操作系统仅使用**按需分页**，这意味着操作系统在访问页面时将页面调入内存，可以说是“按需”。当然，操作系统可以猜测某个页面即将被使用，从而提前将其引入；这种行为称为**预取**，只有在有合理的成功机会时才应执行。例如，某些系统会假设如果将代码页 P 放入内存中，则该代码页 P+1 可能很快就会被访问，因此也应该放入内存中。

![image-20240405103420900](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Most-OS-Do-On-Demand.png)

另一个策略决定操作系统如何将页面写入磁盘。当然，它们可以简单地一次写出一个；然而，许多系统反而将大量待处理的写入集中在内存中，并通过一次（更有效的）写入将它们写入磁盘。这种行为通常称为**集群**或简单的**写入分组**，并且由于磁盘驱动器的性质而有效，磁盘驱动器执行单个大写操作比许多小写操作更有效。

## 11 抖动

当内存被过度占用，运行进程的内存需求超过了可用的物理内存时，操作系统该怎么办？在这种情况下，系统会不断地分页，这种情况有时被称为 “抖动”（thrashing）。

![image-20240405105226249](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Degree-of-multiprogramming-CPU-Utilization.png)

一些早期的操作系统有一套相当复杂的机制，可以检测并处理发生的抖动。例如，在给定一组进程的情况下，系统可以决定不运行其中的一个子进程集，希望减少的进程集的工作集（它们正在积极使用的页面）能够适合内存，从而取得进展。这种方法通常被称为 "准入控制"（acmission control），它指出，有时少做一点工作比什么都做不好要好，这是我们在现实生活和现代计算机系统中经常遇到的情况。

当前的一些系统对内存超载采取了更为严厉的措施。例如，某些版本的 Linux 会在内存被超量占用时运行一个 "内存不足杀手"；这个守护进程会选择一个内存密集型进程并将其杀死，从而以一种不太明显的方式减少内存。这种方法虽然能成功减少内存压力，但也会带来一些问题，例如，它可能会杀死 X 服务器，从而导致任何需要显示的应用程序无法使用。
# Program Explanation

这个模拟器 `paging-policy.py` 允许您尝试不同的页面替换策略。例如，让我们检查一下 LRU 如何使用大小为 3 的缓存处理一系列页面引用： 

> 0 1 2 0 1 3 0 3 1 2 1

为此，请按如下方式运行模拟器：

```shell
❯ python paging-policy.py --addresses=0,1,2,0,1,3,0,3,1,2,1 --policy=LRU --cachesize=3 -c
ARG addresses 0,1,2,0,1,3,0,3,1,2,1
ARG addressfile 
ARG numaddrs 10
ARG policy LRU
ARG clockbits 2
ARG cachesize 3
ARG maxpage 10
ARG seed 0
ARG notrace False

Solving...

Access: 0  MISS LRU ->          [0] <- MRU Replaced:- [Hits:0 Misses:1]
Access: 1  MISS LRU ->       [0, 1] <- MRU Replaced:- [Hits:0 Misses:2]
Access: 2  MISS LRU ->    [0, 1, 2] <- MRU Replaced:- [Hits:0 Misses:3]
Access: 0  HIT  LRU ->    [1, 2, 0] <- MRU Replaced:- [Hits:1 Misses:3]
Access: 1  HIT  LRU ->    [2, 0, 1] <- MRU Replaced:- [Hits:2 Misses:3]
Access: 3  MISS LRU ->    [0, 1, 3] <- MRU Replaced:2 [Hits:2 Misses:4]
Access: 0  HIT  LRU ->    [1, 3, 0] <- MRU Replaced:- [Hits:3 Misses:4]
Access: 3  HIT  LRU ->    [1, 0, 3] <- MRU Replaced:- [Hits:4 Misses:4]
Access: 1  HIT  LRU ->    [0, 3, 1] <- MRU Replaced:- [Hits:5 Misses:4]
Access: 2  MISS LRU ->    [3, 1, 2] <- MRU Replaced:0 [Hits:5 Misses:5]
Access: 1  HIT  LRU ->    [3, 2, 1] <- MRU Replaced:- [Hits:6 Misses:5]

FINALSTATS hits 6   misses 5   hitrate 54.55
```

下页列出了分页策略的完整可能参数集，其中包括许多用于改变策略的选项、如何指定/生成地址以及其他重要参数（例如缓存大小）。

```shell
❯ python paging-policy.py -h
Usage: paging-policy.py [options]

Options:
  -h, --help            show this help message and exit
  -a ADDRESSES, --addresses=ADDRESSES
                        a set of comma-separated pages to access; -1 means
                        randomly generate
  -f ADDRESSFILE, --addressfile=ADDRESSFILE
                        a file with a bunch of addresses in it
  -n NUMADDRS, --numaddrs=NUMADDRS
                        if -a (--addresses) is -1, this is the number of addrs
                        to generate
  -p POLICY, --policy=POLICY
                        replacement policy: FIFO, LRU, OPT, UNOPT, RAND, CLOCK
  -b CLOCKBITS, --clockbits=CLOCKBITS
                        for CLOCK policy, how many clock bits to use
  -C CACHESIZE, --cachesize=CACHESIZE
                        size of the page cache, in pages
  -m MAXPAGE, --maxpage=MAXPAGE
                        if randomly generating page accesses, this is the max
                        page number
  -s SEED, --seed=SEED  random number seed
  -N, --notrace         do not print out a detailed trace
  -c, --compute         compute answers for me
```

像往常一样，“-c”用于解决特定问题，而如果没有它，则仅列出访问（并且程序不会告诉您特定访问是否成功）。要生成随机问题，您可以传入“-n/--numaddrs”作为程序应随机生成的地址数，而不是使用“-a/--addresses”传递某些页面引用，其中“ -s/--seed”用于指定不同的随机种子。例如：

```shell
❯ python paging-policy.py -s 10 -n 3
ARG addresses -1
ARG addressfile 
ARG numaddrs 3
ARG policy FIFO
ARG clockbits 2
ARG cachesize 3
ARG maxpage 10
ARG seed 10
ARG notrace False

Assuming a replacement policy of FIFO, and a cache of size 3 pages,
figure out whether each of the following page references hit or miss
in the page cache.

Access: 5  Hit/Miss?  State of Memory?
Access: 4  Hit/Miss?  State of Memory?
Access: 5  Hit/Miss?  State of Memory?
```

正如您所看到的，在本例中，我们指定“-n 3”，这意味着程序应生成 3 个随机页面引用，它确实这样做了：5、4 和 5。还指定了随机种子 (10)，即是让我们得到这些特定的数字。自己解决之后，让程序通过传入相同的参数但使用“-c”（此处仅显示相关部分）来为您解决问题：

```shell
Solving...

Access: 5  MISS FirstIn ->          [5] <- Lastin  Replaced:- [Hits:0 Misses:1]
Access: 4  MISS FirstIn ->       [5, 4] <- Lastin  Replaced:- [Hits:0 Misses:2]
Access: 5  HIT  FirstIn ->       [5, 4] <- Lastin  Replaced:- [Hits:1 Misses:2]

FINALSTATS hits 1   misses 2   hitrate 33.33
```

默认策略是 FIFO，但也有其他策略可用，包括 LRU、MRU、OPT（最优替换策略，展望未来以查看最佳替换内容）、UNOPT（悲观替换）、RAND（随机替换）和 CLOCK（执行时钟算法）。 CLOCK 算法还采用另一个参数 (-b)，它规定每页应保留多少位；时钟位越多，算法在确定将哪些页面保留在内存中时就应该越好。

其他选项包括：“-C/--cachesize”，它更改页面缓存的大小； “-m/--maxpage”，这是模拟器为您生成引用时将使用的最大页码；和“-f/--addressfile”，它允许您指定一个包含地址的文件，以防您希望从真实应用程序获取跟踪或以其他方式使用长跟踪作为输入。

> 有趣的两个例子：
>
> ```shell
> > python paging-policy.py -C 3 -a 1,2,3,4,1,2,5,1,2,3,4,5
> Access: 1  MISS FirstIn ->          [1] <- Lastin  Replaced:- [Hits:0 Misses:1]
> Access: 2  MISS FirstIn ->       [1, 2] <- Lastin  Replaced:- [Hits:0 Misses:2]
> Access: 3  MISS FirstIn ->    [1, 2, 3] <- Lastin  Replaced:- [Hits:0 Misses:3]
> Access: 4  MISS FirstIn ->    [2, 3, 4] <- Lastin  Replaced:1 [Hits:0 Misses:4]
> Access: 1  MISS FirstIn ->    [3, 4, 1] <- Lastin  Replaced:2 [Hits:0 Misses:5]
> Access: 2  MISS FirstIn ->    [4, 1, 2] <- Lastin  Replaced:3 [Hits:0 Misses:6]
> Access: 5  MISS FirstIn ->    [1, 2, 5] <- Lastin  Replaced:4 [Hits:0 Misses:7]
> Access: 1  HIT  FirstIn ->    [1, 2, 5] <- Lastin  Replaced:- [Hits:1 Misses:7]
> Access: 2  HIT  FirstIn ->    [1, 2, 5] <- Lastin  Replaced:- [Hits:2 Misses:7]
> Access: 3  MISS FirstIn ->    [2, 5, 3] <- Lastin  Replaced:1 [Hits:2 Misses:8]
> Access: 4  MISS FirstIn ->    [5, 3, 4] <- Lastin  Replaced:2 [Hits:2 Misses:9]
> Access: 5  HIT  FirstIn ->    [5, 3, 4] <- Lastin  Replaced:- [Hits:3 Misses:9]
> 
> FINALSTATS hits 3   misses 9   hitrate 25.00
> 
> python paging-policy.py -C 4 -a 1,2,3,4,1,2,5,1,2,3,4,5
> Access: 1  MISS FirstIn ->          [1] <- Lastin  Replaced:- [Hits:0 Misses:1]
> Access: 2  MISS FirstIn ->       [1, 2] <- Lastin  Replaced:- [Hits:0 Misses:2]
> Access: 3  MISS FirstIn ->    [1, 2, 3] <- Lastin  Replaced:- [Hits:0 Misses:3]
> Access: 4  MISS FirstIn -> [1, 2, 3, 4] <- Lastin  Replaced:- [Hits:0 Misses:4]
> Access: 1  HIT  FirstIn -> [1, 2, 3, 4] <- Lastin  Replaced:- [Hits:1 Misses:4]
> Access: 2  HIT  FirstIn -> [1, 2, 3, 4] <- Lastin  Replaced:- [Hits:2 Misses:4]
> Access: 5  MISS FirstIn -> [2, 3, 4, 5] <- Lastin  Replaced:1 [Hits:2 Misses:5]
> Access: 1  MISS FirstIn -> [3, 4, 5, 1] <- Lastin  Replaced:2 [Hits:2 Misses:6]
> Access: 2  MISS FirstIn -> [4, 5, 1, 2] <- Lastin  Replaced:3 [Hits:2 Misses:7]
> Access: 3  MISS FirstIn -> [5, 1, 2, 3] <- Lastin  Replaced:4 [Hits:2 Misses:8]
> Access: 4  MISS FirstIn -> [1, 2, 3, 4] <- Lastin  Replaced:5 [Hits:2 Misses:9]
> Access: 5  MISS FirstIn -> [2, 3, 4, 5] <- Lastin  Replaced:1 [Hits:2 Misses:10]
> 
> FINALSTATS hits 2   misses 10   hitrate 16.67
> ```
>
> 正如书中所述。当缓存变为4时，FIFO的性能反而下降，而LRU不会遇到这个问题。因为LRU 具有所谓的堆栈属性 。对于具有此属性的算法，大小为 N + 1 的缓存自然包含大小为 N 的缓存的内容。因此，当增加缓存大小时，命中率将保持不变或提高。 FIFO 和 Random（以及其他）显然不遵守堆栈属性，因此容易受到异常行为的影响。

# Homework
1. 使用以下参数生成随机地址： `-s 0 -n 10` 、 `-s 1 -n 10` 和 `-s 2 -n 10` 。将策略从 FIFO 更改为 LRU，再更改为 OPT。计算所述地址跟踪中的每个访问是命中还是未命中。

	```shell
	❯ python paging-policy.py -s 0 -n 10 -c
	Access: 8  MISS FirstIn ->          [8] <- Lastin  Replaced:- [Hits:0 Misses:1]
	Access: 7  MISS FirstIn ->       [8, 7] <- Lastin  Replaced:- [Hits:0 Misses:2]
	Access: 4  MISS FirstIn ->    [8, 7, 4] <- Lastin  Replaced:- [Hits:0 Misses:3]
	Access: 2  MISS FirstIn ->    [7, 4, 2] <- Lastin  Replaced:8 [Hits:0 Misses:4]
	Access: 5  MISS FirstIn ->    [4, 2, 5] <- Lastin  Replaced:7 [Hits:0 Misses:5]
	Access: 4  HIT  FirstIn ->    [4, 2, 5] <- Lastin  Replaced:- [Hits:1 Misses:5]
	Access: 7  MISS FirstIn ->    [2, 5, 7] <- Lastin  Replaced:4 [Hits:1 Misses:6]
	Access: 3  MISS FirstIn ->    [5, 7, 3] <- Lastin  Replaced:2 [Hits:1 Misses:7]
	Access: 4  MISS FirstIn ->    [7, 3, 4] <- Lastin  Replaced:5 [Hits:1 Misses:8]
	Access: 5  MISS FirstIn ->    [3, 4, 5] <- Lastin  Replaced:7 [Hits:1 Misses:9]
	
	FINALSTATS hits 1   misses 9   hitrate 10.00
	❯ python paging-policy.py -s 0 -n 10 -c --policy=LRU
	Access: 8  MISS LRU ->          [8] <- MRU Replaced:- [Hits:0 Misses:1]
	Access: 7  MISS LRU ->       [8, 7] <- MRU Replaced:- [Hits:0 Misses:2]
	Access: 4  MISS LRU ->    [8, 7, 4] <- MRU Replaced:- [Hits:0 Misses:3]
	Access: 2  MISS LRU ->    [7, 4, 2] <- MRU Replaced:8 [Hits:0 Misses:4]
	Access: 5  MISS LRU ->    [4, 2, 5] <- MRU Replaced:7 [Hits:0 Misses:5]
	Access: 4  HIT  LRU ->    [2, 5, 4] <- MRU Replaced:- [Hits:1 Misses:5]
	Access: 7  MISS LRU ->    [5, 4, 7] <- MRU Replaced:2 [Hits:1 Misses:6]
	Access: 3  MISS LRU ->    [4, 7, 3] <- MRU Replaced:5 [Hits:1 Misses:7]
	Access: 4  HIT  LRU ->    [7, 3, 4] <- MRU Replaced:- [Hits:2 Misses:7]
	Access: 5  MISS LRU ->    [3, 4, 5] <- MRU Replaced:7 [Hits:2 Misses:8]
	
	FINALSTATS hits 2   misses 8   hitrate 20.00
	❯ python paging-policy.py -s 0 -n 10 -c --policy=OPT
	Access: 8  MISS Left  ->          [8] <- Right Replaced:- [Hits:0 Misses:1]
	Access: 7  MISS Left  ->       [8, 7] <- Right Replaced:- [Hits:0 Misses:2]
	Access: 4  MISS Left  ->    [8, 7, 4] <- Right Replaced:- [Hits:0 Misses:3]
	Access: 2  MISS Left  ->    [7, 4, 2] <- Right Replaced:8 [Hits:0 Misses:4]
	Access: 5  MISS Left  ->    [7, 4, 5] <- Right Replaced:2 [Hits:0 Misses:5]
	Access: 4  HIT  Left  ->    [7, 4, 5] <- Right Replaced:- [Hits:1 Misses:5]
	Access: 7  HIT  Left  ->    [7, 4, 5] <- Right Replaced:- [Hits:2 Misses:5]
	Access: 3  MISS Left  ->    [4, 5, 3] <- Right Replaced:7 [Hits:2 Misses:6]
	Access: 4  HIT  Left  ->    [4, 5, 3] <- Right Replaced:- [Hits:3 Misses:6]
	Access: 5  HIT  Left  ->    [4, 5, 3] <- Right Replaced:- [Hits:4 Misses:6]
	
	FINALSTATS hits 4   misses 6   hitrate 40.00
	```

2. 对于大小为 5 的缓存，为以下每个策略生成最坏情况的地址引用流：FIFO、LRU 和 MRU（最坏情况的引用流会导致最多可能的丢失）。对于最坏情况的引用流，需要多大的缓存才能显着提高性能并接近 OPT？

	```shell
	> python paging-policy.py --addresses=0,1,2,3,4,5,0,1,2,3,4,5 --policy=FIFO --cachesize=5 -c
	Access: 0  MISS FirstIn ->          [0] <- Lastin  Replaced:- [Hits:0 Misses:1]
	Access: 1  MISS FirstIn ->       [0, 1] <- Lastin  Replaced:- [Hits:0 Misses:2]
	Access: 2  MISS FirstIn ->    [0, 1, 2] <- Lastin  Replaced:- [Hits:0 Misses:3]
	Access: 3  MISS FirstIn -> [0, 1, 2, 3] <- Lastin  Replaced:- [Hits:0 Misses:4]
	Access: 4  MISS FirstIn -> [0, 1, 2, 3, 4] <- Lastin  Replaced:- [Hits:0 Misses:5]
	Access: 5  MISS FirstIn -> [1, 2, 3, 4, 5] <- Lastin  Replaced:0 [Hits:0 Misses:6]
	Access: 0  MISS FirstIn -> [2, 3, 4, 5, 0] <- Lastin  Replaced:1 [Hits:0 Misses:7]
	Access: 1  MISS FirstIn -> [3, 4, 5, 0, 1] <- Lastin  Replaced:2 [Hits:0 Misses:8]
	Access: 2  MISS FirstIn -> [4, 5, 0, 1, 2] <- Lastin  Replaced:3 [Hits:0 Misses:9]
	Access: 3  MISS FirstIn -> [5, 0, 1, 2, 3] <- Lastin  Replaced:4 [Hits:0 Misses:10]
	Access: 4  MISS FirstIn -> [0, 1, 2, 3, 4] <- Lastin  Replaced:5 [Hits:0 Misses:11]
	Access: 5  MISS FirstIn -> [1, 2, 3, 4, 5] <- Lastin  Replaced:0 [Hits:0 Misses:12]
	
	FINALSTATS hits 0   misses 12   hitrate 0.00
	> python paging-policy.py --addresses=0,1,2,3,4,5,0,1,2,3,4,5 --policy=LRU --cachesize=5 -c
	Access: 0  MISS LRU ->          [0] <- MRU Replaced:- [Hits:0 Misses:1]
	Access: 1  MISS LRU ->       [0, 1] <- MRU Replaced:- [Hits:0 Misses:2]
	Access: 2  MISS LRU ->    [0, 1, 2] <- MRU Replaced:- [Hits:0 Misses:3]
	Access: 3  MISS LRU -> [0, 1, 2, 3] <- MRU Replaced:- [Hits:0 Misses:4]
	Access: 4  MISS LRU -> [0, 1, 2, 3, 4] <- MRU Replaced:- [Hits:0 Misses:5]
	Access: 5  MISS LRU -> [1, 2, 3, 4, 5] <- MRU Replaced:0 [Hits:0 Misses:6]
	Access: 0  MISS LRU -> [2, 3, 4, 5, 0] <- MRU Replaced:1 [Hits:0 Misses:7]
	Access: 1  MISS LRU -> [3, 4, 5, 0, 1] <- MRU Replaced:2 [Hits:0 Misses:8]
	Access: 2  MISS LRU -> [4, 5, 0, 1, 2] <- MRU Replaced:3 [Hits:0 Misses:9]
	Access: 3  MISS LRU -> [5, 0, 1, 2, 3] <- MRU Replaced:4 [Hits:0 Misses:10]
	Access: 4  MISS LRU -> [0, 1, 2, 3, 4] <- MRU Replaced:5 [Hits:0 Misses:11]
	Access: 5  MISS LRU -> [1, 2, 3, 4, 5] <- MRU Replaced:0 [Hits:0 Misses:12]
	
	FINALSTATS hits 0   misses 12   hitrate 0.00
	> python paging-policy.py --addresses=0,1,2,3,4,5,4,5,4,5,4,5 --policy=MRU --cachesize=5 -c
	Access: 0  MISS LRU ->          [0] <- MRU Replaced:- [Hits:0 Misses:1]
	Access: 1  MISS LRU ->       [0, 1] <- MRU Replaced:- [Hits:0 Misses:2]
	Access: 2  MISS LRU ->    [0, 1, 2] <- MRU Replaced:- [Hits:0 Misses:3]
	Access: 3  MISS LRU -> [0, 1, 2, 3] <- MRU Replaced:- [Hits:0 Misses:4]
	Access: 4  MISS LRU -> [0, 1, 2, 3, 4] <- MRU Replaced:- [Hits:0 Misses:5]
	Access: 5  MISS LRU -> [0, 1, 2, 3, 5] <- MRU Replaced:4 [Hits:0 Misses:6]
	Access: 4  MISS LRU -> [0, 1, 2, 3, 4] <- MRU Replaced:5 [Hits:0 Misses:7]
	Access: 5  MISS LRU -> [0, 1, 2, 3, 5] <- MRU Replaced:4 [Hits:0 Misses:8]
	Access: 4  MISS LRU -> [0, 1, 2, 3, 4] <- MRU Replaced:5 [Hits:0 Misses:9]
	Access: 5  MISS LRU -> [0, 1, 2, 3, 5] <- MRU Replaced:4 [Hits:0 Misses:10]
	Access: 4  MISS LRU -> [0, 1, 2, 3, 4] <- MRU Replaced:5 [Hits:0 Misses:11]
	Access: 5  MISS LRU -> [0, 1, 2, 3, 5] <- MRU Replaced:4 [Hits:0 Misses:12]
	
	FINALSTATS hits 0   misses 12   hitrate 0.00
	```

3. 生成随机跟踪（使用 python 或 perl）。您期望不同的策略在这样的跟踪上执行如何？

	```shell
	> python paging-policy.py -s 0 -n 10 -c
	FINALSTATS hits 1   misses 9   hitrate 10.00
	
	> python paging-policy.py -s 0 -n 10 -c --policy=LRU
	FINALSTATS hits 2   misses 8   hitrate 20.00
	
	> python paging-policy.py -s 0 -n 10 -c --policy=OPT
	FINALSTATS hits 4   misses 6   hitrate 40.00
	
	> python paging-policy.py -s 0 -n 10 -c --policy=UNOPT
	FINALSTATS hits 0   misses 10   hitrate 0.00
	
	> python paging-policy.py -s 0 -n 10 -c --policy=RAND
	FINALSTATS hits 0   misses 10   hitrate 0.00
	
	> python paging-policy.py -s 0 -n 10 -c --policy=CLOCK
	FINALSTATS hits 1   misses 9   hitrate 10.00
	```

4. 现在生成一个具有一定局部性的轨迹。如何生成这样的轨迹？LRU 的性能如何？LRU 比 RAND 好多少？CLOCK 的性能如何？不同时钟比特数的 CLOCK 效果如何？

	`generate-trace.py`用来生成具有一定局部性的轨迹。通过二八定律实现，数据保存在`vpn.txt`中。

	```shell
	❯ python generate-trace.py
	[3, 0, 6, 6, 6, 6, 7, 0, 6, 6]
	❯ python paging-policy.py -f vpn.txt --policy=LRU -c
	FINALSTATS hits 6   misses 4   hitrate 60.00
	❯ python paging-policy.py -f vpn.txt --policy=RAND -c
	FINALSTATS hits 5   misses 5   hitrate 50.00
	❯ python paging-policy.py -f vpn.txt --policy=CLOCK -c -b 0
	FINALSTATS hits 5   misses 5   hitrate 50.00
	❯ python paging-policy.py -f vpn.txt --policy=CLOCK -c -b 1
	FINALSTATS hits 5   misses 5   hitrate 50.00
	❯ python paging-policy.py -f vpn.txt --policy=CLOCK -c -b 2
	FINALSTATS hits 4   misses 6   hitrate 40.00
	❯ python paging-policy.py -f vpn.txt --policy=CLOCK -c -b 3
	FINALSTATS hits 5   misses 5   hitrate 50.00
	```

5. 使用像 `valgrind` 这样的程序来检测真实的应用程序并生成虚拟页面引用流。例如，运行 `valgrind --tool=lackey --trace-mem=yes ls` 将输出程序 `ls` 进行的每条指令和数据引用的几乎完整的引用跟踪。为了使其对上面的模拟器有用，您必须首先将每个虚拟内存引用转换为虚拟页号引用（通过屏蔽偏移量并将结果位向下移动来完成）。为了满足大部分请求，您的应用程序跟踪需要多大的缓存？随着缓存大小的增加，绘制其工作集的图表。

	```shell
	> valgrind --tool=lackey --trace-mem=yes ls &> ls-trace.txt
	> python transform.py // transform to VPN
	> sh run.sh			  // run the script
	> python plot.py 	  // plot the figure of the workset
	```

	![workload](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/homework-workload.png)
