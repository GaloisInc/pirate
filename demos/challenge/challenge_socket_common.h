#ifndef _CHALLENGE_SOCKET_COMMON_H
#define _CHALLENGE_SOCKET_COMMON_H

#include <unistd.h>

int reader_open(int port);
int writer_open(int port);
void test_rv(ssize_t rv, ssize_t nbytes, const char *msg);

#endif // _CHALLENGE_SOCKET_COMMON_H
