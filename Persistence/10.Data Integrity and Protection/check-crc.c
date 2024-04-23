#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <assert.h>

#define POLY 0x1021   // 0001 0000 0010 0001

// Function to get the current time in seconds
double Time_GetSeconds() {
    struct timeval t;
    int rc = gettimeofday(&t, NULL);
    assert(rc == 0); // Ensure gettimeofday() call succeeds
    return (double)((double)t.tv_sec + (double)t.tv_usec / 1e6);
}

int main(int argc, char *argv[]) {
    // Check if the correct number of command-line arguments is provided
    if (argc != 2) {
        printf("Usage: %s <input>\n", argv[0]);
        return 1;
    }
    
    unsigned short crc = 0xFFFF; // Initialize CRC register with 0xFFFF
    int fd;
    char *filename = argv[1]; // Get the input file path
    
    // Open the input file
    if ((fd = open(filename, O_RDONLY)) == -1) {
        perror("open"); // Print error message if open() fails
        return 1;
    }
    
    unsigned char buffer; // Buffer to store read characters
    int count, xor_flag;
    double t = Time_GetSeconds(); // Record the start time
    
    // Read the file byte by byte and compute the CRC
    while (read(fd, &buffer, 1) > 0) {
        unsigned short v = 0x80; // Initialize mask for each bit in the byte
        for(size_t j = 0; j < 8; j++) { // Iterate through each bit in the byte
            if (crc & 0x8000) // Check if the MSB of CRC is 1
                xor_flag = 1; // Set xor_flag if MSB is 1
            else
                xor_flag = 0; // Clear xor_flag if MSB is 0

            crc = crc << 1; // Shift CRC register left by 1 bit

            // Append next bit of message to end of CRC if it is not zero.
            // If it is zero, it's already appended at the above line.
            if (buffer & v)
                crc += 1; // Add 1 to CRC if current bit of message is 1

            if (xor_flag)
                crc = crc ^ POLY; // XOR CRC with polynomial if MSB was 1
            v = v >> 1; // Shift mask right by 1 bit for next iteration
        }
    }

    // Augment the message by appending 16 zero bits to the end of it.
    for (size_t j = 0; j < 16; j++) {
        if (crc & 0x8000) // Check if the MSB of CRC is 1
            xor_flag = 1; // Set xor_flag if MSB is 1
        else
            xor_flag = 0; // Clear xor_flag if MSB is 0

        crc = crc << 1; // Shift CRC register left by 1 bit

        if (xor_flag)
            crc = crc ^ POLY; // XOR CRC with polynomial if MSB was 1
    }

    // Print the computed CRC and the time taken
    printf("16-bit CRC: 0x%X\n", crc); // Print CRC value in hexadecimal format
    printf("time(seconds): %f\n", Time_GetSeconds() - t); // Print time taken to compute CRC
    close(fd); // Close the input file

    return 0; // Return success
}
