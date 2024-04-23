#include "udp.h"

// create a socket and bind it to a port on the current machine
// used to listen for incoming packets
int UDP_Open(int port) {
    // the second argument is the type of socket, 
    int fd = socket(AF_INET, SOCK_DGRAM, 0);    // SOCK_DGRAM is for UDP, SOCK_STREAM is for TCP
    if (fd == -1) {
        perror("socket");
        return -1;
    }

    // set up the bind
    struct sockaddr_in myaddr;              // store the socket address information
    bzero(&myaddr, sizeof(myaddr));         // zero out the structure

    myaddr.sin_family       = AF_INET;      // set the address family, IPv4
    myaddr.sin_port         = htons(port);  // set the port number  
    myaddr.sin_addr.s_addr  = INADDR_ANY;   // set the IP address to any interface on the machine

    // bind the socket to the port
    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) {
        perror("bind");
        UDP_Close(fd);
        return -1;
    }

    return fd;
}

// close the socket
int UDP_Close(int fd) {
    return close(fd);
}

// read a message from a socket
int UDP_Read(int fd, struct sockaddr_in *addr, char *buffer, int n) {
    int length = sizeof(struct sockaddr_in);
    int rc = recvfrom(fd, buffer, n, 0, (struct sockaddr *)addr, &length);
    return rc;
}

// write a message to a socket
int UDP_Write(int fd, struct sockaddr_in *addr, char *buffer, int n) {
    return sendto(fd, buffer, n, 0, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
}

// fill in the socket address structure
int UDP_FillSockAddr(struct sockaddr_in *addr, char *hostName, int port) {
    bzero(addr, sizeof(struct sockaddr_in));
    if (hostName == NULL) {
        return 0;   // it's OK just to clear the address
    }
    addr->sin_family = AF_INET;     // host byte order
    addr->sin_port = htons(port);   // short, network byte order

    // convert the host name into the IP address
    struct hostent *host = gethostbyname(hostName);
    if (host == NULL) {
        fprintf(stderr, "Unknown host: %s\n", hostName);
        return -1;
    }

    // copy the IP address into the sockaddr_in structure
    addr->sin_addr = *((struct in_addr *)host->h_addr);

    return 0;
}