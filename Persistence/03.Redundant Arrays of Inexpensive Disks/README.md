# Note/Translation
## 1 引言

当我们使用磁盘时，

* 有时我们希望它更快； I/O 操作速度很慢，因此可能成为整个系统的瓶颈。
* 有时我们希望它更大；越来越多的数据被放到网上，因此我们的磁盘变得越来越满。
* 有时我们希望它更加可靠；当磁盘发生故障时，如果我们的数据没有备份，所有有价值的数据都会消失。

所以关键问题是：我们怎样才能制作大型、快速、可靠的存储系统？有哪些关键技术？不同方法之间的权衡是什么？

在本章中，我们将介绍<font color="red">廉价磁盘冗余阵列，即 RAIDs</font>，这是一种<font color="red">协同使用多个磁盘来构建更快、更大、更可靠的磁盘系统的技术</font>。该术语是由加州大学伯克利分校的一组研究人员（由 David Patterson 和 Randy Katz 教授以及当时的学生 Garth Gibson 领导）于 20 世纪 80 年代末提出的。大约在这个时候，许多不同的研究人员同时得出了使用多个磁盘来构建更好的存储系统的基本思想。

从外部来看，RAIDs 看起来像一个磁盘：一组可以读取或写入的块。在内部，RAID 是一个复杂的系统，由多个磁盘、内存（易失性和非易失性）以及一个或多个用于管理系统的处理器组成。硬件 RAID 非常类似于计算机系统，专门用于管理一组磁盘的任务。

与单个磁盘相比，RAID 具有如下优点。

1. **性能**。并行使用多个磁盘可以大大加快 I/O 时间。
2. **容量**。大数据集需要大磁盘。
3. **可靠性**；将数据分布在多个磁盘上（没有 RAID 技术）使得数据容易受到单个磁盘丢失的影响；通过某种形式的冗余，RAID 可以容忍磁盘丢失并继续运行，就像没有发生任何问题一样。

> <center>提示：透明性有助于部署</center>
>
> 在考虑如何为系统添加新功能时，应始终考虑能否以透明的方式添加这些功能，即不要求更改系统的其他部分。要求完全重写现有软件（或彻底改变硬件）会降低想法产生影响的几率。RAID 就是一个很好的例子，当然，它的透明性也是其成功的原因之一；管理员可以安装一个基于 SCSI 的 RAID 存储阵列，而不是 SCSI 磁盘，系统的其他部分（主机、操作系统等）无需做任何改动即可开始使用。通过解决部署问题，RAID 从一开始就取得了巨大成功。

令人惊奇的是，RAID 为使用 RAID 的系统提供了透明的优势，也就是说，对于主机系统而言，RAID 就像一个大磁盘。当然，透明性的好处在于，它使人们能够简单地用 RAID 更换磁盘，而无需更改任何软件；操作系统和客户端应用程序无需修改即可继续运行。通过这种方式，透明性大大提高了 RAID 的可部署性，使用户和管理员在使用 RAID 时不必担心软件兼容性问题。

我们现在讨论 RAID 的一些重要方面。我们首先讨论**接口和故障模型**，然后讨论如何从**容量**、**可靠性**和**性能**这三个重要方面评估 RAID 设计。然后，我们将讨论对 RAID 设计和实施很重要的其他一些问题。

## 2 接口和 RAID 内部结构

对于上面的文件系统来说，RAID 看起来就像一个大的、（希望）快速且（希望）可靠的磁盘。就像单个磁盘一样，它表现为块的线性阵列，每个块都可以由文件系统（或其他客户端）读取或写入。

当文件系统向 RAID 发出逻辑 I/O 请求时，RAID 内部必须计算要访问哪个磁盘（或多个磁盘）才能完成请求，然后发出一个或多个物理 I/O 来完成该请求。这些物理 I/O 的确切性质取决于 RAID 级别，我们将在下面详细讨论。然而，作为一个简单的例子，考虑一个 RAID，它保留每个块的两个副本（每个副本位于一个单独的磁盘上）；当写入此类镜像 RAID 系统时，RAID 必须为其发出的每一个逻辑 I/O 执行两次物理 I/O。

RAID 系统通常是一个独立的硬件盒，通过标准连接（如 SCSI 或 SATA）与主机相连。不过，RAID 的内部结构相当复杂，包括一个**运行固件的微控制器**，用于指导 RAID 的运行；**DRAM 等易失性内存**，用于在数据块读写时对其进行缓冲；在某些情况下，非易失性内存用于对写入进行安全缓冲；甚至可能还包括用于执行奇偶校验计算的专用逻辑（在某些 RAID 级别中非常有用，下文将详细介绍）。从高层次来看，RAID 在很大程度上是一种专用计算机系统：它有处理器、内存和磁盘；但它运行的不是应用程序，而是专门用于操作 RAID 的软件。

## 3 故障模型

为了理解 RAID 并比较不同的方法，我们必须有一个故障模型。 RAID 旨在检测某些类型的磁盘故障并从中恢复；因此，准确地了解会出现哪些故障对于实现可行的设计至关重要。

我们假设的第一个故障模型非常简单，被称为<font color="red">故障停止故障模型</font>。在此模型中，磁盘可以恰好处于两种状态之一：工作或故障。使用工作磁盘，所有块都可以读取或写入。相反，当磁盘发生故障时，我们假设它永久丢失。

故障停止模型的一个关键方面是它对故障检测的假设。具体来说，当磁盘出现故障时，我们假设很容易检测到这一点。例如，在 RAID 阵列中，我们假设 RAID 控制器硬件（或软件）可以立即观察到磁盘发生故障。

因此，目前我们不必担心更复杂的“静默”故障，例如磁盘损坏。我们也不必担心单个块在其他工作磁盘上变得无法访问（有时称为潜在扇区错误）。稍后我们将考虑这些更复杂（不幸的是，更现实）的磁盘故障。

## 4 如何评估 RAID

构建 RAID 有多种不同的方法。这些方法中的每一种都有不同的特征，值得评估，以便了解它们的优点和缺点。

具体来说，我们将依据三个指标评估每个 RAID 设计。

1. 第一个指标是**容量**；给定一组 N 个磁盘，每个磁盘有 B 个块，RAID 客户端有多少可用容量？如果没有冗余，答案是$N\cdot B$；相反，如果我们有一个系统保留每个块的两个副本（称为镜像），则我们获得的有用容量为$\frac{N\cdot B}{2}$。不同的方案（例如基于奇偶校验的方案）往往介于两者之间。

