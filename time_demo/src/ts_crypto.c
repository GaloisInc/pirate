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

#include <stdio.h>
#include <string.h>
#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/objects.h>
#include <openssl/rand.h>
#include <openssl/ts.h>
#include <openssl/x509_vfy.h>
#include "ts_crypto.h"


static void print_err(const char *msg) {
    char *err_str = NULL;
    BIO *err_bio = BIO_new(BIO_s_mem());

    if (err_bio == NULL) {
        fprintf(stderr, "%s: failed to allocate error buffer\n", msg);
        return;
    }

    ERR_print_errors(err_bio);

    if (BIO_get_mem_data(err_bio, &err_str) != 1) {
        fprintf(stderr, "%s: failed to get error data\n", msg);
        goto end;
    }

    fprintf(stderr, "%s : %s\n", msg, err_str);
end:
    BIO_free(err_bio);
}


/*
 * Generate SHA256 digest from the content of a file.
 * If path is NULL, then the output digest is filled with random data
 */
int ts_create_request(const char *path, proxy_request_t *req) {
    FILE *f = NULL;
    SHA256_CTX sha256;
    unsigned char buf[4096];
    int rd;
    int ret = -1;

    if (path == NULL) {
        if (RAND_bytes(req->digest, SHA256_DIGEST_LENGTH) == 1) {
            ret = 0;
            goto end;
        }
    }

    if ((f = fopen(path, "rb")) == NULL) {
        fprintf(stderr, "Failed to open %s\n", path);
        goto end;
    }


    if (SHA256_Init(&sha256) != 1) {
        goto end;
    }

    while ((rd = fread(buf, 1, sizeof(buf), f)) != 0) {
        if (SHA256_Update(&sha256, buf, rd) != 1) {
            goto end;
        }
    }

    if (SHA256_Final(req->digest, &sha256) != 1) {
        goto end;
    }

    ret = 0;
end:
    if (ret != 0) {
        print_err("Failed to generate request digest");
    }

    if (f != NULL) {
        fclose(f);
        f = NULL;
    }

    return ret;
}


static ASN1_INTEGER *create_nonce(unsigned len) {
    ASN1_INTEGER *nonce = NULL;
    unsigned char buf[32];
    unsigned off = 0;   // Offset of the first non-zero byte of the random data
    int err = 1;

    if (len >sizeof(buf)) {
        fprintf(stderr, "Max nonce size is %lu\n", sizeof(buf));
        goto end;
    }

    if (RAND_bytes(buf, len) != 1) {
        goto end;
    }

    for (off = 0; (off < len) && (buf[off] == 0); ++off) {
        continue;
    }

    if ((nonce = ASN1_INTEGER_new()) == NULL) {
        goto end;
    }

    OPENSSL_free(nonce->data);
    nonce->length = len - off;
    if ((nonce->data = OPENSSL_malloc(nonce->length)) == NULL) {
        goto end;
    }

    memcpy(nonce->data, buf + off, nonce->length);
    err = 0;
end:
    if (err != 0) {
        print_err("Failed to create noncer");
        ASN1_INTEGER_free(nonce);
        nonce = NULL;
    }

    return nonce;
}

