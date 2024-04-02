#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define handle_error(msg)  \
  do {                     \
    perror(msg);           \
    exit(EXIT_FAILURE);    \
  } while (0)

int main(int argc, char *argv[]) {
  // Check if the correct number of command-line arguments is provided
  if (argc < 3) {
    fprintf(stderr, "Usage: %s pages trials\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // Retrieve the page size
  long PAGESIZE = sysconf(_SC_PAGESIZE); // 4096
  // Calculate the jump size
  long jump = PAGESIZE / sizeof(int);    // 1024

  // Parse the input arguments
  int pages = atoi(argv[1]);
  int trials = atoi(argv[2]);
  // Check if the input values are valid
  if (pages <= 0 || trials <= 0) {
    fprintf(stderr, "Invalid input\n");
    exit(EXIT_FAILURE);
  }

  // Allocate memory for the array
  int *a = calloc(pages, PAGESIZE);
  // Initialize the start time
  struct timespec start, end;
  // Get the start time
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start) == -1)
    handle_error("clock_gettime");

  // Perform the specified number of trials
  for (int j = 0; j < trials; j++) {
    // Access each page in the array
    for (int i = 0; i < pages * jump; i += jump)
      a[i] += 1;
  }

  // Get the end time
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end) == -1)
    handle_error("clock_gettime");

  // Calculate and print the average time per access (in nanoseconds)
  printf("%f\n",
         ((end.tv_sec - start.tv_sec) * 1E9 + end.tv_nsec - start.tv_nsec) /
             (trials * pages));

  // Free the allocated memory
  free(a);
  return 0;
}
