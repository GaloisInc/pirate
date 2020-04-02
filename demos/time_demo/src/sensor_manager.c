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
#include <openssl/bio.h>
#include <openssl/ts.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"
#include "gaps_packet.h"
#include "ts_crypto.h"
#include "video_sensor.h"
#include "xwin_display.h"

#ifndef GAPS_DISABLE
#pragma pirate enclave declare(orange)
#endif

typedef struct {
    verbosity_t verbosity;
    uint32_t validate;
    uint32_t request_delay_ms;
    uint32_t display;
    const char *ca_path;
    const char *cert_path;
    const char *tsr_dir;
    const char *video_device;
    gaps_app_t app;
} client_t;


/* Command-line options */
extern const char *argp_program_version;
static struct argp_option options[] = {
    { "ca_path",      'C', "PATH", 0, "CA path",                          0 },
    { "cert_path",    'S', "PATH", 0, "Signing certificate path",         0 },
    { "verify",       'V', NULL,   0, "Verify timestamp signatures",      0 },
    { "save_path",    'O', "PATH", 0, "TSR output directory",             0 },
    { "video_device", 'D', "PATH", 0, "video device path",                0 },
    { "req_delay",    'd', "MS",   0, "Request delay in milliseconds",    0 },
    { "verbose",      'v', NULL,   0, "Increase verbosity level",         0 },
    { "headless",     'x', NULL,   0, "Run in headless mode",             0 },
    { "client-to-proxy", 1000, "CONFIG", 0, "Client to proxy channel",    1 },
    { "proxy-to-client", 1001, "CONFIG", 0, "Proxy to client channel",    1 },
    { NULL,            0,  NULL,   0, GAPS_CHANNEL_OPTIONS,               2 },
    { 0 }
};

void sensor_manager_terminate() {
    video_sensor_terminate();
    xwin_display_terminate();
}

#define DEFAULT_TSR_OUT_DIR "."

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    client_t *client = (client_t*) state->input;

    switch (key) {

    case 1000:
        strncpy(client->app.ch[0].conf, arg, 64);
        break;

    case 1001:
        strncpy(client->app.ch[1].conf, arg, 64);
        break;

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

    case 'D':
        client->video_device = arg;
        break;

    case 'd':
        client->request_delay_ms = strtol(arg, NULL, 10);
        break;

    case 'x':
        client->display = 0;
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

    strncpy(client->app.ch[0].conf, "pipe,/tmp/client.proxy.gaps", 64);
    strncpy(client->app.ch[1].conf, "pipe,/tmp/proxy.client.gaps", 64);

    argp_parse(&argp, argc, argv, 0, 0, client);
}

/* Generate sensor data */
static int read_sensor(int idx) {
    return video_sensor_read(idx);
}

/* Save sensor data to a file */
static int save_sensor(const client_t* client, uint32_t idx,
    void* buffer, unsigned int buffer_length) {
    char path[TS_PATH_MAX];
    FILE *f_out = NULL;

    if (client->tsr_dir == NULL) {
        return -1;
    }

    snprintf(path, sizeof(path) - 1, "%s/%04u.jpg", client->tsr_dir, idx);

    if ((f_out = fopen(path, "wb")) == NULL) {
        ts_log(ERROR, "Failed to open sensor output file");
        return -1;
    }

    if (fwrite(buffer, buffer_length, 1, f_out) != 1) {
        ts_log(ERROR, "Failed to save sensor content");
        fclose(f_out);
        return -1;
    }

    fclose(f_out);

    if (client->display) {
        xwin_display_render_jpeg(buffer, buffer_length);
    }

    return 0;
}