int ts_create_query(proxy_request_t *req, tsa_request_t *tsa_req) {
    BIO *ts_req_bio = NULL;
    TS_REQ *ts_req = NULL;
    const EVP_MD *md = NULL;
    TS_MSG_IMPRINT *msg_imprint = NULL;
    X509_ALGOR *algo = NULL;
    ASN1_INTEGER *nonce = NULL;
    unsigned char *ts_req_bio_raw = NULL;

    int ret = -1;

    if ((md = EVP_get_digestbyname("sha256")) == NULL) {
        goto end;
    }

    if ((ts_req = TS_REQ_new()) == NULL) {
        goto end;
    }

    if (TS_REQ_set_version(ts_req, 1) != 1) {
        goto end;  
    }

    if ((msg_imprint = TS_MSG_IMPRINT_new()) == NULL) {
        goto end;
    }

    if ((algo = X509_ALGOR_new()) == NULL) {
        goto end;
    }

    if ((algo->algorithm = OBJ_nid2obj(EVP_MD_type(md))) == NULL) {
        goto end;
    }

    if ((algo->parameter = ASN1_TYPE_new()) == NULL) {
        goto end;
    }
    algo->parameter->type = V_ASN1_NULL;

    if (TS_MSG_IMPRINT_set_algo(msg_imprint, algo) != 1) {
        goto end;
    }

    if (TS_MSG_IMPRINT_set_msg(msg_imprint, req->digest, 
                                sizeof(req->digest)) != 1) {
        goto end;
    }

    if (TS_REQ_set_msg_imprint(ts_req, msg_imprint) != 1) {
        goto end;
    }

    if ((nonce = create_nonce(NONCE_LENGTH)) == NULL) {
        goto end;
    }

    if ((TS_REQ_set_nonce(ts_req, nonce)) != 1) {
        goto end;
    }

    if (TS_REQ_set_cert_req(ts_req, 1) != 1) {
        goto end;
    }

    if ((ts_req_bio = BIO_new(BIO_s_mem())) == NULL) {
        goto end;
    }

    if (i2d_TS_REQ_bio(ts_req_bio, ts_req) != 1) {
        goto end;
    }

    if ((tsa_req->len = BIO_get_mem_data(ts_req_bio, &ts_req_bio_raw)) <= 0) {
        goto end;
    }

    if (tsa_req->len > sizeof(tsa_req->req)) {
        fprintf(stderr, "TS REQ exceeds size of %lu\n", sizeof(tsa_req->req));
        tsa_req->len = 0;
        goto end;
    }

    memcpy(tsa_req->req, ts_req_bio_raw, tsa_req->len);
    ret = 0;
end:
    if (ret != 0) {
        print_err("Failed to create TS request query");
        BIO_free_all(ts_req_bio);
        ts_req_bio = NULL;
    }

    TS_MSG_IMPRINT_free(msg_imprint);
    X509_ALGOR_free(algo);
    ASN1_INTEGER_free(nonce);
    TS_REQ_free(ts_req);
    BIO_free_all(ts_req_bio);

    return ret;
}


static CONF *ts_load_config(const char *conf_path) {
    int err = 1;
    long err_line = -1;
    CONF *conf = NULL;
    STACK_OF(CONF_VALUE) *oids = NULL;
    
    BIO *f_bio = NULL;
    if ((f_bio = BIO_new_file(conf_path, "rb")) == NULL) {
        goto end;
    }

    if ((conf = NCONF_new(NULL)) == NULL) {
        goto end;
    }

    if (NCONF_load_bio(conf, f_bio, &err_line) <= 0) {
        if (err_line <= 0) {
            fprintf(stderr, "Cannot load %s\n", conf_path);
        } else {
            fprintf(stderr, "%s: error on line %lu", conf_path, err_line);
        }
        goto end;
    }

    if ((oids = NCONF_get_section(conf, OID_SECTION)) == NULL) {
        goto end;
    }

    for (int i = 0; i < sk_CONF_VALUE_num(oids); i++) {
        CONF_VALUE *c = sk_CONF_VALUE_value(oids, i);
        if (OBJ_create(c->value, c->name, c->name) == NID_undef) {
            fprintf(stderr, "Error creating object %s=%s\n", c->name, c->value);
            goto end;
        }
    }

    err = 0;
end:
    if (err != 0) {
        NCONF_free(conf);
        conf = NULL;
        print_err("Error loading configuration file");
    }

    BIO_free(f_bio);
    return conf;
}

static ASN1_INTEGER *serial_cb(TS_RESP_CTX *ctx, void *data) {
    (void) ctx, (void) data;
    int err = 1;
    ASN1_INTEGER *serial = NULL;
    static int n = 0;

    if ((serial = ASN1_INTEGER_new()) == NULL) {
        goto end;
    }

    if (!ASN1_INTEGER_set(serial, ++n)) {
        goto end;
    }

    err = 0;
end:
    if (err != -0) {
        ASN1_INTEGER_free(serial);
        serial = NULL;
        print_err("Failed to generate serial number");
    }

    return serial;
}

