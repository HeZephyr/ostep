在本作业中，我们将使用信号量来解决一些众所周知的并发问题。其中许多内容取自Downey优秀的《The Little Book of Semaphores》，该书很好地汇集了一些经典问题，并引入了一些新的变体。

以下每个问题都提供了代码骨架；您的工作是填写代码以使其在给定信号量的情况下工作。在 Linux 上，您将使用本机信号量；在 Mac 上（没有信号量支持），您必须首先构建一个实现（使用锁和条件变量，如本章所述）。祝你好运！

1. 第一个问题只是实现和测试 `fork/join` 问题的解决方案，如文中所述。尽管文中描述了该解决方案，但您自己输入它的行为是值得的；甚至Bach也会重写Vivaldi，让一位即将成为大师的人向现有大师学习。有关详细信息，请参阅 `fork-join.c` 。将调用 `sleep(1)` 添加到子线程以确保其正常工作。

	是的，我先用锁和条件变量实现了信号量，然后在`fork-join.c`中使用了它，能正常工作。

	```c
	#include <stdio.h>
	#include <unistd.h>
	#include <pthread.h>
	
	#include "zemaphore.h"
	#include "common_threads.h"
	
	Zem_t s; 
	
	void *child(void *arg) {
	    sleep(1);
	    printf("child\n");
	    // use semaphore here
	    Zem_post(&s);
	    return NULL;
	}
	
	int main(int argc, char *argv[]) {
	    pthread_t p;
	    printf("parent: begin\n");
	    // init semaphore here
	    Zem_init(&s, 0);
	    Pthread_create(&p, NULL, child, NULL);
	    // use semaphore here
	    Zem_wait(&s);
	    printf("parent: end\n");
	    return 0;
	}
	```

2. 现在让我们通过研究会合问题来概括一下这一点。问题如下：你有两个线程，每个线程都即将进入代码中的会合点。双方都不应该在对方进入这部分代码之前退出。考虑使用两个信号量来完成此任务，有关详细信息，请参阅 `rendezvous.c` 。

	*The Little Book of Semaphore* chapter 3.3

	```c
	#include <stdio.h>
	#include <unistd.h>
	
	#include "common_threads.h"
	#include "zemaphore.h"
	
	// If done correctly, each child should print their "before" message
	// before either prints their "after" message. Test by adding sleep(1)
	// calls in various locations.
	
	Zem_t s1, s2;
	
	void *child_1(void *arg) {
	    sleep(1);
	    printf("child 1: before\n");
	    // what goes here?
	    Zem_post(&s2);
	    Zem_wait(&s1);
	    printf("child 1: after\n");
	    return NULL;
	}
	
	void *child_2(void *arg) {
	    printf("child 2: before\n");
	    // what goes here?
	    Zem_post(&s1);
	    Zem_wait(&s2);
	    printf("child 2: after\n");
	    return NULL;
	}
	
	int main(int argc, char *argv[]) {
	    pthread_t p1, p2;
	    printf("parent: begin\n");
	    // init semaphores here
	    Zem_init(&s1, 0);
	    Zem_init(&s2, 0);
	    Pthread_create(&p1, NULL, child_1, NULL);
	    Pthread_create(&p2, NULL, child_2, NULL);
	    Pthread_join(p1, NULL);
	    Pthread_join(p2, NULL);
	    printf("parent: end\n");
	    return 0;
	}
	```

	- **s1**：用于协调child_1和child_2线程的执行顺序。child_1在执行之前需要等待s1信号量，而child_2在执行之后会发送s1信号量，以通知child_1可以继续执行了。
	- **s2**：同样用于协调child_1和child_2线程的执行顺序。child_2在执行之前需要等待s2信号量，而child_1在执行之后会发送s2信号量，以通知child_2可以继续执行了。

