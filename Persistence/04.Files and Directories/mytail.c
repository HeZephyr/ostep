#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <string.h>

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)

int main(int argc, char *argv[]) {
    struct stat sb;
    int fd, offset, lines;
    char *pathname = "";

    if (argc != 3 || strlen(argv[1]) <= 1 || argv[1][0] != '-') {
        fprintf(stderr, "Usage: %s -<number of lines> <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    lines = atoi(argv[1] + 1);
    pathname = argv[2];

    if (stat(pathname, &sb) == -1) {
        handle_error("stat");
    }

    if ((fd = open(pathname, O_RDONLY)) == -1) {
        handle_error("open");
    }

    // Set the file offset to one byte forward from the end of the file
    offset = lseek(fd, -1, SEEK_END);

    char buffer[sb.st_size];
    while (lines > 0) {
        // If the byte read is a newline character, reduce the number of lines to find
        if (read(fd, buffer, 1) == -1) {
            handle_error("read");
        }
        if (buffer[0] == '\n') {
            lines--;
        }
        // 往前移动两个字节， 一个是当前字节，一个是上一个字节，因为当前字节已经读取过了
        offset = lseek(fd, -2, SEEK_CUR);
        if (offset == -1) {
            break;
        }
    }

    if (offset > 0 || lines == 0) {
        if (lseek(fd, 2, SEEK_CUR) == -1)
            handle_error("lseek");
    } else {
        if (lseek(fd, 0, SEEK_SET) == -1)
            handle_error("lseek");
    }

    memset(buffer, 0, sb.st_size);
    if (read(fd, buffer, sb.st_size) == -1)
        handle_error("read");

    printf("%s", buffer);
    close(fd);

    return 0;
}