2. 第二个指标是**可靠性**。给定的设计可以容忍多少个磁盘故障？根据我们的故障模型，我们假设只有整个磁盘可能发生故障；在之后（即数据完整性），我们将考虑如何处理更复杂的故障模式。

3. 最后一个指标是**性能**。评估性能有些困难，因为它很大程度上取决于磁盘阵列的工作负载。因此，在评估性能之前，我们将首先介绍一组应该考虑的典型工作负载。

	> 我们现在考虑三种重要的 RAID 设计：RAID Level 0（条带化）、RAID Level 1（镜像）和 RAID Levels 4/5（基于奇偶校验的冗余）。将这些设计中的每一个命名为“level”源于atterson，Gibson和Katz在伯克利的开创性工作。

在分析 RAID 性能时，可以考虑两种不同的性能指标。首先是<font color="red">单请求延迟</font>。了解 RAID 的单个 I/O 请求的延迟非常有用，因为它揭示了单个逻辑 I/O 操作期间可以存在多少并行性。第二个是 RAID 的<font color="red">稳态吞吐量</font>，即许多并发请求的总带宽。由于 RAID 通常用于高性能环境，因此稳态带宽至关重要，因此将成为我们分析的主要焦点。

为了更详细地了解吞吐量，我们需要提出一些感兴趣的工作负载。在本次讨论中，我们假设有两种类型的工作负载：顺序工作负载和随机工作负载。对于顺序工作负载，我们假设对数组的请求来自大的连续块；例如，访问 1 MB 数据的请求（或一系列请求），从块 $x$ 开始到块 ($x+1$ MB) 结束，将被视为连续的。顺序工作负载在许多环境中都很常见（想象一下在大文件中搜索关键字），因此被认为很重要。

对于随机工作负载，我们假设每个请求都相当小，并且每个请求都发送到磁盘上不同的随机位置。例如，随机请求流可能首先访问逻辑地址 10 处的 4KB，然后访问逻辑地址 550,000，然后访问 20,100，等等。一些重要的工作负载，例如数据库管理系统 (DBMS) 上的事务工作负载，表现出这种类型的访问模式，因此它被认为是重要的工作负载。

当然，真正的工作负载并不那么简单，并且通常混合了顺序和看似随机的组件以及两者之间的行为。为了简单起见，我们只考虑这两种可能性。

正如您所知，顺序和随机工作负载将导致磁盘的性能特征存在很大差异。通过顺序访问，磁盘以其最有效的模式运行，花费很少的时间寻道和等待旋转，而大部分时间用于传输数据。对于随机访问，情况恰恰相反：大部分时间都花在寻道和等待旋转上，而花在传输数据上的时间相对较少。为了在我们的分析中捕捉到这种差异，我们假设磁盘在顺序工作负载下可以以 $S\text{ MB/s}$ 的速度传输数据，在随机工作负载下可以以$R\text{ MB/s}$的速度传输数据。一般来说，S 远大于 R（即 S ≫ R）。

为了确保我们理解这种差异，让我们做一个简单的练习。具体来说，我们根据以下磁盘特性来计算 S 和 R。假设顺序传输平均大小为 10 MB，随机传输平均大小为 10 KB。

另外，假设以下磁盘特性： 

* 平均寻道时间 7 ms 
* 平均旋转延迟 3 ms 
* 磁盘传输速率 50 MB/s 

为了计算 S，我们需要首先计算出典型的 10 MB 传输所花费的时间。首先，我们花费 7 毫秒进行寻道，然后花费 3 毫秒进行旋转。最后，传输开始；$\frac{10\text{ MB}}{50\text{ MB/s}}$ 导致传输时间为 $\frac{1}{5}s$，即 200 毫秒。因此，对于每个 10 MB 请求，我们花费 210 毫秒完成请求。要计算 S，我们只需：
$$
S=\frac{\text{Amount of Data}}{\text{Time to access}}=\frac{10\text{ MB}}{210ms}=47.62\text{ MB/s}
$$
正如我们所看到的，由于传输数据花费大量时间，S 非常接近磁盘的峰值带宽（寻道和旋转成本已摊销）。我们可以类似地计算 R。寻道和旋转是一样的；然后我们计算传输所花费的时间，即$\frac{10\text{ KB}}{50\text{ MB/s}}$ ，即 0.195 毫秒。
$$
R=\frac{\text{Amount of Data}}{\text{Time to access}}=\frac{10\text{ KB}}{10ms+0.195ms}=0.981\text{ MB/s}
$$
我们可以看到，R 小于 1 MB/s，S/R 接近 50 MB/s

## 5 RAID Level 0：条带化

### 5.1 基本介绍

第一个 RAID Level实际上根本不是 RAID Level，因为没有冗余。不过，RAID Level（或称条带化）是性能和容量的绝佳上限，因此值得了解。

最简单的条带化形式是在系统磁盘上对数据块进行条带化，如下所示（这里假设有 4 个磁盘阵列）：

![image-20240414220046035](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/RAID_0_Simple_Striping.png)

从上表中，你可以了解到基本概念：将磁盘阵列的数据块以循环方式分布在磁盘上。这种方法的设计目的是在请求连续的磁盘阵列块时，从磁盘阵列中提取最大的并行性（例如，在大的顺序读取中）。我们将同一行中的块称为一个条带；因此，块 0、1、2 和 3 位于上述同一条带中。

在示例中，我们做了一个简化假设，即在移动下一个磁盘之前，每个磁盘上只放置一个块（每个块的大小为 4KB）。然而，这种安排并不一定是必须的。例如，我们可以将块跨越多个磁盘进行排列，如下表所示：

![image-20240414220112661](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Striping_With_A_Bigger_Chunk_Size.png)

在本例中，我们在每个磁盘上放置两个 4KB 的数据块，然后再移动到下一个磁盘。因此，该 RAID 阵列的数据块大小为 8KB，一个磁条由 4 个数据块或 32KB 的数据组成。

> <center>RAID 映射问题</center>
>
> 在研究 RAID 的容量、可靠性和性能特征之前，我们首先介绍一下所谓的映射问题。所有RAID阵列都会出现这个问题；简而言之，给定一个要读取或写入的逻辑块，RAID 如何准确地知道要访问哪个物理磁盘和偏移量？
>
> 对于这些简单的 RAID levels，我们不需要太复杂就能将逻辑块正确映射到其物理位置。以上面的第一个条带化示例为例（块大小=1，数据块 = 4KB）。在这种情况下，给定逻辑块地址 A，RAID 可以使用两个简单的方程轻松计算所需的磁盘和偏移量： 
>
> ```c
> Disk   = A % number_of_disks
> Offset = A / number_of_disks
> ```
>
> 请注意，这些都是整数运算（例如，4 / 3 = 1 而不是 1.33333...）。让我们通过一个简单的例子来看看这些方程是如何工作的。想象一下，在上面的第一个 RAID 中，块 14 的请求到达。假设有 4 个磁盘，这意味着我们感兴趣的磁盘是 (14 % 4 = 2)：磁盘 2。确切的块计算公式为： （14 / 4 = 3)：块3。因此，块14应该在第三个磁盘（磁盘2，从0开始）的第四个块（块3，从0开始）上找到，这正是它所在的位置。

