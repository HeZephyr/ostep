# Note/Translation
有时有人说，操作系统在解决大多数空间管理问题时会采用两种方法之一。第一种方法是将事物切成<font color="red">可变大小的块</font>，正如我们在虚拟内存中的分段中看到的那样。不幸的是，这个解决方案有其固有的困难。特别是，当将空间划分为不同大小的块时，空间本身可能会变得碎片化，因此随着时间的推移，分配变得更具挑战性。

因此，可能值得考虑第二种方法：将空间切成固定大小的块。在虚拟内存中，我们将这种想法称为<font color="red">分页</font>，它可以追溯到一个早期且重要的系统，Atlas。我们不是将进程的地址空间划分为一定数量的可变大小的逻辑段（例如代码、堆、栈），而是将其划分为固定大小的单元，每个单元称为<font color="red">页面</font>。相应地，我们将物理内存视为一组固定大小的<font color="red">槽（称为页框）</font>；每个框都可以包含一个虚拟内存页；每个进程都需要<font color="red">页表</font>来将虚拟地址转换为物理地址。。我们的挑战：

> <center>关键：如何使用页面虚拟化内存 ?</center>
>
> 如何使用页面虚拟化内存，从而避免分段问题？有哪些基本技术？我们如何以最小的空间和时间开销使这些技术发挥良好作用？

## 1 示例：一个简单的分页

为了更清楚地说明这种方法，让我们来看一个简单的例子。下图展示了一个很小的地址空间，总大小只有 64 字节，有四个 16 字节的页面（虚拟页面 0、1、2 和 3）。

![image-20240401184253614](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/A-Simple-Page-Example-1.png)

当然，真实的地址空间要大得多，通常为 32 位，因此有 4GB 的地址空间，甚至 64 位。

如下图所示，物理内存也由许多固定大小的槽组成，在本例中是 8 个页框（对于 128 字节的物理内存来说，也小得离谱）。虚拟地址空间的页面被放置在整个物理内存的不同位置；该图还显示操作系统自身使用一些物理内存。

![image-20240401184650191](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/A-Simple-Page-Example-2.png)

正如我们将看到的，分页比我们以前的方法有许多优点。最重要的改进可能是灵活性：通过完全开发的分页方法，系统将能够有效地支持地址空间的抽象，无论进程如何使用地址空间；例如，我们不会对堆和栈的增长方向以及它们的使用方式做出假设。

另一个优点是分页提供的可用空间管理的简单性。例如，当操作系统希望将我们微小的 64 字节地址空间放入我们的八页物理内存中时，它只会找到四个空闲页；也许操作系统为此保留了所有空闲页面的空闲列表，并且只从该列表中获取前四个空闲页面。在示例中，操作系统已将地址空间 (AS) 的虚拟页 0 放置在物理页 3 中，将 AS 的虚拟页 1 放置在物理页7 中，将页 2 放置在物理页 5 中，将页 3 放置在物理页 2 中。 页框 1 、4 和 6 目前空闲。

为了记录地址空间的每个虚拟页在物理内存中的位置，操作系统通常会为每个进程保存一个名为<font color="red">页表</font>的数据结构。页表的主要作用是存储地址空间中每个虚拟页的地址转换，从而让我们知道每个页位于物理内存中的位置。对于我们的简单示例，页表将具有以下四个条目：（虚拟页 0 → 物理页3）、（VP 1 → PF 7）、（VP 2 → PF 5）和（VP 3 → PF 2）。

重要的是要记住，这个页表是一个每个进程的数据结构（我们讨论的大多数页表结构都是每个进程的结构；我们将提到一个例外，即<font color="red">反向页表</font>）。如果在上面的示例中运行另一个进程，则操作系统必须为其管理不同的页表，因为它的虚拟页面显然映射到不同的物理页面（除非有任何共享正在进行）。

现在，我们知道足够多以执行地址转换示例。让我们想象一下具有微小地址空间（64字节）的进程正在执行内存访问：

```asm
movl <virtual address>, %eax
```

具体来说，让我们注意将数据从地址` <virtual address> `显式加载到寄存器 eax 中（从而忽略之前必须发生的指令获取）。

为了转换进程生成的虚拟地址，我们必须首先将其分为两个部分：<font color="red">虚拟页号（Vitrual page number,VPN）和页面内的偏移量</font>。对于此示例，由于进程的虚拟地址空间为 64 字节，因此我们的虚拟地址总共需要 6 位 ($2^6 = 64$)。因此，我们的虚拟地址可以概念化如下：

