#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <memory> <time>\n", argv[0]);
        return 1;
    }

    printf("Current Process ID = %d\n", getpid());

    long long int size = ((long long int)atoi(argv[1])) * 1024 * 1024;
    int* buffer = (int*)malloc(size);

    // Run the while loop for given amount of time.
    time_t endwait, seconds, start;
    seconds = atoi(argv[2]);
    start = time(NULL);
    // print format start time
    printf("Start Time: %s", ctime(&start));
    endwait = start + seconds;

    while (start < endwait) {
        for (long long int i = 0; i < size / sizeof(int); i++) {
            buffer[i] = i;
        }
        start = time(NULL);
    }
    
    printf("End Time: %s", ctime(&start));
    free(buffer);
    return 0;
}
