#include <stdio.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input file> <output file>\n", argv[0]);
        return 1;
    }
    FILE *input = fopen(argv[1], "rb");
    if (input == NULL) {
        perror("fopen");
        return 1;
    }
    FILE *output = fopen(argv[2], "wb");
    if (output == NULL) {
        perror("fopen");
        return 1;
    }
    unsigned char buffer[BUFFER_SIZE];
    int count = 0;
    unsigned char xor = 0;
    while ((count = fread(buffer, 1, BUFFER_SIZE, input)) != 0 && count != 0) {
        for (int i = 0; i < count; i++) {
            xor ^= buffer[i];
        }
        if (fwrite(&xor, sizeof(xor), 1, output) != 1) {
            perror("fwrite");
            return 1;
        } else {
            xor = 0;
        }
    }
    fclose(input);
    fclose(output);
    return 0;
}