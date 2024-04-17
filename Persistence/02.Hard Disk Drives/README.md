# Note/Translation
几十年来，硬盘驱动器驱动器一直是计算机系统中持久数据存储的主要形式，文件系统技术的大部分发展都是基于它们的行为。因此，在构建管理磁盘的文件系统软件之前，有必要了解磁盘操作的细节。

> <center>关键问题</center>
>
> 现代硬盘驱动器如何存储数据？接口是什么？数据实际上是如何布局和访问的？磁盘调度如何提高性能？

## 1 接口

让我们先来了解一下现代磁盘驱动器的接口。所有现代硬盘的基本接口都很简单。硬盘由大量扇区（512 字节块）组成，每个扇区都可读写。在有 `n` 个扇区的磁盘上，扇区的编号从 `0` 到 `n - 1`。因此，我们可以将磁盘视为一个扇区数组；`0` 至 `n - 1` 就是**硬盘的地址空间**。

多扇区操作是可能的；事实上，许多文件系统一次可以读写 4KB （或更多）。不过，在更新磁盘时，硬盘制造商唯一能保证的是<font color="red">单次 512 字节的写入是原子性的（即要么全部完成，要么根本不完成）</font>；因此，如果发生意外断电，可能只会完成较大写入的一部分（有时称为撕裂写入）。

大多数磁盘驱动器客户端都会做出一些假设，但这些假设并没有在接口中直接指定；Schlosser 和 Ganger 将此称为磁盘驱动器的 "不成文契约"。具体来说，我们通常可以假定，<font color="red">访问硬盘地址空间内相邻的两个区块会比访问相距较远的两个区块更快</font>。我们通常还可以假设，<font color="red">以连续块（即顺序读取或写入）方式访问块是最快的访问模式，通常比任何随机访问模式都要快得多</font>。

## 2 基本几何

让我们开始了解现代磁盘的一些组成部分。我们先从**盘片**开始，盘片是一个圆形硬表面，通过磁性变化将数据持久地存储在上面。磁盘可能有一个或多个盘片；每个盘片有两个面，每个面称为一个表面。这些盘片通常由某种硬质材料（如铝）制成，然后涂上一层薄薄的磁层，使硬盘即使在关机时也能持久存储比特数据。

盘片都围绕**主轴**结合在一起，主轴与电机相连，电机以恒定（固定）的速度带动盘片旋转（当硬盘接通电源时）。转速通常以每分钟转数（RPM）为单位，现代的典型值在 7,200 RPM 到 15,000 RPM 之间。请注意，我们通常会对单次旋转的时间感兴趣，例如，转速为 10,000 RPM 的硬盘意味着单次旋转大约需要 6 毫秒（6 毫秒）。

数据以同心圆扇形编码在每个表面上；我们称这样的同心圆为一个**磁道**。单个表面包含成千上万条轨道，它们紧密地排列在一起，数百条轨道的宽度仅相当于人的头发丝。

要从磁盘表面读写，我们需要一种机制，让我们能够感知（即读取）磁盘上的磁性图案，或引起磁性图案的变化（即写入）。读写过程由**磁头**完成；驱动器的每个表面都有一个磁头。磁头连接在单个**磁盘臂**上，磁盘臂在磁盘表面移动，将磁头定位在所需磁道上。

## 3 简单的磁盘驱动器

### 3.1 工作原理

磁盘结构如下：

![image-20240413220625584](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Hard_Disk_Example.png)

让我们通过一次一个磁道构建模型来了解磁盘的工作原理。假设我们有一个单磁道的简单磁盘，如下图所示。

![image-20240413143435952](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/A_Disk_With_Just_A_Single_Track.png)

该磁道只有 12 个扇区，每个扇区大小为 512 字节（我们典型的扇区大小），因此通过数字 0 到 11 进行寻址。我们这里的单盘片围绕主轴旋转，电机连接到主轴上。当然磁道本身并不重要；我们希望能够读取或写入这些扇区，因此我们需要一个磁头，连接到磁盘臂上，就像下图所示。在图中，连接到臂末端的磁盘头位于扇区 6 上方，并且表面逆时针旋转。

![image-20240413212732509](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/A_Single_Track_Plus_A_Head.png)

### 3.2 单磁道延迟：旋转延迟

 为了了解简单的单磁道磁盘如何处理请求，设想我们现在收到一个读取 0 号数据块的请求。磁盘应如何处理该请求？

