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
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <regex.h>
#include <pthread.h>
#include <sys/stat.h>
#include "primitives.h"
#include "tiny.h"



#define HIGH_TO_LOW_CH 0
#define LOW_TO_HIGH_CH 1

#define DATA_LEN            (32 << 10)         // 32 KB
typedef struct {
    char buf[DATA_LEN];
    int len;
} data_t;

typedef enum {
    LEVEL_HIGH,
    LEVEL_LOW
} level_e;

#define LOW_NAME  "\033[1;32mLOW\033[0m"
#define HIGH_NAME "\033[1;31mHIGH\033[0m"

#ifdef HIGH
#define NAME                HIGH_NAME
#elif LOW
#define NAME                LOW_NAME
#else
#define NAME                "undefined"
#endif /* LOW */

static int load_web_content_high(data_t* data, const char *path) {
    struct stat sbuf;

    if (stat(path, &sbuf) < 0) {
        fprintf(stderr, "ERROR: could not find file %s\n", path);
        return 404;
    }

    if (sbuf.st_size >= (long)sizeof(data->buf)) {
        fprintf(stderr, "ERROR: file %s exceeds size limits\n", path);
        return 500;
    }

    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR failed to open %s\n", path);
        return 500;
    }

    if (fread(data->buf, sbuf.st_size, 1, fp) != 1) {
        fprintf(stderr, "Failed to read %s file\n", path);
        return 500;
    }

    fclose(fp);

    data->buf[sbuf.st_size] = '\0';
    data->len = sbuf.st_size;
    return 200;
}

static int load_web_content_low(data_t* data, char *path) {
    int rv, len;
    ssize_t num;

    len = strnlen(path, PATHSIZE);
    num = pirate_write(LOW_TO_HIGH_CH, &len, sizeof(int));
    if (num != sizeof(int)) {
        fprintf(stderr, "Failed to send request length\n");
        return 500;
    }

    num = pirate_write(LOW_TO_HIGH_CH, path, len);
    if (num != len) {
        fprintf(stderr, "Failed to send request path\n");
        return 500;
    }

    printf("Sent read request to the %s side\n", HIGH_NAME);

    num = pirate_read(HIGH_TO_LOW_CH, &rv, sizeof(rv));
    if (num != sizeof(rv)) {
        fprintf(stderr, "Failed to receive status code\n");
        return 500;
    }
    if (rv != 200) {
        return rv;
    }

    /* Read and validate response length */
    num = pirate_read(HIGH_TO_LOW_CH, &len, sizeof(len));
    if (num != sizeof(len)) {
        fprintf(stderr, "Failed to receive response length\n");
        return 500;
    }

    if (len >= (long)sizeof(data->buf)) {
        fprintf(stderr, "Response length %d is too large\n", len);
        return 500;
    }

    /* Read back the response */
    num = pirate_read(HIGH_TO_LOW_CH, data->buf, len);
    if (num != len) {
        fprintf(stderr, "Failed to read back the response\n");
        return 500;
    }

    /* Success */
    data->len = len;
    printf("Received %d bytes from the %s side\n", data->len, HIGH_NAME);
    return 200;
}

static int load_web_content(data_t* data, char *path, level_e level) {
    switch (level) {
        case LEVEL_HIGH:
            return load_web_content_high(data, path);
    
        case LEVEL_LOW:
            return load_web_content_low(data, path);

        default:
            return 500;
    }
}


