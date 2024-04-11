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
        // 注意这里没有释放s1，因为不让其他线程进入room2
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