在我们的简单磁盘中，磁盘无需做太多工作。特别是，<font color="red">它必须等待所需的扇区在磁头下旋转</font>。这种等待在现代硬盘中经常发生，是 I/O 服务时间的重要组成部分，因此有一个专门的名称：**旋转延迟**。在示例中，如果全部旋转延迟为$R$，那么磁盘在等待 0 进入读/写磁头（如果我们从 6 开始）时，需要大约 $\frac{R}{2}$ 的旋转延迟。在此单磁道上，最坏的情况是向扇区 5 提出请求，为了满足这样的请求，磁盘几乎要产生一个完整的旋转延迟。

### 3.3 多磁道：寻道时间

到目前为止，我们的磁盘只有一条磁道，这不太现实；现代磁盘当然有数百万条磁道。因此，让我们来看看更加逼真的磁盘表面，这个磁盘有三个磁道如下图所示。

![image-20240413222542362](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Three_Tracks_Plus_A_Head_Example.png)

在图中，磁头当前位于最内侧的磁道上（包含 24 到 35 个扇区）；下一个磁道包含下一组扇区（12 到 23），最外侧的磁道包含第一个扇区（0 到 11）。

为了了解驱动器如何访问给定扇区，我们现在跟踪对远程扇区的请求会发生什么，例如，对扇区 11 的读取。为了服务此读取，驱动器必须首先将磁盘臂移动到正确的磁道（在本例中为最外层），这一过程称为<font color="red">寻道</font>。查找和旋转是成本最高的磁盘操作之一。

应该注意的是，寻道有多个阶段：首先是磁盘臂移动的加速阶段；然后当磁盘臂全速移动时滑行，然后当磁盘臂减慢时减速；当磁头小心地定位在正确的磁道上时，最终稳定下来。稳定时间通常非常重要，例如 0.5 到 2 毫秒，因为驱动器必须确保找到正确的磁道。

寻道后，磁盘臂将磁头定位在正确的磁道上。下图描述了寻道。

![image-20240414105224859](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Three_Tracks_Plus_A_Head_With_Seek_Example.png)

正如我们所看到的，在寻道过程中，磁盘臂已移动到所需的磁道，并且盘片当然也旋转了，在本例中大约旋转了 3 个扇区。这样，扇区9即将从磁头下方经过，我们只需忍受短暂的旋转延迟即可完成传输。

当扇区 11 经过磁盘头下方时，将发生 I/O 的最后阶段，称为传输，其中数据从表面读取或写入表面。这样，我们就有了 I/O 时间的完整情况：首先是寻道，然后等待旋转延迟，最后是传输。

### 3.4 其他一些细节

关于硬盘驱动器的运行方式还有一些其他有趣的细节。许多驱动器采用某种<font color="red">磁道倾斜</font>来确保即使在跨越磁道边界时也可以正确服务顺序读取。在我们的简单示例磁盘中，这可能如下图所示。

![image-20240414105619864](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Three_Tracks_Track_Skew_Of_2.png)

扇区通常会出现这样的倾斜，因为从一个磁道切换到另一个磁道时，磁盘需要时间来重新定位磁头（甚至是相邻的磁道）。如果没有这种偏斜，磁头会移动到下一个磁道，但所需的下一个区块已经在磁头下方旋转，因此硬盘需要等待几乎整个旋转延迟才能访问下一个区块。

另一个实际情况是，外磁道往往比内磁道有更多的扇区，这是几何学的结果；因为外磁道有更多的空间。这些磁道通常被称为**多分区磁盘驱动器**，即磁盘被组织成多个区，一个区是表面上连续的一组磁道。每个区的每个磁道都有相同数量的扇区，外区的扇区数量多于内区。

最后，现代磁盘驱动器的一个重要组成部分是缓存，由于历史原因，有时也称为磁道缓冲区。这种缓存只是一小部分内存（通常约为 8 或 16 MB），磁盘驱动器可以用它来保存从磁盘读取或写入磁盘的数据。例如，从磁盘读取扇区时，磁盘驱动器可能会决定读入该磁道上的所有扇区，并将其缓存在内存中；这样做可让磁盘驱动器快速响应对同一磁道的任何后续请求。

在写入时，磁盘驱动器可以选择：是在将数据存入内存时确认写入已完成，还是在数据实际写入磁盘后确认？前者称为<font color="red">回写缓存（有时也称为即时报告）</font>，后者称为<font color="red">直写缓存</font>。回写缓存有时会让磁盘驱动器看起来 "更快"，但也可能是危险的；如果文件系统或应用程序要求按一定顺序将数据写入磁盘以保证正确性，回写缓存可能会导致问题。