### 5.2 块大小

块大小主要影响阵列的性能。例如，较小的块大小意味着许多文件将在许多磁盘上进行条带化，从而提高单个文件读写的并行性；然而，跨多个磁盘访问块的定位时间会增加，因为整个请求的定位时间由跨所有驱动器的请求定位时间的最大值决定。

另一方面，大的块大小会降低这种文件内并行性，从而依赖多个并发请求来实现高吞吐量。然而，大的块大小会减少定位时间；例如，如果单个文件适合一个块并因此被放置在单个磁盘上，则访问它时产生的定位时间将只是单个磁盘的定位时间。

因此，确定“最佳”块大小很难，因为它需要大量有关磁盘系统的工作负载的知识。对于本次讨论的其余部分，我们将假设数组使用单个块 (4KB) 的块大小。大多数数组使用较大的块大小（例如 64 KB），但对于我们下面讨论的问题，确切的块大小并不重要；因此，为了简单起见，我们使用单个块。

### 5.3 评估 条带化

现在让我们来评估条带化的容量、可靠性和性能。从容量的角度来看，条带化是完美的：给定 N 块磁盘，每块磁盘的大小为 B 块，条带化就能提供$N\cdot B$ 个有用容量容量块。从可靠性的角度来看，条带化也是完美的，但也是糟糕的：任何磁盘故障都会导致数据丢失。

最后，性能也非常出色：所有磁盘都能利用，而且往往是并行利用，为用户的 I/O 请求提供服务。例如，从延迟的角度来看，单块请求的延迟应与单个磁盘的延迟基本相同；毕竟 RAID-0 只需将该请求重定向到其中一个磁盘即可。从稳态吞吐量的角度来看，我们希望获得系统的全部带宽。因此，吞吐量等于 N（磁盘数量）乘以 S（单个磁盘的连续带宽）。对于大量随机 I/O，我们可以再次使用所有磁盘，从而获得 $N\cdot R$MB/s。我们将在下文中看到，这些值既是最简单的计算值，也是与其他 RAID levels相比的上限。

## 6 RAID Level 1：镜像

### 6.1 基本介绍

除条带化之外，我们的第一个 RAID Level称为 RAID Level 1，或镜像。在镜像系统中，我们只需为系统中的每个数据块制作一个以上的副本；当然，每个副本都应放置在单独的磁盘上。通过这种方法，我们可以<font color="red">容忍磁盘故障</font>。

在典型的镜像系统中，我们假设 RAID 会为每个逻辑块保留两个物理副本。下面是一个例子：

![image-20240414224917155](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Simple_RAID_1_Mirroring.png)

在示例中，磁盘 0 和磁盘 1 的内容完全相同，磁盘 2 和磁盘 3 的内容也完全相同；<font color="red">数据在这些镜像对中进行条带化处理</font>。事实上，你可能已经注意到，在磁盘上放置块拷贝有多种不同的方法。上面的排列方式很常见，有时也被称为 **RAID-10** 或（**RAID 1+0**），因为它使用镜像对（RAID-1），然后在其上使用条带（RAID-0）；另一种常见的排列方式是 **RAID-01**（或 **RAID 0+1**），它包含两个大型条带（RAID-0）阵列，然后在其上使用镜像（RAID-1）。现在，我们只讨论假设上述布局的镜像。

从镜像阵列读取数据块时，RAID 有一个选择：可以读取任一副本。例如，如果向 RAID 发出读取逻辑块 5 的命令，RAID 可以自由选择从磁盘 2 或磁盘 3 读取。但在写入逻辑块时，就没有这样的选择了：RAID 必须更新数据的两个副本，以保持可靠性。但请注意，这些写入可以并行进行；例如，对逻辑块 5 的写入可以同时写入磁盘 2 和磁盘 3。

### 6.2 RAID-1分析

让我们评估一下 RAID-1。

* 从容量的角度来看，RAID-1 是昂贵的；镜像级别 = 2 时，我们只能获得峰值有用容量的一半。在 N 块磁盘上有 B 个数据块时，RAID-1 的有用容量为 $\frac{N\cdot B}{2}$。
* 从可靠性的角度来看，RAID-1 表现出色。它可以承受任何一块磁盘的故障。如果运气好的话，RAID-1 还能做得更好。想象一下，在上图中，磁盘 0 和磁盘 2 同时发生故障。在这种情况下，数据不会丢失！一般来说，镜像系统（镜像级别为 2）可以承受 1 个磁盘的故障，最多可以承受 $\frac{N}{2}$ 个磁盘的故障，具体取决于哪些磁盘发生故障。在实践中，我们通常不喜欢听天由命，因此大多数人认为镜像可以很好地处理单个故障。
* 最后，我们分析一下性能。从单个读取请求的延迟角度来看，我们可以看到它与单个磁盘的延迟相同；RAID-1 所做的只是将读取指向其中一个副本。写入则略有不同：它需要两次物理写入才能完成。这两次写入是并行进行的，因此时间与单个写入的时间大致相同；但是，由于逻辑写入必须等待两次物理写入完成，因此会受到两个请求中最坏情况下的寻道和旋转延迟，因此（平均而言）会略高于写入单个磁盘的时间。

