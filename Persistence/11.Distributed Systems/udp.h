#ifndef __UDP_H__
#define __UDP_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#include <sys/socket.h>
#include <sys/wait.h>

#include <netinet/in.h>

#define CLIENT_PORT 20000
#define SERVER_PORT 7000

// prototypes

int UDP_Open(int port);
int UDP_Close(int fd);

int UDP_Read(int fd, struct sockaddr_in *addr, char *buffer, int n);
int UDP_Write(int fd, struct sockaddr_in *addr, char *buffer, int n);

int UDP_FillSockAddr(struct sockaddr_in *addr, char *hostName, int port);

#endif // __UDP_H__