## 4 I/O时间

现在我们有了磁盘的抽象模型，可以通过一些分析来更好地了解磁盘性能。特别是，我们现在可以将 I/O 时间表示为三个主要部分的总和：
$$
T_{I/O}=T_{seek}+T_{rotation}+T_{transfer}
$$
请注意，I/O 速率 ($R_{I/O}$) 通常更容易用于驱动器之间的比较（如下所示），可以轻松地根据时间计算出来。只需将传输大小除以所花费的时间即可：
$$
R_{I/O}=\frac{Size_{Transfer}}{T_{I/O}}
$$
为了更好地了解 I/O 时间，让我们执行以下计算。假设有两个我们感兴趣的工作负载。第一个称为<font color="red">随机工作负载</font>，向磁盘上的随机位置发出小（例如 4KB）读取。随机工作负载在许多重要应用程序中很常见，包括数据库管理系统。第二种称为<Font color="red">顺序工作负载</，它只是从磁盘连续读取大量扇区，而不会跳转。顺序访问模式非常常见，因此也很重要。

为了了解随机工作负载和顺序工作负载之间的性能差异，我们需要首先对磁盘驱动器做出一些假设。让我们看一下`Seagate`的几款现代磁盘。第一个称为 Cheetah 15K.5，是一款高性能 SCSI 驱动器。第二个是 Barracuda，是一款专为容量而设计的硬盘。两者的详细信息如下图所示。

![image-20240414124805189](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Disk_Drive_Specs_SCSI_Versus.png)

正如您所看到的，这些驱动器具有截然不同的特征，并且在许多方面很好地概括了磁盘驱动器市场的两个重要组成部分。第一个是**“高性能”**驱动器市场，该市场的驱动器设计为尽可能快地旋转、提供较短的寻道时间并快速传输数据。第二个是**“容量”**市场，其中每字节成本是最重要的方面；因此，驱动器速度较慢，但可以将尽可能多的位装入可用空间。

根据这些数字，我们可以开始计算驱动器在上述两种工作负载下的表现如何。让我们首先看看随机工作负载。假设每次 4 KB 读取发生在磁盘上的随机位置，我们可以计算每次此类读取需要多长时间。关于`Cheetah`：
$$
T_{seek}=4ms,T_{rotation}=2ms,T_{transfer}=30\mu s
$$
平均寻道时间（4 毫秒）是根据制造商报告的平均时间得出的；请注意，完全寻道（从表面的一端到另一端）可能需要三倍的时间。

> 所有寻道距离的平均值（对于具有N个磁道的磁盘），在两个磁道x和y之间。
>
> $\frac{1}{N^2}\sum\limits_{x=0}^N\sum\limits_{y=0}^N|x-y|\approx \frac{1}{N^2}\int_{x=0}^N\int_{y=0}^N|x-y|dxdy=\frac{1}{N^2}(\frac{1}{3}x^3-\frac{N}{2}x^2+\frac{N^2}{2}x)|_0^N=\frac{N}{3}$

平均旋转延迟是直接根据转速计算出来的。15000 RPM 等于 250 RPS（每秒旋转次数）；因此，每次旋转需要 4 毫秒。磁盘平均旋转半圈，因此平均时间为 2 毫秒。最后，传输时间只是峰值传输速率的传输大小；在这里，它非常小，只需30 微秒。

因此，根据上述公式，Cheetah的 $T_{I/O}$大概等于 6 毫秒。要计算 I/O 速率，我们只需用传输大小除以平均时间，即可得出 Cheetah 在随机工作负载下的 RI/O 速率约为 0.66 MB/s。对 Barracuda 进行同样的计算后，$T_{I/O}$ 约为 13.2 毫秒，速度慢了一倍多，因此传输速率约为 0.31 MB/s。

现在我们来看看顺序工作负载。在这里，我们可以假设在进行一次很长的传输之前，有一次寻道和旋转。为简单起见，假设传输大小为 100 MB。因此，Cheetah 和 Barracuda 的 $T_{I/O}$分别约为 800 毫秒和 950 毫秒。因此，I/O 速率非常接近 125 MB/s 和 105 MB/s 的峰值传输速率。下图总结了这些计算。

![image-20240414134514487](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Disk_Drive_Performance_Example2.png)

该图向我们展示了一些重要信息。首先，也是最重要的一点，随机和顺序工作负载之间的硬盘性能差距很大，Cheetah 几乎相差 200 倍左右，而 Barracuda 则相差 300 倍以上。这就是计算史上最明显的设计提示。