![image-20240401192124164](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/virtual-address-conceptualized.png)

其中，Va5是虚拟地址的最高位，Va0是最低位。因为我们知道页面大小（16字节），则前两位代表虚拟页号，后四位代表页面偏移量。在 64 字节地址空间中，页面大小为 16 字节；因此我们需要能够选择 4 个页面，地址的前 2 位就可以做到这一点。因此，我们有一个 2 位虚拟页码 (VPN)。剩余的位则表示页面偏移量。

当进程生成虚拟地址时，操作系统和硬件必须结合起来将其转换为有意义的物理地址。例如，假设上述加载到虚拟地址 21：

```asm
movl 21, %eax
```

将“21”转换为二进制形式，我们得到“010101”，因此我们可以检查这个虚拟地址并查看它如何分解为虚拟页号（VPN）和偏移量：

![image-20240401192754374](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/virtual-address-example-5.png)

因此，虚拟地址“21”位于虚拟页“01”（或1）的第5（“0101”）字节。有了虚拟页号，我们现在可以索引页表并找到虚拟页 1 所在的物理页。在上面的页表中，物理页号 (Physical page number, PPN)（有时也称为物理帧号或 PFN）为 7（二进制 111）。因此，我们可以通过用 PPN 替换 VPN 来转换这个虚拟地址，然后向物理内存加载数据，如下图所示。

![image-20240401193152226](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/virtual-address-translation-example-1.png)

请注意，偏移量保持不变（即，它没有被转换），因为偏移量只是告诉我们我们想要页面中的哪个字节。我们的最终物理地址是 1110101（十进制为 117），这正是我们希望加载获取数据的位置。

## 2 页表存储在哪？

页表可以变得非常大，比我们之前讨论的小分段表或基址/边界对大得多。例如，想象一个典型的 32 位地址空间，具有 4KB 页面。该虚拟地址分为 20 位 VPN 和 12 位偏移量（回想一下，1KB 页面大小需要 10 位，只需再添加两位即可达到 4KB）。 

20 位 VPN 意味着操作系统必须为每个进程管理 $2^{20}$ 个转换（大约一百万个）；假设每个页表项 (page table entry, PTE) 需要 4 个字节来保存物理转换以及任何其他有用的内容，那么每个页表需要 4MB 的巨大内存！那是相当大的。现在假设有 100 个进程正在运行：这意味着操作系统需要 400MB 内存来用于所有这些地址转换！即使在机器拥有千兆字节内存的现代，将大量内存用于地址转换似乎也有点疯狂，不是吗？我们甚至没有考虑对于 64 位地址空间来说这样的页表有多大；那太可怕了，也许会把你完全吓跑。

由于页表太大，我们没有在MMU中保留任何特殊的片上硬件来存储当前运行进程的页表。相反，我们将每个进程的页表存储在内存中的某个位置。现在我们假设页表位于操作系统管理的物理内存中，下图是操作系统内存中页表的图片。

![image-20240401200708977](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Example-Page-Table.png)

## 3 页表中到底有什么？

让我们来谈谈页表的组织结构。页表只是一种数据结构，用于将虚拟地址（或实际上是虚拟页码）映射到物理地址（物理页号）。因此，任何数据结构都可以使用。最简单的形式称为<font color="red">线性页表</font>，它只是一个数组。操作系统根据虚拟页码（VPN）对数组进行索引，并查找该索引下的页表项（PTE），以找到所需的物理页号（PFN）。目前，我们将假设这种简单的线性结构；在后面的章节中，我们将使用更高级的数据结构来帮助解决分页中的一些问题。

至于每个 PTE 的内容，我们有许多不同的bit位值得在一定程度上了解。**有效位**通常用于指示特定转换是否有效。例如，当程序开始运行时，其地址空间的一端是代码和堆，另一端是栈。中间所有未使用的空间都会被标记为无效，如果进程试图访问这些内存，就会向操作系统发出中断，操作系统很可能会终止进程。因此，有效位对于支持稀疏地址空间至关重要；只需将地址空间中所有未使用的页面标记为无效，我们就无需为这些页面分配物理页号，从而节省了大量内存。

我们还可以使用**保护位**来表明是否可以读取、写入或执行页面。同样，如果以这些位不允许的方式访问页面，就会向操作系统发出中断。

