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
    printf("usage: primitives_bench_lat message-size roundtrip-count "
           "[devicepath1] [devicepath2]\n");
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

  if (argc >= 5) {
    if (strncmp(argv[4], "shmem", 6) == 0) {
      pirate_set_channel_type(2, SHMEM);
    } else {
      pirate_set_channel_type(2, DEVICE);
      pirate_set_pathname(2, argv[4]);
    }
  }

  buf = malloc(size);
  if (buf == NULL) {
    perror("malloc");
    return 1;
  }

  if (!fork()) {
    // child
    if (pirate_get_channel_type(1) == SHMEM) {
      if (pirate_set_shmem_size(1, 8 * size) < 0) {
        perror("pirate_set_shmem_size channel 1");
        return 1;
      }
    }
    if (pirate_get_channel_type(2) == SHMEM) {
      if (pirate_set_shmem_size(2, 8 * size) < 0) {
        perror("pirate_set_shmem_size channel 2");
        return 1;
      }
    }
    if (pirate_open(1, O_RDONLY) < 0) {
      perror("pirate_open channel 1");
      return 1;
    }
    if (pirate_open(2, O_WRONLY) < 0) {
      perror("pirate_open channel 2");
      return 1;
    }
    // check /proc/sys/fs/pipe-max-size on failure
    switch (pirate_get_channel_type(1)) {
    case PIPE:
      if (pirate_fcntl1_int(1, O_RDONLY, F_SETPIPE_SZ, 8 * size) < 0) {
        perror("pirate_fcntl1 channel 1");
        return 1;
      }
      break;
    case DEVICE:
      if (pirate_ioctl1_int(1, O_RDONLY, F_SETPIPE_SZ, 8 * size) < 0) {
        perror("pirate_ioctl1 channel 1");
        return 1;
      }
      break;
    default:
      break;
    }
    switch (pirate_get_channel_type(2)) {
    case PIPE:
      if (pirate_fcntl1_int(2, O_WRONLY, F_SETPIPE_SZ, 8 * size) < 0) {
        perror("pirate_fcntl1 channel 2");
        return 1;
      }
      break;
    case DEVICE:
      if (pirate_ioctl1_int(2, O_WRONLY, F_SETPIPE_SZ, 8 * size) < 0) {
        perror("pirate_ioctl1 channel 2");
        return 1;
      }
      break;
    default:
      break;
    }
    for (i = 0; i < count; i++) {
      int nbytes = size;
      while (nbytes > 0) {
        if ((rv = pirate_read(1, buf, size)) <= 0) {
          perror("pirate_read channel 1");
          return 1;
        }
        nbytes -= rv;
      }
      nbytes = size;
      while (nbytes > 0) {
        if ((rv = pirate_write(2, buf, size)) <= 0) {
          perror("pirate_write channel 2");
          return 1;
        }
        nbytes -= rv;
      }
    }
    pirate_close(1, O_RDONLY);
    pirate_close(2, O_WRONLY);
  } else {
    // parent
    if (pirate_get_channel_type(1) == SHMEM) {
      if (pirate_set_shmem_size(1, 8 * size) < 0) {
        perror("pirate_set_shmem_size channel 1");
        return 1;
      }
    }
    if (pirate_get_channel_type(2) == SHMEM) {
      if (pirate_set_shmem_size(2, 8 * size) < 0) {
        perror("pirate_set_shmem_size channel 2");
        return 1;
      }
    }
    if (pirate_open(1, O_WRONLY) < 0) {
      perror("pirate_open channel 1");
      return 1;
    }
    if (pirate_open(2, O_RDONLY) < 0) {
      perror("pirate_open channel 2");
      return 1;
    }
    // check /proc/sys/fs/pipe-max-size on failure
    switch (pirate_get_channel_type(1)) {
    case PIPE:
      if (pirate_fcntl1_int(1, O_WRONLY, F_SETPIPE_SZ, 8 * size) < 0) {
        perror("pirate_fcntl1 channel 1");
        return 1;
      }
      break;
    case DEVICE:
      if (pirate_ioctl1_int(1, O_WRONLY, F_SETPIPE_SZ, 8 * size) < 0) {
        perror("pirate_ioctl1 channel 1");
        return 1;
      }
      break;
    default:
      break;
    }
    switch (pirate_get_channel_type(2)) {
    case PIPE:
      if (pirate_fcntl1_int(2, O_RDONLY, F_SETPIPE_SZ, 8 * size) < 0) {
        perror("pirate_fcntl1 channel 2");
        return 1;
      }
      break;
    case DEVICE:
      if (pirate_ioctl1_int(2, O_RDONLY, F_SETPIPE_SZ, 8 * size) < 0) {
        perror("pirate_ioctl1 channel 2");
        return 1;
      }
      break;
    default:
      break;
    }
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
      perror("clock_gettime");
      return 1;
    }
    for (i = 0; i < count; i++) {
      int nbytes = size;
      while (nbytes > 0) {
        if ((rv = pirate_write(1, buf, size)) <= 0) {
          perror("pirate_write channel 1");
          return 1;
        }
        nbytes -= rv;
      }
      nbytes = size;
      while (nbytes > 0) {
        if ((rv = pirate_read(2, buf, size)) <= 0) {
          perror("pirate_read channel 2");
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
    pirate_close(2, O_RDONLY);
    // nanoseconds difference
    delta = ((stop.tv_sec - start.tv_sec) * 1000000000 +
             (stop.tv_nsec - start.tv_nsec));
    printf("average latency: %li ns\n", delta / (count * 2));
  }
  free(buf);
}
