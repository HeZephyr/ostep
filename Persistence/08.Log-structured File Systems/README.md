# Note/Translation
## 1 引言

20 世纪 90 年代初，伯克利分校的一个由 John Ousterhout 教授和研究生 Mendel Rosenblum 领导的小组开发了一种新的文件系统，称为<font color="red">日志结构文件系统</font>。他们这样做的动机基于以下观察： 

* **系统内存不断增长**：随着内存变大，内存中可以缓存更多数据。随着越来越多的数据被缓存，磁盘流量越来越多地由写入组成，因为读取由缓存提供服务。因此，文件系统的性能很大程度上取决于其写入性能。
* **随机I/O 性能和顺序I/O 性能之间存在很大差距**：多年来硬盘传输带宽大幅增加；随着更多的位被封装到驱动器的表面，访问所述位时的带宽增加。然而，寻道和旋转延迟成本却缓慢下降；让廉价的小型电机更快地旋转盘片或更快地移动磁盘臂是一项挑战。因此，如果您能够以顺序方式使用磁盘，那么与导致寻道和旋转的方法相比，您将获得相当大的性能优势。
* **现有文件系统在许多常见工作负载上表现不佳**：例如，FFS将执行大量写入来创建一个大小为一个块的新文件：一个用于新的inode，一个用于更新inode位图，一个用于包含该文件的目录数据块，一个用于更新目录inode，一个用于作为新文件一部分的新数据块，并且还需要对数据位图进行一次写入以标记数据块已被分配。因此，尽管 FFS 将所有这些块放置在同一块组内，但 FFS 需要进行许多短寻道和随后的旋转延迟，因此性能远低于峰值顺序带宽。
* **文件系统不支持RAID**：例如，RAID-4 和RAID-5 都存在**小写入问题**，即对单个块的逻辑写入会导致发生4 个物理I/O。现有文件系统不会尝试避免这种最坏情况的 RAID 写入行为。

因此，理想的文件系统将关注写入性能，并尝试利用磁盘的顺序带宽。此外，它在常见工作负载上表现良好，这些工作负载不仅写出数据，而且还经常更新磁盘上的元数据结构。最后，它在 RAID 和单个磁盘上都能很好地工作。 Rosenblum 和 Ousterhout 推出的新型文件系统称为 **LFS**，是**日志结构文件系统**的缩写。当写入磁盘时，LFS 首先将所有更新（包括元数据！）缓冲在内存段中；当该段已满时，它会通过一次长的、顺序的传输写入未使用的磁盘部分。 LFS 永远不会覆盖现有数据，而是始终将段写入空闲位置。由于段很大，因此磁盘（或 RAID）可以得到有效利用，文件系统的性能也接近顶峰。

> <center>关键：如何使所有写入顺序写入？
>     
> </center>
>
> 文件系统如何将所有写入转换为顺序写入？对于读取，此任务是不可能的，因为要读取的所需块可能位于磁盘上的任何位置。然而，对于写入，文件系统总是有一个选择，而我们希望利用的正是这个选择。

## 2 按顺序写入磁盘

因此，我们面临的第一个挑战是：<font color="red">如何将文件系统状态的所有更新转化为一系列对磁盘的顺序写入？</font>为了更好地理解这一点，让我们举一个简单的例子。假设我们正在向文件写入一个数据块 D。将数据块写入磁盘可能会导致以下磁盘布局，D 被写入磁盘地址 A0：

![image-20240420101924372](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/write_D_To_Disk_A0.png)

然而，当用户写入数据块时，写入磁盘的不仅是数据，还有其他需要更新的元数据。在这种情况下，我们也把文件的 inode (I) 写入磁盘，并让它指向数据块 D。写入磁盘后，数据块和 inode 的如下图所示（**注意**，inode 看起来和数据块一样大，但一般情况下并非如此；在大多数系统中，数据块的大小为 4 KB，而 inode 则小得多，约为 128 字节）：

![image-20240420102116005](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/write_D_To_Disk_Metadata.png)

这种简单地将所有更新（如数据块、inodes 等）按顺序写入磁盘的基本思想是 LFS 的核心。理解了这一点，你就掌握了基本思想。但正如所有复杂的系统一样，细节决定成败。

## 3 顺序有效地写入

不幸的是，顺序写入磁盘（单独）不足以保证高效写入。例如，想象一下，如果我们在时间 $T$ 向地址 $A$ 写入一个块。然后我们等待一会儿，并在时间 $T + \delta$ 的地址 $A + 1$（按顺序排列的下一个块地址）写入磁盘。不幸的是，在第一次和第二次写入之间，磁盘发生了旋转；当您发出第二次写入时，它将在提交之前等待大部分旋转（具体来说，如果旋转需要时间 $T_{rotation}$，则磁盘将等待 $T_{rotation}-\delta$，然后才能将第二次写入提交到磁盘表面）。因此，您可以看到，仅仅按顺序写入磁盘不足以实现峰值性能；相反，您必须向驱动器<font color="red">发出大量连续写入（或一次大型写入）才能获得良好的写入性能</font>。

为了实现这一目标，LFS 使用一种称为**写入缓冲**的古老技术。在写入磁盘之前，LFS 会跟踪内存中的更新；当它收到足够数量的更新时，它会立即将它们全部写入磁盘，从而确保磁盘的有效使用。

LFS 一次写入的大块更新被称为一个段。尽管这个术语在计算机系统中被滥用，但在这里它只是指 LFS 用来分组写入的一个相对较大的块。因此，当写入磁盘时，LFS 将更新缓冲在内存中的段中，然后将该段全部写入磁盘。只要段足够大，这些写入就会高效。

下面是一个示例，其中 LFS 将两组更新缓冲到一个小段中；实际的段更大（几MB）。第一个更新是对文件 `j` 的四个块写入；第二个是向文件 `k` 添加一个块。然后，LFS 将七个块的整个段一次性提交到磁盘。这些块的最终磁盘布局如下：

