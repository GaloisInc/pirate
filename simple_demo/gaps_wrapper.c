#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define USAGE(name) fprintf(stderr, "%s [high|low] <args ...>\n", name)

/* Symbols from the linked binary blobs */
extern uint8_t _binary_high_start;
extern uint8_t _binary_high_end;
extern uint8_t _binary_low_start;
extern uint8_t _binary_low_end;

static int run_bin_blob(const char* name, uint8_t* start, uint8_t* end, char* argv[]) {
    long len = end - start;

    int fd = open(name, O_CREAT|O_RDWR|O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
    if (fd == -1) {
        perror("Failed to create mapped file");
        return -1;
    }

    if (ftruncate(fd, len) != 0) {
        perror("Failed to set size of the mapped file");
        return -1;
    }

    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("Failed to stat the mapped file");
        return -1;
    }

    if (st.st_size != len) {
        fprintf(stderr, "Mapped file size mismatch %lu != %lu\n", len, st.st_size);
        return -1;
    }

    write(fd, start, st.st_size);
    close(fd);

    if (execv(name, argv) == -1) {
        perror("Failed to start the program");
        return -1;
    }

    /* Should never get here */
    return -1;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        USAGE(argv[0]);
        return -1;
    }

    if (strcmp(argv[1], "high") == 0) {
        return run_bin_blob("high_elf", &_binary_high_start, &_binary_high_end, &argv[1]);
    } else if (strcmp(argv[1], "low") == 0) {
        return run_bin_blob("low_elf", &_binary_low_start, &_binary_low_end, &argv[1]);
    } else {
        USAGE(argv[0]);
        return -1;
    }

    /* Should never get here */
    return -1;
}