/* Save timestamp sign response to a file */
static int save_ts_response(const client_t *client, uint32_t idx, const tsa_response_t* rsp) {
    char path[TS_PATH_MAX];
    FILE *f_out = NULL;
    BIO *ts_resp_bio = NULL;
    BIO *display_bio = NULL;
    BUF_MEM *display_buf_mem = NULL;
    TS_RESP *response = NULL;
    PKCS7 *pkcs7 = NULL;

    if ((client->tsr_dir== NULL) ||
        (rsp->hdr.status != OK) ||
        (rsp->hdr.len > sizeof(rsp->ts))) {
        return 0;
    }

    snprintf(path, sizeof(path) - 1, "%s/%04u.tsr", client->tsr_dir, idx);

    if ((f_out = fopen(path, "wb")) == NULL) {
        ts_log(ERROR, "Failed to open TSR output file");
        return -1;
    }

    if (fwrite(rsp->ts, rsp->hdr.len, 1, f_out) != 1) {
        ts_log(ERROR, "Failed to save TSR content");
        fclose(f_out);
        return -1;
    }

    fclose(f_out);

    if (client->display) {
        ts_resp_bio = BIO_new_mem_buf(rsp->ts, rsp->hdr.len);
        if (ts_resp_bio == NULL) {
            ts_log(ERROR, "Failed to load TSR content");
            return -1;
        }

        response = d2i_TS_RESP_bio(ts_resp_bio, NULL);
        if (response == NULL) {
            BIO_free(ts_resp_bio);
            ts_log(ERROR, "Failed to convert TSR content");
            return -1;
        }

        display_bio = BIO_new(BIO_s_mem());
        if (display_bio == NULL) {
            ts_log(ERROR, "Failed to create TSR display");
            return -1;
        }

        pkcs7 = TS_RESP_get_token(response);
        PKCS7_print_ctx(display_bio, pkcs7, 0, NULL);
        BIO_get_mem_ptr(display_bio, &display_buf_mem);
        xwin_display_show_response(display_buf_mem->data, display_buf_mem->length);

        BIO_free(display_bio);
        TS_RESP_free(response);
        BIO_free(ts_resp_bio);
    }

    return 0;
}

/* Acquire trusted timestamps */
static void *client_thread(void *arg) {
    client_t *client = (client_t *)arg;
    void* jpeg_buffer;
    unsigned int jpeg_buffer_length;
    int idx = 0;

    proxy_request_t req = PROXY_REQUEST_INIT;
    tsa_response_t rsp = TSA_RESPONSE_INIT;

    const struct timespec ts = {
        .tv_sec = client->request_delay_ms / 1000,
        .tv_nsec = (client->request_delay_ms % 1000) * 1000000
    };

    while (gaps_running()) {
        /* Read sensor data */
        if (read_sensor(idx)) {
            ts_log(ERROR, "Failed to read sensor data");
            gaps_terminate();
            continue;
        }

        jpeg_buffer = video_sensor_get_buffer();
        jpeg_buffer_length = video_sensor_get_buffer_length();

        /* Save sensor data */
        if (save_sensor(client, idx, jpeg_buffer, jpeg_buffer_length)) {
            ts_log(ERROR, "Failed to save sensor data");
            gaps_terminate();
            continue;
        }

        /* Compose a request */
        if (ts_create_request_from_data(jpeg_buffer, jpeg_buffer_length, &req) != 0) {
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
            if (ts_verify_data(jpeg_buffer, jpeg_buffer_length,
                    client->ca_path, client->cert_path, &rsp) == 0) {
                ts_log(INFO, BCLR(GREEN, "Timestamp VERIFIED"));
            } else {
                ts_log(WARN, BCLR(RED, "FAILED to validate the timestamp"));
            }
            fflush(stdout);
        }

        /* Save the timestamp signature */
        if (save_ts_response(client, idx, &rsp) != 0) {
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

int sensor_manager_main(int argc, char *argv[]) PIRATE_ENCLAVE_MAIN("orange") {
    client_t client = {
        .verbosity = VERBOSITY_NONE,
        .validate = 0,
        .display = 1,
        .request_delay_ms = DEFAULT_REQUEST_DELAY_MS,
        .ca_path = DEFAULT_CA_PATH,
        .tsr_dir = DEFAULT_TSR_OUT_DIR,
        .video_device = DEFAULT_VIDEO_DEVICE,

        .app = {
            .threads = {
                THREAD_ADD(client_thread, &client, "ts_client"),
                THREAD_END
            },

            .on_shutdown = sensor_manager_terminate,

            .ch = {
                GAPS_CHANNEL(&CLIENT_TO_PROXY, O_WRONLY, "client->proxy"),
                GAPS_CHANNEL(&PROXY_TO_CLIENT, O_RDONLY, "client<-proxy"),
                GAPS_CHANNEL_END
            }
        }
    };

    parse_args(argc, argv, &client);

    ts_log(INFO, "Starting sensor manager");

    if (video_sensor_initialize(client.video_device, client.display) != 0) {
        return -1;
    }
    if (client.display) {
        if (xwin_display_initialize() != 0) {
            return -1;
        }
    } else {
        ts_log(INFO, "Headless mode. Using stock images");
    }

    if (gaps_app_run(&client.app) != 0) {
        ts_log(ERROR, "Failed to initialize the timestamp client");
        return -1;
    }

    return gaps_app_wait_exit(&client.app);
}

#ifdef GAPS_DISABLE
int main(int argc, char *argv[]) {
    return sensor_manager_main(argc, argv);
}
#endif
