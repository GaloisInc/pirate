#include <errno.h>

int pirate_listen() {
    errno = ENOSYS;
    return -1;
}

int pirate_yield(int enclave_id) {
    (void) enclave_id;
    errno = ENOSYS;
    return -1;
}
