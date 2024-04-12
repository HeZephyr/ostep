#ifdef __linux__
#define _GNU_SOURCE // accept4
#endif

#include "connection.h"
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <time.h>   // clock_gettime
#include <unistd.h> // close

// 回调函数：读取客户端请求并处理
void readcb(struct bufferevent *bev, void *ctx) {
    char buf[BUFSIZ] = "";
    bufferevent_read(bev, buf, BUFSIZ);  // 从 bufferevent 读取数据
    int fd = open(buf, O_RDONLY | O_NONBLOCK);  // 打开文件描述符
    if (fd == -1)
        handle_error("open");
    struct evbuffer *output = bufferevent_get_output(bev);
    evbuffer_set_flags(output, EVBUFFER_FLAG_DRAINS_TO_FD);  // 输出数据到文件描述符
    evbuffer_add_file(output, fd, 0, -1);
}

// 错误回调函数：处理连接错误
void errorcb(struct bufferevent *bev, short error, void *ctx) {
    if (error & BEV_EVENT_EOF) {
        if (--num_reqs == 0) {
            struct event_base *base = bufferevent_get_base(bev);
            event_base_loopexit(base, NULL);  // 退出事件循环
        }
    } else if (error & BEV_EVENT_ERROR) {
        fprintf(stderr, "error: %s\n", strerror(error));  // 输出错误信息
    }
    bufferevent_free(bev);  // 释放 bufferevent 资源
}

// 接受连接请求并创建新的 bufferevent 处理
void do_accept(int sfd, short event, void *arg) {
    struct event_base *base = arg;
#ifdef __linux__
    int cfd = accept4(sfd, NULL, NULL, SOCK_NONBLOCK);  // 接受连接请求（非阻塞）
    if (cfd == -1)
        handle_error("accept4");
#else
    int cfd = accept(sfd, NULL, NULL);  // 接受连接请求
    if (cfd == -1)
        handle_error("accept");
    evutil_make_socket_nonblocking(cfd);  // 设置 socket 为非阻塞模式
#endif
    struct bufferevent *bev = bufferevent_socket_new(base, cfd, BEV_OPT_CLOSE_ON_FREE);  // 创建新的 bufferevent
    bufferevent_setcb(bev, readcb, NULL, errorcb, NULL);  // 设置 bufferevent 的回调函数
    bufferevent_enable(bev, EV_READ | EV_WRITE);  // 启用读写事件监听
}

// 主函数
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s numReqs", argv[0]);
        exit(EXIT_FAILURE);
    }

    // 获取程序开始运行的时间
    struct timespec start, end;
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
        handle_error("clock_gettime");

    num_reqs = atoi(argv[1]);  // 解析命令行参数，确定请求数量
    int sfd = init_socket(1, 1);  // 初始化服务器套接字

    struct event_base *base = event_base_new();  // 创建 event_base
    struct event *listener_event = event_new(base, sfd, EV_READ | EV_PERSIST, do_accept, (void *)base);  // 创建监听事件
    event_add(listener_event, NULL);  // 添加监听事件到事件循环
    event_base_dispatch(base);  // 进入事件循环
    event_base_free(base);  // 释放 event_base 资源

    // 获取程序结束运行的时间并计算运行时间
    if (clock_gettime(CLOCK_MONOTONIC, &end) == -1)
        handle_error("clock_gettime");
    // 输出程序运行时间（单位：纳秒）
    printf("%f\n", ((end.tv_sec - start.tv_sec) * 1E9 + end.tv_nsec - start.tv_nsec));
    return 0;
}
