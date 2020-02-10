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
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/signalfd.h>
#include "common.h"
#include "ts_crypto.h"

const char *argp_program_version = DEMO_VERSION;

volatile sig_atomic_t terminated = 0;

// Register an empty signal handler for SIGUSR1.
// Replaces the default action for SIGUSR1
// which is to terminate the process.
// This application uses SIGUSR1 to wakeup threads
// that are blocked on system calls.
static void siguser_handler(int signo) {
    (void)signo;
}

int gaps_app_run(gaps_app_t *ctx) {
    sigset_t mask;
    struct sigaction saction;

    // Block the SIGINT and SIGTERM signals in the parent thread.
    // A new thread inherits a copy of its creator's signal mask.
    // The main thread will wait on signalfd() to handle
    // SIGINT and SIGTERM signals.
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    if (pthread_sigmask(SIG_BLOCK, &mask, NULL) < 0) {
        ts_log(ERROR, "sigprocmask failed");
        return -1;
    }

    // Register the empty signal handler for SIGUSR1
    // This application uses SIGUSR1 to wakeup threads
    // that are blocked on system calls.
    memset(&saction, 0, sizeof(struct sigaction));
    saction.sa_handler = &siguser_handler;
    if (sigaction(SIGUSR1, &saction, NULL) == -1) {
        ts_log(ERROR, "sigaction failed");
        return -1;
    }

    ctx->signal_fd = signalfd(-1, &mask, 0);
    if (ctx->signal_fd < 0) {
        ts_log(ERROR, "signalfd failed");
        return -1;
    }

    // Initialize and open GAPS channels
    for (int i = 0; i < MAX_APP_GAPS_CHANNELS; i++) {
        gaps_channel_ctx_t *c = &ctx->ch[i];
        if (c->num == -1) {
            break;
        }

        if (pirate_set_channel_type(c->num, c->type)) {
            ts_log(ERROR, "Failed to set channel type for %s", c->desc);
            return -1;
        }

        if (pirate_set_pathname(c->num, c->path)) {
            ts_log(ERROR, "Failed to set path %s for %u", c->path, c->num);
            return -1;
        }

        if (pirate_open(c->num, c->flags) != c->num) {
            ts_log(ERROR, "Failed to open %s for %s", c->path, c->desc);
            return -1;
        }
    }

    // Start threads
    for (int i = 0; i < MAX_APP_THREADS; i++) {
        thread_ctx_t *t = &ctx->threads[i];
        if (t->func == NULL) {
            break;
        }

        if (pthread_create(&t->tid, NULL, t->func, t->arg) != 0) {
            ts_log(ERROR, "Failed to start '%s' thread", t->name);
            return -1;
        }

        if (pthread_setname_np(t->tid, t->name) != 0) {
            ts_log(ERROR, "Failed to set thread name '%s'", t->name);
            return -1;
        }
    }

    return 0;
}

int gaps_app_wait_exit(gaps_app_t *ctx) {
    int rv = 0;
    struct signalfd_siginfo siginfo;

    if (read(ctx->signal_fd, &siginfo, sizeof(siginfo)) < 0) {
        ts_log(ERROR, "read signal file descriptor");
        rv = -1;
    }

    gaps_terminate();

    // Close GAPS channels
    for (int i = 0; i < MAX_APP_GAPS_CHANNELS; i++) {
        gaps_channel_ctx_t *c = &ctx->ch[i];
        if (c->num == -1) {
            break;
        }

        pirate_close(c->num, c->flags);
    }

    // Stop worker threads
    for (int i = 0; i < MAX_APP_THREADS; i++) {
        thread_ctx_t *t = &ctx->threads[i];
        if (t->func == NULL) {
            break;
        }

        if (pthread_kill(t->tid, SIGUSR1) < 0) {
            ts_log(ERROR, "pthread_kill '%s'", t->name);
            rv = -1;
        }
    }

    for (int i = 0; i < MAX_APP_THREADS; i++) {
        thread_ctx_t *t = &ctx->threads[i];
        if (t->func == NULL) {
            break;
        }

        if (pthread_join(t->tid, NULL) < 0) {
            ts_log(ERROR, "pthread_join '%s'", t->name);
            rv = -1;
        }
    }

    return rv;
}

