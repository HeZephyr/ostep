#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main(int argc, char *argv[]) {
    int fd = open("foo", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
    assert(fd > -1);
    
    char buffer[] = "Hello, World!";
    size_t size = sizeof(buffer) - 1;
    
    ssize_t rc = write(fd, buffer, size);
    assert(rc == size);

    rc = fsync(fd);
    assert(rc == 0);

    close(fd);
    printf("Data written to disk successfully.\n");
    return 0;
}