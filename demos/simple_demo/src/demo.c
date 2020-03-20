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

#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <unistd.h>

#include "libpirate.h"
#include "tiny.h"

#ifdef GAPS_DISABLE
#define GAPS_MAIN(name)
#else
#ifndef __GAPS__
#error "gaps compiler must be used"
#endif
#pragma pirate enclave declare(high)
#pragma pirate enclave declare(low)
#define GAPS_MAIN(name) __attribute__((pirate_enclave_main(name)))
#endif

int high_to_low, low_to_high;

#define DATA_LEN            (32 << 10)         // 32 KB
typedef struct {
    char buf[DATA_LEN];
    int len;
} data_t;

typedef struct {
    int rv;
    pthread_t tid;
} pthread_alloc_t;

typedef enum {
    LEVEL_HIGH,
    LEVEL_LOW
} level_e;

typedef struct {
    level_e level;
    short port;
} webserver_t;

static volatile sig_atomic_t terminated = 0;

#define LOW_NAME  "\033[1;32mLOW\033[0m"
#define HIGH_NAME "\033[1;31mHIGH\033[0m"
static const char *NAME = NULL;

// Register an empty signal handler for SIGUSR1.
// Replaces the default action for SIGUSR1
// which is to terminate the process.
// This application uses SIGUSR1 to wakeup threads
// that are blocked on system calls.
static void siguser_handler(int signo) {
    (void)signo;
}

static int load_web_content_high(data_t* data, const char *path) {
    struct stat sbuf;

    if (stat(path, &sbuf) < 0) {
        fprintf(stderr, "ERROR: could not find file %s\n", path);
        return 404;
    }

    if (sbuf.st_size >= (long)sizeof(data->buf)) {
        fprintf(stderr, "ERROR: file %s exceeds size limits\n", path);
        return 500;
    }

    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR failed to open %s\n", path);
        return 500;
    }

    if (fread(data->buf, 1, sbuf.st_size, fp) != (size_t) sbuf.st_size) {
        fprintf(stderr, "Failed to read %s file\n", path);
        return 500;
    }

    fclose(fp);

    data->buf[sbuf.st_size] = '\0';
    data->len = sbuf.st_size;
    return 200;
}

static int load_web_content_low(data_t* data, char *path) {
    int rv, len;
    ssize_t num;

    len = strnlen(path, PATHSIZE);
    num = pirate_write(low_to_high, &len, sizeof(int));
    if (num != sizeof(int)) {
        fprintf(stderr, "Failed to send request length\n");
        return 500;
    }

    num = pirate_write(low_to_high, path, len);
    if (num != len) {
        fprintf(stderr, "Failed to send request path\n");
        return 500;
    }

    fputs("Sent read request to the HIGH_NAME side\n", stdout);

    num = pirate_read(high_to_low, &rv, sizeof(rv));
    if (num != sizeof(rv)) {
        fprintf(stderr, "Failed to receive status code\n");
        return 500;
    }
    if (rv != 200) {
        return rv;
    }

    /* Read and validate response length */
    num = pirate_read(high_to_low, &len, sizeof(len));
    if (num != sizeof(len)) {
        fprintf(stderr, "Failed to receive response length\n");
        return 500;
    }

    if (len >= (long)sizeof(data->buf)) {
        fprintf(stderr, "Response length %d is too large\n", len);
        return 500;
    }

    /* Read back the response */
    num = pirate_read(high_to_low, data->buf, len);
    if (num != len) {
        fprintf(stderr, "Failed to read back the response\n");
        return 500;
    }

    /* Success */
    data->len = len;
    printf("Received %d bytes from the %s side\n", data->len, HIGH_NAME);
    return 200;
}

static int load_web_content(data_t* data, char *path, level_e level) {
    switch (level) {
        case LEVEL_HIGH:
            return load_web_content_high(data, path);

        case LEVEL_LOW:
            return load_web_content_low(data, path);

        default:
            return 500;
    }
}

static void terminate() {
    terminated = 1;
    kill(getpid(), SIGTERM);
}

