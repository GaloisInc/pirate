#include <argp.h>
#include <stdio.h>
#include <string.h>
#include "gaps_packet.h"
#include "common.h"
#include "ts_crypto.h"

typedef struct {
    verbosity_t verbosity;

    struct {
        const char *conf_file;
        const char *conf_sect;
        void *tsa;
    } ts;

    gaps_app_t app;
} signer_t;

/* Command-line options */
const char *argp_program_version = DEMO_VERSION;
static struct argp_option options[] = {
    { "conf",      'c', "PATH",    0, "Configuration file path",    0 },
    { "conf_sect", 's', "SECTION", 0, "Configuration section",      0 },
    { "verbose",   'v', NULL,      0, "Increase verbosity level",   0 },
    { 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    signer_t *signer = (signer_t *) state->input;

    switch (key) {

    case 'c':
        signer->ts.conf_file = arg;
        break;

    case 's':
        signer->ts.conf_sect = arg;
        break;

    case 'v':
        if (signer->verbosity < VERBOSITY_MAX) {
            signer->verbosity++;
        }
        break;

    default:
        break;

    }

    return 0;
}

static void parse_args(int argc, char *argv[], signer_t *signer) {
    struct argp argp = {
        .options = options,
        .parser = parse_opt,
        .args_doc = NULL,
        .doc = "Timestamp signing service",
        .children = NULL,
        .help_filter = NULL,
        .argp_domain = NULL
    };

    argp_parse(&argp, argc, argv, 0, 0, signer);
}

/* Run the signer */
static void *signer_thread(void *arg) {
    signer_t *signer = (signer_t *) arg;
    ssize_t len = 0;
    tsa_request_t req = TSA_REQUEST_INIT;
    tsa_response_t rsp = TSA_RESPONSE_INIT;

    while (gaps_running()) {
        /* Receive sign request */
        len = gaps_packet_read(PROXY_TO_SIGNER, &req, sizeof(req));
        if (len != sizeof(req)) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to receive sign request");
                gaps_terminate();
            }
            continue;
        }
        log_tsa_req(signer->verbosity, "Timestamp request received", &req);

        /* Sign with a timestamp */
        ts_sign(signer->ts.tsa, &req, &rsp);

        /* Reply */
        if (gaps_packet_write(SIGNER_TO_PROXY, &rsp, sizeof(rsp)) != 0) {
            if (gaps_running()) {
                ts_log(ERROR, "Failed to send sign response");
                gaps_terminate();
            }
            continue;
        }
        log_tsa_rsp(signer->verbosity, "Timestamp response sent", &rsp);
    }

    return 0;  /* Should never get here */
}

/* Release signer resources */
static void signer_term(signer_t *signer) {
    /* Terminate the timestamp authority */
    ts_term(signer->ts.tsa);
    signer->ts.tsa = NULL;
}


int main(int argc, char *argv[]) {
    signer_t signer = {
        .verbosity = VERBOSITY_NONE,
        
        .ts = {
            .conf_file = DEFAULT_CONF_PATH,
            .conf_sect = DEFAULT_CONF_SECTION
        },

        .app = {
            .threads = {
                THREAD_ADD(signer_thread, &signer, "ts_signer"),
                THREAD_END
            },

            .ch = {
#ifdef GAPS_SERIAL
                GAPS_CHANNEL(PROXY_TO_SIGNER, O_RDONLY, SERIAL, "/dev/ttyUSB1",
                            "proxy->signer"),
                GAPS_CHANNEL(SIGNER_TO_PROXY, O_WRONLY, SERIAL, "/dev/ttyUSB2", 
                            "proxy<-signer"),
#else
                GAPS_CHANNEL(PROXY_TO_SIGNER, O_RDONLY, PIPE, NULL,
                            "proxy->signer"),
                GAPS_CHANNEL(SIGNER_TO_PROXY, O_WRONLY, PIPE, NULL, 
                            "proxy<-signer"),
#endif
                GAPS_CHANNEL_END
            }
        }
    };

    parse_args(argc, argv, &signer);

    /* Initialize signer crypto resources */
    signer.ts.tsa = ts_init(signer.ts.conf_file, signer.ts.conf_sect);
    if (signer.ts.tsa == NULL) {
        ts_log(ERROR, "Failed to initialize timestamp context");
        return -1;
    }

    /* Run the signer */
    if (gaps_app_run(&signer.app) != 0) {
        ts_log(ERROR, "Failed to start the signing proxy");
        signer_term(&signer);
        return -1;
    }

    int rv = gaps_app_wait_exit(&signer.app);
    
    /* Release signer crypto resources */
    ts_term(signer.ts.tsa);
    signer.ts.tsa = NULL;
    return rv;
}
