#include "libpirate.h"
#include "libpirate_internal.h"

#include <errno.h>
#include <poll.h>
#include <stdlib.h>

struct pollfd gaps_pollfd[PIRATE_NUM_CHANNELS];
nfds_t gaps_nfds = 0;

extern int gaps_reader_gds[PIRATE_NUM_CHANNELS];
extern int gaps_reader_gds_num;

extern char* gaps_enclave_names_sorted[PIRATE_NUM_ENCLAVES];
extern int gaps_writer_control_gds[PIRATE_NUM_ENCLAVES];

static int pirate_cooperative_listen_setup() {
    for (int i = 0; i < gaps_reader_gds_num; i++) {
        int fd = pirate_get_fd(gaps_reader_gds[i]);
        if (fd < 0) {
            return fd;
        }
        gaps_pollfd[i].fd = fd;
        gaps_pollfd[i].events = POLLIN;
    }
    gaps_nfds = gaps_reader_gds_num;
    return 0;
}

int pirate_listen() {
    int rv;
    nfds_t ready;
    pirate_channel_param_t *param;

    if (gaps_nfds == 0) {
        rv = pirate_cooperative_listen_setup();
        if (rv < 0) {
            return rv;
        }
    }
    if (gaps_nfds == 0) {
        return 0;
    }
    for (;;) {
        rv = poll(gaps_pollfd, gaps_nfds, -1);
        if (rv < 0) {
            return rv;
        }
        ready = gaps_nfds;
        for (nfds_t i = 0; i < gaps_nfds; i++) {
            if (gaps_pollfd[i].revents == POLLIN) {
                ready = i;
                break;
            }
        }
        if (ready == gaps_nfds) {
            continue;
        }
        int gd = gaps_reader_gds[ready];
        param = pirate_get_channel_param_ref(gd);
        if (param == NULL) {
            errno = EBADF;
            return -1;
        }
        if (param->control) {
            unsigned char ctrl = 0;
            // consume the control message and resume execution
            pirate_read(gd, &ctrl, sizeof(ctrl));
            return 0;
        } else {
            errno = ENOSYS;
            return -1;
        }
    }
}

static int pirate_yield_id(size_t enclave_id) {
    uint8_t msg = 0;
    int gd = gaps_writer_control_gds[enclave_id];
    ssize_t rv = pirate_write(gd, &msg, sizeof(msg));
    if (rv != sizeof(msg)) {
        return -1;
    }
    return 0;
}

int pirate_yield(const char* enclave) {
    char **tgt = (char**) bsearch(&enclave, gaps_enclave_names_sorted,
        PIRATE_NUM_ENCLAVES, sizeof(char*), pirate_enclave_cmpfunc);
    if (tgt == NULL) {
        errno = EINVAL;
        return -1;
    }
    size_t index = (tgt - gaps_enclave_names_sorted);
    return pirate_yield_id(index);
}
