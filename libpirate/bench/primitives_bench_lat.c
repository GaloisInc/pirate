/*
 * This work was authored by Two Six Labs, LLC and is sponsored by a subcontract
 * agreement with Galois, Inc.  This material is based upon work supported by
 * the Defense Advanced Research Projects Agency (DARPA) under Contract No.
 * HR0011-19-C-0103.
 *
 * The Government has unlimited rights to use, modify, reproduce, release,
 * perform, display, or disclose computer software or computer software
 * documentation marked with this legend. Any reproduction of technical data,
 * computer software, or portions thereof marked with this legend must also
 * reproduce this marking.
 *
 * Copyright 2019 Two Six Labs, LLC.  All rights reserved.
 */

#define _GNU_SOURCE

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "primitives.h"


static int set_buffer_size(int gd, int size) {
  size = 8 * size;
  switch (pirate_get_channel_type(gd)) {
  case SHMEM:
    if (pirate_set_buffer_size(gd, size) < 0) {
      perror("pirate_set_buffer_size");
      return -1;
    }
    break;
  case UNIX_SOCKET:
    if (size > 212992) {
      // check /proc/sys/net/core/wmem_max on failure
      if (pirate_set_buffer_size(gd, size) < 0) {
        perror("pirate_set_buffer_size");
        return -1;
      }
    }
    break;
  default:
    break;
  }
  return 0;
}

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
    } else if (strncmp(argv[3], "unix", 5) == 0) {
      pirate_set_channel_type(1, UNIX_SOCKET);
    } else if (strncmp(argv[3], "uio", 4) == 0) {
      pirate_set_channel_type(1, UIO_DEVICE);
    } else {
      pirate_set_channel_type(1, DEVICE);
      pirate_set_pathname(1, argv[3]);
    }
  }

  if (argc >= 5) {
    if (strncmp(argv[4], "shmem", 6) == 0) {
      pirate_set_channel_type(2, SHMEM);
    } else if (strncmp(argv[4], "unix", 5) == 0) {
      pirate_set_channel_type(2, UNIX_SOCKET);
    } else if (strncmp(argv[4], "uio", 4) == 0) {
      pirate_set_channel_type(2, UIO_DEVICE);
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
    if (set_buffer_size(1, size) < 0) {
      return 1;
    }
    if (set_buffer_size(2, size) < 0) {
      return 1;
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
        if ((rv = pirate_read(1, buf, nbytes)) <= 0) {
          perror("pirate_read channel 1");
          return 1;
        }
        nbytes -= rv;
      }
      nbytes = size;
      while (nbytes > 0) {
        if ((rv = pirate_write(2, buf, nbytes)) <= 0) {
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
    if (set_buffer_size(1, size) < 0) {
      return 1;
    }
    if (set_buffer_size(2, size) < 0) {
      return 1;
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
        if ((rv = pirate_write(1, buf, nbytes)) <= 0) {
          perror("pirate_write channel 1");
          return 1;
        }
        nbytes -= rv;
      }
      nbytes = size;
      while (nbytes > 0) {
        if ((rv = pirate_read(2, buf, nbytes)) <= 0) {
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