还有一些其他的位也很重要，但我们现在就不多说了。**存在位**表示该页面是在物理内存中还是在磁盘上（即已被换出）。当我们研究如何将部分地址空间交换到磁盘以支持比物理内存更大的地址空间时，我们将进一步了解这一机制；交换允许操作系统通过将很少使用的页面移动到磁盘来释放物理内存。**脏位（dirty bit）**也很常见，表示页面进入内存后是否被修改过。

**参考位（又称访问位）**有时用于跟踪页面是否被访问过，它有助于确定哪些页面受欢迎，因此应保留在内存中；在**页面替换**过程中，这种知识至关重要。

下图显示了 x86 架构中的一个页表条目示例。它包含一个存在位 (P)；一个读/写位 (R/W)，用于确定是否允许写入该页面；一个用户/监管者位 (U/S)，用于确定用户模式进程是否可以访问该页面；几个位（PWT、PCD、PAT 和 G），用于确定这些页面的硬件缓存工作方式；一个已访问位 (A) 和一个脏位 (D)；最后是物理页号（PFN）本身。

![image-20240401201712524](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/An-x86-Page-Table-Entry.png)

## 4 分页：也太慢

对于内存中的页表，我们已经知道它们可能过大。事实证明，它们也会拖慢运行速度。举个简单的例子：

```asm
movl 21, %eax
```

同样，我们只检查对地址 21 的显式引用，而不用担心取指令。在这个例子中，我们假设硬件为我们执行地址转换。为了获取所需的数据，系统必须首先将虚拟地址（21）转换为正确的物理地址（117）。因此，在从地址 117 获取数据之前，系统必须首先从进程的页表中获取正确的页表条目，执行转换，然后从物理内存加载数据。

为此，硬件必须知道当前运行进程的页表在哪里。现在我们假设单个页表基址寄存器包含页表起始位置的物理地址。为了找到所需 PTE 的位置，硬件将执行以下功能：

```c
VPN = (VirtualAddress & VPN_MASK) >> SHIFT
PTEAddr = PageTableBaseRegister + (VPN * sizeof(PTE))
```

在我们的示例中，`VPN_MASK` 将设置为 0x30（十六进制 30，或二进制 110000），它从完整虚拟地址中挑选出 VPN 位； `SHIFT` 设置为 4（偏移量中的位数），以便我们将 VPN 位向下移动以形成正确的整数虚拟页号。例如，对于虚拟地址21（010101），掩码将该值变成010000；根据需要，移位会将其变为 01 或虚拟页 1。然后，我们使用该值作为页表基址寄存器指向的 PTE 数组的索引。

一旦知道这个物理地址，硬件就可以从内存中获取 PTE，提取 PFN，并将其与虚拟地址的偏移量连接起来，形成所需的物理地址。具体来说，可以认为 PFN 通过 SHIFT 左移，然后与偏移量按位或运算形成最终地址，如下所示：

```c
offset = VirtualAddress & OFFSET_MASK
PhysAddr = (PFN << SHIFT) | offset
```

最后，硬件可以从内存中取出所需的数据并将其放入寄存器eax中。程序现在已成功从内存加载一个值！

总而言之，我们现在描述每个内存引用上发生的情况的初始协议。下面这段代码显示了基本方法。

```python
# Extract the VPN from the virtual address
VPN = (VirtualAddress & VPN_MASK) >> SHIFT

# Form the address of the page-table entry (PTE)
PTEAddr = PTBR + (VPN * sizeof(PTE))

# Fetch the PTE
PTE = AccessMemory(PTEAddr)

# Check if process can access the page
if (PTE.Valid == False)
    RaiseException(SEGMENTATION_FAULT)
else if (CanAccess(PTE.ProtectBits) == False)
    RaiseException(PROTECTION_FAULT)
else
    # Access is OK: form physical address and fetch it
    offset = VirtualAddress & OFFSET_MASK
    PhysAddr = (PTE.PFN << PFN_SHIFT) | offset
    Register = AccessMemory(PhysAddr)
```

对于每个内存引用（无论是指令获取还是显式加载或存储），分页要求我们执行一次额外的内存引用，以便首先从页表中获取。这是很多工作！额外的内存引用成本高昂，在这种情况下可能会使进程减慢两倍或更多。

现在您有望看到我们必须解决两个真正的问题。如果没有仔细设计硬件和软件，页表会导致系统运行速度过慢，并且占用过多的内存。虽然这似乎是满足我们内存虚拟化需求的一个很好的解决方案，但必须首先克服这两个关键问题。

