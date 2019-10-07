#include <regex.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include "primitives.h"
#include "common.h"
#include "tiny.h"

#define HTML_PATH           "./index.html"
#define HTML_BOLD_REGEX     "<b>[0-9]*,</b>\\s*"

struct {
    struct {
        int wr;                     // high to low channel
        int rd;                     // low to high channel
    } pirate;

    struct {
        example_data_t high;
        example_data_t low;
    } data;
} ctx;

void __attribute__ ((constructor())) pirate_init(int argc, char* argv[]) {
    (void) argc, (void)argv;

    /* start clean */
    memset(&ctx, 0x00, sizeof(ctx));

    /* Open GAPS channels in order from lowest to highest */

    /* Open GAPS write channel */
    ctx.pirate.wr = pirate_open(HIGH_TO_LOW_CH, O_WRONLY);
    if (ctx.pirate.wr == -1) {
        printf("Failed to open write channel\n");
        exit(-1);
    }
    printf("INIT: HIGH->LOW (WR) channel created: CH %d\n", ctx.pirate.wr);

    /* Open GAPS read channel */
    ctx.pirate.rd = pirate_open(LOW_TO_HIGH_CH, O_RDONLY);
    if (ctx.pirate.rd == -1) {
        printf("Failed to open read channel\n");
        exit(-1);
    }
    printf("INIT: HIGH<-LOW (RD) channel created: CH %d\n", ctx.pirate.rd);
}

void __attribute__ ((destructor())) pirate_term() {
    if (ctx.pirate.rd > 0) {
        pirate_close(LOW_TO_HIGH_CH, O_RDONLY);
        ctx.pirate.rd = -1;
        printf("TERM: HIGH<-LOW (RD) channel closed: CH %d\n", ctx.pirate.rd);
    }

    if (ctx.pirate.wr > 0) {
        pirate_close(HIGH_TO_LOW_CH, O_WRONLY);
        ctx.pirate.wr = -1;
        printf("TERM: HIGH->LOW (WR) channel closed: CH %d\n", ctx.pirate.wr);
    }
}

static int load_high_data(example_data_t* to, const char* from) {
    struct stat sbuf;
    if (stat(from, &sbuf) < 0) {
        fprintf(stderr, "ERROR: could not find file %s\n", from);
        return -1;
    }

    if (sbuf.st_size >= (long)sizeof(to->buf)) {
        fprintf(stderr, "ERROR: file %s exceeds size limits\n", from);
        return -1;
    }

    FILE* fp = fopen(from, "r");
    if (fp == NULL) {
        fprintf(stderr, "ERROR failed to open %s\n", from);
        return -1;
    }

    if (fread(to->buf, sbuf.st_size, 1, fp) != 1) {
        fprintf(stderr, "Failed to read %s file\n", from);
        return -1;
    }

    to->buf[sbuf.st_size] = '\0';
    to->len = sbuf.st_size;
    fclose(fp);
    return 0;
}


static int load_low_data(example_data_t* high, example_data_t* low) {
    regmatch_t match;

    /* Filter out bold html text */
    regex_t regex;
    int ret = regcomp(&regex, HTML_BOLD_REGEX, REG_EXTENDED);
    if (ret != 0) {
        fprintf(stderr, "Failed to compile regex\n");
        return -1;
    }

    char* search = high->buf;
    char* wr = low->buf;
    while ((ret = regexec(&regex, search, 1, &match, 0)) == 0) {
        memcpy(wr, search, match.rm_so);
        wr += match.rm_so;
        search += match.rm_eo;
    }
    int tail_len = high->len - (search - high->buf);
    memcpy(wr, search, tail_len);
    wr[tail_len] = '\0';
    low->len = strlen(low->buf);

    return 0;
}


static int high_handler(short port) {
    server_t si;
    client_t ci;
    request_t ri;

    /* create, initialize, bind, listen on server socket */
    server_connect(&si, port);

    /*
     * wait for a connection request, parse HTTP, serve high requested content,
     * close connection.
     */
    while (1) {
        /* accept client's connection and open fstream */
        client_connect(&si, &ci);

        /* process client request */
        client_request_info(&ci, &ri);

        /* tiny only supports the GET method */
        if (strcasecmp(ri.method, "GET")) {
            cerror(ci.stream, ri.method, "501", "Not Implemented",
                    "Tiny does not implement this method");
            client_disconnect(&ci);
            continue;
        }

        /* Serve high content */
        example_data_t* data = &ctx.data.high;
        int ret = serve_static_content(&ci, &ri, data->buf, data->len);
        if  (ret < 0) {
            client_disconnect(&ci);
            continue;
        }

        client_disconnect(&ci);
    }

    return 0;
}


static void* low_handler(void *arg) {
    const example_data_t* data = (const example_data_t* )arg;

    while (1) {
        /* Low side requests data by writing zero */
        int len = 0;
        ssize_t num = pirate_read(ctx.pirate.rd, &len, sizeof(len));
        if (num != sizeof(len)) {
            fprintf(stderr, "Failed to read request from the low side\n");
            continue;
        }

        if (len != 0) {
            fprintf(stderr, "Invalied request from the low side %d\n", len);
            continue;
        }

        /* Reply back. Data length is sent first */
        num = pirate_write(ctx.pirate.wr, &data->len, sizeof(data->len));
        if (num != sizeof(data->len)) {
            fprintf(stderr, "Failed to send response length\n");
            continue;
        }

        num = pirate_write(ctx.pirate.wr, &data->buf, data->len);
        if (num != data->len) {
            fprintf(stderr, "Failed to send response content\n");
            continue;
        }

        printf("Send %d bytes to the low side\n", data->len);
    }

    return NULL;
}


int main(int argc, char* argv[]) {
    short port;

    /* Validate and parse command-line options */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return -1;
    }

    port = atoi(argv[1]);

    /* Load high data from a static file */
    int ret = load_high_data(&ctx.data.high, HTML_PATH);
    if (ret != 0) {
        fprintf(stderr, "Failed to load high data\n");
        return ret;
    }

    /* Load low-side data */
    ret = load_low_data(&ctx.data.high, &ctx.data.low);
    if (ret != 0) {
        fprintf(stderr, "Failed to generate low data\n");
        return ret;
    }

    /* Start the PIRATE handler thread */
    pthread_t tid;
    ret = pthread_create(&tid, NULL, low_handler, &ctx.data.low);
    if (ret != 0) {
        fprintf(stderr, "Failed to start low handler thread\n");
        return ret;
    }

    /* Handle high webserver requests */
    return high_handler(port);
}
