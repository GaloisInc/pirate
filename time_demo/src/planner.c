#include <argp.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include "gaps_packet.h"
#include "ts_crypto.h"
#include "common.h"

#define MAX_INPUT_COUNT 128

typedef struct {
    char path[PATH_MAX];
    uint32_t len;
} client_data_t;

typedef struct {
    verbosity_t verbosity;
    uint32_t validate;
    uint32_t request_delay_ms;
    const char *ca_path;
    const char *tsr_dir;

    gaps_app_t app;

    uint32_t count;
    client_data_t data[MAX_INPUT_COUNT];
} client_t;

const char *argp_program_version = DEMO_VERSION;
static struct argp_option options[] = {
    { "ca_path",   'C', "PATH", 0, "CA Path",                                  0 },
    { "validate",  'V', NULL,   0, "Validate timestamp signatures",            0 },
    { "save",      's', "PATH", 0, "Save timestamp signatures to a directory", 0 },
    { "req_delay", 'd', "MS",   0, "Request delay in milliseconds",            0 },
    { "verbose",   'v', NULL,   0, "Increase verbosity level",                 0 },
    { 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    client_t *client = (client_t*) state->input;

    switch (key) {

    case 'C':
        client->ca_path = arg;
        break;

    case 'V':
        client->validate = 1;
        break;

    case 's':
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

    case ARGP_KEY_ARG:
    {
        struct stat sb;
        client_data_t *d = NULL;
        if (client->count >=  MAX_INPUT_COUNT) {
            argp_failure(state, 1, 0, "exceeded input limit");
        }

        d = &client->data[client->count++];
        if (realpath(arg, d->path) == NULL) {
            argp_failure(state, 1, 0, "failed to resolve %s path\n", arg);
        }

        if (stat(d->path, &sb) != 0) {
            argp_failure(state, 1, 0, "Failed to stat %s\n", d->path);
        }

        d->len = sb.st_size;
    }
        break;

    case ARGP_KEY_END:
        if (client->count == 0) {
            argp_failure(state, 1, 0, "no input files");
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


/* Save timestamp sign response to a file */
static int save_ts_response(const char *in_path, const char *out_dir, 
    uint32_t idx, const tsa_response_t* rsp ) {
    int rv = 0;
    const char *in_file = in_path + strlen(in_path);
    char out_path[PATH_MAX];
    FILE *f_out = NULL;

    if ((out_dir == NULL) || 
        (rsp->status != OK) || 
        (rsp->status > sizeof(rsp->ts))) {
        return 0;
    }

    do {
        in_file--;
    } while((in_path < in_file) && (*(in_file - 1) != '/'));

    snprintf(out_path, sizeof(out_path) - 1, "%s/%04u_%s.tsr", 
        out_dir, idx, in_file);

    if ((f_out = fopen(out_path, "wb")) == NULL) {
        ts_log(ERROR, "Failed to open TSR output file");
        return -1;
    }

    if (fwrite(rsp->ts, rsp->len, 1, f_out) != 1) {
        ts_log(ERROR, "Failed to open TSR content");
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
    const gaps_channel_ctx_t *proxy_wr = &client->app.ch[0];
    const gaps_channel_ctx_t *proxy_rd = &client->app.ch[1];

    const struct timespec ts = {
        .tv_sec = client->request_delay_ms / 1000,
        .tv_nsec = (client->request_delay_ms % 1000) * 1000000
    };

    while (gaps_running()) {
        client_data_t *d = &client->data[idx % client->count];

        if (client->request_delay_ms != 0) {
            nanosleep(&ts, NULL);
        }

        /* Compose a request */
        if (ts_create_request(d->path, &req) != 0) {
            ts_log(ERROR, "Failed to generate TS request");
            gaps_terminate();
            continue;
        }

        /* Send request */
        int sts = gaps_packet_write(proxy_wr->fd, &req, sizeof(req));
        if (sts == -1) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to send signing request to proxy");
                gaps_terminate();
            }
            continue;
        }
        log_proxy_req(client->verbosity, "Request sent to proxy", &req);

        /* Get response */
        sts = gaps_packet_read(proxy_rd->fd, &rsp, sizeof(rsp));
        if (sts != sizeof(rsp)) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to receive response");
                gaps_terminate();
            }
            continue;
        }
        log_tsa_rsp(client->verbosity, "Timestamp response received", &rsp);

        /* Optionally validate the signature */
        if (client->validate != 0) {
            if (ts_verify(d->path, client->ca_path, &rsp) == 0) {
                ts_log(INFO, BCLR(GREEN, "Timestamp VERIFIED"));
            } else {
                ts_log(WARN, BCLR(RED, "FAILED to validate the timestamp"));
            }
            fflush(stdout);
        }

        /* Optionally save the timestamp signature */
        if (save_ts_response(d->path, client->tsr_dir, idx, &rsp) != 0) {
            ts_log(ERROR, "Failed to save timestamp response");
            gaps_terminate();
            continue;
        }

        idx++;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    client_t client = {
        .verbosity = VERBOSITY_NONE,
        .validate = 0,
        .request_delay_ms = 0,
        .ca_path = DEFAULT_CA_PATH,
        .tsr_dir = NULL,

        .app = {
            .threads = {
                THREAD_ADD(client_thread, &client, "ts_client"),
                THREAD_END
            },

            .ch = {
                GAPS_CHANNEL(CLIENT_TO_PROXY, O_WRONLY, PIPE, "client->proxy"),
                GAPS_CHANNEL(PROXY_TO_CLIENT, O_RDONLY, PIPE, "client<-proxy"),
                GAPS_CHANNEL_END
            }
        },

        .count = 0
    };

    parse_args(argc, argv, &client);

    if (gaps_app_run(&client.app) != 0) {
        ts_log(ERROR, "Failed to initialize the timestamp client");
        return -1;
    }

    return gaps_app_wait_exit(&client.app);;
}
