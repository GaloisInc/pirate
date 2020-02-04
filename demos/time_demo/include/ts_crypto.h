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

#ifndef _TS_CRYPTO_H_
#define _TS_CRYPTO_H_

#include "common.h"

#define DEFAULT_CONF_PATH    "pki/tsa.conf"
#define DEFAULT_CONF_SECTION "tsa"
#define DEFAULT_CA_PATH      "../ca/tsa_ca.pem"
#define DEFAULT_CERT_PATH    "pki/tsa_cert.pem"
#define OID_SECTION          "oids"


/* Client */
int ts_create_request_from_data(const void *data, uint32_t len, 
    proxy_request_t *req);
int ts_create_request_from_file(const char *path, proxy_request_t *req);

/* Proxy */
int ts_create_query(proxy_request_t *req, tsa_request_t *tsa_req);


/* Sign */
void *ts_init(const char *path, const char *sect);
void ts_term(void *ctx);
void ts_sign(void *ctx, const tsa_request_t *req, tsa_response_t *rsp);


/* Verify */
int ts_verify_data(const void *data, uint32_t len, 
    const char *ca, const char *cert, tsa_response_t* rsp);
int ts_verify_file(const char *path,
    const char *ca, const char *cert, tsa_response_t* rsp);

int ts_print_proxy_req(FILE* out, const proxy_request_t *req);
int ts_print_tsa_req(FILE* out, const tsa_request_t *req);
int ts_print_tsa_rsp(FILE* out, const tsa_response_t *rsp);
#endif /* _TS_CRYPTO_H_ */