第二点更为微妙：高端 "性能 "硬盘与低端 "容量 "硬盘之间的性能差异很大。出于这个原因（还有其他原因），人们往往愿意花高价购买前者，而尽可能便宜地购买后者。

## 5 磁盘调度

由于 I/O 的成本很高，操作系统在决定向磁盘发出 I/O 的顺序方面一直扮演着重要角色。更具体地说，给定一组 I/O 请求后，**磁盘调度程序**会检查这些请求，并决定下一步调度哪个请求。在作业调度中，每个作业的长度通常是未知的，而磁盘调度则不同，我们可以很好地猜测一个 "作业"（即磁盘请求）需要多长时间。通过估算请求的寻道时间和可能的旋转延迟，磁盘调度程序可以知道每个请求需要多长时间，从而（贪婪地）挑选出服务时间最短的请求。因此，磁盘调度程序在运行时会尽量遵循 SJF（最短作业优先）原则。

### 5.1 SSTF：最短寻道时间优先

一种早期的磁盘调度方法称为<font color="red">最短寻道时间优先 (SSTF)</font>（也称为最短寻道优先或 SSF）。 SSTF 按磁道对 I/O 请求队列进行排序，选择最近磁道上的请求首先完成。例如，假设磁头当前位置在内磁道上方，并且我们有对扇区21（中磁道）和2（外磁道）的请求，那么我们首先向21发出请求，等待其完成，然后向 2 发出请求，如下图所示。

![image-20240414134912561](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/SSTF_Scheduling_Request_21_And_2.png)

 SSTF 在此示例中效果很好，首先搜索中磁道，然后搜索外磁道。然而，SSTF 并不是万能的，原因如下。首先，驱动器几何结构对于主机操作系统不可用；相反，它看到的是一个块数组。幸运的是，这个问题很容易解决。操作系统可以简单地实现<font color="red">最近块优先（NBF）</font>，而不是 SSTF，它接下来使用最近的块地址来调度请求。

第二个问题更为根本：**饥饿**。想象一下，在上面的示例中，如果磁头当前所在的内部轨道有稳定的请求流。然后，纯 SSTF 方法将完全忽略对任何其他轨道的请求。因此问题的关键是：如何处理磁盘饥饿？我们如何实现类似 SSTF 的调度但避免饥饿？

### 5.2 电梯调度（又称 SCAN 或 C-SCAN）

这个问题的答案早在几年前就已提出，而且相对简单明了。该算法最初被称为 **SCAN**，它只是在磁盘上来回移动，按顺序在磁道上处理请求。我们把在磁盘上的单次移动（从外磁道到内磁道，或从内磁道到外磁道）称为一次扫描。因此，如果磁道上的某个块的请求已经在这次磁盘扫描中得到过处理，那么该请求不会立即得到处理，而是会排队等待下一次扫描（另一个方向）。

SCAN 有许多变体，其作用都差不多。例如，Coffman 等人提出了 F-SCAN，在进行扫描时冻结待处理队列；这一操作将扫描过程中收到的请求放入队列，稍后再处理。这样做可以通过延迟服务晚到（但距离较近）的请求来避免远端请求的饥饿。

<font color="red">C-SCAN</font> 是另一种常见的变体，是 Circular SCAN 的缩写。该算法不是双向扫描磁盘，而是只从外向内扫描，然后在外层磁道重置，重新开始。这样做对内磁道和外磁道都比较公平，因为纯粹的来回 SCAN 会偏向于中间磁道，也就是说，在扫描完外磁道后，SCAN 会经过中间磁道两次，然后再回到外磁道。

现在应该很清楚SCAN 算法（及其同类算法）有时被称为**电梯算法**的原因，因为它的行为就像一部电梯，要么上行，要么下行，而不仅仅是根据哪个楼层更近来处理对哪个楼层的请求。试想一下，如果你从 10 楼下到 1 楼，有人在 3 楼上了电梯并按了 4 楼，而电梯却因为 4 楼比 1 楼 "近 "而上了 4 楼，那该有多烦人！正如你所看到的，电梯算法在现实生活中的使用，可以防止在电梯上发生打斗。在磁盘中，它只是防止饥饿。

遗憾的是，SCAN 及其类似技术并不代表最好的调度技术。特别是，SCAN（甚至是 SSTF）实际上并没有尽可能地遵循 SJF 原则。尤其是，它们忽略了旋转。因此，这也是另一个关键症结所在：我们如何实现一个算法，更加接近SJF，并考虑寻道和旋转？

### 5.3 SPTF： 最短定位时间优先

