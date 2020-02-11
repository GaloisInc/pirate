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

#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/queue.h>
#include "gaps_packet.h"
#include "common.h"
#include "ts_crypto.h"

#ifdef GAPS_ENABLE
#pragma enclave declare(yellow)
#endif

/* Default values */
#define DEFAULT_POLL_PERIOD_MS      1000
#define DEFAULT_REQUEST_QUEUE_LEN   4

typedef struct proxy_request_entry_s {
    proxy_request_t req;
    uint32_t simulated;
    STAILQ_ENTRY(proxy_request_entry_s) entry;
} proxy_request_entry_t;

typedef struct {
    pthread_mutex_t lock;
    STAILQ_HEAD(queuehead, proxy_request_entry_s) head;
} request_queue_t;


typedef struct {
    uint32_t poll_period_ms;
    uint32_t queue_len;
    verbosity_t verbosity;

    gaps_app_t app;

    struct {
        request_queue_t free;
        request_queue_t req;
    } queue;
} proxy_t;

/* Command-line options */
extern const char *argp_program_version;
static struct argp_option options[] = {
    { "period",    'p', "MS",  0, "Request polling period",      0 },
    { "queue-len", 'q', "LEN", 0, "Request queue length",        0 },
    { "verbose",   'v', NULL,  0, "Increase verbosity level",    0 },
    { 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    proxy_t *proxy = (proxy_t *) state->input;

    switch (key) {

    case 'p':
        proxy->poll_period_ms = strtol(arg, NULL, 10);
        break;

    case 'q':
        proxy->queue_len = strtol(arg, NULL, 10);
        break;

    case 'v':
        if (proxy->verbosity < VERBOSITY_MAX) {
            proxy->verbosity++;
        }
        break;

    default:
        break;

    }

    return 0;
}

static void parse_args(int argc, char *argv[], proxy_t *proxy) {
    struct argp argp  = {
        .options = options,
        .parser = parse_opt,
        .args_doc = NULL,
        .doc = "Proxy between the client and timestamp signing service",
        .children = NULL,
        .help_filter = NULL,
        .argp_domain = NULL
    };

    argp_parse(&argp, argc, argv, 0, 0, proxy);
}

/* Push an intem on a queue */
static void request_queue_push(request_queue_t *queue,
    proxy_request_entry_t *entry) {
    pthread_mutex_lock(&queue->lock);
    STAILQ_INSERT_TAIL(&queue->head, entry, entry);
    pthread_mutex_unlock(&queue->lock);
}

/* Pop an item from a queue */
static proxy_request_entry_t *request_queue_pop(request_queue_t *queue) {
    proxy_request_entry_t *entry = NULL;
    pthread_mutex_lock(&queue->lock);
    entry = STAILQ_FIRST(&queue->head);
    if (entry != NULL) {
        STAILQ_REMOVE_HEAD(&queue->head, entry);
    }
    pthread_mutex_unlock(&queue->lock);
    return entry;
}

/* Initialize a request queue */
static int request_queue_init(request_queue_t *queue, uint32_t num) {
    STAILQ_INIT(&queue->head);
    pthread_mutex_init(&queue->lock, NULL);

    /* Allocate queue entries */
    for (uint32_t i = 0; i < num; i++) {
        proxy_request_entry_t *entry = (proxy_request_entry_t*)
            calloc(1, sizeof(proxy_request_entry_t));
        if (entry == NULL) {
            ts_log(ERROR, "Failed to allocate memory for a signing request");
            return -1;
        }

        request_queue_push(queue, entry);
    }

    return 0;
}

/* Cleanup a request queue */
static void request_queue_term(request_queue_t *queue) {
    proxy_request_entry_t *entry = NULL;
    while ((entry = request_queue_pop(queue)) != NULL) {
        free(entry);
    }
    pthread_mutex_destroy(&queue->lock);
}

/* Initialize proxy queues */
static int queues_init(proxy_t *proxy) {
    /* Initialize request queues */
    if ((request_queue_init(&proxy->queue.free, proxy->queue_len) != 0) ||
        (request_queue_init(&proxy->queue.req, 0) != 0)) {
        ts_log(ERROR, "Failed to initialize request queues");
        return -1;
    }

    return 0;
}

/* Release proxy queues */
static void queues_term(proxy_t *proxy) {
    /* Release request queues */
    request_queue_term(&proxy->queue.free);
    request_queue_term(&proxy->queue.req);
}

/* Thread for generating simulated requests */
static void *sim_request_gen(void *argp) {
    proxy_t *proxy = (proxy_t *)argp;

    /* Generate simulated requests at 1/2 * poll period */
    const uint64_t gen_period_ns = proxy->poll_period_ms * (10000000 / 2);
    const struct timespec ts = {
        .tv_sec  = gen_period_ns / 1000000000,
        .tv_nsec = gen_period_ns % 1000000000
    };

    while (gaps_running()) {
        nanosleep(&ts, NULL);

        proxy_request_entry_t *entry = NULL;
        pthread_mutex_lock(&proxy->queue.req.lock);
        entry = STAILQ_FIRST(&proxy->queue.req.head);
        pthread_mutex_unlock(&proxy->queue.req.lock);

        if (entry != NULL) {
            continue;
        }

        /* Generate simulated request and place it on the request queue */
        if ((entry = request_queue_pop(&proxy->queue.free)) == NULL) {
            ts_log(ERROR, "Failed to pop a request entry");
            gaps_terminate();
            continue;
        }

        if (ts_create_request_from_file(NULL, &entry->req) != 0) {
            ts_log(ERROR, "Failed to generate random request data");
            gaps_terminate();
            continue;
        }

        entry->simulated = 1;
        request_queue_push(&proxy->queue.req, entry);
        log_proxy_req(proxy->verbosity, "Simulated request added", &entry->req);
    }

    return NULL;
}


/* Sign request receive thread */
static void *request_receive(void *argp) {
    proxy_t *proxy = (proxy_t *) argp;
    ssize_t len;
    proxy_request_t req;
    proxy_request_entry_t *entry = NULL;

    while (gaps_running()) {
        len = gaps_packet_read(CLIENT_TO_PROXY, &req, sizeof(req));
        if (len != sizeof(req)) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to receive request");
                gaps_terminate();
            }
            continue;
        }