static void* gaps_thread(void *arg) {
    (void)arg;
    data_t high_data;
    data_t low_data;

    while (1) {
        /* Low side requests data by writing zero */
        int rv, len = 0;
        char path[PATHSIZE];

        ssize_t num = pirate_read(LOW_TO_HIGH_CH, &len, sizeof(len));
        if (num != sizeof(len)) {
            fprintf(stderr, "Failed to read request from the low side\n");
            exit(-1);
        }

        if ((len <= 0) || (len >= (long)sizeof(path))) {
            fprintf(stderr, "Invalid request length from the low side %d\n", len);
            continue;
        }

        memset(path, 0, sizeof(path));
        num = pirate_read(LOW_TO_HIGH_CH, &path, len);
        if (num != len) {
            fprintf(stderr, "Invalid request path from the low side %d\n", len);
            continue;
        }

        printf("Received data request from the %s side\n", LOW_NAME);

        /* Read in high data */
        rv = load_web_content_high(&high_data, path);
        num = pirate_write(HIGH_TO_LOW_CH, &rv, sizeof(rv));
        if (num != sizeof(rv)) {
            fprintf(stderr, "Failed to send status code\n");
            continue;
        }
        if (rv != 200) {
            continue;
        }

        /* Create filtered data (remove bold text) */
        regmatch_t match;
        regex_t regex;
        int ret = regcomp(&regex, "<b>[0-9]*,</b>\\s*", REG_EXTENDED);
        if (ret != 0) {
            fprintf(stderr, "Failed to compile regex\n");
            continue;
        }

        char* search = high_data.buf;
        char* wr = low_data.buf;
        while ((ret = regexec(&regex, search, 1, &match, 0)) == 0) {
            memcpy(wr, search, match.rm_so);
            wr += match.rm_so;
            search += match.rm_eo;
        }
        int tail_len = high_data.len - (search - high_data.buf);
        memcpy(wr, search, tail_len);
        wr[tail_len] = '\0';
        low_data.len = strnlen(low_data.buf, sizeof(low_data.buf));

        /* Reply back. Data length is sent first */
        num = pirate_write(HIGH_TO_LOW_CH, &low_data.len, sizeof(low_data.len));
        if (num != sizeof(low_data.len)) {
            fprintf(stderr, "Failed to send response length\n");
            continue;
        }

        num = pirate_write(HIGH_TO_LOW_CH, &low_data.buf, low_data.len);
        if (num != low_data.len) {
            fprintf(stderr, "Failed to send response content\n");
            continue;
        }

        printf("Sent %d bytes to the %s side\n\n", low_data.len, LOW_NAME);
    }
    return NULL;
}


static int run_gaps()
{
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, gaps_thread, NULL);
    if (ret != 0) {
        fprintf(stderr, "Failed to start the GAPS thread\n");
        return ret;
    }
    return 0;
}


static int web_server(int port, level_e level) {
    int rv;
    server_t si;
    client_t ci;
    request_t ri;
    data_t data;

    /* Create, initialize, bind, listen on server socket */
    server_connect(&si, port);

    /*
     * Wait for a connection request, parse HTTP, serve high requested content,
     * close connection.
     */
    while (1) {
        /* accept client's connection and open fstream */
        client_connect(&si, &ci);

        /* process client request */
        client_request_info(&ci, &ri);

        /* tiny only supports the GET method */
        if (strcasecmp(ri.method, "GET")) {
            cerror(ci.stream, ri.method, 405, "Not Implemented");
            client_disconnect(&ci);
            continue;
        }

        /* Get data from the high side */
        rv = load_web_content(&data, ri.filename, level);
        if (rv != 200) {
            cerror(ci.stream, ri.filename, rv, "Loading Data");
            client_disconnect(&ci);
            continue;
        }

        /* Serve low content */
        rv = serve_static_content(&ci, &ri, data.buf, data.len);
        if  (rv < 0) {
            client_disconnect(&ci);
            continue;
        }
        printf("%s served %d bytes of web content\n", NAME, data.len);

        client_disconnect(&ci);
    }

    return 0;
}


int main_high(int argc, char* argv[]) {
    /* Validate and parse command-line options */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return -1;
    }

    const short port = atoi(argv[1]);
    printf("\n%s web server on port %d\n\n", NAME, port);

    if (pirate_open(HIGH_TO_LOW_CH, O_WRONLY) < 0) {
        perror("Unable to open high to low channel in write-only mode");
        return -1;
    }

    if (pirate_open(LOW_TO_HIGH_CH, O_RDONLY) < 0) {
        perror("Unable to open low to high channel in read-only mode");
        return -1;
    }

    if (run_gaps() != 0) {
        fprintf(stderr, "Failed to start the GAPS handler");
        return -1;
    }

    /* Web server */
    return web_server(port, LEVEL_HIGH);
}


int main_low(int argc, char* argv[]) {
    /* Validate and parse command-line options */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return -1;
    }

    const short port = atoi(argv[1]);
    printf("\n%s web server on port %d\n\n", NAME, port);

    if (pirate_open(HIGH_TO_LOW_CH, O_RDONLY) < 0) {
        perror("Unable to open high to low channel in read-only mode");
        return -1;
    }
    if (pirate_open(LOW_TO_HIGH_CH, O_WRONLY) < 0) {
        perror("Unable to open low to high channel in write-only mode");
        return -1;
    }

    /* Web server */
    return web_server(port, LEVEL_LOW);
}


int main(int argc, char* argv[]) {
#ifdef HIGH
    return main_high(argc, argv);
#endif

#ifdef LOW
    return main_low(argc, argv);
#endif
}