> <center>RAID 一致性更新问题</center>
>
> 在分析 RAID-1 之前，让我们首先讨论任何多磁盘 RAID 系统中都会出现的一个问题，即一致性更新问题 。写入任何必须在单个逻辑操作期间更新多个磁盘的 RAID 时都会出现此问题。在这种情况下，假设我们正在考虑镜像磁盘阵列。
>
> 想象一下，向 RAID 发出写操作，然后 RAID 决定必须将其写入两个磁盘，即磁盘 0 和磁盘 1。然后，RAID 向磁盘 0 发出写操作，但就在 RAID 可以向磁盘发出请求之前1、发生断电（或系统崩溃）。在这种不幸的情况下，让我们假设对磁盘 0 的请求已完成（但显然对磁盘 1 的请求没有完成，因为它从未发出）。
>
> 这种过早断电的结果是该块的两个副本现在不一致；磁盘 0 上的副本是新版本，磁盘 1 上的副本是旧版本。我们希望发生的是两个磁盘的状态都以原子方式改变，即，要么两个磁盘最终都成为新版本，要么都不成为新版本。
>
> 解决这个问题的一般方法是在执行之前使用某种<font color="red">预写日志</font>首先记录 RAID 将要执行的操作（即用某条数据更新两个磁盘）。通过采用这种方法，我们可以确保在发生崩溃时，会发生正确的事情；通过运行将所有待处理事务重播到 RAID 的恢复过程，我们可以确保没有两个镜像副本（在 RAID-1 情况下）不同步。
>
> 最后一点：由于每次写入时记录到磁盘的成本非常昂贵，因此大多数 RAID 硬件都包含少量非易失性 RAM（例如，电池供电的 RAM），用于执行此类记录。因此，无需花费高昂的日志记录到磁盘的成本即可提供一致的更新。

为了分析稳态吞吐量，我们从顺序工作负载开始。当顺序写入磁盘时，每次逻辑写入必须导致两次物理写入；例如，当我们写入逻辑块0（上图）时，RAID内部会将其同时写入磁盘0和磁盘1。因此，我们可以得出镜像阵列顺序写入时获得的最大带宽为（$\frac{N}{2}\cdot S$)，或峰值带宽的一半。

不幸的是，我们在顺序读取期间获得了完全相同的性能。人们可能认为顺序读取可以做得更好，因为它只需要读取数据的一份副本，而不是两者都读取。然而，让我们用一个例子来说明为什么这没有多大帮助。假设我们需要读取块 0、1、2、3、4、5、6 和 7。假设我们将 0 的读取发送到磁盘 0，将 1 的读取发送到磁盘 2，将 2 的读取发送到磁盘 1 ，以及将 3 读取到磁盘 3。我们继续分别向磁盘 0、2、1 和 3 发出对 4、5、6 和 7 的读取。人们可能天真地认为，由于我们利用了所有磁盘，因此我们实现了阵列的全部带宽。

然而，要知道情况并非（必然）如此，请考虑单个磁盘（例如磁盘 0）收到的请求。首先，它收到对块0的请求；然后，它收到对块 4 的请求（跳过块 2）。事实上，每个磁盘都会收到对每个其他块的请求。当它在跳过的块上旋转时，它不会向客户端提供有用的带宽。因此，每个磁盘只能提供其峰值带宽的一半。因此，顺序读取只能获得($\frac{N}{2}\cdot S$)MB/s的带宽。

随机读取是镜像 RAID 的最佳情况。在这种情况下，我们可以将读取分布在所有磁盘上，从而获得全部可能的带宽。因此，对于随机读取，RAID-1 提供 $N\cdot R$ MB/s。

最后，随机写入的性能如您所料：$\frac{N}{2}\cdot R$ MB/s。每个逻辑写入必须转化为两个物理写入，因此当所有磁盘都在使用时，客户端只会将其视为可用带宽的一半。尽管对逻辑块 $x$ 的写入变成了对两个不同物理磁盘的两次并行写入，但许多小请求的带宽仅达到我们在条带化中看到的一半。正如我们很快就会看到的，获得一半的可用带宽实际上非常好！

## 7 RAID Level 4：使用奇偶校验节省空间

### 7.1 基本介绍

我们现在介绍一种不同的向磁盘阵列添加冗余的方法，称为**奇偶校验**。基于奇偶校验的方法试图使用更少的容量，从而克服镜像系统所付出的巨大空间代价。然而，这种方法以**性能**为代价。

下面是一个五磁盘 RAID-4 系统示例。

![image-20240415095640999](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/RAID_4_With_Parity_Example.png)

我们为每个数据条带添加了一个奇偶校验块，用于存储该数据块条带的冗余信息。例如，奇偶校验块 P1 具有从块 4、5、6 和 7 计算出的冗余信息。

要计算奇偶校验，我们需要使用一个数学函数，使我们能够承受条带中任何一个块的丢失。事实证明，简单的函数 XOR 就能很好地做到这一点。对于一组给定的bit，如果bit中 1 的个数为偶数，则所有这些bit的 XOR 返回 0；如果 1 的个数为奇数，则返回 1。例如：

![image-20240415100052993](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/RAID_4_with_Parity_Calculate.png)

在第一行（0,0,1,1）中，有两个 1（C2、C3），因此所有这些值的 XOR 将是 0 (P)；同样，在第二行中只有一个 1（C1），因此 XOR 必须是 1 (P)。您可以用一种简单的方法记住这一点：任何一行中 1 的个数必须是偶数（而不是奇数）；这就是 RAID 必须保持的**不变性**，这样奇偶校验才会正确。

从上面的例子中，您或许还能猜到如何使用奇偶校验信息来从故障中恢复。假设标有 C2 的列丢失了。要想知道该列中应该有哪些值，我们只需读入该行中的所有其他值（包括 XOR 的奇偶校验位），然后重建正确的答案。具体来说，假设第一行 C2 列的值丢失了（它是一个 1）；通过读取该行中的其他值（C0 中的 0、C1 中的 0、C3 中的 1 和奇偶校验列 P 中的 0），我们得到了 0、0、1 和 0。因为我们知道 XOR 在每一行中保持偶数个 1，所以我们知道丢失的数据一定是：一个 。请注意我们是如何计算重构值的：我们只需将数据位和奇偶校验位一起 XOR 即可，与最初计算奇偶校验的方法相同。

现在你可能想知道：我们说的是将所有这些bit进行 XOR，但从上面我们知道 RAID 在每个磁盘上放置了 4KB （或更大）的数据块；我们如何对一堆数据块应用 XOR 来计算奇偶校验呢？事实证明这也很简单。只需对数据块的每个位执行逐位 XOR 即可；将每个逐位 XOR 的结果放入奇偶校验块的相应位槽中即可。例如，如果我们有大小为 4 位的数据块，它们可能看起来像这样：

![image-20240415100519049](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/RAID_4_With_Parity_Example_2.png)

从表中可以看出，每个数据块的每个bit都要计算奇偶校验，并将计算结果放入奇偶校验数据块中。

### 7.2 RAID-4 分析

现在让我们分析一下RAID-4。从容量的角度来看，RAID-4 使用 1 个磁盘来为其保护的每组磁盘提供奇偶校验信息。因此，RAID 组的有用容量为 $(N − 1) · B$。