在讨论**最短定位时间优先或 SPTF 调度**（有时也称为最短访问时间优先或 SATF）（这是我们问题的解决方案）之前，让我们确保更详细地理解这个问题。下图展示了一个例子。

![image-20240414140304936](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/SSTF_Sometimes_Not_Good_Enough.png)

在该示例中，磁头当前位于内轨道的第 30 个扇区上方。因此，调度程序必须决定：下一个请求应该调度 16 号扇区（位于中间磁道）还是 8 号扇区（位于外磁道）。那么，下一次应该为哪个扇区提供服务呢？

答案当然是 "视情况而定"。这里所说的取决于寻道时间与旋转时间的相对比。在我们的例子中，如果寻道时间远高于旋转延迟，那么 SSTF（及其变体）就没有问题。但是，设想一下：

* 如果寻道时间比旋转时间快很多。那么，在我们的示例中，在外磁道上进一步寻道以服务请求 8 将比在中间磁道上执行较短的寻道以服务请求 16 更有意义，因为请求 16 在经过磁盘头之前必须旋转一圈。
* 如果旋转时间比寻道时间快很多。那么，服务请求16则比服务请求8更好。

如上所述，在现代硬盘上，寻道和旋转大致相同（当然，这取决于具体的请求），因此 SPTF 非常有用并能提高性能。然而，在操作系统中实现 SPTF 就更加困难了，因为操作系统通常并不清楚磁道边界的位置或磁头当前的位置（旋转意义上）。因此，SPTF 通常在硬盘内部执行。

### 5.4 其他调度问题

在简要介绍基本磁盘操作、调度和相关主题时，我们还没有讨论其他许多问题。其中一个问题是：在现代系统中，磁盘调度是在哪里执行的？在旧系统中，所有的调度工作都是由操作系统完成的；在查看了一系列待处理请求后，操作系统会挑选出最佳请求并将其发送给磁盘。该请求完成后，再选择下一个请求，以此类推。那时的磁盘比较简单。

在现代系统中，磁盘可以容纳多个未处理请求，而且本身就有复杂的内部调度器（可以准确执行 **SPTF**；在磁盘控制器内部，所有相关细节都是可用的，包括磁头的准确位置）。因此，操作系统调度程序通常会选择它认为最好的几个请求（比如 16 个），并将它们全部发送给磁盘；然后，磁盘会利用其内部的磁头位置知识和详细的磁道布局信息，以最佳的（SPTF）顺序为上述请求提供服务。

磁盘调度程序执行的另一项重要相关任务是 <font color="red">I/O 合并</font>。例如，假设有一系列读取区块 33、8、34 的请求，如上图所示。在这种情况下，调度程序应将对第 33 和第 34 块的请求合并为一个单一的双块请求；调度程序所做的任何重新排序都是根据合并后的请求执行的。合并在操作系统层面尤为重要，因为它可以减少发送到磁盘的请求数量，从而降低开销。

现代调度程序要解决的最后一个问题是：在向磁盘发出 I/O 之前，系统应该等待多长时间？人们可能会天真地认为，磁盘一旦有了哪怕一个 I/O，就应该立即向驱动器发出请求；这种方法被称为<font color="red">工作保护</font>，因为如果有请求需要服务，磁盘就永远不会闲置。然而，对预期磁盘调度的研究表明，有时等待一下会更好，这就是所谓的非工作保护方法。通过等待，新的、"更好的 "请求可能会到达磁盘，从而提高整体效率。当然，决定何时等待、等待多长时间可能很棘手。
# Program Explanation
`disk.py` 来让您熟悉现代硬盘驱动器的工作原理。它有很多不同的选项，并且与大多数其他模拟不同，它有一个图形动画器可以准确地向您展示磁盘运行时会发生什么。

注意：还有一个实验程序 `disk-precise.py` 。此版本的模拟器使用 python Decimal 包进行精确的浮点计算，因此在某些极端情况下给出的答案比 `disk.py` 稍好。然而，它还没有经过非常仔细的测试，所以请谨慎使用。

我们先做一个简单的例子。要运行模拟器并计算一些基本的搜索、旋转和传输时间，您首先必须向模拟器提供请求列表。这可以通过指定确切的请求来完成，也可以通过让模拟器随机生成一些请求来完成。

我们将从自己指定请求列表开始。让我们先做一个请求：