        log_proxy_req(proxy->verbosity, "Client request received", &req);

        if ((entry = request_queue_pop(&proxy->queue.free)) == NULL) {
            tsa_response_t rsp = TSA_RESPONSE_INIT;
            rsp.hdr.status = BUSY;

            if (gaps_packet_write(PROXY_TO_CLIENT, &rsp.hdr, sizeof(rsp.hdr)) != 0) {
                if (gaps_running()) {
                    ts_log(ERROR, "Failed to send response header");
                    gaps_terminate();
                }
                continue;
            }

            if (proxy->verbosity >= VERBOSITY_MIN) {
                ts_log(INFO, "BUSY");
            }
        }

        entry->req = req;
        entry->simulated = 0;
        request_queue_push(&proxy->queue.req, entry);
    }

    return NULL;
}


/* Proxy request polling and signing */
static void *proxy_thread(void *arg) {
    proxy_t *proxy = (void *)arg;
    int sts = -1;
    ssize_t len = 0;
    tsa_request_t req = TSA_REQUEST_INIT;
    tsa_response_t rsp = TSA_RESPONSE_INIT;
    proxy_request_entry_t *entry = NULL;

    const struct timespec ts = {
        .tv_sec  = proxy->poll_period_ms / 1000,
        .tv_nsec = (proxy->poll_period_ms % 1000) * 1000000
    };

    while (gaps_running()) {
        /* Periodically poll */
        nanosleep(&ts, NULL);

        /* Get a request, there always should be one */
        if ((entry = request_queue_pop(&proxy->queue.req)) == NULL) {
            ts_log(ERROR, "Request queue empty");
            gaps_terminate();
            continue;
        }
        log_proxy_req(proxy->verbosity, "Processing next request", &entry->req);

        /* Use the timestamp service to sign */
        if (ts_create_query(&entry->req, &req) != 0) {
            ts_log(ERROR, "Failed to generate timestamp sign query");
            gaps_terminate();
            continue;
        }
        log_tsa_req(proxy->verbosity, "Timestamp request sent", &req);

        request_queue_push(&proxy->queue.free, entry);
        sts = gaps_packet_write(PROXY_TO_SIGNER, &req, sizeof(req));
        if (sts != 0) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to send timestamp request");
                gaps_terminate();
            }
            continue;
        }

        len = gaps_packet_read(SIGNER_TO_PROXY, &rsp.hdr, sizeof(rsp.hdr));
        if (len != sizeof(rsp.hdr)) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to receive timestamp response header");
                gaps_terminate();
            }
            continue;
        }
        len = gaps_packet_read(SIGNER_TO_PROXY, &rsp.ts, rsp.hdr.len);
        if (len != rsp.hdr.len) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to receive timestamp response body");
                gaps_terminate();
            }
            continue;
        }
        log_tsa_rsp(proxy->verbosity, "Timestamp response received", &rsp);

        if (entry->simulated != 0) {
            continue;
        }

        sts = gaps_packet_write(PROXY_TO_CLIENT, &rsp.hdr, sizeof(rsp.hdr));
        if (sts != 0) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to send response header");
                gaps_terminate();
            }
            continue;
        }

        sts = gaps_packet_write(PROXY_TO_CLIENT, &rsp.ts, rsp.hdr.len);
        if (sts != 0) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to send response body");
                gaps_terminate();
            }
            continue;
        }

        log_tsa_rsp(proxy->verbosity, "Timestamp response sent to client",
            &rsp);
    }

    return NULL;
}


