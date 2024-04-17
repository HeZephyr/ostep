#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>
#include <string.h>

#define STRINGSIZE 1024

void print_file(struct stat sb) {
    printf("%4lo  ", (unsigned long) sb.st_mode);
    printf("%3ld  ", (long) sb.st_nlink);
    printf("%3ld  %3ld  ", (long) sb.st_uid, (long) sb.st_gid);
    printf("%4lld  ", (long long) sb.st_size);
    char timeString[STRINGSIZE] = "";
    strftime(timeString, STRINGSIZE, "%b %d %H:%M", localtime(&sb.st_mtime));
    printf("%s  ", timeString);
}

int main(int argc, char *argv[]) {
    struct stat sb;
    int opt;
    char *pathname = ".";
    bool list = false;
    if (argc > 3) {
        fprintf(stderr, "Usage: %s [-l] [pathname]\n", argv[0]);
        exit(EXIT_FAILURE);
    } else if (argc == 3) {
        if (strcmp(argv[1], "-l") == 0) {
            list = true;
        } else {
            fprintf(stderr, "Usage: %s [-l] [pathname]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        pathname = argv[2];
    } else if (argc == 2) {
        if (strcmp(argv[1], "-l") == 0) {
            list = true;
        } else {
            pathname = argv[1];
        }
    }
    if (stat(pathname, &sb) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    if (S_ISDIR(sb.st_mode)) {
        DIR *dp = opendir(pathname);
        if (dp == NULL) {
            perror("opendir");
            exit(EXIT_FAILURE);
        }
        struct dirent *d;
        while ((d = readdir(dp)) != NULL) {
            if (list) {
                char filePath[STRINGSIZE] = "";
                strncpy(filePath, pathname, strlen(pathname));
                strncat(filePath, "/", 1);
                strncat(filePath, d->d_name, strlen(d->d_name));
                if (stat(filePath, &sb) == -1) {
                    perror("stat");
                    exit(EXIT_FAILURE);
                }
                print_file(sb);
            }
            printf("%s\n", d->d_name);
        }
        closedir(dp);
    } else {
        if (list) {
            print_file(sb);
        }
        printf("%s\n", pathname);
    }
    return 0;
}