static TS_RESP_CTX *ts_create_ctx(CONF *conf, const char *section) {
    int err = 1;
    TS_RESP_CTX *ctx = NULL;

    if ((section = TS_CONF_get_tsa_section(conf, section)) == NULL) {
        goto end;
    }

    if ((ctx = TS_RESP_CTX_new()) == NULL) {
        goto end;
    }

    if (!TS_CONF_set_serial(conf, section, serial_cb, ctx)) {
        goto end;
    }

    if (!TS_CONF_set_crypto_device(conf, section, NULL)) {
        goto end;
    }

    if (!TS_CONF_set_signer_cert(conf, section, NULL, ctx)) {
        goto end;
    }

    if (!TS_CONF_set_certs(conf, section, NULL, ctx)) {
        goto end;
    }

    if (!TS_CONF_set_signer_key(conf, section, NULL, NULL, ctx)) {
        goto end;
    }

    if (!TS_CONF_set_signer_digest(conf, section, NULL, ctx)) {
        goto end;
    }

    if (!TS_CONF_set_ess_cert_id_digest(conf, section, ctx)) {
        goto end;
    }
    
    if (!TS_CONF_set_def_policy(conf, section, NULL, ctx)) {
        goto end;
    }
    
    if (!TS_CONF_set_policies(conf, section, ctx)) {
        goto end;
    }

    if (!TS_CONF_set_digests(conf, section, ctx)) {
        goto end;
    }

    if (!TS_CONF_set_accuracy(conf, section, ctx)) {
        goto end;
    }

    if (!TS_CONF_set_clock_precision_digits(conf, section, ctx)) {
        goto end;
    }

    if (!TS_CONF_set_ordering(conf, section, ctx)) {
        goto end;
    }

    if (!TS_CONF_set_tsa_name(conf, section, ctx)) {
        goto end;
    }

    if (!TS_CONF_set_ess_cert_id_chain(conf, section, ctx)) {
        goto end;
    }

    err = 0;
end:
    if (err != 0) {
        TS_RESP_CTX_free(ctx);
        ctx = NULL;
        print_err("Failed to create TS RESP context");
    }

    return ctx;
}

void *ts_init(const char *path, const char *sect) {
    CONF* conf = NULL;
    TS_RESP_CTX *ctx = NULL;

    /* Load configuration file */
    if ((conf = ts_load_config(path)) == NULL) {
        fprintf(stderr, "Failed to load config file %s", path);
        return ctx;
    }

    /* Create TS response context */
    if ((ctx = ts_create_ctx(conf, sect)) == NULL) {
        fprintf(stderr, "Failed to create TS response context\n");
    }

    NCONF_free(conf);
    return ctx;
}

/* Release the OpenSSL Time-Stamp Response Context */
void ts_term(void *ctx) {
    TS_RESP_CTX *ts_ctx = (TS_RESP_CTX *)ctx;
    TS_RESP_CTX_free(ts_ctx);
}


void ts_sign(void *ctx, const tsa_request_t *req, tsa_response_t *rsp) {
    rsp->status = ERR;
    TS_RESP_CTX *ts_ctx = (TS_RESP_CTX *)ctx;
    TS_RESP *ts_rsp = NULL;
    BIO *ts_req_bio = NULL;
    BIO *ts_rsp_bio = NULL;

    if (req->len == 0 || req->len > sizeof(req->req)) {
        goto end;
    }

    if ((ts_req_bio = BIO_new_mem_buf(req->req, req->len)) == NULL) {
        goto end;
    }

    if ((ts_rsp = TS_RESP_create_response(ts_ctx, ts_req_bio)) == NULL) {
        goto end;
    }

    if ((ts_rsp_bio = BIO_new(BIO_s_mem())) == NULL) {
        goto end;
    }

    if (!i2d_TS_RESP_bio(ts_rsp_bio, ts_rsp)) {
        goto end;
    }

    if ((rsp->len = BIO_read(ts_rsp_bio, rsp->ts, sizeof(rsp->ts))) < 0) {
        goto end;
    }

    rsp->status = OK;
end:
    if (rsp->status != OK) {
        print_err("Failed to sign the timestamp request");
    }

    TS_RESP_free(ts_rsp);
    BIO_free_all(ts_req_bio);
    BIO_free_all(ts_rsp_bio);
}


static int ts_verify_cb(int ok, X509_STORE_CTX *ctx) {
    (void) ctx;
    return ok;
}