可靠性也很容易理解：<font color="red">RAID-4 只能容忍 1 个磁盘故障，不能再出现更多故障。</font>如果多个磁盘丢失，则根本无法重建丢失的数据。

最后，还有性能。这次，让我们从分析稳态吞吐量开始。顺序读取性能可以利用除奇偶校验磁盘之外的所有磁盘，从而提供 $(N − 1) · S\text{ MB/s}$ 的峰值有效带宽（一个简单的情况）。

要了解顺序写入的性能，我们必须首先了解它们是如何完成的。将大块数据写入磁盘时，RAID-4 可以执行称为<font color="red">全条带写入的简单优化</font>。例如，想象一下块 0、1、2 和 3 已作为写入请求的一部分发送到 RAID 的情况（如下表所示）。

![image-20240415101029694](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Full_Stripe_Write_In_RAID_4_Example.png)

在这种情况下，RAID可以简单地计算P0的新值（通过对块0、1、2和3执行异或），然后将所有块（包括奇偶校验块）并行写入上面的五个磁盘中（表中以灰色突出显示）。因此，全条带写入是 RAID-4 写入磁盘的最有效方式。

一旦我们了解了全条带写入，计算 RAID-4 上顺序写入的性能就很容易了；有效带宽也是$(N − 1) · S\text{ MB/s}$。即使在操作过程中不断使用奇偶校验磁盘，客户端也无法从中获得性能优势。

现在我们来分析一下随机读取的性能。从上表中还可以看到，一组 1-block 随机读取将分布在系统的数据磁盘上，但不会分布在奇偶校验磁盘上。因此，有效性能为：$(N − 1) · R\text{ MB/s}$。

我们最后保存的随机写入呈现了 RAID-4 最有趣的情况。假设我们希望覆盖上面示例中的块 1。我们可以直接覆盖它，但这会给我们带来一个问题：奇偶校验块 P0 将不再准确地反映条带的正确奇偶校验值；在此示例中，P0 也必须更新。如何才能既正确又高效地更新呢？

事实证明有两种方法。第一个称为<font color="red">加法奇偶校验</font>，要求我们执行以下操作。要计算新奇偶校验块的值，请并行读入条带中的所有其他数据块（在示例中为块 0、2 和 3），并将这些数据块与新块 (1) 进行异或。结果就是新的奇偶校验块。为了完成写入，您可以将新数据和新奇偶校验写入各自的磁盘，同样是并行的。

该技术的问题在于它会随着磁盘数量的增加而扩展，因此在较大的 RAID 中需要大量读取来计算奇偶校验。因此，<font color="red">采用减法奇偶校验法</font>。

例如，想象一下这一串位（4 个数据位，一个奇偶校验）：

![image-20240415101714960](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/A_Strpe_RAID_4_Example_1.png)

假设我们希望用一个新值覆盖位 C2，我们将其称为$C2_{new}$ 。减法分三个步骤进行。

1. 首先，我们读入C2处的旧数据（$C2_{old}=1$）和旧奇偶校验（$P_{old}=0$）。

2. 然后，我们比较旧数据和新数据；

	* 如果它们相同（例如，$C2_{new}=C2_{old}$），那么我们知道奇偶校验位也将保持相同（即，$P_{new}=P_{old}$）。
	* 然而，如果它们不同，那么我们必须将旧奇偶校验位翻转到其当前状态的相反状态，即，如果($P_{old}==1$)，$P_{new}$将被设置为0；如果 ($P_{old}==0$)，$P_{new}$将被设置为 1。我们可以用 XOR 巧妙地表达整个过程（其中 $\oplus$ 是 XOR 运算符）：

	$$
	P_{new}=(C_{old}\oplus C_{new})\oplus P_{old}
	$$

由于我们处理的是数据块而不是bit，因此我们对数据块中的所有bit进行计算（例如，每个数据块中的 4096 个字节乘以每个字节的 8 个bit）。因此，在大多数情况下，新的数据块会与旧的数据块不同，因此新的奇偶校验数据块也会不同。现在，你应该能算出何时使用加法奇偶校验计算，何时使用减法奇偶校验计算。想一想，系统中需要有多少磁盘才能使加法计算法的 I/O 次数少于减法计算法；交叉点是多少？

在进行性能分析时，我们假设使用的是减法。因此，每写一次，RAID 必须执行 4 次物理 I/O（两次读和两次写）。现在假设有大量的写操作提交给 RAID，那么 RAID-4 可以并行执行多少次写操作呢？要理解这一点，让我们再看看 RAID-4 布局，如下表所示。

![image-20240415102557259](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Write_To_4_13_And_Parity_Blocks_Example.png)

现在假设大约在同一时间有 2 个小写入提交到 RAID-4，即块 4 和 13（表中用 * 标记）。这些磁盘的数据位于磁盘 0 和 1 上，因此数据的读取和写入可以并行发生，这很好。出现的问题是奇偶校验磁盘；两个请求都必须读取 4 和 13 的相关奇偶校验块、奇偶校验块 P1 和 P3（用 + 标记）。希望问题现在已经清楚了：奇偶校验磁盘是此类工作负载下的瓶颈；因此，我们有时将其称为基于奇偶校验的 RAID 的**小写入问题**。因此，<font color="red">即使数据磁盘可以并行访问，奇偶校验磁盘也会阻止任何并行性的实现；由于奇偶校验磁盘的存在，对系统的所有写入都将被序列化</font>。由于奇偶校验磁盘每个逻辑 I/O 必须执行两次 I/O（一次读，一次写），因此我们可以通过计算奇偶校验磁盘在这两个 I/O 上的性能来计算 RAID-4 中小型随机写入的性能，因此我们达到了 $\frac{R}{2}$ MB/s。随机小写入下的 RAID-4 吞吐量很糟糕；当您向系统添加磁盘时，它不会得到改善。

最后，我们将分析 RAID-4 中的 I/O 延迟。大家现在都知道，单次读取（假设没有故障）只是映射到单个磁盘，因此其延迟相当于单个磁盘请求的延迟。单次写入的延迟需要两次读取，然后两次写入；读取和写入可以并行进行，因此总延迟大约是单个磁盘的两倍（存在一些差异，因为我们必须等待两次读取完成，从而获得最坏情况下的定位时间，但更新不会产生寻道成本，因此可能是比平均定位成本更好的定位时间）。

## 8 RAID Level 5：旋转奇偶校验

### 8.1 基本介绍

为了解决（至少部分地）小写入问题，Patterson、Gibson 和 Katz 引入了 RAID-5。 RAID-5 的工作方式几乎与 RAID-4 相同，只是它在驱动器之间旋转奇偶校验块，如下表所示。