int signing_proxy_main(int argc, char *argv[]) GAPS_ENCLAVE_MAIN("yellow") {
    proxy_t proxy = {
        .poll_period_ms = DEFAULT_POLL_PERIOD_MS,
        .queue_len = DEFAULT_REQUEST_QUEUE_LEN,
        .verbosity = VERBOSITY_NONE,

        .app = {
            .threads = {
                THREAD_ADD(sim_request_gen, &proxy, "sim_request"),
                THREAD_ADD(request_receive, &proxy, "rx_request"),
                THREAD_ADD(proxy_thread, &proxy, "proxy"),
            },
            .on_shutdown = NULL,
            .ch = {
                GAPS_CHANNEL(CLIENT_TO_PROXY, O_RDONLY, PIPE, NULL,
                    "client->proxy"),
                GAPS_CHANNEL(PROXY_TO_CLIENT, O_WRONLY, PIPE, NULL,
                    "client<-proxy"),
#ifdef GAPS_SERIAL
                GAPS_CHANNEL(PROXY_TO_SIGNER, O_WRONLY, SERIAL,
                    PROXY_TO_SIGNER_WR, "proxy->signer"),
                GAPS_CHANNEL(SIGNER_TO_PROXY, O_RDONLY, SERIAL,
                    SIGNER_TO_PROXY_RD, "proxy<-signer")
#else
                GAPS_CHANNEL(PROXY_TO_SIGNER, O_WRONLY, PIPE, NULL,
                    "proxy->signer"),
                GAPS_CHANNEL(SIGNER_TO_PROXY, O_RDONLY, PIPE, NULL,
                    "proxy<-signer")
#endif
            }
        }
    };

    /* Parse command-line options */
    parse_args(argc, argv, &proxy);

    ts_log(INFO, "Starting signing proxy");

    if (queues_init(&proxy) != 0) {
        ts_log(ERROR, "Failed to initialize proxy queues");
        return -1;
    }

    if (gaps_app_run(&proxy.app) != 0) {
        ts_log(ERROR, "Failed to start the signing proxy");
        return -1;
    }

    /* Cleanup */
    int rv = gaps_app_wait_exit(&proxy.app);
    queues_term(&proxy);
    return rv;
}

#ifndef GAPS_ENABLE
int main(int argc, char *argv[]) {
    return signing_proxy_main(argc, argv);
}
#endif
