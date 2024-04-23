#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    // 创建 UDP 套接字
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error in socket creation\n";
        return -1;
    }

    // 服务器地址
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888); // 服务器端口号8888
    serverAddr.sin_addr.s_addr = inet_addr("10.233.4.102"); // 服务器 IP 地址

    // 发送消息
    const char *msg = "Hello from client";
    sendto(sockfd, msg, strlen(msg), 0, (const struct sockaddr *)&serverAddr, sizeof(serverAddr));

    // 接收确认消息
    char buffer[1024];
    socklen_t serverAddrLen = sizeof(serverAddr);
    int recvLen = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&serverAddr, &serverAddrLen);
    buffer[recvLen] = '\0';
    std::cout << "Confirmation from server: " << buffer << std::endl;

    // 关闭套接字
    close(sockfd);

    return 0;
}