```shell
❯ python disk.py -a 10
OPTIONS seed 0
OPTIONS addr 10
OPTIONS addrDesc 5,-1,0
OPTIONS seekSpeed 1
OPTIONS rotateSpeed 1
OPTIONS skew 0
OPTIONS window -1
OPTIONS policy FIFO
OPTIONS compute False
OPTIONS graphics False
OPTIONS zoning 30,30,30
OPTIONS lateAddr -1
OPTIONS lateAddrDesc 0,-1,0

z 0 30
z 1 30
z 2 30
0 30 0
0 30 1
0 30 2
0 30 3
0 30 4
0 30 5
0 30 6
0 30 7
0 30 8
0 30 9
0 30 10
0 30 11
1 0 30 12
1 0 30 13
1 0 30 14
1 0 30 15
1 0 30 16
1 0 30 17
1 0 30 18
1 0 30 19
1 0 30 20
1 0 30 21
1 0 30 22
1 0 30 23
2 0 30 24
2 0 30 25
2 0 30 26
2 0 30 27
2 0 30 28
2 0 30 29
2 0 30 30
2 0 30 31
2 0 30 32
2 0 30 33
2 0 30 34
2 0 30 35
REQUESTS ['10']


For the requests above, compute the seek, rotate, and transfer times.
Use -c or the graphical mode (-G) to see the answers.
```

为了能够计算此请求的寻道、旋转和传输时间，您必须了解有关扇区布局、磁盘头起始位置等的更多信息。要查看大部分信息，请在图形模式下运行模拟器 (-G)：

此时，会出现一个窗口，如下图所示。

<img src="https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Disk_Graph_Seek_Example.png" alt="image-20240414184331768" style="zoom:50%;" />

上面有我们的简单磁盘。磁盘磁头位于外侧磁道上，位于扇区 6 的中间。如您所见，扇区 10（我们的示例扇区）位于同一磁道上，大约是绕道的三分之一。旋转方向为逆时针方向。要运行模拟，请在模拟器窗口突出显示时按“s”键。

模拟完成后，您应该能够看到磁盘花费了 105 个时间单位进行旋转，并花费了 30 个时间单位进行传输，以便访问扇区 10，并且没有寻道时间。按“q”关闭模拟器窗口。

要计算此值（而不仅仅是运行模拟），您需要了解有关磁盘的一些详细信息。首先，旋转速度默认设置为每时间单位 1 度。因此，完成一圈需要 360 个时间单位。其次，传输开始和结束于扇区之间的中点，因此，要读取扇区 10，传输在 9 和 10 之间开始，在 10 和 11 之间结束。最后，在默认磁盘中，每个磁道有 12 个扇区，这意味着每个扇区占用 30 度的旋转空间。因此，读取一个扇区需要 30 个时间单位（给定我们默认的旋转速度）。

有了这些信息，您现在应该能够计算访问扇区 10 的寻道、旋转和传输时间。因为磁头与 10 位于同一磁道上，所以没有寻道时间。因为磁盘以 1 度/时间单位旋转，所以需要 105 个时间单位才能到达扇区 10 的开头，即 9 和 10 之间的中间位置（注意，正好是 90 度到扇区 9 的中间，另外 15 度到扇区 9 的中间。最后，传输扇区需要30个时间单位。

现在我们来做一个稍微复杂一点的例子：

```shell
❯ python disk.py -a 10,11 -G
```

正如您可能猜到的，该模拟只需要多花 30 个时间单位来传输下一个扇区 11。因此，寻道和旋转时间保持不变，但请求的传输时间加倍。事实上，您可以在模拟器窗口的顶部看到这些总和；它们还会打印到控制台，如下所示：

```shell
REQUESTS ['10', '11']

Block:  10  Seek:  0  Rotate:105  Transfer: 30  Total: 135
Block:  11  Seek:  0  Rotate:  0  Transfer: 30  Total:  30

TOTALS      Seek:  0  Rotate:105  Transfer: 60  Total: 165
```

现在让我们用一个搜索来做一个例子。尝试以下一组请求：

```c
❯ python disk.py -a 10,18 -G
```

要计算这将花费多长时间，您需要知道一次查找将花费多长时间。每个磁道之间的距离默认为 40 个距离单位，默认查找速率为每单位时间 1 个距离单位。因此，从外磁道到中磁道的查找需要40个时间单位。

您还必须了解调度策略。不过，默认值为 FIFO，因此现在您可以假设处理顺序与通过 `-a` 标志指定的列表匹配来计算请求时间。

