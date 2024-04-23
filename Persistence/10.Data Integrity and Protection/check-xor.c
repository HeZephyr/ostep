#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>

double Time_GetSeconds() {
    struct timeval t;
    int rc = gettimeofday(&t, NULL);
    assert(rc == 0);
    return (double) ((double)t.tv_sec + (double)t.tv_usec / 1e6);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <input>\n", argv[0]);
        return 1;
    }
    unsigned char xor = 0;
    int fd;
    char *filename = argv[1];
    if ((fd = open(filename, O_RDONLY)) == -1) {
        perror("open");
        return 1;
    }
    unsigned char buffer;
    double start = Time_GetSeconds();
    while (read(fd, &buffer, 1) > 0) {
        if (buffer != '\n') {
            xor ^= buffer;
        }
    }
    double end = Time_GetSeconds();
    printf("XOR-based checksum: %d\n", xor);
    printf("Time taken: %f seconds\n", end - start);
    close(fd);
    return 0;
}