![image-20240415103321139](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/RAID_5_With_Rotated_Parity.png)

正如您所看到的，每个条带的奇偶校验块现在在磁盘上旋转，以消除 RAID-4 的奇偶校验磁盘瓶颈。

### 8.2 RAID 5 分析

RAID-5 的大部分分析与 RAID-4 相同。例如，两个级别的有效容量和容错能力是相同的。顺序读写性能也是如此。单个请求（无论是读还是写）的延迟也与 RAID-4 相同。

随机读取性能好一点，因为我们现在可以利用所有磁盘。最后，随机写入性能比 RAID-4 显着提高，因为它<font color="red">允许跨请求并行</font>。想象一下对块 1 的写入和对块 10 的写入；这将变成对磁盘 1 和磁盘 4 的请求（针对块 1 及其奇偶校验P0）以及对磁盘 0 和磁盘 2 的请求（针对块 10 及其奇偶校验P2）。因此，它们可以并行进行。事实上，我们通常可以假设，给定大量随机请求，我们将能够保持所有磁盘均匀忙碌。如果是这样的话，那么我们小写的总带宽将是 $\frac{N}{4}\cdot R$ MB/s。 4 个丢失的因素是由于每次 RAID-5 写入仍然生成 4 次总 I/O 操作，这只是使用基于奇偶校验的 RAID 的成本。

因为 RAID-5 基本上与 RAID-4 相同，除了在少数情况下更好之外，在市场上几乎完全取代了 RAID-4。唯一没有被取代的地方是那些知道自己永远不会执行大写操作的系统，从而完全避免小写问题；在这些情况下，有时会使用 RAID-4，因为它构建起来稍微简单一些。

## 9 RAID 比较

现在，如下表所示，我们总结了 RAID Levels的简化比较。

![image-20240415103909530](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/RAID_Capacity_Reliability_And_Performance.png)

请注意，我们省略了许多细节以简化我们的分析。例如，在镜像系统中写入时，平均寻道时间比仅写入单个磁盘时稍长，因为寻道时间是两次寻道（每个磁盘上一次）的最大值。因此，两个磁盘的随机写入性能通常会略低于单个磁盘的随机写入性能。此外，在更新 RAID-4/5 中的奇偶校验磁盘时，第一次读取旧奇偶校验可能会导致完全寻道和旋转，但第二次写入奇偶校验只会导致旋转。

然而，表中的比较确实捕捉到了本质差异，并且对于理解跨 RAID Levels的权衡很有用。对于延迟分析，我们简单地使用 T 来表示对单个磁盘的请求所花费的时间。

总而言之：

* 如果您严格<font color="red">要求性能而不关心可靠性</font>，那么<font color="red">条带化</font>显然是最好的。
* 然而，如果您想要<font color="red">随机 I/O 性能和可靠性</font>，<font color="red">镜像</font>是最好的；您所付出的成本是损失容量。
* 如果<font color="red">容量和可靠性</font>是您的主要目标，那么 <font color="red">RAID-5 就是首选</font>；您付出的代价是小写性能。最后，如果您<font color="red">始终执行顺序 I/O 并希望最大化容量</font>，那么 RAID-5 也是最有意义的。

# Program Explanation
## 1 `raid.py`

`raid.py` 一个简单的RAID模拟器，您可以使用它来增强您对RAID系统工作原理的了解。它有许多选项，如下所示：

```shell
❯ python raid.py -h
Usage: raid.py [options]

Options:
  -h, --help            show this help message and exit
  -s SEED, --seed=SEED  the random seed
  -D NUMDISKS, --numDisks=NUMDISKS
                        number of disks in RAID
  -C CHUNKSIZE, --chunkSize=CHUNKSIZE
                        chunk size of the RAID
  -n NUMREQUESTS, --numRequests=NUMREQUESTS
                        number of requests to simulate
  -S SIZE, --reqSize=SIZE
                        size of requests
  -W WORKLOAD, --workload=WORKLOAD
                        either "rand" or "seq" workloads
  -w WRITEFRAC, --writeFrac=WRITEFRAC
                        write fraction (100->all writes, 0->all reads)
  -R RANGE, --randRange=RANGE
                        range of requests (when using "rand" workload)
  -L LEVEL, --level=LEVEL
                        RAID level (0, 1, 4, 5)
  -5 RAID5TYPE, --raid5=RAID5TYPE
                        RAID-5 left-symmetric "LS" or left-asym "LA"
  -r, --reverse         instead of showing logical ops, show physical
  -t, --timing          use timing mode, instead of mapping mode
  -c, --compute         compute answers for me
```

在其基本模式下，您可以使用它来了解不同的 RAID 级别如何将逻辑块映射到底层磁盘和偏移量。例如，假设我们想看看具有四个磁盘的简单条带化 RAID （RAID-0） 如何执行此映射。

```shell
❯ python raid.py -n 5 -L 0 -R 20
ARG blockSize 4096
ARG seed 0
ARG numDisks 4
ARG chunkSize 4k
ARG numRequests 5
ARG reqSize 4k
ARG workload rand
ARG writeFrac 0
ARG randRange 20
ARG level 0
ARG raid5 LS
ARG reverse False
ARG timing False

16 1
LOGICAL READ from addr:16 size:4096
  Physical reads/writes?

8 1
LOGICAL READ from addr:8 size:4096
  Physical reads/writes?

10 1
LOGICAL READ from addr:10 size:4096
  Physical reads/writes?

15 1
LOGICAL READ from addr:15 size:4096
  Physical reads/writes?

9 1
LOGICAL READ from addr:9 size:4096
  Physical reads/writes?
```

在此示例中，我们模拟五个请求 （`-n 5`），指定 RAID Level 0 （`-L 0`），并将随机请求的范围限制为 RAID 的前 20 个块 （`-R 20`）。结果是对 RAID 的前 20 个块进行一系列随机读取;然后，模拟器会要求你猜测，对于每个逻辑读取，访问了哪些基础磁盘/偏移量来为请求提供服务。

在这种情况下，计算答案很容易：在 RAID-0 中，回想一下为请求提供服务的底层磁盘和偏移量是通过模算术计算的：

```go
disk   = address % number_of_disks
offset = address / number_of_disks
```

因此，对 16 的第一个请求应由磁盘 0 在偏移量 4 处提供服务。等等。像往常一样，你可以通过使用方便的“-c”标志来计算结果来查看答案。

