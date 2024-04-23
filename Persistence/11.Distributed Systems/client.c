#include <stdio.h>
#include "udp.h"

#define BUFFER_SIZE 1024

int main() {
    struct sockaddr_in addrSend, addrRecv;

    int fd = UDP_Open(CLIENT_PORT);
    int rc = UDP_FillSockAddr(&addrSend, "localhost", SERVER_PORT);

    char message[BUFFER_SIZE];
    sprintf(message, "hello world");

    printf("client:: sending message [%s]\n", message);
    rc = UDP_Write(fd, &addrSend, message, BUFFER_SIZE);
    if (rc < 0) {
        fprintf(stderr, "Client:: Failed to send message\n");
        return -1;
    }

    printf("client:: waiting for reply...\n");
    rc = UDP_Read(fd, &addrRecv, message, BUFFER_SIZE);
    if (rc < 0) {
        fprintf(stderr, "Client:: Failed to read message\n");
        return -1;
    }
    printf("client:: read %d bytes (message: %s)\n", rc, message);
    
    return 0;
}