void gaps_terminate() {
    terminated = 1;
    kill(getpid(), SIGTERM);
}

int gaps_running() {
    return terminated == 0;
}

void ts_log(log_level_t l, const char *fmt, ...) {
    FILE *stream = NULL;
    char *msg = NULL;
    size_t len = 0;
    va_list args;

    struct timeval tv;
    struct tm *now_tm;
    char ts[64];
    int off;

    const char *evt_str = l == INFO ? BCLR(WHITE, "INFO") :
                          l == WARN ? BCLR(YELLOW, "WARN") : BCLR(RED, "ERR ");

    if (gettimeofday(&tv, NULL) != 0) {
        ts_log(ERROR, "Failed to get the current time");
        return;
    }

    if ((now_tm = localtime(&tv.tv_sec)) == NULL) {
        ts_log(ERROR, "Failed to convert time");
        return;
    }

    off = strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", now_tm);
    snprintf(ts + off, sizeof(ts) - off, ".%06ld", tv.tv_usec);

    if ((stream = open_memstream(&msg, &len)) == NULL) {
        ts_log(ERROR, "Failed to create output stream");
        return;
    }


    fprintf(stream, "[%s] %s ", ts, evt_str);
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);

    if ((l != INFO) && (errno != OK)) {
        fprintf(stream, " (%s)", strerror(errno));
    }

    fflush(stream);
    fclose(stream);
    puts(msg);
    free(msg);
}

const char *ts_status_str(ts_status_t sts) {
    static const char *ret = "UNKNOWN";

    switch (sts) {
    case OK:
        ret = BCLR(GREEN, "OK");
        break;
    case BUSY:
        ret = BCLR(YELLOW, "BUSY");
        break;
    case ERR:
        ret = BCLR(RED, "ERROR");
        break;
    case UNKNOWN:
    default:
        break;
    }

    return ret;
}

void log_proxy_req(verbosity_t v, const char* msg, const proxy_request_t *req) {
    if (v >= VERBOSITY_MIN) {
        ts_log(INFO, BCLR(CYAN, "%s"), msg);
        if (v >= VERBOSITY_MAX) {
            char *msg = NULL;
            size_t len = 0;
            FILE *stream = open_memstream(&msg, &len);
            if (stream == NULL) {
                ts_log(INFO, "Failed to open memory stream");
                return;
            }
            fprintf(stream, "%s", CLR(CYAN, "Proxy request:\n"));
            ts_print_proxy_req(stream, req);
            fflush(stream);
            fclose(stream);
            puts(msg);
            free(msg);
        }
    }
}

void log_tsa_req(verbosity_t v, const char* msg, const tsa_request_t *req) {
    if (v >= VERBOSITY_MIN) {
        ts_log(INFO, BCLR(BLUE, "%s"), msg);
        if (v >= VERBOSITY_MAX) {
            char *msg = NULL;
            size_t len = 0;
            FILE *stream = open_memstream(&msg, &len);
            if (stream == NULL) {
                ts_log(INFO, "Failed to open memory stream");
                return;
            }
            fprintf(stream, "%s", CLR(BLUE, "Timestamp request:\n"));
            ts_print_tsa_req(stream, req);
            fflush(stream);
            fclose(stream);
            puts(msg);
            free(msg);
        }
    }
}

void log_tsa_rsp(verbosity_t v, const char* msg, const tsa_response_t* rsp) {
    if (v >= VERBOSITY_MIN) {
        ts_log(INFO, BCLR(MAGENTA, "%s : status %s"), msg,
            ts_status_str(rsp->hdr.status));
        if (v >= VERBOSITY_MAX) {
            char *msg = NULL;
            size_t len = 0;
            FILE *stream = open_memstream(&msg, &len);
            if (stream == NULL) {
                ts_log(INFO, "Failed to open memory stream");
                return;
            }
            fprintf(stream, "%s", CLR(MAGENTA, "Timestamp response:\n"));
            ts_print_tsa_rsp(stream, rsp);
            fflush(stream);
            fclose(stream);
            puts(msg);
            free(msg);
        }
    }
}