![image-20240420103434967](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Write_Data_To_Disk_Efficient.png)

## 4 缓冲多大

这就提出了以下问题：在写入磁盘之前，LFS 应该缓冲多少个更新？当然，答案取决于磁盘本身，特别是定位开销与传输速率相比有多高。

例如，假设每次写入之前的定位（即旋转和寻道开销）大约需要 $T_{position}$秒。进一步假设磁盘传输速率为$R_{peak}\text{ MB/s}$。在这样的磁盘上运行时，LFS 在写入之前应该缓冲多少？

思考这个问题的方法是，每次写入时，您都会付出固定的定位成本开销。因此，您需要写多少才能摊销该成本？你写的越多越好（显然），并且你越接近达到峰值带宽。

为了获得具体的答案，我们假设我们正在写 $D\text{ MB}$。写这块数据的时间（$T_{write}$）是定位时间$T_{position}$加上传输时间$\frac{D}{R_{peak}}$，或者：
$$
T_{write}=T_{position}+\frac{D}{R_{peak}}
$$
因此，有效写入率（$R_{effective}$）就是写入的数据量除以写入的总时间：
$$
R_{effective}=\frac{D}{T_{write}}=\frac{D}{T_{position}+\frac{D}{R_{peak}}}
$$
我们感兴趣的是让有效率 ($R_{effective}$) 接近峰值率。具体来说，我们希望有效速率是峰值速率的某个分数 $F$，其中 $0 < F < 1$（典型的 F 可能是 $0.9$，或峰值速率的 $90\%$）。在数学形式上，这意味着我们需要$R_{effective}=F\times R_{peak}$ 。

至此，我们可以求解$D$：
$$
R_{effective}=\frac{D}{T_{write}}=\frac{D}{T_{position}+\frac{D}{R_{peak}}}
$$

$$
D=F\times R_{peak}\times(T_{position}+\frac{D}{R_{peak}})
$$

$$
D=(F\times R_{peak}\times T_{position})+(F\times R_{peak}\times \frac{D}{R_{peak}})
$$

$$
D=\frac{F}{1-F}\times R_{peak}\times T_{position}
$$

举个例子，磁盘的定位时间为$10\text{ ms}$，峰值传输率为$100\text{ MB/s}$；假设我们想要峰值的 $90\%$ 的有效带宽 ($F = 0.9$)。在本例中，$D = \frac{0.9}{0.1}\times 100\text{ MB/s} \times 0.01 \text{ s} = 9\text{ MB}$。尝试一些不同的值，看看我们需要缓冲多少才能接近峰值带宽。需要多少才能达到峰值的 $95\%$？ $99\%$？

## 5 问题：查找 Inode

为了了解如何在 LFS 中查找 inode，让我们简要回顾一下如何在典型的 UNIX 文件系统中查找 inode。在典型的文件系统（例如 FFS）甚至旧的 UNIX 文件系统中，查找 inode 很容易，因为它们被组织在数组中并放置在磁盘上的固定位置。

例如，旧的 UNIX 文件系统将所有inode保存在磁盘的固定部分。因此，给定 inode number和起始地址，要查找特定 inode，只需将 inode number乘以 inode 的大小，然后将其添加到磁盘阵列的起始地址，即可计算出其准确的磁盘地址。 基于数组的索引（给定 inode number）既快速又简单。

在 FFS 中查找给定 inode number的 inode 只是稍微复杂一些，因为 FFS 将 inode 表分割成块，并将一组 inode 放置在每个柱面组中。因此，我们必须知道每个inode块有多大以及每个inode的起始地址。之后的计算类似，也很容易。

在LFS，生活更加困难。为什么？好吧，我们已经成功地将inode分散在整个磁盘上！更糟糕的是，我们永远不会就地覆盖，因此最新版本的索引节点（即我们想要的）不断移动。

## 6 通过间接解决方案：Inode Map

为了解决这个问题，LFS 的设计者通过一个名为 inode map（imap）的数据结构，在 inode number和 inode 之间引入了一层间接关系。imap 是一种将 inode number作为输入并生成该 inode 最新版本磁盘地址的结构。因此，可以想象它通常是作为一个简单的数组来实现的，每个条目有 4 个字节（磁盘指针）。当 inode 被写入磁盘时，imap 就会根据新的位置进行更新。

不幸的是，imap 需要保持持久性（即写入磁盘），这样做可以让 LFS 在崩溃时跟踪 inode 的位置，从而按预期运行。因此，有一个问题：imap 应该放在磁盘的哪个位置？

当然，它可以位于磁盘的固定位置。遗憾的是，由于它经常更新，这就需要在更新文件结构后再写入 imap，因此性能会受到影响（也就是说，<font color="red">在每次更新和 imap 的固定位置之间会有更多的磁盘寻道</font>）。

相反，LFS 会在写入所有其他新信息的位置旁边放置 inode 映射块。因此，在向文件 `k` 添加数据块时，LFS 实际上是将新数据块、其 inode 和 inode 映射的一部分一起写入磁盘，如下所示：

![image-20240420111904512](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Inode_Map_Example_1.png)

在这张图中，<font color="red">存储在标记为 imap 的块中的 imap 数组的一块告诉 LFS inode k 位于磁盘地址 A1；这个 inode 又告诉 LFS 它的数据块 D 位于地址 A0</font>。

## 7 完成解决方案：检查点区域

### 7.1 如何找到inode map

你可能已经注意到这里的问题了。既然inode map的各个部分也分布在磁盘上，我们如何找到inode map呢？归根结底，没有什么神奇的：文件系统必须在磁盘上有一些固定且已知的位置才能开始文件查找。

LFS 在磁盘上为此提供了一个固定位置，称为<font color="red">检查点区域 (CR)</font>。检查点区域包含指向最新的 inode map片段的指针（即地址），因此可以通过首先读取 CR 来找到 inode map片段。请注意，检查点区域仅定期更新（例如每 30 秒左右），因此性能不会受到不良影响。因此，磁盘布局的整体结构包含一个检查点区域（指向 inode map的最新部分）；每个 inode 映射片段都包含 inode 的地址； inode 指向文件（和目录），就像典型的 UNIX 文件系统一样。