3. 现在更进一步，实施屏障同步的通用解决方案。假设一段连续的代码中有两个点，称为 P 1 和 P 2 。在 P 1 和 P 2 之间放置屏障可保证所有线程将在任何一个线程执行 P 2 之前执行 P 1 。您的任务：编写代码来实现可以以此方式使用的 `barrier()` 函数。可以安全地假设您知道 N（正在运行的程序中的线程总数）并且所有 N 个线程都将尝试进入屏障。同样，您可能应该使用两个信号量来实现解决方案，并使用一些其他整数来计数。有关详细信息，请参阅 `barrier.c` 。

	*The Little Book of Semaphore* chapter 3.6

	首先用一个信号量实现互斥访问计数器，另一个信号量则用来实现屏障同步，具体做法是让每一个到达的线程都调用`Zem_wait`等待，然后让最后一个线程释放所有等待的线程。

	```c
	typedef struct __barrier_t {
	    // add semaphores and other information here
	    Zem_t mutex;
	    Zem_t barrier;
	    int num_threads;
	    int num_arrived;
	} barrier_t;
	
	
	// the single barrier we are using for this program
	barrier_t b;
	
	void barrier_init(barrier_t *b, int num_threads) {
	    // initialization code goes here
	    Zem_init(&b->mutex, 1);
	    Zem_init(&b->barrier, 0);
	    b->num_threads = num_threads;
	    b->num_arrived = 0;
	}
	
	void barrier(barrier_t *b) {
	    // barrier code goes here
	    Zem_wait(&b->mutex); // enter critical section
	    b->num_arrived++;
	    Zem_post(&b->mutex); // leave critical section
	
	    if (b->num_arrived == b->num_threads) {
	        for (int i = 0; i < b->num_threads; i++) {
	            Zem_post(&b->barrier);
	        }
	    }
	    Zem_wait(&b->barrier);
	}
	```

4. 现在，让我们来解决读者—写者问题，也如文中所述。在第一次尝试中，不用担心饥饿问题。详情请参阅 `reader-writer.c` 中的代码。在你的代码中添加 sleep() 调用，以证明它能按照你的预期运行。你能证明饥饿问题的存在吗？

	当其他线程（读者或写者）在临界区时，写者无法进入临界区，并且当写者在临界区时，没有其他线程可以进入。所以很容易导致饥饿问题，例如写者需要等待读者释放读锁后才能获取写锁，而读者可能会连续获取读锁，导致写者一直无法获取到写锁，造成写者线程饥饿。

5. 让我们再看看读者—写者问题，但这一次，担心饥饿。如何确保所有读者和作者最终都能取得进展？有关详细信息，请参阅 `reader-writer-nostarve.c` 。

	添加一个`readlock`信号量，这样写者就可以获取`readlock`来阻止其他读者进入临界区，解决了饥饿问题。