int ts_verify(const char *path, const char *ca, tsa_response_t* rsp) {
    int ret = 1;
    BIO *ts_req_bio = NULL;
    TS_RESP *ts_resp = NULL;
    TS_VERIFY_CTX *ctx = NULL;
    BIO *data = NULL;
    X509_STORE *cert_ctx = NULL;
    X509_LOOKUP *lkp = NULL;

    int f = TS_VFY_VERSION | TS_VFY_SIGNER | TS_VFY_DATA | TS_VFY_SIGNATURE;

    if ((ts_req_bio = BIO_new_mem_buf(rsp->ts, rsp->len)) == NULL) {
        goto end;
    }

    if ((ts_resp = d2i_TS_RESP_bio(ts_req_bio, NULL)) == NULL) {
        goto end;
    }

    if ((ctx = TS_VERIFY_CTX_new()) == NULL) {
        goto end;
    }

    if ((data = BIO_new_file(path, "rb")) == NULL) {
        goto end;
    }

    if (TS_VERIFY_CTX_set_data(ctx, data) == NULL) {
        goto end;
    }
    data = NULL;         /* Freed by TS_VERIFY_CTX_free */

    TS_VERIFY_CTX_add_flags(ctx, f);

    if ((cert_ctx = X509_STORE_new()) == NULL) {
        goto end;
    }

    X509_STORE_set_verify_cb(cert_ctx, ts_verify_cb);

    if ((lkp = X509_STORE_add_lookup(cert_ctx, X509_LOOKUP_file())) == NULL) {
        goto end;
    }

    if (!X509_LOOKUP_load_file(lkp, ca, X509_FILETYPE_PEM)) {
        goto end;
    }

    if (TS_VERIFY_CTX_set_store(ctx, cert_ctx) == NULL) {
        goto end;
    }
    cert_ctx = NULL;     /* Freed by TS_VERIFY_CTX_free */

    if (!TS_RESP_verify_response(ctx, ts_resp)) {
        goto end;
    }

    ret = 0;
end:
    if (ret) {
        print_err("Failed to validate response");
    }

    TS_RESP_free(ts_resp);
    TS_VERIFY_CTX_free(ctx);
    BIO_free_all(ts_req_bio);
    BIO_free_all(data);
    X509_STORE_free(cert_ctx);

    return ret;
}

int ts_print_proxy_req(FILE* out, const proxy_request_t *req) {
    fprintf(out, "sha256:\n");
    BIO_dump_indent_fp(out, (const char *)req->digest, sizeof(req->digest), 4);
    return 0;
}

int ts_print_tsa_req(FILE* out, const tsa_request_t *req) {
    int ret = -1;
    char buf[512] = { 0 };
    TS_REQ *ts_req = NULL;
    BIO *req_bio = NULL;
    BIO *req_bio_print = NULL;

    if ((req_bio = BIO_new_mem_buf(req->req, req->len)) == NULL) {
        goto end;
    }

    if (d2i_TS_REQ_bio(req_bio, &ts_req) == NULL) {
        goto end;
    }

    if ((req_bio_print = BIO_new(BIO_s_mem())) == NULL) {
        goto end;
    }
    
    if (!TS_REQ_print_bio(req_bio_print, ts_req)) {
        goto end;
    }

    if (BIO_read(req_bio_print, buf, sizeof(buf)) <= 0) {
        goto end;
    }

    fprintf(out, "%s", buf);
    ret = 0;
end:
    if (ret != 0) {
        print_err("Failed to print TS_REQ");
    }
    BIO_free_all(req_bio);
    BIO_free_all(req_bio_print);
    TS_REQ_free(ts_req);
    return ret;
}

int ts_print_tsa_rsp(FILE* out, const tsa_response_t *rsp) {
    int ret = -1;
    char buf[1024] = { 0 };
    TS_RESP *ts_rsp = NULL;
    BIO *rsp_bio = NULL;
    BIO *rsp_bio_print = NULL;

    if ((rsp_bio = BIO_new_mem_buf(rsp->ts, rsp->len)) == NULL) {
        goto end;
    }

    if (d2i_TS_RESP_bio(rsp_bio, &ts_rsp) == NULL) {
        goto end;
    }

    if ((rsp_bio_print = BIO_new(BIO_s_mem())) == NULL) {
        goto end;
    }
    
    if (!TS_RESP_print_bio(rsp_bio_print, ts_rsp)) {
        goto end;
    }

    if (BIO_read(rsp_bio_print, buf, sizeof(buf)) <= 0) {
        goto end;
    }

    fprintf(out, "%s", buf);
    ret = 0;
end:
    if (ret != 0) {
        print_err("Failed to print TS_RESP");
    }
    BIO_free_all(rsp_bio);
    BIO_free_all(rsp_bio_print);
    TS_RESP_free(ts_rsp);
    return ret;
}