下面是检查点区域（注意它位于磁盘的起始位置，地址为 0）以及单个 imap 块、inode 和数据块的示例。一个真正的文件系统当然会有一个大得多的 CR（事实上，它会有两个，我们稍后会了解到）、许多 imap 块，当然还有更多的 inode、数据块等。

![image-20240420132426842](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/CR_Example_1.png)

### 7.2 从磁盘读取文件

为了确保你理解 LFS 的工作原理，现在让我们来了解一下从磁盘读取文件的过程。假设内存中什么都没有。我们必须读取的第一个磁盘数据结构是检查点区域。检查点区域包含指向整个 inode map的指针（即磁盘地址），因此 LFS 会读入整个 inode map并缓存在内存中。在此之后，当得到文件的 inode number时，LFS 只需在 imap 中查找 inode number到 inode磁盘地址的映射，然后读入最新版本的 inode。

此时，LFS 会根据需要使用直接指针、间接指针或双向间接指针，完全按照典型 UNIX 文件系统的方式读取文件块。在普通情况下，**LFS 从磁盘读取文件时执行的 I/O 次数应与典型文件系统相同**；整个 imap 已被缓存，因此 LFS 在读取过程中所做的额外工作就是在 imap 中查找 inode 的地址。

### 7.3 关于目录

到目前为止，我们已经通过仅考虑inode和数据块来简化了我们的讨论。但是，要访问文件系统中的文件（例如 `/home/zfhe/foo`），还必须访问某些目录。那么LFS是如何存储目录数据的呢？

幸运的是，目录结构与经典 UNIX 文件系统基本相同，因为目录只是（名称、inode number）映射的集合。例如，当在磁盘上创建文件时，LFS 必须写入新的 inode、一些数据以及引用该文件的目录数据及其 inode。请记住，LFS 将在磁盘上按顺序执行此操作（在缓冲更新一段时间后）。因此，在目录中创建文件`foo` 将导致磁盘上出现以下新结构：

![image-20240420133133412](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Directories_Example_LFS_1.png)

inode map的片段包含目录文件 `dir` 以及新创建的文件 `f` 的位置信息。因此，当访问文件 `foo` （inode number为 $k$）时，您首先会在inode map（通常缓存在内存中）中查找目录 `dir` ($A3$) 的inode的位置；然后读取目录 inode，它给出目录数据的位置 ($A2$)；读取此数据块即可获得 `(foo, k)` 的名称到 inode number的映射。然后再次查阅inode map，找到inode number k（$A1$）的位置，最后在地址$A0$处读取所需的数据块。

LFS 中 inode 映射还解决了另一个严重问题，称为<font color="red">递归更新问题</font>。任何从不就地更新（例如 LFS），而是将更新移动到磁盘上的新位置的文件系统都会出现此问题。

具体来说，每当更新inode时，它在磁盘上的位置就会发生变化。如果我们不小心的话，这也会导致指向该文件的目录的更新，然后会强制要求更改该目录的父目录，依此类推，一直沿着文件系统树向上更新。

LFS通过inode map巧妙地避免了这个问题。尽管inode的位置可能会发生变化，但这种变化永远不会反映在目录本身中；相反，当目录保存相同的名称到inode number映射时，imap 结构会被更新。因此，通过间接，LFS 避免了递归更新问题。

## 8 新问题：垃圾回收

### 8.1 基本介绍

您可能已经注意到 LFS 的另一个问题；它将文件的最新版本（包括其inode和数据）重复写入磁盘上的新位置。此过程在保持写入效率的同时，意味着 LFS 会将旧版本的文件结构分散在整个磁盘上。我们称这些旧版本为垃圾。例如，假设我们有一个由inode number $k$ 引用的现有文件，它指向单个数据块 $D0$。我们现在更新该块，生成新的inode和新的数据块。 LFS 的最终磁盘布局看起来像这样（注意，为了简单起见，我们省略了 imap 和其他结构；新的 imap 块也必须写入磁盘以指向新的 inode）：

![image-20240420150442686](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/LFS_Write_Have_Garbage.png)

在图中，您可以看到磁盘上的inode和数据块都有两个版本，一个是旧版本（左侧），另一个是当前的、即时的版本（右侧）。通过（逻辑上）更新数据块这一简单行为，LFS 必须持久化大量新结构，从而在磁盘上留下旧版本的数据块。

举个例子，想象我们将一个块附加到原始文件 `k` 上。在这种情况下，会生成新版本的 inode，但旧数据块仍由 inode 指向。因此它仍然是有效的，并且完全属于当前文件系统：

![image-20240420150821548](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/New_Append_In_LFS_Example.png)

那么我们应该如何处理这些旧版本的inode、数据块等呢？可以保留这些旧版本并允许用户恢复旧文件版本（例如，当他们不小心覆盖或删除文件时，这样做可能非常方便）；这种文件系统称为**版本控制文件系统**，因为它跟踪文件的不同版本。

然而，LFS 仅保留文件的最新实时版本；因此（在后台），LFS 必须定期查找文件数据、inode和其他结构的这些旧的无效版本，并清理它们；因此，清理应该使磁盘上的块再次空闲以供后续写入使用。请注意，清理过程是垃圾回收的一种形式，这是编程语言中出现的一种技术，可以自动释放程序未使用的内存。

前面我们讨论了段的重要性，因为它们是在 LFS 中实现对磁盘进行大量写入的机制。事实证明，它们对于有效清理也是不可或缺的。想象一下，如果 LFS 清理器在清理过程中简单地遍历并释放单个数据块、inode等，会发生什么。结果：文件系统在磁盘上分配的空间之间混合了一定数量的空闲孔。写入性能将大幅下降，因为 LFS 无法找到大的连续区域来顺序且高性能地写入磁盘。

