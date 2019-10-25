#define _GNU_SOURCE

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "primitives.h"

PIRATE_NO_CHANNELS;

int main(int argc, char *argv[]) {
    int64_t i, size, count, delta;
    char *buf;
    struct timespec start, stop;

    if (argc != 3) {
        printf("usage: primitives_bench <message-size> <roundtrip-count>\n");
        return 1;
    }

    size = atol(argv[1]);
    count = atol(argv[2]);

    buf = malloc(size);
    if (buf == NULL) {
        perror("malloc");
        return 1;
    }

    if (!fork()) {
        // child
        if(pirate_open(1, O_RDONLY) < 0) {
            perror("pirate_open");
            return 1;
        }
        // check /proc/sys/fs/pipe-max-size on failure
        if (pirate_fcntl1(1, O_RDONLY, F_SETPIPE_SZ, 8 * size) < 0) {
            perror("pirate_fcntl1");
            return 1;
        }
        for (i = 0; i < count; i++) {
            if (pirate_read(1, buf, size) < size) {
                perror("pirate_read");
                return 1;
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
        if (pirate_fcntl1(1, O_WRONLY, F_SETPIPE_SZ, 8 * size) < 0) {
            perror("pirate_fcntl1");
            return 1;
        }
        if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
            perror("clock_gettime");
            return 1;
        }
        for (i = 0; i < count; i++) {
            if (pirate_write(1, buf, size) < size) {
                perror("pirate_write");
                return 1;
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

}