## 5 内存跟踪

在结束之前，我们现在通过一个简单的内存访问示例来演示使用分页时发生的所有结果内存访问。我们感兴趣的代码片段（在 C 语言中，在名为 array.c 的文件中）如下：

```c
int array[1000];
...
for (i = 0; i < 1000; i++)
	array[i] = 0;
```

我们编译 array.c 并使用以下命令运行它：

```shell
gcc -o array array.c -Wall -O
./array
```

当然，为了真正理解内存访问此代码片段（它只是初始化一个数组）会产生什么，我们必须知道（或假设）更多的事情。首先，我们必须反汇编生成的二进制文件（在 Linux 上使用 objdump，在 Mac 上使用 otool）以查看使用哪些汇编指令来初始化循环中的数组。这是生成的汇编代码：

```asm
0x1024 movl $0x0,(%edi,%eax,4) #Power of CISC! edi+eax*4
0x1028 incl %eax #Increase counter
0x102c cmpl $0x03e8,%eax #Check if last element
0x1030 jne 0x1024 #Implicit (eflags) Zero bit access
```

如果您了解一点 x86，该代码实际上很容易理解。第一条指令将值零（显示为 $0x0）移动到数组位置的虚拟内存地址中；该地址是通过获取 %edi 的内容并添加 %eax 乘以四来计算的。因此，%edi 保存数组的基地址，而 %eax 保存数组索引 (i)；我们乘以四，因为该数组是整数数组，每个整数大小为四个字节。

第二条指令递增 %eax 中保存的数组索引，第三条指令将该寄存器的内容与十六进制值 0x03e8 或十进制 1000 进行比较。如果比较显示两个值还不相等（这就是 jne 指令测试的结果），第四条指令跳回循环顶部。

为了了解该指令序列访问哪些内存（在虚拟和物理级别），我们必须假设代码片段和数组在虚拟内存中的位置，以及页表的内容和位置。对于此示例，我们假设虚拟地址空间大小为 64KB（小得不切实际）。我们还假设页面大小为 1KB。

我们现在需要知道的是页表的内容及其在物理内存中的位置。假设我们有一个线性（基于数组）页表，并且它位于物理地址 1KB (1024)。

至于其内容，我们只需要担心在本示例中映射了几个虚拟页面。首先，代码所在的虚拟页面。由于页面大小为 1KB，因此虚拟地址 1024 驻留在虚拟地址空间的第二页上（VPN=1，因为 VPN=0 是第一页）。我们假设该虚拟页面映射到物理帧 4 (VPN 1 → PFN 4)。

接下来是数组本身。它的大小是 4000 字节（1000 个整数），我们假设它驻留在虚拟地址 40000 到 44000（不包括最后一个字节）。此十进制范围的虚拟页面为 VPN=39 ... VPN=42。因此，我们需要这些页面的映射。我们假设以下虚拟到物理映射为示例：(VPN 39 → PFN 7)、(VPN 40 → PFN 8)、(VPN 41 → PFN 9)、(VPN 42 → PFN 10)。

我们现在准备跟踪程序的内存引用。当它运行时，每个指令获取都会生成两个内存引用：一个到页表以查找指令所在的物理页，另一个到指令本身以将其获取到 CPU 进行处理。此外，还有一个以 mov 指令形式出现的显式内存引用；这首先增加了另一个页表访问（将数组虚拟地址转换为正确的物理地址），然后才是数组访问本身。

下图描述了前五个循环迭代的整个过程。最下面的图以黑色显示 y 轴上的指令内存引用（左边是虚拟地址，右边是实际物理地址）；中间的图以深灰色显示数组访问（同样是左边是虚拟地址，右边是物理地址）；最后，最上面的图以浅灰色显示页表内存访问（只是物理访问，因为本例中的页表位于物理内存中）。整个跟踪的 x 轴显示了循环前五次迭代的内存访问；每个循环有 10 次内存访问，其中包括四次指令取回、一次内存显式更新和五次页表访问，以转换这四次取回和一次显式更新。