static void* gaps_thread(void *arg) {
    (void)arg;
    char *begin, *end;
    data_t data;

    memset(data.buf, 0, sizeof(data.buf));

    while (!terminated) {
        /* Low side requests data by writing zero */
        int rv, len = 0;
        char path[PATHSIZE];

        ssize_t num = pirate_read(low_to_high, &len, sizeof(len));
        if (num != sizeof(len)) {
            if (!terminated) {
                fprintf(stderr, "Failed to read request from the low side\n");
                terminate();
            }
            continue;
        }

        if ((len <= 0) || (len >= (long)sizeof(path))) {
            fprintf(stderr, "Invalid request length from the low side %d\n", len);
            terminate();
            continue;
        }

        memset(path, 0, sizeof(path));
        num = pirate_read(low_to_high, &path, len);
        if (num != len) {
            fprintf(stderr, "Invalid request path from the low side %d\n", len);
            terminate();
            continue;
        }

        fputs("Received data request from the LOW_NAME side\n", stdout);

        /* Read in high data */
        rv = load_web_content_high(&data, path);
        num = pirate_write(high_to_low, &rv, sizeof(rv));
        if (num != sizeof(rv)) {
            fprintf(stderr, "Failed to send status code\n");
            terminate();
            continue;
        }
        if (rv != 200) {
            continue;
        }

        /* Create filtered data (remove bold text) */
        begin = strstr(data.buf, "<b>");
        while (begin != NULL) {
            char *last = data.buf + data.len;
            end = strstr(begin, "</b>");
            if (end == NULL) {
                *begin = (char) 0;
                data.len -= last - begin;
                break;
            } else {
                char *tail = end + 4;
                memmove(begin, tail, last - tail);
                data.len -= tail - begin;
            }
            begin = strstr(begin, "<b>");
        }

        /* Reply back. Data length is sent first */
        num = pirate_write(high_to_low, &data.len, sizeof(data.len));
        if (num != sizeof(data.len)) {
            fprintf(stderr, "Failed to send response length\n");
            terminate();
            continue;
        }

        num = pirate_write(high_to_low, &data.buf, data.len);
        if (num != data.len) {
            fprintf(stderr, "Failed to send response content\n");
            terminate();
            continue;
        }

        printf("Sent %d bytes to the %s side\n\n", data.len, LOW_NAME);
    }
    return NULL;
}

static void* webserver_thread(void *arg) {
    int rv;
    char *err;
    short port;
    level_e level;
    server_t si;
    client_t ci;
    request_t ri;
    data_t data;
    webserver_t *webargs;

    webargs = (webserver_t*) arg;
    port = webargs->port;
    level = webargs->level;
    memset(ri.buf, 0, BUFSIZE);
    /* Create, initialize, bind, listen on server socket */
    err = server_connect(&si, port);
    if (err != NULL) {
        perror(err);
        terminate();
        return NULL;
    }

    /*
     * Wait for a connection request, parse HTTP, serve high requested content,
     * close connection.
     */
    while (!terminated) {
        /* accept client's connection and open fstream */
        err = client_connect(&si, &ci);
        if (err != NULL) {
            if (!terminated) {
                perror(err);
            }
            continue;
        }

        /* process client request */
        client_request_info(&ci, &ri);

        /* tiny only supports the GET method */
        if (strncasecmp(ri.method, "GET", 4)) {
            cerror(ci.stream, ri.method, 405, "Not Implemented");
            client_disconnect(&ci);
            continue;
        }

        /* Get data from the high side */
        rv = load_web_content(&data, ri.filename, level);
        if (rv != 200) {
            cerror(ci.stream, ri.filename, rv, "Loading Data");
            client_disconnect(&ci);
            continue;
        }

        /* Serve low content */
        rv = serve_static_content(&ci, &ri, data.buf, data.len);
        if  (rv < 0) {
            client_disconnect(&ci);
            continue;
        }
        printf("%s served %d bytes of web content\n", NAME, data.len);

        err = client_disconnect(&ci);
        if (err != NULL) {
            perror(err);
        }
    }

    err = server_disconnect(&si);
    if (err != NULL) {
        perror(err);
    }

    return NULL;
}

static pthread_alloc_t run_gaps() {
    pthread_alloc_t ret;
    ret.rv = pthread_create(&ret.tid, NULL, gaps_thread, NULL);
    return ret;
}

static pthread_alloc_t run_webserver(webserver_t *webargs) {
    pthread_alloc_t ret;
    ret.rv = pthread_create(&ret.tid, NULL, webserver_thread, (void*) webargs);
    return ret;
}