为了计算磁盘服务这些请求需要多长时间，我们首先计算访问扇区 10 需要多长时间，从上面我们知道这是 135 个时间单位（105 个旋转，30 个传输）。一旦该请求完成，磁盘开始寻找扇区18所在的中间磁道，需要40个时间单位。然后磁盘旋转到第18扇区，并转了30个时间单位，从而完成模拟。但最后一次轮换需要多长时间？

要计算 18 的旋转延迟，首先计算磁盘从访问扇区 10 结束到访问扇区 18 开始需要多长时间（假设零成本寻道）。从模拟器中可以看到，外侧磁道上的扇区 10 与中间磁道上的扇区 22 对齐，并且 22 和 18 之间有 7 个扇区（23、12、13、14、15、16 和 17，当磁盘逆时针旋转时）。旋转 7 个扇区需要 210 个时间单位（每个扇区 30 个时间单位）。然而，该轮换的第一部分实际上用于寻找中间磁道，持续 40 个时间单位。因此，访问扇区18的实际旋转延迟是210减40，即170个时间单位。运行模拟器来亲自看看；请注意，您可以在没有图形的情况下运行，并使用“-c”标志来仅查看结果而不查看图形。

```c
❯ python disk.py -a 10,18 -c
    REQUESTS ['10', '18']

Block:  10  Seek:  0  Rotate:105  Transfer: 30  Total: 135
Block:  18  Seek: 40  Rotate:170  Transfer: 30  Total: 240

TOTALS      Seek: 40  Rotate:275  Transfer: 60  Total: 375
```

您现在应该对模拟器的工作原理有基本的了解。下面是一些帮助选项：

```shell
❯ python disk.py -h
Usage: disk.py [options]

Options:
  -h, --help            show this help message and exit
  -s SEED, --seed=SEED  Random seed
  -a ADDR, --addr=ADDR  Request list (comma-separated) [-1 -> use addrDesc]
  -A ADDRDESC, --addrDesc=ADDRDESC
                        Num requests, max request (-1->all), min request
  -S SEEKSPEED, --seekSpeed=SEEKSPEED
                        Speed of seek
  -R ROTATESPEED, --rotSpeed=ROTATESPEED
                        Speed of rotation
  -p POLICY, --policy=POLICY
                        Scheduling policy (FIFO, SSTF, SATF, BSATF)
  -w WINDOW, --schedWindow=WINDOW
                        Size of scheduling window (-1 -> all)
  -o SKEW, --skewOffset=SKEW
                        Amount of skew (in blocks)
  -z ZONING, --zoning=ZONING
                        Angles between blocks on outer,middle,inner tracks
  -G, --graphics        Turn on graphics
  -l LATEADDR, --lateAddr=LATEADDR
                        Late: request list (comma-separated) [-1 -> random]
  -L LATEADDRDESC, --lateAddrDesc=LATEADDRDESC
                        Num requests, max request (-1->all), min request
  -c, --compute         Compute the answers
```
# Homework
1. 计算以下请求组的查找、旋转和传输时间： `-a 0,-a 6,-a 30,-a 7,30,8` ，最后是 `-a 10,11,12,13`

	```shell
	❯ python disk.py -a 0 -G
	❯ python disk.py -a 6 -G
	❯ python disk.py -a 30 -G
	❯ python disk.py -a 7,30,8 -G
	❯ python disk.py -a 10,11,12,13 -G
	```

2. 执行上面相同的请求，但将查找速率更改为不同的值： `-S 2,-S 4,-S 8,-S 10,-S 40,-S 0.1` 。时间如何变化？

	```shell
	❯ python disk.py -a 7,30,8 -G -S 2
	❯ python disk.py -a 7,30,8 -G -S 4
	```

	寻道时间较短。默认值为 1。

3. 执行与上述相同的请求，但更改旋转速率： `-R 0.1,-R 0.5,-R 0.01` 。时间如何变化？

	默认值为1。旋转时间和传输时间较长。

4. FIFO 并不总是最好的，例如，对于请求流 `-a 7,30,8` ，请求应该按什么顺序处理？在此工作负载上运行最短寻道时间优先 (SSTF) 调度程序 ( `-p SSTF` )；服务每个请求需要多长时间（寻道、旋转、传输）？

	```shell
	❯ python disk.py -a 7,30,8 -G -p SSTF
	```

	FIFO顺序：7、30、8；SSTF：7、8、30

5. 现在使用最短访问时间优先（SATF）调度程序（ `-p SATF` ）。它对 `-a 7,30,8` 工作负载有什么影响吗？找到一组 SATF 优于 SSTF 的请求；更一般地说，SATF 什么时候比 SSTF 更好？

	没有影响。

	```shell
	❯ python disk.py -a 31,6 -c -p SATF -S 40 -R 1
	❯ python disk.py -a 31,6 -c -p SSTF -S 40 -R 1
	```

	当寻道时间短于旋转时间。

