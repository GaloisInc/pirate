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
#include <math.h>
#include <stdlib.h>
#include "gaps_packet.h"
#include "ts_crypto.h"
#include "common.h"

#ifdef GAPS_ENABLE
#pragma enclave declare(orange)
#endif

typedef struct {
    verbosity_t verbosity;
    uint32_t validate;
    uint32_t request_delay_ms;
    const char *ca_path;
    const char *cert_path;
    const char *tsr_dir;

    gaps_app_t app;
} client_t;

#pragma pack(1)
typedef struct {
    int32_t x;
    int32_t y;
} xy_sensor_data_t;
#pragma pack()

/* Command-line options */
extern const char *argp_program_version;
static struct argp_option options[] = {
    { "ca_path",   'C', "PATH", 0, "CA path",                          0 },
    { "cert_path", 'S', "PATH", 0, "Signing certificate path",         0 },
    { "verify",    'V', NULL,   0, "Verify timestamp signatures",      0 },
    { "save_path", 'O', "PATH", 0, "TSR output directory",             0 },
    { "req_delay", 'd', "MS",   0, "Request delay in milliseconds",    0 },
    { "verbose",   'v', NULL,   0, "Increase verbosity level",         0 },
    { 0 }
};

#define DEFAULT_TSR_OUT_DIR "."

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    client_t *client = (client_t*) state->input;

    switch (key) {

    case 'C':
        client->ca_path = arg;
        break;

    case 'S':
        client->cert_path = arg;
        break;

    case 'V':
        client->validate = 1;
        break;

    case 'O':
        client->tsr_dir = arg;
        break;

    case 'd':
        client->request_delay_ms = strtol(arg, NULL, 10);
        break;

    case 'v':
        if (client->verbosity < VERBOSITY_MAX) {
            client->verbosity++;
        }
        break;

    default:
        break;
    }

    return 0;
}


static void parse_args(int argc, char *argv[], client_t *client) {
    struct argp argp = {

        .options = options,
        .parser = parse_opt,
        .args_doc = "[FILE] [FILE] ...",
        .doc = "Sign files with the trusted timestamp service",
        .children = NULL,
        .help_filter = NULL,
        .argp_domain = NULL
    };

    argp_parse(&argp, argc, argv, 0, 0, client);
}


/* Generate sensor data */
static void read_xy_sensor(xy_sensor_data_t *d) {
    static const double RADIUS = 100000;
    static const double NOISE = 100;
    static const double PI = 3.14159265;
    static uint32_t idx = 0;

    d->x = RADIUS * cos((idx * PI) / 180) + (NOISE * rand()) / RAND_MAX;
    d->y = RADIUS * sin((idx * PI) / 180) + (NOISE * rand()) / RAND_MAX;
    idx++;
}


/* Save sensor data to a file */
static int save_xy_sensor(const char *dir, uint32_t idx,
    const xy_sensor_data_t *data) {
    int rv = 0;
    char path[PATH_MAX];
    FILE *f_out = NULL;

    if (dir == NULL) {
        return -1;
    }

    snprintf(path, sizeof(path) - 1, "%s/%04u.data", dir, idx);

    if ((f_out = fopen(path, "wb")) == NULL) {
        ts_log(ERROR, "Failed to open sensor output file");
        return -1;
    }

    if (fwrite(data, sizeof(xy_sensor_data_t), 1, f_out) != 1) {
        ts_log(ERROR, "Failed to save sensor content");
        rv = -1;
    }

    fclose(f_out);
    return rv;
}

/* Save timestamp sign response to a file */
static int save_ts_response(const char *dir, uint32_t idx,
    const tsa_response_t* rsp ) {
    int rv = 0;
    char path[PATH_MAX];
    FILE *f_out = NULL;

    if ((dir == NULL) ||
        (rsp->hdr.status != OK) ||
        (rsp->hdr.len > sizeof(rsp->ts))) {
        return 0;
    }

    snprintf(path, sizeof(path) - 1, "%s/%04u.tsr", dir, idx);

    if ((f_out = fopen(path, "wb")) == NULL) {
        ts_log(ERROR, "Failed to open TSR output file");
        return -1;
    }

    if (fwrite(rsp->ts, rsp->hdr.len, 1, f_out) != 1) {
        ts_log(ERROR, "Failed to save TSR content");
        rv = -1;
    }

    fclose(f_out);
    return rv;
}