![image-20240401224435986](https://raw.githubusercontent.com/unique-pure/NewPicGoLibrary/main/img/Memory-Trace-Example-1.png)

# Program Explanation
在本作业中，您将使用一个简单的程序（称为 paging-linear-translate.py）来查看您是否了解简单的虚拟到物理地址转换如何与线性页表一起工作。要运行该程序请输入以下命令：`python paging-linear-translate.py`。当您使用 -h（帮助）标志运行它时，您会看到：

```shell
❯ python paging-linear-translate.py -h
Usage: paging-linear-translate.py [options]

Options:
  -h, --help            show this help message and exit
  -A ADDRESSES, --addresses=ADDRESSES
                        a set of comma-separated pages to access; -1 means
                        randomly generate
  -s SEED, --seed=SEED  the random seed
  -a ASIZE, --asize=ASIZE
                        address space size (e.g., 16, 64k, 32m, 1g)
  -p PSIZE, --physmem=PSIZE
                        physical memory size (e.g., 16, 64k, 32m, 1g)
  -P PAGESIZE, --pagesize=PAGESIZE
                        page size (e.g., 4k, 8k, whatever)
  -n NUM, --numaddrs=NUM
                        number of virtual addresses to generate
  -u USED, --used=USED  percent of virtual address space that is used
  -v                    verbose mode
  -c                    compute answers for me
```

首先，不带任何参数运行程序：

```shell
❯ python paging-linear-translate.py
ARG seed 0
ARG address space size 16k
ARG phys mem size 64k
ARG page size 4k
ARG verbose False
ARG addresses -1


The format of the page table is simple:
The high-order (left-most) bit is the VALID bit.
  If the bit is 1, the rest of the entry is the PFN.
  If the bit is 0, the page is not valid.
Use verbose mode (-v) if you want to print the VPN # by
each entry of the page table.

Page Table (from entry 0 down to the max size)
  0x8000000c
  0x00000000
  0x00000000
  0x80000006

Virtual Address Trace
  VA 0x00003229 (decimal:    12841) --> PA or invalid address?
  VA 0x00001369 (decimal:     4969) --> PA or invalid address?
  VA 0x00001e80 (decimal:     7808) --> PA or invalid address?
  VA 0x00002556 (decimal:     9558) --> PA or invalid address?
  VA 0x00003a1e (decimal:    14878) --> PA or invalid address?

For each virtual address, write down the physical address it translates to
OR write down that it is an out-of-bounds address (e.g., segfault).
```

对于每个虚拟地址，记下它转换为的物理地址或记下它是越界地址（例如，分段错误）。正如你所看到的，程序为你提供的是一个特定进程的页表（记住，在一个具有线性页表的真实系统中，每个进程都有一个页表；这里我们只关注一个进程，它的地址空间，因此是一个页表）。页表告诉您，对于地址空间的每个虚拟页号 (VPN)，虚拟页映射到特定的物理帧号 (PFN)，因此有效或无效。

页表项的格式很简单：最左边（高位）位是有效位；其余位，如果有效为 1，则为 PFN。在上面的示例中，页表将 VPN 0 映射到 PFN 0xc（十进制 12），将 VPN 3 映射到 PFN 0x6（十进制 6），并将其他两个虚拟页面 1 和 2 保留为无效。

因为页表是一个线性数组，所以上面打印的内容是您自己查看这些位时在内存中看到的内容的复制品。但是，如果使用详细标志 (-v) 运行，有时会更容易使用此模拟器；该标志还将 VPN（索引）打印到页表中。在上面的示例中，使用 -v 标志运行：

```shell
Page Table (from entry 0 down to the max size)
  [       0]  0x8000000c
  [       1]  0x00000000
  [       2]  0x00000000
  [       3]  0x80000006
```

那么，您的工作就是使用此页表将跟踪中提供给您的虚拟地址转换为物理地址。让我们看第一个：VA 0x3229。要将虚拟地址转换为物理地址，我们首先必须将其分解为组成部分：虚拟页号和偏移量。我们通过记下地址空间的大小和页面大小来做到这一点。在本例中，地址空间设置为16KB（非常小的地址空间），页面大小为4KB。因此，我们知道虚拟地址有 14 位，偏移量为 12 位，为 VPN 留下 2 位。因此，对于我们的地址 0x3229（二进制 11 0010 0010 1001），我们知道前两位指定 VPN。因此，0x3229 位于虚拟页 3 上，偏移量为 0x229。

接下来我们查看页表，看看 VPN 3 是否有效并映射到某个物理帧或无效，我们看到它确实有效，并映射到物理页 6。因此，我们可以通过获取物理页 6 并将其添加到偏移量上来形成最终的物理地址，如下所示：0x6000（物理页，移位到正确的位置）或 0x0229（偏移量） ，产生最终的物理地址：0x6229。因此，我们可以看到在此示例中虚拟地址 0x3229 转换为物理地址 0x6229。

要查看其余的解决方案，只需使用 -c 标志运行：

```shell
Virtual Address Trace
  VA 0x00003229 (decimal:    12841) --> 00006229 (decimal    25129) [VPN 3]
  VA 0x00001369 (decimal:     4969) -->  Invalid (VPN 1 not valid)
  VA 0x00001e80 (decimal:     7808) -->  Invalid (VPN 1 not valid)
  VA 0x00002556 (decimal:     9558) -->  Invalid (VPN 2 not valid)
  VA 0x00003a1e (decimal:    14878) --> 00006a1e (decimal    27166) [VPN 3]
```

# Homework
1. 在进行任何转换之前，让我们使用模拟器来研究线性页表在给定不同参数的情况下如何改变大小。计算不同参数变化时线性页表的大小。下面是一些建议的输入；通过使用 -v 标志，您可以查看填充了多少页表条目。首先，了解线性页表大小如何随着地址空间的增长而变化：

	```shell
	python paging-linear-translate.py -P 1k -a 1m -p 512m -v -n 0
	python paging-linear-translate.py -P 1k -a 2m -p 512m -v -n 0
	python paging-linear-translate.py -P 1k -a 4m -p 512m -v -n 0
	```

	然后，了解线性页表大小如何随着页面大小的增长而变化：

	```shell
	python paging-linear-translate.py -P 1k -a 1m -p 512m -v -n 0
	python paging-linear-translate.py -P 2k -a 1m -p 512m -v -n 0
	python paging-linear-translate.py -P 4k -a 1m -p 512m -v -n 0
	```

	在运行任何这些之前，请尝试考虑预期的趋势。随着地址空间的增长，页表大小应该如何变化？随着页面大小的增长？为什么我们不应该一般使用非常大的页面？

	页表大小=地址空间大小/页大小

	使用非常大的页面会造成浪费。因为，大多数进程使用很少的内存。

2. 现在我们来做一些转换。从一些小例子开始，用 -u 标志改变有效映射的比例。例如

	```shell
	python paging-linear-translate.py -P 1k -a 16k -p 32k -v -u 0
	python paging-linear-translate.py -P 1k -a 16k -p 32k -v -u 25
	python paging-linear-translate.py -P 1k -a 16k -p 32k -v -u 50
	python paging-linear-translate.py -P 1k -a 16k -p 32k -v -u 75
	python paging-linear-translate.py -P 1k -a 16k -p 32k -v -u 100
	```

	增加每个地址空间分配的页面百分比会发生什么情况？

	会使更多页面有效。

3. 现在让我们尝试一些不同的随机种子和一些不同的（有时非常疯狂的）地址空间参数，以实现多样性：

	```shell
	python paging-linear-translate.py -P 8 -a 32 -p 1024 -v -s 1
	python paging-linear-translate.py -P 8k -a 32k -p 1m -v -s 2
	python paging-linear-translate.py -P 1m -a 256m -p 512m -v -s 3
	```

	这些参数组合中哪些是不现实的？为什么？

	第一个， 内存太小

4. 使用该程序尝试一些其他问题。你能找到程序不再工作的限制吗？例如，如果地址空间大小大于物理内存会发生什么？

	```shell
	python paging-linear-translate.py -a 65k -v -c
	ARG address space size 65k
	ARG phys mem size 64k
	Error: physical memory size must be GREATER than address space size (for this simulation)
	
	python paging-linear-translate.py -a 0 -v -c
	ARG address space size 0
	Error: must specify a non-zero address-space size.
	
	python paging-linear-translate.py -p 0 -v -c
	ARG phys mem size 0
	Error: must specify a non-zero physical memory size.
	
	python paging-linear-translate.py -P 0 -v -c
	Traceback (most recent call last):
	File "./paging-linear-translate.py", line 85, in <module>
	    mustbemultipleof(asize, pagesize, 'address space must be a multiple of the pagesize')
	File "./paging-linear-translate.py", line 14, in mustbemultipleof
	    if (int(float(bignum)/float(num)) != (int(bignum) / int(num))):
	ZeroDivisionError: float division by zero
	
	python paging-linear-translate.py -P 32k -v -c
	Error in argument: address space must be a multiple of the pagesize
	```

	