6. 使用信号量构建一个 no-starve 互斥锁，其中任何尝试获取互斥锁的线程最终都会获得它。有关详细信息，请参阅 `mutex-nostarve.c` 中的代码。

	*The Little Book of Semaphore* chapter 4.3

	```c
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <pthread.h>
	
	#include "zemaphore.h"
	#include "common_threads.h"
	
	//
	// Here, you have to write (almost) ALL the code. Oh no!
	// How can you show that a thread does not starve
	// when attempting to acquire this mutex you build?
	//
	
	typedef struct __ns_mutex_t {
	    Zem_t s1;       // room2的信号量
	    Zem_t s2;       // room3的信号量
	    Zem_t mutex;    // 互斥锁 mutex，用于对整个结构体的访问进行同步
	    int room1;      // 教室中等待的人数
	    int room2;      // 面试等待室中等待的人数
	} ns_mutex_t;
	
	void ns_mutex_init(ns_mutex_t *m) {
	    Zem_init(&m->s1, 1);        // s1初始值为1，允许1个线程进入修改room2
	    Zem_init(&m->s2, 0);        // s2初始值为0，等待获取room2能进入room3的信号
	    Zem_init(&m->mutex, 1);     // lock初始值为1，一次只允许1个线程进入修改room1
	    m->room1 = 0;
	    m->room2 = 0;
	}
	
	void ns_mutex_acquire(ns_mutex_t *m) {
	    Zem_wait(&m->mutex);    // 等待进入room1
	    m->room1++;             // 进入room1
	    Zem_post(&m->mutex);
	
	    // 等待点1：room1
	
	    // 离开room1，进入room2
	    Zem_wait(&m->s1);       // 等待进入room2
	    m->room2++;             // 进入room2
	    Zem_wait(&m->mutex);    // 获取mutex修改room1
	    m->room1--;             // 离开room1
	
	    if (m->room1 == 0) {
	        // 若room1内无线程等待，本线程是最后一个线程，开启room3的锁，允许room2的进入room3
	        Zem_post(&m->s2);
	    } else {
	        // 若room1内还有线程等待，开启room2的锁，本线程需要将它放入room2，然后本线程也等待在room2
	        Zem_post(&m->s1);
	    }
	    Zem_post(&m->mutex);
	
	    // 等待点2：room2
	
	    //  离开room2，进入room3（也就是开始执行）
	    Zem_wait(&m->s2);
	    m->room2--;             // 离开room2
	}
	
	void ns_mutex_release(ns_mutex_t *m) {
	    // 本线程执行完后，放行一个来自room2的线程
	    if (m->room2) {
	        Zem_post(&m->s2);   // 打开room3的门
	    } else {
	        // room2没有线程等待，打开room2的门
	        Zem_post(&m->s1);
	    }
	}
	
	ns_mutex_t m;
	int counter = 0;
	
	void *worker(void *arg) {
	    ns_mutex_acquire(&m);
	    counter++;
	    ns_mutex_release(&m);
	    return NULL;
	}
	
	int main(int argc, char *argv[]) {
	    assert(argc == 2);
	    int num_threads = atoi(argv[1]);
	    ns_mutex_init(&m);
	    pthread_t p[num_threads];
	    printf("parent: begin\n");
	    for (int i = 0; i < num_threads; i++) {
	        Pthread_create(&p[i], NULL, worker, NULL);
	    }
	    for (int i = 0; i < num_threads; i++) {
	        Pthread_join(p[i], NULL);
	    }
	    printf("counter = %d\n", counter);
	    printf("parent: end\n");
	    return 0;
	}
	```

	我们设置有三个线程等待空间：room1，room2，room3。其中room3是隐式设置的，里面同一时间只能有一个线程，也可以说是互斥的。

	原理如下：

	- 先将一段时间内发出获取锁请求的线程都放入room1
	- 由于信号量s1初始值为1，第一个进入room1的线程可以继续进入room2。
	- 它在room2中进行了一个操作，就是判断自己之后是否还有线程在room1中被阻塞，若有的话，随机唤醒一个进入room2，若没有了，就通知room2中的线程可以依次进入room3。
	- 当room1中的线程全部进入room2，最后一个线程发出s2的`post`，room2中的随机一个线程被唤醒，，离开room2（进入room3），执行线程的主体程序，然后释放锁。
	- 在释放锁时，线程会回头判断身后room2中是否还有线程被阻塞，若有，它会释放`s2`信号量，随机唤醒一个进入room3，若没有了，就释放`s1`信号量，通知这段时间内被阻塞在room2门外也就是代码中`Zem_wait(&m->s1);       // 等待进入room2`的线程可以依次进入room2。

	可以做一个比喻：

	- room1是教室
	- room2是面试等待室
	- room3是面试场
	- 同学依次进入教室，每次只能进一个人，先握住门把手，把自己的名字写在教室的名单上，再放开门把手。
	- 一批同学在教室里，一起进入面试等待室，此时每个同学需要依次同时握住教室的门把手和面试等待室的门把手，并同时先从教室的名单上划去自己的名字，然后同时在面试等待室的名单上写上自己的名字，然后松开两个教室的门把手，进入面试等待室。
	- 这个进入面试等待室的同学，如果发现教室内还有同学，他就要把等待室的锁开着，自己在等待室等着；如果教室内没有同学了，自己是最后一个同学了，他就打开面试场的锁，且没有打开面试等候室的锁，**即不让其他人进入面试等候室**。
	- 面试场的锁开了，等待室的某一个同学握住面试场的门把手，在等待室的名单上划去自己的名字。然后进入面试场地。
	- 同学离开面试场地时，如果等待室有同学，就把面试场的门锁开着，否则自己是最后一个人了，那这一批结束，**打开面试等候室的锁**，等待下一批同学来到面试等候室。