int main_high(int argc, char* argv[]) GAPS_MAIN("high")
{
    NAME = HIGH_NAME;
    int retval = 0, signal_fd;
    sigset_t mask;
    pthread_alloc_t gaps, webserver;
    webserver_t webargs;
    struct sigaction saction;
    struct signalfd_siginfo siginfo;
    pirate_channel_param_t param;

    /* Validate and parse command-line options */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return -1;
    }

    // Block the SIGINT and SIGTERM signals in the parent thread.
    // A new thread inherits a copy of its creator's signal mask.
    // The main thread will wait on signalfd() to handle
    // SIGINT and SIGTERM signals.
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) < 0) {
        perror("sigprocmask");
        return -1;
    }

    // Register the empty signal handler for SIGUSR1
    // This application uses SIGUSR1 to wakeup threads
    // that are blocked on system calls.
    memset(&saction, 0, sizeof(struct sigaction));
    saction.sa_handler = &siguser_handler;
    if (sigaction(SIGUSR1, &saction, NULL) == -1) {
        perror("sigaction");
        return -1;
    }

    signal_fd = signalfd(-1, &mask, 0);
    if (signal_fd < 0) {
        perror ("signalfd");
        return -1;
    }

    webargs.port = atoi(argv[1]);
    webargs.level = LEVEL_HIGH;
    printf("\n%s web server on port %d\n\n", NAME, webargs.port);

    pirate_init_channel_param(PIPE, &param);

    high_to_low = pirate_open_param(&param, O_WRONLY);
    if (high_to_low < 0) {
        perror("open high to low channel in write-only mode");
        return -1;
    }

    low_to_high = pirate_open_param(&param, O_RDONLY);
    if (low_to_high < 0) {
        perror("open low to high channel in read-only mode");
        return -1;
    }

    gaps = run_gaps();
    if (gaps.rv) {
        perror("pthread_create gaps thread");
        return -1;
    }

    webserver = run_webserver(&webargs);
    if (webserver.rv) {
        perror("pthread_create webserver thread");
        return -1;
    }

    if (read(signal_fd, &siginfo, sizeof(siginfo)) < 0) {
        perror("read signal file descriptor");
        retval = -1;
    }

    terminated = 1;

    if (pthread_kill(gaps.tid, SIGUSR1) < 0) {
        perror("pthread_kill gaps thread");
        retval = -1;
    }
    if (pthread_kill(webserver.tid, SIGUSR1) < 0) {
        perror("pthread_kill webserver thread");
        retval = -1;
    }
    if (pthread_join(gaps.tid, NULL) < 0) {
        perror("pthread_join gaps thread");
        retval = -1;
    }
    if (pthread_join(webserver.tid, NULL) < 0) {
        perror("pthread_join webserver thread");
        retval = -1;
    }

    if (pirate_close(high_to_low) < 0) {
        perror("close high to low channel in write-only mode");
        retval = -1;
    }
    if (pirate_close(low_to_high) < 0) {
        perror("close low to high channel in read-only mode");
        retval = -1;
    }

    return retval;
}


int main_low(int argc, char* argv[]) GAPS_MAIN("low")
{
    NAME = LOW_NAME;
    int retval = 0, signal_fd;
    sigset_t mask;
    pthread_alloc_t webserver;
    webserver_t webargs;
    struct sigaction saction;
    struct signalfd_siginfo siginfo;
    pirate_channel_param_t param;

    /* Validate and parse command-line options */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return -1;
    }

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) < 0) {
        perror("sigprocmask");
        return -1;
    }

    memset(&saction, 0, sizeof(struct sigaction));
    saction.sa_handler = &siguser_handler;
    if (sigaction(SIGUSR1, &saction, NULL) == -1) {
        perror("sigaction");
        return -1;
    }

    signal_fd = signalfd(-1, &mask, 0);
    if (signal_fd < 0) {
        perror("signalfd");
        return -1;
    }

    webargs.port = atoi(argv[1]);
    webargs.level = LEVEL_LOW;
    printf("\n%s web server on port %d\n\n", NAME, webargs.port);


    pirate_init_channel_param(PIPE, &param);

    high_to_low = pirate_open_param(&param, O_RDONLY);
    if (high_to_low < 0) {
        perror("open high to low channel in read-only mode");
        return -1;
    }
    low_to_high = pirate_open_param(&param, O_WRONLY);
    if (low_to_high < 0) {
        perror("open low to high channel in write-only mode");
        return -1;
    }

    webserver = run_webserver(&webargs);
    if (webserver.rv) {
        perror("pthread_create webserver thread");
        return -1;
    }

    if (read(signal_fd, &siginfo, sizeof(siginfo)) < 0) {
        perror("read signal file descriptor");
        retval = -1;
    }

    terminated = 1;

    if (pthread_kill(webserver.tid, SIGUSR1) < 0) {
        perror("pthread_kill webserver thread");
        retval = -1;
    }
    if (pthread_join(webserver.tid, NULL) < 0) {
        perror("pthread_join webserver thread");
        retval = -1;
    }

    if (pirate_close(high_to_low) < 0) {
        perror("close high to low channel in read-only mode");
        retval = -1;
    }
    if (pirate_close(low_to_high) < 0) {
        perror("close low to high channel in write-only mode");
        retval = -1;
    }

    return retval;
}


#ifdef GAPS_DISABLE
int main(int argc, char* argv[]) {
#ifdef HIGH
    return main_high(argc, argv);
#elif LOW
    return main_low(argc, argv);
#else
    #error "Must specify application type"
#endif
}
#endif
