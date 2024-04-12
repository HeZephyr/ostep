#include <stdio.h>
#include <signal.h>

void handle(int arg) {
    printf("stop wakin’ me up...\n");
}

int main(int argc, char *argv[]) {
    signal(SIGHUP, handle);
    while (1)
        ; // doin’ nothin’ except catchin’ some sigs
    return 0;
}