7. 简单版本—吸烟者问题。假设一个系统有<font color="red">三个抽烟者进程和一个供应者进程</font>。每个抽烟者不停地卷烟并抽掉它，但是要卷起并抽掉一支烟，抽烟者需要有三种材料：<font color="red">烟草、纸和火柴</font>。三个抽烟者中，<font color="red">第一个拥有烟草、 第二个拥有纸、第三个拥有火柴</font>。供应者进程无限地提供三种材料，供应者每次将两种材料放桌子上，<font color="red">拥有剩下那种材料的抽烟者卷一根烟并抽掉它，并给供应者进程一个信号告诉完成了</font>，供应者就会放另外两种材料再桌上，这个过程一直重复（让三个抽烟者轮流地抽烟）

	本质上这题也属于“生产者-消费者”问题，更详细的说应该是“可生产多种产品的单生产者-多消费者”。

	1. 关系分析。找出题目中描述的各个进程，分析它们之间的同步、互斥关系。

		为同步关系。抽烟者需要等待供应者发出信号，然后抽烟者取走材料。

	2. 整理思路。根据个进程的操作流程确定P、V操作的大致顺序

		由于是同步关系，故前V后P

	3. 设置信号量处置。互斥信号量一般为1，同步信号量初值要看对应资源的初始值是多少。

	所以这题很简单，只需要根据组合来唤醒对应的抽烟者即可。代码如下所示：

	```c
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	
	#include "common_threads.h"
	#include "zemaphore.h"
	
	#define NUM_SMOKERS 3
	
	/*
	* Combination 1: Tobacco + Match
	* Combination 2: Tobacco + Paper
	* Combination 3: Match + Paper
	*/
	Zem_t tobacco_paper;
	Zem_t tobacco_match;
	Zem_t paper_match;
	Zem_t finish;
	void smoker_1() {
	    while(1) {
	        Zem_wait(&tobacco_paper);
	        printf("Smoker 1 is smoking\n");
	        Zem_post(&finish);  // inform agent that smoking is done
	    }
	}
	void smoker_2() {
	    while(1) {
	        Zem_wait(&tobacco_match);
	        printf("Smoker 2 is smoking\n");
	        Zem_post(&finish);
	    }
	}
	void smoker_3() {
	    while(1) {
	        Zem_wait(&paper_match);
	        printf("Smoker 3 is smoking\n");
	        Zem_post(&finish);
	    }
	}
	void agent() {
	    while(1) {
	        int r = rand() % NUM_SMOKERS;
	        switch(r) {
	            case 0:
	                Zem_post(&tobacco_paper);
	                break;
	            case 1:
	                Zem_post(&tobacco_match);
	                break;
	            case 2:
	                Zem_post(&paper_match);
	                break;
	        }
	        Zem_wait(&finish);
	        sleep(1);
	    }
	
	}
	int main() {
	    pthread_t tids[NUM_SMOKERS + 1];
	    Zem_init(&tobacco_paper, 0);
	    Zem_init(&tobacco_match, 0);
	    Zem_init(&paper_match, 0);
	    Zem_init(&finish, 0);
	    Pthread_create(&tids[0], NULL, (void *)agent, NULL);
	    Pthread_create(&tids[1], NULL, (void *)smoker_1, NULL);
	    Pthread_create(&tids[2], NULL, (void *)smoker_2, NULL);
	    Pthread_create(&tids[3], NULL, (void *)smoker_3, NULL);
	    for (int i = 0; i < NUM_SMOKERS + 1; i++) {
	        Pthread_join(tids[i], NULL);
	    }
	    return 0;
	}
	```

