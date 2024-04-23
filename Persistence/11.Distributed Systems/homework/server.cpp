#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

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
    serverAddr.sin_port = htons(8888); // 端口号8888
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    // 绑定地址
    if (bind(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error in binding\n";
        return -1;
    }
    printf("Server started at port 8888\n");
    while (1) {
        // 接收消息
        char buffer[1024];
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int recvLen = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
        buffer[recvLen] = '\0';
        std::cout << "Message from client: " << buffer << std::endl;

        // 发送确认消息
        const char *ackMsg = "Message received";
        sendto(sockfd, ackMsg, strlen(ackMsg), 0, (const struct sockaddr *)&clientAddr, clientAddrLen);
    }
    // 关闭套接字
    close(sockfd);

    return 0;
}
