#include <poll.h>
#include "libpirate.h"
#include "channel.h"

void GreenListener::listen() {
    struct pollfd fds[4];
    int fd;
    nfds_t ready = 4, nfds = 4;
    
    fds[0].fd = _uavFd;
    fds[1].fd = _rfFd;
    fds[2].fd = _gpsFd;
    fds[3].fd = _readCtrlFd;
    fds[0].events = POLLIN;
    fds[1].events = POLLIN;
    fds[2].events = POLLIN;
    fds[3].events = POLLIN;
    
    while (1) {
        int rv = poll(fds, nfds, -1);
        if (rv < 0) {
            perror("poll error");
            exit(-1);
        }
        for (nfds_t i = 0; i < nfds; i++) {
            if (fds[i].revents == POLLIN) {
                ready = i;
                break;
            }
        }
        if (ready != nfds) {
            unsigned char ctrl = 0;
            fd = fds[ready].fd;
            if (fd == _uavFd) {
                Position position;
                pirate_read(_uavGd, &position, sizeof(Position));
                _uavFunc(position);
                pirate_write(_writeCtrlGd, &ctrl, sizeof(ctrl));
            } else if (fd == _rfFd) {
                Distance distance;
                pirate_read(_rfGd, &distance, sizeof(Distance));
                _rfFunc(distance);
                pirate_write(_writeCtrlGd, &ctrl, sizeof(ctrl));
            } else if (fd == _gpsFd) {
                Position position;
                pirate_read(_gpsGd, &position, sizeof(Position));
                _gpsFunc(position);
                // both ends of this channel have same sensitivity level
                // do not wait for control message from other sensitivity
                return;
                // pirate_write(_writeCtrlGd, &ctrl, sizeof(ctrl));
            } else if (fd == _readCtrlFd) {
                // consume the control message and resume execution
                pirate_read(_readCtrlGd, &ctrl, sizeof(ctrl));
                return;
            }
        }
    }
}

void OrangeListener::listen() {
    struct pollfd fds[2];
    int fd;
    nfds_t ready = 2, nfds = 2;
    
    fds[0].fd = _gpsFd;
    fds[1].fd = _readCtrlFd;
    fds[0].events = POLLIN;
    fds[1].events = POLLIN;

    while (1) {
        int rv = poll(fds, nfds, -1);
        if (rv < 0) {
            perror("poll error");
            exit(-1);
        }
        for (nfds_t i = 0; i < nfds; i++) {
            if (fds[i].revents == POLLIN) {
                ready = i;
                break;
            }
        }
        if (ready != nfds) {
            unsigned char ctrl = 0;
            fd = fds[ready].fd;
            if (fd == _gpsFd) {
                Position position;
                pirate_read(_gpsGd, &position, sizeof(Position));
                _gpsFunc(position);
                pirate_write(_writeCtrlGd, &ctrl, sizeof(ctrl));
            } else if (fd == _readCtrlFd) {
                // consume the control message and resume execution
                pirate_read(_readCtrlGd, &ctrl, sizeof(ctrl));
                return;
            }
        }
    }
}