8. 复杂的吸烟者问题。假设现在3个代理人，3个吸烟者。每个吸烟者手上缺少一种材料（烟草、纸和火柴），而另外三个人（代理者）手上分别有这三种材料。任何时刻都只有一个吸烟者在吸烟，吸烟者必须有三种材料才能吸烟。代理者会将两种材料放到桌上，吸烟者只要拿到缺少的一种材料就可以吸烟了。

	```c
	#include "common_threads.h"
	#include "zemaphore.h"
	#include <stdbool.h>
	#include <stdio.h>
	#include <stdlib.h> // free, malloc
	
	// Little Book of Semaphores: chapter 4.5
	Zem_t tobacco_sem, paper_sem, match_sem, agent_sem, lock;
	bool has_tobacco = false;
	bool has_paper = false;
	bool has_match = false;
	
	void init_sem() {
	  Zem_init(&tobacco_sem, 0);
	  Zem_init(&paper_sem, 0);
	  Zem_init(&match_sem, 0);
	  Zem_init(&agent_sem, 1);
	  Zem_init(&lock, 1);
	}
	
	void tobacco_pusher() {
	  if (has_paper) {
	    has_paper = false;
	    Zem_post(&match_sem);
	  } else if (has_match) {
	    has_match = false;
	    Zem_post(&paper_sem);
	  } else
	    has_tobacco = true;
	}
	
	void paper_pusher() {
	  if (has_match) {
	    has_match = false;
	    Zem_post(&tobacco_sem);
	  } else if (has_tobacco) {
	    has_tobacco = false;
	    Zem_post(&match_sem);
	  } else
	    has_paper = true;
	}
	
	void match_pusher() {
	  if (has_paper) {
	    has_paper = false;
	    Zem_post(&tobacco_sem);
	  } else if (has_tobacco) {
	    has_tobacco = false;
	    Zem_post(&paper_sem);
	  } else
	    has_match = true;
	}
	
	void *agent(void *arg) {
	  int type = *(int *)arg;
	  Zem_wait(&agent_sem);
	  Zem_wait(&lock);
	  switch (type) {
	  case 0:
	    tobacco_pusher();
	    paper_pusher();
	    break;
	  case 1:
	    paper_pusher();
	    match_pusher();
	    break;
	  case 2:
	    tobacco_pusher();
	    match_pusher();
	  }
	  Zem_post(&lock);
	  return NULL;
	}
	
	void *smoker(void *arg) {
	  int type = *(int *)arg;
	  switch (type) {
	  case 0:
	    Zem_wait(&tobacco_sem);
	    printf("Smoker with tobacco is getting a little bit of cancer.\n");
	    Zem_post(&agent_sem);
	    break;
	  case 1:
	    Zem_wait(&paper_sem);
	    printf("Smoker with paper is getting a little bit of cancer.\n");
	    Zem_post(&agent_sem);
	    break;
	  case 2:
	    Zem_wait(&match_sem);
	    printf("Smoker with match is getting a little bit of cancer.\n");
	    Zem_post(&agent_sem);
	  }
	  return NULL;
	}
	
	int main() {
	  pthread_t agents[3];
	  pthread_t smokers[3];
	  int types[] = {0, 1, 2};
	  init_sem();
	  for (int i = 0; i < 3; i++) {
	    Pthread_create(&agents[i], NULL, agent, &types[i]);
	    Pthread_create(&smokers[i], NULL, smoker, &types[i]);
	  }
	  for (int i = 0; i < 3; i++) {
	    Pthread_join(agents[i], NULL);
	    Pthread_join(smokers[i], NULL);
	  }
	  return 0;
	}
	```

	

