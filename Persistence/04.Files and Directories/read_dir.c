#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <assert.h>

int main(int argc, char *argv[]) {
    DIR *dp = opendir(".");                 // Open current directory
    assert(dp != NULL);
    struct dirent *d;
    while ((d = readdir(dp)) != NULL) {  // Read one directory entry
        // Print the inode number and name
        printf("%lu %s\n", (unsigned long) d->d_ino, d->d_name);
    }
    closedir(dp);                       // Close the directory
    return 0;
}