相反，LFS 清理器逐段工作，从而为后续写入清理大块空间。基本清理过程如下。 LFS 清理器定期读取一些旧的（部分使用的）段，确定这些段中哪些块是有效的，然后写出一组新的段，其中仅包含有效的块，从而释放旧的段以供写入。具体来说，我们期望清理程序读取 $M$ 个现有段，将其内容压缩为 $N$ 个新段（其中 $N < M$ ），然后将 $N$ 个段写入磁盘的新位置。然后，旧的 $M$ 段将被释放，可供文件系统用于后续写入。

然而，我们现在面临两个问题。

* 第一个是**机制**：LFS 如何判断段内哪些块是有效块，哪些块是无效块？
* 第二个是**策略**：清理程序应该多久运行一次，以及应该选择清理哪些部分？

### 8.2 确定块有效性

我们首先解决机制问题。给定磁盘段 $S$ 内的数据块 $D$，LFS 必须能够确定 $D$ 是否处于有效状态。为此，LFS 向描述每个块的每个段添加了一些额外信息。具体来说，LFS包括每个数据块$D$包括它的inode number（它属于哪个文件）和它的偏移量（它是文件的哪个块）。该信息记录在段头部的结构中，称为<font color="red">段摘要块</font>。

有了这些信息，就可以很容易地确定一个块是有效的还是无效的。对于位于磁盘上地址 $A$ 的块 $D$，查看段摘要块并找到其inode number $N$ 和偏移量 $T$ 。接下来，在 imap 中查找 $N$ 所在的位置并从磁盘读取 $N$（也许它已经在内存中，这样更好）。最后，使用偏移量 $T$ ，查看 inode（或某个间接块）以查看 inode 认为该文件的第 $T$ 个块位于磁盘上的位置。如果它准确地指向磁盘地址A，LFS可以断定块D是有效的。如果它指向其他地方，LFS 可以断定 D 没有在使用中（即它已失效），从而知道不再需要该版本。这是伪代码摘要：

```c
(N, T) = SegmentSummary[A];
inode = Read(imap[N]);
if (inode[T] == A)
// block D is alive
else
// block D is garbage
```

下面是描述该机制的图，其中段摘要块（标记为 $SS$）记录了地址 $A0$ 处的数据块实际上是文件 `k` 偏移量 0 处的一部分。通过检查 `k` 的 imap，可以找到 inode，并看到它确实指向该位置。

![image-20240420153237660](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Mechanism_Find_Active_Block.png)

LFS 会采取一些快捷方式来提高确定有效性过程的效率。例如，当文件被截断或删除时，LFS 会增加其版本号，并在 imap 中记录新的版本号。通过在磁盘段中记录版本号，LFS 只需将磁盘上的版本号与 imap 中的版本号进行比较，就能缩短上述较长时间的检查，从而避免额外的读取。

### 8.3 策略问题：清理哪些块以及何时清理

除上述机制外，LFS 还必须包含一套策略，<font color="red">以确定何时清理以及哪些块值得清理；确定何时清理比较简单：定期、空闲时或磁盘已满而不得不清理时。</font>

