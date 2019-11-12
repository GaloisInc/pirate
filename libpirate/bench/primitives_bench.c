#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "primitives.h"

int main(int argc, char *argv[]) {
  int64_t i, size, count, delta;
  int rv;
  char *buf;
  struct timespec start, stop;

  if (argc < 3) {
    printf("usage: primitives_bench [message-size] [roundtrip-count] [device, "
           "optional]\n");
    return 1;
  }

  size = atol(argv[1]);
  count = atol(argv[2]);

  if (argc >= 4) {
    if (strncmp(argv[3], "shmem", 6) == 0) {
      pirate_set_channel_type(1, SHMEM);
    } else {
      pirate_set_channel_type(1, DEVICE);
      pirate_set_pathname(1, argv[3]);
    }
  }

  buf = malloc(size);
  if (buf == NULL) {
    perror("malloc");
    return 1;
  }

  if (!fork()) {
    // child
    if (pirate_open(1, O_RDONLY) < 0) {
      perror("pirate_open");
      return 1;
    }
    // check /proc/sys/fs/pipe-max-size on failure
    if (pirate_get_channel_type(1) == PIPE) {
      if (pirate_fcntl1_int(1, O_RDONLY, F_SETPIPE_SZ, 8 * size) < 0) {
        perror("pirate_fcntl1");
        return 1;
      }
    }
    for (i = 0; i < count; i++) {
      int nbytes = size;
      while (nbytes > 0) {
        if ((rv = pirate_read(1, buf, size)) < 1) {
          perror("pirate_read");
          return 1;
        }
        nbytes -= rv;
      }
    }
    pirate_close(1, O_RDONLY);
  } else {
    // parent
    if (pirate_open(1, O_WRONLY) < 0) {
      perror("pirate_open");
      return 1;
    }
    // check /proc/sys/fs/pipe-max-size on failure
    if (pirate_get_channel_type(1) == PIPE) {
      if (pirate_fcntl1_int(1, O_WRONLY, F_SETPIPE_SZ, 8 * size) < 0) {
        perror("pirate_fcntl1");
        return 1;
      }
    }
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      perror("clock_gettime");
      return 1;
    }
    for (i = 0; i < count; i++) {
      int nbytes = size;
      while (nbytes > 0) {
        if ((rv = pirate_write(1, buf, size)) < 1) {
          perror("pirate_write");
          return 1;
        }
        nbytes -= rv;
      }
    }
    wait(NULL);
    if (clock_gettime(CLOCK_MONOTONIC, &stop) < 0) {
      perror("clock_gettime");
      return 1;
    }
    pirate_close(1, O_WRONLY);
    // nanoseconds difference
    delta = ((stop.tv_sec - start.tv_sec) * 1000000000 +
             (stop.tv_nsec - start.tv_nsec));
    // 8 bits per byte
    // 1e9 nanoseconds per second
    // 1e6 bytes per megabytes
    printf("average throughput: %f Mb/s\n",
           (8.0 * (1e9 / 1e6) * count * size) / delta);
  }
  free(buf);
}
