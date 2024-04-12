#include "connection.h"
#include <liburing.h>
#include <stdio.h>
#include <time.h>   // clock_gettime
#include <unistd.h> // close

// 使用 io_uring 实现并发服务器处理多个请求
// 详细参考文档：https://kernel.dk/io_uring.pdf
// 参考 liburing GitHub 仓库：https://github.com/axboe/liburing
// 参考 Linux 内核源码：https://github.com/torvalds/linux/blob/master/fs/io_uring.c
// 参考 Linux 内核头文件：https://github.com/torvalds/linux/blob/master/include/uapi/linux/io_uring.h
// 了解 io_uring 更多内容：https://lwn.net/Kernel/Index/#io_uring
struct user_data {
	char buf[BUFSIZ];  // 用于存储数据的缓冲区
	int socket_fd;      // 客户端套接字文件描述符
	int file_fd;        // 文件套接字文件描述符
	int index;          // 数据数组的索引
	int io_op;          // IO 操作类型
};

struct user_data data_arr[LISTEN_BACKLOG];  // 存储用户数据的数组

int numAccepts = 0, numReqs = 0;  // 初始接受的请求数量和待处理的请求数量

// 准备接受连接请求
void prep_accept(struct io_uring *ring, int sfd) {
	struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
	if (sqe == NULL)
		handle_error("io_uring_get_sqe");

	io_uring_prep_accept(sqe, sfd, NULL, NULL, 0);
	data_arr[--numAccepts].io_op = IORING_OP_ACCEPT;
	data_arr[numAccepts].index = numAccepts;
	io_uring_sqe_set_data(sqe, &data_arr[numAccepts]);
	if (io_uring_submit(ring) < 0)
		handle_error("io_uring_submit");
}

// 准备接收数据
void prep_recv(struct io_uring *ring, int sfd, int cfd, int index) {
	struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
	if (sqe == NULL)
		handle_error("io_uring_get_sqe");

	data_arr[index].io_op = IORING_OP_RECV;
	data_arr[index].socket_fd = cfd;
	memset(data_arr[index].buf, 0, BUFSIZ);
	io_uring_prep_recv(sqe, cfd, data_arr[index].buf, BUFSIZ, 0);
	io_uring_sqe_set_data(sqe, &data_arr[index]);
	if (numAccepts > 0)
		prep_accept(ring, sfd);
	else if (io_uring_submit(ring) < 0)
		handle_error("io_uring_submit");
}

// 准备读取文件内容
void prep_read(struct io_uring *ring, int index) {
	struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
	if (sqe == NULL)
		handle_error("io_uring_get_sqe");

	int file_fd = open(data_arr[index].buf, O_RDONLY);
	if (file_fd == -1) {
		fprintf(stderr, "buf: %s\n", data_arr[index].buf);
		handle_error("open");
	}

	data_arr[index].io_op = IORING_OP_READ;
	data_arr[index].file_fd = file_fd;
	memset(data_arr[index].buf, 0, BUFSIZ);
	io_uring_prep_read(sqe, file_fd, data_arr[index].buf, BUFSIZ, 0);
	io_uring_sqe_set_data(sqe, &data_arr[index]);
	if (io_uring_submit(ring) < 0)
		handle_error("io_uring_submit");
}

// 准备发送数据给客户端
void prep_send(struct io_uring *ring, int index) {
	struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
	if (sqe == NULL)
		handle_error("io_uring_get_sqe");

	close(data_arr[index].file_fd);
	data_arr[index].io_op = IORING_OP_SEND;
	io_uring_prep_send(sqe, data_arr[index].socket_fd, data_arr[index].buf,
	                   BUFSIZ, 0);
	io_uring_sqe_set_data(sqe, &data_arr[index]);
	if (io_uring_submit(ring) < 0)
		handle_error("io_uring_submit");
}

// 主函数
int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s numReqs\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// 获取程序开始运行的时间
	struct timespec start, end;
	if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
		handle_error("clock_gettime");

	// 解析命令行参数，确定待处理的请求数量
	numReqs = atoi(argv[1]);
	if (numReqs <= 0) {
		fprintf(stderr, "Get out\n");
		exit(EXIT_FAILURE);
	}
	numAccepts = numReqs;

	// 初始化服务器套接字和 io_uring
	int sfd = init_socket(1, 0);
	struct io_uring ring;
	if (io_uring_queue_init(LISTEN_BACKLOG, &ring, IORING_SETUP_SQPOLL))
		handle_error("io_uring_queue_init");
	prep_accept(&ring, sfd);

	// 处理请求循环
	while (numReqs > 0) {
		struct io_uring_cqe *cqe;
		if (io_uring_wait_cqe(&ring, &cqe))
			handle_error("io_uring_wait_cqe");
		if (cqe->res < 0) {
			fprintf(stderr, "I/O error: %s\n", strerror(-cqe->res));
			exit(EXIT_FAILURE);
		}
		struct user_data *data = io_uring_cqe_get_data(cqe);
		switch (data->io_op) {
		case IORING_OP_ACCEPT:
			prep_recv(&ring, sfd, cqe->res, data->index);
			break;
		case IORING_OP_RECV:
			prep_read(&ring, data->index);
			break;
		case IORING_OP_READ:
			prep_send(&ring, data->index);
			break;
		case IORING_OP_SEND:
			close(data_arr[data->index].socket_fd);
			numReqs--;
			break;
		default:
			handle_error("Unknown I/O");
		}
		io_uring_cqe_seen(&ring, cqe);
	}
	io_uring_queue_exit(&ring);
	close(sfd);

	// 获取程序结束运行的时间并计算运行时间
	if (clock_gettime(CLOCK_MONOTONIC, &end) == -1)
		handle_error("clock_gettime");
	// 输出程序运行时间（单位：纳秒）
	printf("%f\n",
	       ((end.tv_sec - start.tv_sec) * 1E9 + end.tv_nsec - start.tv_nsec));
	return 0;
}
