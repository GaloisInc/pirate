#ifndef _TS_CRYPTO_H_
#define _TS_CRYPTO_H_

#include "common.h"

#define DEFAULT_CONF_PATH    "pki/tsa.conf"
#define DEFAULT_CONF_SECTION "tsa"
#define DEFAULT_CA_PATH      "../ca/tsa_ca.pem"
#define OID_SECTION          "oids"


/* Client */
int ts_create_request(const char *path, proxy_request_t *req);

/* Proxy */
int ts_create_query(proxy_request_t *req, tsa_request_t *tsa_req);


/* Sign */
void *ts_init(const char *path, const char *sect);
void ts_term(void *ctx);
void ts_sign(void *ctx, const tsa_request_t *req, tsa_response_t *rsp);


/* Verify */
int ts_verify(const char *path, const char *ca, tsa_response_t* rsp);

int ts_print_proxy_req(FILE* out, const proxy_request_t *req);
int ts_print_tsa_req(FILE* out, const tsa_request_t *req);
int ts_print_tsa_rsp(FILE* out, const tsa_response_t *rsp);
#endif /* _TS_CRYPTO_H_ */