而确定清理哪些块则更具挑战性，这也是许多研究论文的主题。在最初的 [LFS 论文](https://people.eecs.berkeley.edu/~brewer/cs262/LFS.pdf)中，作者描述了一种试图分离热段和冷段的方法。热段是指内容经常被覆盖的段，因此，对于这样的段，最好的策略是等待很长时间再进行清理，因为越来越多的数据块被覆盖（在新的段中），从而被释放出来以供使用。

相比之下，冷段可能会有一些无效块，但其余内容相对稳定。因此，作者得出结论，应该尽早清理冷段，晚些清理热段，并开发了一种启发式方法来实现这一目标。然而，与大多数策略一样，这种策略并不完美；[后来的方法](https://dl.acm.org/doi/10.1145/268998.266700)展示了如何做得更好。

## 9 崩溃恢复和日志

最后一个问题：如果 LFS 写入磁盘时系统崩溃，会发生什么？更新期间的崩溃对于文件系统来说是很棘手的，因此 LFS 也必须考虑这一点。

<font color="red">在正常操作期间，LFS 缓冲段中的写入，然后（当段已满或经过一定时间时）将该段写入磁盘。 LFS 将这些写入组织在日志中，即检查点区域指向头段和尾段，每个段都指向下一个要写入的段。</font> LFS 还定期更新检查点区域。在这些操作（写入段、写入 CR）期间显然可能会发生崩溃。那么 LFS 如何处理写入这些结构期间的崩溃呢？

我们先来说第二种情况。为了确保 CR 更新以原子方式发生，LFS 实际上**保留了两个 CR**，分别位于磁盘的两端，并交替写入。 LFS 在使用指向 inode map的最新指针和其他信息更新 CR 时还实现了谨慎的协议；具体来说，它首先写出一个标头（带有时间戳），然后写出 CR 的主题，最后写出最后一个块（也带有时间戳）。如果系统在 CR 更新期间崩溃，LFS 可以通过查看一对不一致的时间戳来检测到这一情况。 LFS总是会选择使用最新的具有一致时间戳的CR，从而实现CR的一致更新。

现在我们来解决第一种情况。由于 LFS 大约每 30 秒写入一次 CR，因此文件系统的最后一个一致快照可能相当旧。因此，重新启动后，LFS 可以通过简单地读取检查点区域、它指向的 imap 片段以及后续文件和目录来轻松恢复；但是，最后几秒的更新将会丢失。

为了改进这一点，LFS 尝试通过数据库社区中称为**前滚**的技术来重建许多这些段。基本思想是从最后一个检查点区域开始，找到日志的末尾（包含在 CR 中），然后使用它来读取接下来的段并查看其中是否有任何有效的更新。如果有，LFS 会相应地更新文件系统，从而恢复自上一个检查点以来写入的大部分数据和元数据。

# Program Explanation
`lfs.py`为日志结构文件系统 LFS 的模拟器。模拟器稍微简化了本书章节的 LFS，但希望留出足够的空间来说明这种文件系统的一些重要属性。

若要开始，请运行以下命令：

```shell
❯ python lfs.py -n 1 -o

INITIAL file system contents:
[   0 ] live checkpoint: 3 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
[   1 ] live [.,0] [..,0] -- -- -- -- -- -- 
[   2 ] live type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
[   3 ] live chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

create file /ku3 

FINAL file system contents:
[   0 ] ?    checkpoint: 7 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
[   1 ] ?    [.,0] [..,0] -- -- -- -- -- -- 
[   2 ] ?    type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
[   3 ] ?    chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
[   4 ] ?    [.,0] [..,0] [ku3,1] -- -- -- -- -- 
[   5 ] ?    type:dir size:1 refs:2 ptrs: 4 -- -- -- -- -- -- -- 
[   6 ] ?    type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
[   7 ] ?    chunk(imap): 5 6 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
```

输出显示空 LFS 的初始文件系统状态，并初始化了几个不同的块。第一个块（块 0）是此 LFS 的“检查点区域”。为简单起见，此 LFS 只有一个检查点区域，并且它始终位于块地址 = 0，并且始终只是单个块的大小。

检查点区域的内容只是磁盘地址：inode map块的位置。在这种情况下，检查点区域具有以下内容：

```
checkpoint: 3 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
```

我们将最左边的条目（此处标有 3）称为第 0 个条目，下一个条目称为第 1 个条目，最后一个条目（因为有 16 个条目）称为第 15 个条目。因此，我们可以将它们视为：

```
checkpoint: 3 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
    entry:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
```

这意味着 inode map的第一个块位于磁盘地址 = 3，其余的 inode 映射块尚未分配（因此标记为“--”）。

现在让我们看一下 inode map的那块（从现在开始是“imap”）。imap 只是一个数组，它告诉您每个 inode number它在磁盘上的当前位置。在上面显示的初始状态中，我们看到：

```
chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
```

这些块（默认情况下）也有 16 个条目，我们可以再次将它们视为：

```
chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
     entry:  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
```

由于 imap 的每个块都有 16 个条目，并且检查点区域 （CR） 有 16 个条目，因此我们现在知道整个 LFS 有 $16\times 16=256$ 个可用于文件的 inode number。一个小文件系统，但对于我们的目的来说已经足够了。

我们现在还知道 imap 的每个块都负责一组连续的 inode，并且我们知道哪些 inode 取决于 CR 中的哪个条目指向该块。具体来说，CR 的条目 0 指向 imap 的块，其中包含有关 inode number 0...15 的信息;CR 的条目 1 指向 inode number 16...31 的 imap 块。

在这个具体的例子中，我们知道 CR 的第 0 个条目指向 block=3，其中第 0 个条目有一个“2”。在我们的模拟器中，根 inode 是 inode number=0，因此这是文件系统根目录的 inode。从 imap 中，我们现在知道 inode number=0 的地址的位置是 block=2。因此，让我们看看第 2 块！我们看到：

```
type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- --
```

此文件元数据是一个简化的 inode，具有文件类型（目录）、大小（1 个块）、引用计数（如果这是一个目录，它引用了多少个目录）和一定数量的数据块指针（在本例中，一个指向块地址=1）。

这最终将我们引向初始状态的最后一点，即目录的内容。此目录中只有一个块（地址 1），其内容如下：

```
[.,0] [..,0] -- -- -- -- -- --
```

这里有一个空目录，其中有 [name，inode number] 对，用于自身 （“.”） 和它的父目录 （“..”）。在这种特殊情况下（根），父目录只是它自己，并且两者都是 inode number=0。呼！我们现在（希望）已经了解了文件系统初始状态的全部内容。

在模拟的默认模式下，接下来发生的事情是针对文件系统运行一个或多个操作，从而更改其状态。在这种情况下，我们知道命令是什么，因为我们让模拟器通过“-o”标志告诉我们（它显示每个操作的运行情况）。该操作是：

```
create file /ku3
```

这意味着在根目录“/”中创建了一个文件“ku3”。若要完成此创建，必须更新许多结构，这意味着日志已写入。您可以看到，在日志的前一端 （address=3） 之外，在块 4...7 处发生了四次写入：

```
[.,0] [..,0] [ku3,1] -- -- -- -- --
type:dir size:1 refs:2 ptrs: 4 -- -- -- -- -- -- --
type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- --
chunk(imap): 5 6 -- -- -- -- -- -- -- -- -- -- -- -- -- --
```

这些更新反映了此版本的 LFS 如何写入磁盘以创建文件：

* 目录块更新，以在根目录中包含“ku3”及其 inode number （1）
* 更新的根 inode，现在引用块 4，可在其中找到此目录的最新内容
* 新创建的文件的新 inode（记下类型）
* 第一个 imap 块的新版本，它现在告诉我们 inode 0 和 inode 1 的位置

然而，这并不能（完全）反映所有必须改变的东西。由于 inode 映射本身已更改，因此检查点区域还必须反映 inode 映射的第一部分的最新块所在的位置。因此，CR 也进行了更新：

```
checkpoint: 7 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
```

您可能还注意到输出中的另一件事。在初始文件系统内容中，磁盘地址和内容之间有一个标记，每个条目都标明“live”，而在最终输出中则有一个“？”。这个“？”在那里，你可以自己确定每个块是否是有效的。从检查点区域开始，看看您是否可以确定可以到达哪组块（因此是有效的）；其余的都无效了，可以再次使用。

要查看您是否正确，请再次运行 `-c` ：

```shell
❯ python lfs.py -n 1 -o -c

INITIAL file system contents:
[   0 ] live checkpoint: 3 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
[   1 ] live [.,0] [..,0] -- -- -- -- -- -- 
[   2 ] live type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
[   3 ] live chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 

create file /ku3 

FINAL file system contents:
[   0 ] live checkpoint: 7 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
[   1 ]      [.,0] [..,0] -- -- -- -- -- -- 
[   2 ]      type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
[   3 ]      chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
[   4 ] live [.,0] [..,0] [ku3,1] -- -- -- -- -- 
[   5 ] live type:dir size:1 refs:2 ptrs: 4 -- -- -- -- -- -- -- 
[   6 ] live type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
[   7 ] live chunk(imap): 5 6 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
```

正如你现在所看到的，每次更新一个结构时，垃圾都会被留下，这是像LFS这样的非就地更新文件系统必须处理的主要问题之一。幸运的是，对于您来说，我们不会太担心这个简化版本的 LFS 中的垃圾回收。您现在拥有了解此版本的 LFS 所需的所有信息。

运行以下命令获取详细选项：

```shell
❯ python lfs.py -h
Usage: lfs.py [options]

Options:
  -h, --help            show this help message and exit
  -s SEED, --seed=SEED  the random seed
  -N, --no_force        Do not force checkpoint writes after updates
  -F, --no_final        Do not show the final state of the file system
  -D, --use_disk_cr     use disk (maybe old) version of checkpoint region
  -c, --compute         compute answers for me
  -o, --show_operations
                        print out operations as they occur
  -i, --show_intermediate
                        print out state changes as they occur
  -e, --show_return_codes
                        show error/return codes
  -v, --show_live_paths
                        show live paths
  -n NUM_COMMANDS, --num_commands=NUM_COMMANDS
                        generate N random commands
  -p PERCENTAGES, --percentages=PERCENTAGES
                        percent chance of:
                        createfile,writefile,createdir,rmfile,linkfile,sync
                        (example is c30,w30,d10,r20,l10,s0)
  -a INODE_POLICY, --allocation_policy=INODE_POLICY
                        inode allocation policy: "r" for "random" or "s" for
                        "sequential"
  -L COMMAND_LIST, --command_list=COMMAND_LIST
                        command list in format:
                        "cmd1,arg1,...,argN:cmd2,arg1,...,argN:... where cmds
                        are: c:createfile, d:createdir, r:delete, w:write,
                        l:link, s:sync format: c,filepath d,dirpath r,filepath
                        w,filepath,offset,numblks l,srcpath,dstpath s
```

# Homework
1. 运行 `python lfs.py -n 3` ，也许会改变种子 （ `-s` ）。您能弄清楚运行了哪些命令来生成最终的文件系统内容吗？你能说出这些命令是按什么顺序发出的吗？最后，您能否确定每个块在最终文件系统状态下的存活度？用于 `-o` 显示已运行的命令，并 `-c` 显示最终文件系统状态的有效性。随着您发出的命令数量的增加（即 `-n 3` 更改为 `-n 5` ），任务对您来说会变得多难？

	```shell
	❯ python lfs.py -n 3
	
	INITIAL file system contents:
	[   0 ] live checkpoint: 3 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   1 ] live [.,0] [..,0] -- -- -- -- -- -- 
	[   2 ] live type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
	[   3 ] live chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	
	command? 
	command? 
	command? 
	
	FINAL file system contents:
	[   0 ] ?    checkpoint: 14 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   1 ] ?    [.,0] [..,0] -- -- -- -- -- -- 
	[   2 ] ?    type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
	[   3 ] ?    chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   4 ] ?    [.,0] [..,0] [ku3,1] -- -- -- -- -- 
	[   5 ] ?    type:dir size:1 refs:2 ptrs: 4 -- -- -- -- -- -- -- 
	[   6 ] ?    type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[   7 ] ?    chunk(imap): 5 6 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   8 ] ?    z0z0z0z0z0z0z0z0z0z0z0z0z0z0z0z0
	[   9 ] ?    type:reg size:8 refs:1 ptrs: -- -- -- -- -- -- -- 8 
	[  10 ] ?    chunk(imap): 5 9 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[  11 ] ?    [.,0] [..,0] [ku3,1] [qg9,2] -- -- -- -- 
	[  12 ] ?    type:dir size:1 refs:2 ptrs: 11 -- -- -- -- -- -- -- 
	[  13 ] ?    type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[  14 ] ?    chunk(imap): 12 9 13 -- -- -- -- -- -- -- -- -- -- -- -- -- 
	```

	* `create file /ku3`
	* ``write file /ku3 offset=7 size=4` 
	* `create file /qg9` 

	```shell
	FINAL file system contents:
	[   0 ] live checkpoint: 14 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   1 ]      [.,0] [..,0] -- -- -- -- -- -- 
	[   2 ]      type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
	[   3 ]      chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   4 ]      [.,0] [..,0] [ku3,1] -- -- -- -- -- 
	[   5 ]      type:dir size:1 refs:2 ptrs: 4 -- -- -- -- -- -- -- 
	[   6 ]      type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[   7 ]      chunk(imap): 5 6 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   8 ] live z0z0z0z0z0z0z0z0z0z0z0z0z0z0z0z0
	[   9 ] live type:reg size:8 refs:1 ptrs: -- -- -- -- -- -- -- 8 
	[  10 ]      chunk(imap): 5 9 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[  11 ] live [.,0] [..,0] [ku3,1] [qg9,2] -- -- -- -- 
	[  12 ] live type:dir size:1 refs:2 ptrs: 11 -- -- -- -- -- -- -- 
	[  13 ] live type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[  14 ] live chunk(imap): 12 9 13 -- -- -- -- -- -- -- -- -- -- -- -- -- 
	```

	随着命令变多，确实会很难，但只要按着文件树的逻辑往下遍历，就还是能找到所有的有效块。

2. 如果您发现上述情况很痛苦，您可以通过显示每个特定命令引起的更新集来帮助自己。为此，请运行 `python lfs.py -n 3 -i` 。现在看看是否更容易理解每个命令必须是什么。更改随机种子以获得不同的命令进行解释（例如， `-s 1` 、 `-s 2` 、 `-s 3` 等）。

	```shell
	❯ python lfs.py -n 3 -i -s 1
	
	INITIAL file system contents:
	[   0 ] live checkpoint: 3 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   1 ] live [.,0] [..,0] -- -- -- -- -- -- 
	[   2 ] live type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
	[   3 ] live chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	
	command? 
	
	[   0 ] ?    checkpoint: 7 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	...
	[   4 ] ?    [.,0] [..,0] [tg4,1] -- -- -- -- -- 
	[   5 ] ?    type:dir size:1 refs:2 ptrs: 4 -- -- -- -- -- -- -- 
	[   6 ] ?    type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[   7 ] ?    chunk(imap): 5 6 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	
	
	command? 
	
	[   0 ] ?    checkpoint: 9 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	...
	[   8 ] ?    type:reg size:6 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[   9 ] ?    chunk(imap): 5 8 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	
	
	command? 
	
	[   0 ] ?    checkpoint: 13 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	...
	[  10 ] ?    [.,0] [..,0] [tg4,1] [lt0,2] -- -- -- -- 
	[  11 ] ?    type:dir size:1 refs:2 ptrs: 10 -- -- -- -- -- -- -- 
	[  12 ] ?    type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[  13 ] ?    chunk(imap): 11 8 12 -- -- -- -- -- -- -- -- -- -- -- -- -- 
	
	
	
	FINAL file system contents:
	[   0 ] ?    checkpoint: 13 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   1 ] ?    [.,0] [..,0] -- -- -- -- -- -- 
	[   2 ] ?    type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
	[   3 ] ?    chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   4 ] ?    [.,0] [..,0] [tg4,1] -- -- -- -- -- 
	[   5 ] ?    type:dir size:1 refs:2 ptrs: 4 -- -- -- -- -- -- -- 
	[   6 ] ?    type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[   7 ] ?    chunk(imap): 5 6 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   8 ] ?    type:reg size:6 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[   9 ] ?    chunk(imap): 5 8 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[  10 ] ?    [.,0] [..,0] [tg4,1] [lt0,2] -- -- -- -- 
	[  11 ] ?    type:dir size:1 refs:2 ptrs: 10 -- -- -- -- -- -- -- 
	[  12 ] ?    type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[  13 ] ?    chunk(imap): 11 8 12 -- -- -- -- -- -- -- -- -- -- -- -- -- 
	```

	* `create file /tg4`
	* `write  file offset=6 size=0`
	* `create file /lt0`

	其他同理

	```shell
	❯ python lfs.py -n 3 -i -s 2
	❯ python lfs.py -n 3 -i -s 3
	```

3. 为了进一步测试你是否有能力找出每条命令对磁盘进行了哪些更新，请运行以下命令： `python lfs.py -o -F -s 100` （也许还有其他一些随机种子）。这只显示一组命令，不显示文件系统的最终状态。你能推理出文件系统的最终状态必须是什么吗？

	```shell
	❯ python lfs.py -o -s 100 -c
	
	INITIAL file system contents:
	[   0 ] live checkpoint: 3 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   1 ] live [.,0] [..,0] -- -- -- -- -- -- 
	[   2 ] live type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
	[   3 ] live chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	
	create file /us7 
	write file  /us7 offset=4 size=0 
	write file  /us7 offset=7 size=7 
	
	FINAL file system contents:
	[   0 ] live checkpoint: 12 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   1 ]      [.,0] [..,0] -- -- -- -- -- -- 
	[   2 ]      type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
	[   3 ]      chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   4 ] live [.,0] [..,0] [us7,1] -- -- -- -- -- 
	[   5 ] live type:dir size:1 refs:2 ptrs: 4 -- -- -- -- -- -- -- 
	[   6 ]      type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[   7 ]      chunk(imap): 5 6 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   8 ]      type:reg size:4 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[   9 ]      chunk(imap): 5 8 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[  10 ] live i0i0i0i0i0i0i0i0i0i0i0i0i0i0i0i0
	[  11 ] live type:reg size:8 refs:1 ptrs: -- -- -- -- -- -- -- 10 
	[  12 ] live chunk(imap): 5 11 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	```

4. 现在，看看是否可以确定哪些文件和目录在执行许多文件和目录操作后处于有效状态。运行 `python lfs.py -n 20 -s 1` 并检查最终文件系统状态。你能弄清楚哪些路径名是有效的吗？运行 `python lfs.py -n 20 -s 1 -c -v` 以查看结果。运行 with `-o` 以查看给定一系列随机命令的答案是否匹配。使用不同的随机种子来获得更多问题。

	```shell
	❯ python lfs.py -n 20 -s 1 -c -v -o
	```

5. 现在让我们发出一些特定的命令。首先，让我们创建一个文件并重复写入它。为此，请使用该 `-L` 标志，该标志允许您指定要执行的特定命令。让我们创建文件“/foo”并写入它四次： `-L c,/foo:w,/foo,0,1:w,/foo,1,1:w,/foo,2,1:w,/foo,3,1 -o` 。查看是否可以确定最终文件系统状态的活跃度;用于 `-c` 检查您的答案。

	```shell
	❯ python lfs.py -L c,/foo:w,/foo,0,1:w,/foo,1,1:w,/foo,2,1:w,/foo,3,1 -o -c
	
	INITIAL file system contents:
	[   0 ] live checkpoint: 3 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   1 ] live [.,0] [..,0] -- -- -- -- -- -- 
	[   2 ] live type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
	[   3 ] live chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	
	create file /foo 
	write file  /foo offset=0 size=1 
	write file  /foo offset=1 size=1 
	write file  /foo offset=2 size=1 
	write file  /foo offset=3 size=1 
	
	FINAL file system contents:
	[   0 ] live checkpoint: 19 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   1 ]      [.,0] [..,0] -- -- -- -- -- -- 
	[   2 ]      type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
	[   3 ]      chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   4 ] live [.,0] [..,0] [foo,1] -- -- -- -- -- 
	[   5 ] live type:dir size:1 refs:2 ptrs: 4 -- -- -- -- -- -- -- 
	[   6 ]      type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[   7 ]      chunk(imap): 5 6 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   8 ] live v0v0v0v0v0v0v0v0v0v0v0v0v0v0v0v0
	[   9 ]      type:reg size:1 refs:1 ptrs: 8 -- -- -- -- -- -- -- 
	[  10 ]      chunk(imap): 5 9 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[  11 ] live t0t0t0t0t0t0t0t0t0t0t0t0t0t0t0t0
	[  12 ]      type:reg size:2 refs:1 ptrs: 8 11 -- -- -- -- -- -- 
	[  13 ]      chunk(imap): 5 12 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[  14 ] live k0k0k0k0k0k0k0k0k0k0k0k0k0k0k0k0
	[  15 ]      type:reg size:3 refs:1 ptrs: 8 11 14 -- -- -- -- -- 
	[  16 ]      chunk(imap): 5 15 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[  17 ] live g0g0g0g0g0g0g0g0g0g0g0g0g0g0g0g0
	[  18 ] live type:reg size:4 refs:1 ptrs: 8 11 14 17 -- -- -- -- 
	[  19 ] live chunk(imap): 5 18 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	```

6. 现在，让我们做同样的事情，但使用一个写入操作而不是四个。运行 `python lfs.py -o -L c,/foo:w,/foo,0,4` 以创建文件“/foo”，并通过单个写入操作写入 4 个块。再次计算有效性，并检查 `-c`。一次写完一个文件（就像我们在这里所做的那样）和一次写一个块（如上所述）之间的主要区别是什么？这说明了在主内存中缓冲更新的重要性，就像真正的LFS一样？

	```shell
	❯ python lfs.py -o -L c,/foo:w,/foo,0,4 -c
	
	INITIAL file system contents:
	[   0 ] live checkpoint: 3 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   1 ] live [.,0] [..,0] -- -- -- -- -- -- 
	[   2 ] live type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
	[   3 ] live chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	
	create file /foo 
	write file  /foo offset=0 size=4 
	
	FINAL file system contents:
	[   0 ] live checkpoint: 13 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   1 ]      [.,0] [..,0] -- -- -- -- -- -- 
	[   2 ]      type:dir size:1 refs:2 ptrs: 1 -- -- -- -- -- -- -- 
	[   3 ]      chunk(imap): 2 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   4 ] live [.,0] [..,0] [foo,1] -- -- -- -- -- 
	[   5 ] live type:dir size:1 refs:2 ptrs: 4 -- -- -- -- -- -- -- 
	[   6 ]      type:reg size:0 refs:1 ptrs: -- -- -- -- -- -- -- -- 
	[   7 ]      chunk(imap): 5 6 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	[   8 ] live v0v0v0v0v0v0v0v0v0v0v0v0v0v0v0v0
	[   9 ] live t1t1t1t1t1t1t1t1t1t1t1t1t1t1t1t1
	[  10 ] live k2k2k2k2k2k2k2k2k2k2k2k2k2k2k2k2
	[  11 ] live g3g3g3g3g3g3g3g3g3g3g3g3g3g3g3g3
	[  12 ] live type:reg size:4 refs:1 ptrs: 8 9 10 11 -- -- -- -- 
	[  13 ] live chunk(imap): 5 12 -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
	```

	缓冲让读取和写入速度更快。一次写入一个块会创建更多垃圾并分离文件数据块。

7. 让我们再举一个具体的例子。首先，运行以下命令： `python lfs.py -L c,/foo:w,/foo,0,1` 。这组命令有什么作用？现在，运行 `python lfs.py -L c,/foo:w,/foo,7,1` ，这组命令有什么作用？两者有何不同？从这两组命令中，您能从中看出 inode 中的`size`字段吗？

	前者在开头写入一个块，`size`为1；后者在末尾写入一个块，`size`为8。

8. 现在，让我们明确地看一下文件创建与目录创建。运行模拟 `python lfs.py -L c,/foo` 并 `python lfs.py -L d,/foo` 创建一个文件，然后创建一个目录。这些运行有什么相似之处，又有什么不同之处？

	创建的文件一开始没有数据块，但目录立即有一个，并且还增加了对根目录的引用。

9. LFS模拟器也支持硬链接。运行以下命令以研究它们的工作原理： `python lfs.py -L c,/foo:l,/foo,/bar:l,/foo,/goo -o -i` .创建硬链接时会写出哪些块？这与创建新文件有什么相似之处，又有什么不同？创建链接时，引用计数字段如何变化？

	会写出：检查点区域、父目录数据块和 inode、imap。

	相同：都更新检查点区域、父目录数据块和 inode、imap。

	不同：创建新文件将为其创建一个新 inode，创建硬链接不会创建新 inode。

	它将链接文件的引用计数字段增加1。

10. 这里有一个简单的问题，我们确实在探索：inode number的选择。首先，运行 `python lfs.py -p c100 -n 10 -o -a s` 以显示“顺序”分配策略的通常行为，该策略尝试使用最接近零的可用 inode number。然后，通过运行 `python lfs.py -p c100 -n 10 -o -a r` 更改为“随机”策略（该 `-p c100` 标志确保 100% 的随机操作是文件创建）。随机策略与顺序策略会导致哪些磁盘差异？这说明了在实际 LFS 中选择inode number的重要性吗？

	新文件的 inode 是随机选择的，因此 inode map处于混乱状态。

11. 我们假设的最后一件事是，LFS 模拟器总是在每次更新后更新检查点区域。在实际的 LFS 中，情况并非如此：它会定期更新以避免长时间搜索。运行 `python lfs.py -N -i -o -s 1000` 以查看某些操作以及检查点区域未强制到磁盘时文件系统的中间和最终状态。如果检查点区域从不更新，会发生什么情况？如果定期更新怎么办？您能否弄清楚如何通过在日志中向前滚动将文件系统恢复到最新状态？

	如果检查点区域从不更新，则所有操作都将毫无意义。定期更新应该没问题。最新的日志有最新的 imap，用它来更新检查点区域。