/* Acquire trusted timestamps */
static void *client_thread(void *arg) {
    client_t *client = (client_t *)arg;
    int idx = 0;

    proxy_request_t req = PROXY_REQUEST_INIT;
    tsa_response_t rsp = TSA_RESPONSE_INIT;

    xy_sensor_data_t data = { 0, 0 };
    const uint32_t data_len = sizeof(data);

    const struct timespec ts = {
        .tv_sec = client->request_delay_ms / 1000,
        .tv_nsec = (client->request_delay_ms % 1000) * 1000000
    };

    while (gaps_running()) {
        /* Read sensor data */
        read_xy_sensor(&data);

        /* Save sensor data */
        if (save_xy_sensor(client->tsr_dir, idx, &data)) {
            ts_log(ERROR, "Failed to save XY sensor data");
            gaps_terminate();
            continue;
        }

        /* Compose a request */
        if (ts_create_request_from_data(&data, data_len, &req) != 0) {
            ts_log(ERROR, "Failed to generate TS request");
            gaps_terminate();
            continue;
        }

        /* Send request */
        int sts = gaps_packet_write(CLIENT_TO_PROXY, &req, sizeof(req));
        if (sts == -1) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to send signing request to proxy");
                gaps_terminate();
            }
            continue;
        }
        log_proxy_req(client->verbosity, "Request sent to proxy", &req);

        /* Get response */
        sts = gaps_packet_read(PROXY_TO_CLIENT, &rsp.hdr, sizeof(rsp.hdr));
        if (sts != sizeof(rsp.hdr)) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to receive response header");
                gaps_terminate();
            }
            continue;
        }
        sts = gaps_packet_read(PROXY_TO_CLIENT, &rsp.ts, rsp.hdr.len);
        if (sts != ((int) rsp.hdr.len)) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to receive response body");
                gaps_terminate();
            }
            continue;
        }
        log_tsa_rsp(client->verbosity, "Timestamp response received", &rsp);

        /* Optionally validate the signature */
        if (client->validate != 0) {
            if (ts_verify_data(&data, data_len, client->ca_path, client->cert_path, &rsp) == 0) {
                ts_log(INFO, BCLR(GREEN, "Timestamp VERIFIED"));
            } else {
                ts_log(WARN, BCLR(RED, "FAILED to validate the timestamp"));
            }
            fflush(stdout);
        }

        /* Save the timestamp signature */
        if (save_ts_response(client->tsr_dir, idx, &rsp) != 0) {
            ts_log(ERROR, "Failed to save timestamp response");
            gaps_terminate();
            continue;
        }

        if (client->request_delay_ms != 0) {
            nanosleep(&ts, NULL);
        }

        idx++;
    }

    return 0;
}

int sensor_manager_main(int argc, char *argv[]) GAPS_ENCLAVE_MAIN("orange") {
    client_t client = {
        .verbosity = VERBOSITY_NONE,
        .validate = 0,
        .request_delay_ms = 0,
        .ca_path = DEFAULT_CA_PATH,
        .tsr_dir = DEFAULT_TSR_OUT_DIR,

        .app = {
            .threads = {
                THREAD_ADD(client_thread, &client, "ts_client"),
                THREAD_END
            },

            .ch = {
                GAPS_CHANNEL(CLIENT_TO_PROXY, O_WRONLY, PIPE, NULL,
                            "client->proxy"),
                GAPS_CHANNEL(PROXY_TO_CLIENT, O_RDONLY, PIPE, NULL,
                            "client<-proxy"),
                GAPS_CHANNEL_END
            }
        }
    };

    parse_args(argc, argv, &client);

    ts_log(INFO, "Starting sensor manager");

    if (gaps_app_run(&client.app) != 0) {
        ts_log(ERROR, "Failed to initialize the timestamp client");
        return -1;
    }

    return gaps_app_wait_exit(&client.app);
}

#ifndef GAPS_ENALBLE
int main(int argc, char *argv[]) {
    return sensor_manager_main(argc, argv);
}
#endif
