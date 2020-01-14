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
#include "common.h"
#include "ts_crypto.h"

#define MAX_INPUT_COUNT 32

typedef struct {
    char *inputs[MAX_INPUT_COUNT];
    uint32_t input_count;

    const char *conf_path;
    const char *conf_sect;
    const char *ca_path;
    uint32_t loops;
    verbosity_t verbosity;

    void *ts_ctx;
} ts_test_t;

static ts_test_t ts_test_g;

const char *argp_program_version = DEMO_VERSION;
static struct argp_option options[] = {
    { "config",    'c', "PATH",       0, "Configuration file",        0 },
    { "ca_path",   'C', "PATH",       0, "CA Path",                   0 },
    { "section",   's', "SECTION",    0, "Configuration TSA section", 0 },
    { "loops",     'n', "ITERATIONS", 0, "Number of test iterations", 0 },
    { "verbosity", 'v', NULL,         0, "Increase verbosity level",  0 },
    { 0 }
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    ts_test_t *ts_test = (ts_test_t *) state->input;

    switch (key) {
        
        case 'c':
            ts_test->conf_path = arg;
            break;

        case 's':
            ts_test->conf_sect = arg;
            break;

        case 'n':
            ts_test->loops = strtol(arg, NULL, 10);
            break;

        case 'v':
            if (ts_test->verbosity < VERBOSITY_MAX) {
                ts_test->verbosity++;
            }
            break;

        case ARGP_KEY_ARG:
            ts_test->inputs[ts_test->input_count++] = arg;
            break;

        case ARGP_KEY_END:
            if (ts_test->input_count == 0) {
                argp_failure(state, 1, 0, "no input files");
            }
            break;
    }

    return 0;
}


static void parse_args(int argc, char * argv[], ts_test_t *ts_test) {
    struct argp argp = {
        .options = options,
        .parser = parse_opt,
        .args_doc = "[FILE] [FILE] ...",
        .doc = "Generate TS request, TS sign, and TS validate test",
        .children = NULL,
        .help_filter = NULL,
        .argp_domain = NULL
    };

    ts_test->conf_path = DEFAULT_CONF_PATH;
    ts_test->conf_sect = DEFAULT_CONF_SECTION;
    ts_test->ca_path = DEFAULT_CA_PATH;
    ts_test->loops = 1;
    ts_test->verbosity = VERBOSITY_NONE;
    argp_parse(&argp, argc, argv, 0 ,0, ts_test);
}


static int ts_req_sign_verify(ts_test_t *ts_test, const char* path) {
    proxy_request_t proxy_req;
    tsa_request_t tsa_req;
    tsa_response_t tsa_rsp;

    /* Create digest */
    if (ts_create_request(path, &proxy_req) != 0) {
        ts_log(ERROR, "Failed to create data digest");
        return -1;
    }
    log_proxy_req(ts_test->verbosity, "Proxy request generated", &proxy_req);
    
    /* Create query */
    if (ts_create_query(&proxy_req, &tsa_req) != 0) {
        ts_log(ERROR, "Failed to create query");
        return -1;
    }
    log_tsa_req(ts_test->verbosity, "Timestamp request generated", &tsa_req);

    /* Create TS response */
    ts_sign(ts_test->ts_ctx, &tsa_req, &tsa_rsp);
    if (tsa_rsp.status != OK) {
        ts_log(ERROR, "Failed to generate response");
        return -1;
    }
    log_tsa_rsp(ts_test->verbosity, "Timestamp sign response", &tsa_rsp);

    /* Verify */
    if (ts_verify(path, ts_test->ca_path, &tsa_rsp) != 0) {
        ts_log(ERROR, "Failed to verify response");
        return -1;
    }

    return 0;
}


static int ts_test_run(ts_test_t *ts_test) {
    for (uint32_t l = 0; l < ts_test->loops; l++) {
        for (uint32_t i = 0; i < ts_test->input_count; i++) {
            if (ts_req_sign_verify(ts_test, ts_test->inputs[i]) != 0) {
                ts_log(ERROR, "FAIL: Loop %d, Input %d", l, i);
                return -1;
            }
        }
    }

    return 0;
}

int main(int argc, char *argv[]) {
    /* Parse command-line options */
    parse_args(argc, argv, &ts_test_g);

    /* Initialize the timestamp test */
    ts_test_g.ts_ctx = ts_init(ts_test_g.conf_path, ts_test_g.conf_sect);
    if (ts_test_g.ts_ctx == 0) {
        ts_log(ERROR, "Failed to initialize the timestamp test");
        return -1;
    }

    /* Run the timestamp test */
    if (ts_test_run(&ts_test_g) != 0) {
        ts_log(ERROR, "Failed to execute timestamp test");
        ts_term(ts_test_g.ts_ctx);
        return -1;
    }

    /* Cleanup the timestamp test */
    ts_term(ts_test_g.ts_ctx);

    return 0;
}