```shell
❯ python raid.py -n 5 -L 0 -R 20 -c
16 1
LOGICAL READ from addr:16 size:4096
  read  [disk 0, offset 4]  

8 1
LOGICAL READ from addr:8 size:4096
  read  [disk 0, offset 2]  

10 1
LOGICAL READ from addr:10 size:4096
  read  [disk 2, offset 2]  

15 1
LOGICAL READ from addr:15 size:4096
  read  [disk 3, offset 3]  

9 1
LOGICAL READ from addr:9 size:4096
  read  [disk 1, offset 2]  
```

我们也可以用“-r”标志反向做这个问题。以这种方式运行模拟器会显示低级磁盘的读取和写入情况，并要求您对必须向 RAID 发出的逻辑请求进行反向工程：

```shell
❯ python raid.py -n 5 -L 0 -R 20 -r
16 1
LOGICAL OPERATION is ?
  read  [disk 0, offset 4]  

8 1
LOGICAL OPERATION is ?
  read  [disk 0, offset 2]  

10 1
LOGICAL OPERATION is ?
  read  [disk 2, offset 2]  

15 1
LOGICAL OPERATION is ?
  read  [disk 3, offset 3]  

9 1
LOGICAL OPERATION is ?
  read  [disk 1, offset 2]  
```

您可以再次使用 -c 来显示答案。为了获得更多的多样性，可以给出不同的随机种子（`-s`）。

通过检查不同的RAID级别，可以获得更多的多样性。在模拟器中，支持 RAID-0（块条带化）、RAID-1（镜像）、RAID-4（块条带化加单个奇偶校验磁盘）和 RAID-5（带旋转奇偶校验的块条带化）。

在下一个示例中，我们将演示如何在镜像模式下运行模拟器。我们展示了节省空间的答案：

```shell
❯ python raid.py -n 5 -L 1 -R 20 -c
16 1
LOGICAL READ from addr:16 size:4096
  read  [disk 0, offset 8]  

8 1
LOGICAL READ from addr:8 size:4096
  read  [disk 0, offset 4]  

10 1
LOGICAL READ from addr:10 size:4096
  read  [disk 1, offset 5]  

15 1
LOGICAL READ from addr:15 size:4096
  read  [disk 3, offset 7]  

9 1
LOGICAL READ from addr:9 size:4096
  read  [disk 2, offset 4] 
```

您可能会注意到有关此示例的一些事项。首先，镜像 RAID-1 采用条带化布局（有些人可能称之为 RAID-10 或镜像条带），其中逻辑块 0 映射到磁盘 0 和 1 的第 0 块，逻辑块 1 映射到磁盘 2 和 3 的第 0 块，依此类推（在此四磁盘示例中）。其次，当从镜像RAID系统读取单个块时，RAID可以选择读取两个块中的哪一个。在这个模拟器中，我们使用了一种相对愚蠢的方法：对于偶数逻辑块，RAID选择对中的偶数磁盘;奇数磁盘用于奇数逻辑块。这样做是为了使每次运行的结果易于猜测（而不是，例如，随机选择）。

我们还可以使用 -w 标志来探索写入的行为（而不仅仅是读取），该标志指定了工作负载的“写入部分”，即写入请求的比例。默认情况下，它设置为零，因此到目前为止的示例是 100% 读取。让我们看看当引入一些写入时，我们的镜像 RAID 会发生什么：

```shell
❯ python raid.py -R 20 -n 5 -L 1 -w 100 -c
ARG blockSize 4096
ARG seed 0
ARG numDisks 4
ARG chunkSize 4k
ARG numRequests 5
ARG reqSize 4k
ARG workload rand
ARG writeFrac 100
ARG randRange 20
ARG level 1
ARG raid5 LS
ARG reverse False
ARG timing False

16 1
LOGICAL WRITE to  addr:16 size:4096
  write [disk 0, offset 8]    write [disk 1, offset 8]  

8 1
LOGICAL WRITE to  addr:8 size:4096
  write [disk 0, offset 4]    write [disk 1, offset 4]  

10 1
LOGICAL WRITE to  addr:10 size:4096
  write [disk 0, offset 5]    write [disk 1, offset 5]  

15 1
LOGICAL WRITE to  addr:15 size:4096
  write [disk 2, offset 7]    write [disk 3, offset 7]  

9 1
LOGICAL WRITE to  addr:9 size:4096
  write [disk 2, offset 4]    write [disk 3, offset 4] 
```

对于写入，RAID 当然必须更新两个磁盘，而不是只生成单个低级磁盘操作，因此会发出两次写入。正如您可能猜到的那样，RAID-4 和 RAID-5 发生了更有趣的事情，如下所示。

```shell
❯ python raid.py -R 20 -n 5 -L 4 -w 100 -c
16 1
LOGICAL WRITE to  addr:16 size:4096
  read  [disk 1, offset 5]    read  [disk 3, offset 5]  
  write [disk 1, offset 5]    write [disk 3, offset 5]  

8 1
LOGICAL WRITE to  addr:8 size:4096
  read  [disk 2, offset 2]    read  [disk 3, offset 2]  
  write [disk 2, offset 2]    write [disk 3, offset 2]  

10 1
LOGICAL WRITE to  addr:10 size:4096
  read  [disk 1, offset 3]    read  [disk 3, offset 3]  
  write [disk 1, offset 3]    write [disk 3, offset 3]  

15 1
LOGICAL WRITE to  addr:15 size:4096
  read  [disk 0, offset 5]    read  [disk 3, offset 5]  
  write [disk 0, offset 5]    write [disk 3, offset 5]  

9 1
LOGICAL WRITE to  addr:9 size:4096
  read  [disk 0, offset 3]    read  [disk 3, offset 3]  
  write [disk 0, offset 3]    write [disk 3, offset 3]  
```

`-C` 标志允许您设置 RAID 的块大小，而不是使用每个块一个 4 KB 块的默认大小。每个请求的大小都可以使用 `-S` 标志进行类似的调整。默认工作负载访问随机块;使用 `W sequential` 探索顺序访问的行为。使用 RAID-5，可以使用两种不同的布局方案，左对称和左非对称;使用 `-5 LS` 或 `-5 LA` 使用 RAID-5 （`-L 5`）。

最后，在计时模式 （`-t`） 中，模拟器使用一个非常简单的磁盘模型来估计一组请求所需的时间，而不仅仅是关注映射。在此模式下，随机请求需要 10 毫秒，而顺序请求需要 0.1 毫秒。假定磁盘每个磁道具有少量的块数 （100），以及同样少量的磁道 （100）。因此，您可以使用模拟器来估计某些不同工作负载下的 RAID 性能。

## 2 `raid-graphics.py`

