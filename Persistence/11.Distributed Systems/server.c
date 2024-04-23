#include <stdio.h>

#include "udp.h"

#define BUFFER_SIZE 1024

int main(int argc, char **argv) {
    int fd = UDP_Open(SERVER_PORT);
    if (fd < 0) {
        fprintf(stderr, "Failed to create socket\n");
        return -1;
    }

    while (1) {
        struct sockaddr_in addr;
        char buffer[BUFFER_SIZE];
        printf("server:: waiting for data...\n");
        int rc = UDP_Read(fd, &addr, buffer, 1024);
        printf("server:: read %d bytes (message: %s)\n", rc, buffer);
        if (rc > 0) {
            char reply[BUFFER_SIZE];
            sprintf(reply, "goodbye world");
            rc = UDP_Write(fd, &addr, reply, BUFFER_SIZE);
	        printf("server:: reply\n");
        }
    }

    return 0;
}