6. 这是一个可以尝试的请求流： `-a 10,11,12,13` 。运行时出现什么问题？尝试添加磁道倾斜来解决此问题 ( `-o skew` )。给定默认寻道率，倾斜应该是多少才能最大限度地提高性能？对于不同的查找率（例如 `-S 2, -S 4` ）怎么样？一般来说，你能写一个公式来计算倾斜吗？

	```c
	❯ python disk.py -a 10,11,12,13 -c
	❯ python disk.py -a 10,11,12,13 -c -o 2
	```

	$$
	倾斜=\frac{轨道距离}{寻道速度}\times \frac{旋转速度}{旋转空间度}
	$$

	`-S 2` ： $倾斜=\frac{40}{2}\times \frac{1}{30}=\frac{2}{3}$

	`-S 4` ： $倾斜=\frac{40}{4}\times \frac{1}{30}=\frac{1}{3}$

7. 指定每个区域具有不同密度的磁盘，例如， `-z 10,20,30` ，它指定外、中和内轨道上块之间的角度差。运行一些随机请求（例如， `-a -1 -A 5,-1,0` ，它指定应通过 `-a -1` 标志使用随机请求，并生成从 0 到最大值的 5 个请求），并计算寻道、旋转传输时间。使用不同的随机种子。外磁道、中磁道和内磁道的带宽（单位时间内的扇区数）是多少？

	```shell
	❯ python disk.py -z 10,20,30 -a -1 -A 5,-1,0 -c
	outer: 3/(135+270+140)=0.0055
	middle: 2/(370+260)=0.0032
	
	❯ python disk.py -z 10,20,30 -a -1 -A 5,-1,0 -s 1 -c
	outer: 3/(255+385+130)=0.0039
	middle: 2/(115+280)=0.0051
	
	❯ python disk.py -z 10,20,30 -a -1 -A 5,-1,0 -s 2 -c
	outer: 2/(85+10)=0.0211
	middle: 3/(130+360+145)=0.0047
	
	❯ python disk.py -z 10,20,30 -a -1 -A 5,-1,0 -s 3 -c
	outer: 5/875=0.0057
	```

8. 调度窗口确定磁盘一次可以检查多少个请求。生成随机工作负载（例如， `-A 1000,-1,0` 具有不同的种子），并查看当调度窗口从 1 更改为请求数时，SATF 调度程序需要多长时间。需要多大的窗口才能最大限度地提高性能？提示：使用 `-c` 标志，不要打开图形 （ `-G` ） 来快速运行这些图形。当计划时段设置为 1 时，使用哪种策略是否重要？

	设置为 1 等于 FIFO。最大化磁盘的性能需求大小 （`-w -1` ）。

	```shel
	❯ python disk.py -A 1000,-1,0 -p SATF -w 1 -c      // 220125
	❯ python disk.py -A 1000,-1,0 -p FIFO -w 1 -c      // 220125
	❯ python disk.py -A 1000,-1,0 -p SSTF -w 1 -c      // 220125
	❯ python disk.py -A 1000,-1,0 -p BSATF -w 1 -c     // 220125
	❯ python disk.py -A 1000,-1,0 -p SATF -w 1000 -c   // 35475
	```

9. 创建一系列请求以匮乏特定请求，假设 SATF 策略。给定该序列，如果使用有界 SATF （BSATF） 调度方法，它的性能如何？在此方法中，您可以指定调度窗口（例如， `-w 4` ）;仅当当前窗口中的所有请求都已处理完毕时，调度程序才会移动到下一个请求窗口。这能解决饥饿问题吗？与 SATF 相比，它的表现如何？一般而言，磁盘应如何在性能和避免匮乏之间做出权衡？

	```shell
	❯ python disk.py -a 12,7,8,9,10,11 -p SATF -c          // 7,8,9,10,11,12 Total: 555
	❯ python disk.py -a 12,7,8,9,10,11 -p BSATF -w 4 -c    // 7,8,9,12,10,11 Total: 525
	```

10. 到目前为止，我们看到的所有调度策略都是贪婪的;他们选择下一个最佳选项，而不是寻找最佳时间表。你能找到一组贪婪不是最优的请求吗？

	```shell
	❯ python disk.py -a 9,20 -c            // 435
	❯ python disk.py -a 9,20 -c -p SATF    // 465
	```

	

