#include "connection.h"
#include <errno.h>
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <time.h>   // clock_gettime
#include <unistd.h> // close

// 使用 epoll 实现并发服务器处理多个请求
int main(int argc, char *argv[]) {
	// 检查命令行参数数量是否正确
	if (argc != 2) {
		printf("Usage: %s numReqs", argv[0]);
		exit(EXIT_FAILURE);
	}

	// 获取程序运行起始时间
	struct timespec start, end;
	if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
		handle_error("clock_gettime");

	// 解析命令行参数，确定需要处理的请求数量
	int numReqs = atoi(argv[1]);

	// 初始化服务器套接字并创建 epoll 实例
	int sfd = init_socket(1, 0);
	int epfd = epoll_create1(0);
	if (epfd == -1)
		handle_error("epoll_create1");

	// 设置 epoll 事件
	struct epoll_event ev;
	struct epoll_event evlist[LISTEN_BACKLOG];
	ev.events = EPOLLIN;
	ev.data.fd = sfd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev) == -1)
		handle_error("epoll_ctl");

	// 循环处理请求，直到处理完指定数量的请求
	while (numReqs > 0) {
		int ready = epoll_wait(epfd, evlist, LISTEN_BACKLOG, -1);
		if (ready == -1) {
			if (errno == EINTR)
				continue; // 如果被信号中断，则重试
			else
				handle_error("epoll_wait");
		}

		// 遍历 epoll 事件列表
		for (int i = 0; i < ready; i++) {
			if (evlist[i].events & EPOLLIN) {
				if (evlist[i].data.fd == sfd) {
					// 如果是服务器套接字，表示有新连接请求
					int cfd = accept(sfd, NULL, NULL);
					if (cfd == -1)
						handle_error("accept");
					ev.events = EPOLLIN;
					ev.data.fd = cfd;
					if (epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev) == -1)
						handle_error("epoll_ctl");
				} else {
					// 否则为客户端套接字，表示有数据可读
					char buff[BUFSIZ] = "";
					if (recv(evlist[i].data.fd, buff, BUFSIZ, 0) == -1)
						handle_error("recv");
					// 打开文件并发送文件内容给客户端
					int fd = open(buff, O_RDONLY);
					if (fd == -1)
						handle_error("open");
					struct stat statbuf;
					if (fstat(fd, &statbuf) == -1)
						handle_error("fstat");
					if (sendfile(evlist[i].data.fd, fd, NULL, statbuf.st_size) == -1)
						handle_error("sendfile");
					close(fd);
					close(evlist[i].data.fd);
					numReqs--; // 减少待处理请求数量
				}
			}
		}
	}

	// 关闭服务器套接字
	close(sfd);

	// 获取程序结束时间并计算运行时间
	if (clock_gettime(CLOCK_MONOTONIC, &end) == -1)
		handle_error("clock_gettime");
	// 输出程序运行时间（单位：纳秒）
	printf("%f\n",
	       ((end.tv_sec - start.tv_sec) * 1E9 + end.tv_nsec - start.tv_nsec));
	return 0;
}