`raid-graphics.py`允许你查看RAID模拟动画，你可以运行以下命令获取帮助：

```shell
❯ python raid-graphics.py -h
Usage: raid-graphics.py [options]

Options:
  -h, --help            show this help message and exit
  -s SEED, --seed=SEED  Random seed
  -m MAPPING, --mapping=MAPPING
                        0-striping, 1-mirroring, 4-raid4, 5-raid5
  -a ADDR, --addr=ADDR  Request list (comma-separated) [-1 -> use addrDesc]
  -r READ_FRACTION, --read_fraction=READ_FRACTION
                        Fraction of requests that are reads
  -A ADDR_DESC, --addr_desc=ADDR_DESC
                        Num requests, max request (-1->all), min request
  -B, --balanced        If generating random requests, balance across disks
  -S SEEK_SPEED, --seek_speed=SEEK_SPEED
                        Speed of seek (1,2,4,5,10,20)
  -p POLICY, --policy=POLICY
                        Scheduling policy (FIFO, SSTF, SATF, BSATF)
  -w WINDOW, --window=WINDOW
                        Size of scheduling window (-1 -> all)
  -D ANIMATE_DELAY, --delay=ANIMATE_DELAY
                        Animation delay; bigger is slower
  -G, --graphics        Turn on graphics
  -c, --compute         Compute the answers
  -P, --print_options   Print the options
```

其中`-G`打开动画，`-D`设置动画延迟，单位为ms。

# Homework
1. 使用模拟器执行一些基本的 RAID 映射测试。使用不同的级别（0、1、4、5）运行，看看是否可以找出一组请求的映射。对于 RAID-5，看看是否可以找出左对称布局和左非对称布局之间的区别。使用一些不同的随机种子来生成与上述不同的问题。

	```shell
	$ python raid.py -L 5 -5 LS -c -W seq
	$ python raid.py -L 5 -5 LA -c -W seq
	
	left-symmetric    left-asymmetric
	0 1 2 P           0 1 2 P
	4 5 P 3           3 4 P 5
	8 P 6 7           6 P 7 8
	```

2. 执行与第一个问题相同的操作，但这次使用 `-C` 更改块大小。块大小如何更改映射？

	```shell
	$ python raid.py -L 5 -5 LS -c -W seq -C 8K -n 12
	
	0  2  4  P
	1  3  5  P
	8 10  P  6
	9 11  P  7
	```

3. 执行与上述相同的操作，但使用 `-r` 标志来反转每个问题的性质。

	```shell
	$ python raid.py -L 5 -5 LS -W seq -C 8K -n 12 -r
	```

4. 现在使用反向标志，但使用该 `-S` 标志增加每个请求的大小。尝试指定 8k、12k 和 16k 的大小，同时更改 RAID 级别。当请求大小增加时，基础 I/O 模式会发生什么情况？请确保在顺序工作负载中也尝试此操作 （ `-W sequential` ）;对于哪些请求大小，RAID-4 和 RAID-5 的 I/O 效率更高？

	```shell
	$ python raid.py -L 4 -S 4k -c -W seq
	$ python raid.py -L 4 -S 8k -c -W seq
	$ python raid.py -L 4 -S 12k -c -W seq
	$ python raid.py -L 4 -S 16k -c -W seq
	$ python raid.py -L 5 -S 4k -c -W seq
	$ python raid.py -L 5 -S 8k -c -W seq
	$ python raid.py -L 5 -S 12k -c -W seq
	$ python raid.py -L 5 -S 16k -c -W seq
	```

	16k

5. 使用模拟器 （ `-t` ） 的计时模式来估计 100 次随机读取 RAID 的性能，同时使用 4 个磁盘改变 RAID 级别。

	```shell
	$ python raid.py -L 0 -t -n 100 -c    // 275.7
	$ python raid.py -L 1 -t -n 100 -c    // 278.7
	$ python raid.py -L 4 -t -n 100 -c    // 386.1
	$ python raid.py -L 5 -t -n 100 -c    // 276.5
	```

6. 执行与上述相同的操作，但增加磁盘数。随着磁盘数量的增加，每个 RAID 级别的性能如何扩展？

	```shell
	$ python raid.py -L 0 -t -n 100 -c -D 8   // 275.7 / 156.5 = 1.76
	$ python raid.py -L 1 -t -n 100 -c -D 8   // 278.7 / 167.8 = 1.66
	$ python raid.py -L 4 -t -n 100 -c -D 8   // 386.1 / 165.0 = 2.34
	$ python raid.py -L 5 -t -n 100 -c -D 8   // 276.5 / 158.6 = 1.74
	```

7. 执行与上述相同的操作，但使用所有写入 （ `-w 100` ） 而不是读取。现在，每个 RAID 级别的性能如何扩展？您能否粗略估计一下完成 100 次随机写入工作负载所需的时间？

	```shell
	$ python raid.py -L 0 -t -n 100 -c -w 100       // 275.7    100 * 10 / 4
	$ python raid.py -L 1 -t -n 100 -c -w 100       // 509.8    100 * 10 / (4 / 2)
	$ python raid.py -L 4 -t -n 100 -c -w 100       // 982.5
	$ python raid.py -L 5 -t -n 100 -c -w 100       // 497.4
	$ python raid.py -L 0 -t -n 100 -c -D 8 -w 100  // 275.7 / 156.5 = 1.76    100 * 10 / 8
	$ python raid.py -L 1 -t -n 100 -c -D 8 -w 100  // 509.8 / 275.7 = 1.85    100 * 10 / (8 / 2)
	$ python raid.py -L 4 -t -n 100 -c -D 8 -w 100  // 982.5 / 937.8 = 1.05
	$ python raid.py -L 5 -t -n 100 -c -D 8 -w 100  // 497.4 / 290.9 = 1.71
	```

8. 最后一次运行计时模式，但这次使用顺序工作负载 （`-W sequential` ）。性能如何随 RAID 级别以及执行读取与写入时有何不同？当改变每个请求的大小时怎么样？使用 RAID-4 或 RAID-5 时，应向 RAID 写入多大大小？

	```shell
	$ python raid.py -L 0 -t -n 100 -c -w 100 -W seq    // 275.7 / 12.5 = 22
	$ python raid.py -L 1 -t -n 100 -c -w 100 -W seq    // 509.8 / 15 = 34
	$ python raid.py -L 4 -t -n 100 -c -w 100 -W seq    // 982.5 / 13.4 = 73
	$ python raid.py -L 5 -t -n 100 -c -w 100 -W seq    // 497.4 / 13.4 = 37
	```

	12k