#include <errno.h>

int pirate_listen() {
    errno = ENOSYS;
    return -1;
}

int pirate_yield(const char *enclave) {
    (void) enclave;
    errno = ENOSYS;
    return -1;
}
