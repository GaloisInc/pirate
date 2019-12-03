#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

int main() {
        int fd;
        char *shmem;

        fd = open("/dev/uio0", O_RDWR|O_SYNC);

        shmem = mmap(NULL, getpagesize(), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

        printf("%s\n", shmem);

        return 0;
}

