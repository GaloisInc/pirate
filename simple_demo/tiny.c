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
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tiny.h"


/* error - wrapper for perror used for bad syscalls */
static void error(char *msg) {
    perror(msg);
    exit(1);
}


/* cerror - returns an error message to the client */
void cerror(FILE *s, char *cause, char *errno, char *shortmsg, char *longmsg) {
    fprintf(s, "HTTP/1.1 %s %s\n", errno, shortmsg);
    fprintf(s, "Content-type: text/html\n");
    fprintf(s, "\n");
    fprintf(s, "<html><title>Tiny Error</title>");
    fprintf(s, "<body bgcolor=ffffff>\n");
    fprintf(s, "%s: %s\n", errno, shortmsg);
    fprintf(s, "<p>%s: %s\n", longmsg, cause);
    fprintf(s, "<hr><em>The Tiny Web server</em>\n");
}


void server_connect(server_t* si, int port) {
    si->portno = port;

    /* open socket descriptor */
    si->parentfd = socket(AF_INET, SOCK_STREAM, 0);
    if (si->parentfd < 0) {
        error("ERROR opening socket");
    }

    /* allows us to restart server immediately */
    int optval = 1;
    int sts = setsockopt(si->parentfd, SOL_SOCKET, SO_REUSEADDR,
                        (const void *)&optval, sizeof(int));
    if (sts != 0) {
        error("ERROR setsockopt SO_REUSEADDR");
    }

    /* bind port to socket */
    bzero((char *)&si->serveraddr, sizeof(si->serveraddr));
    si->serveraddr.sin_family = AF_INET;
    si->serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    si->serveraddr.sin_port = htons((unsigned short)si->portno);
    sts = bind(si->parentfd, (struct sockaddr *)&si->serveraddr,
                sizeof(si->serveraddr));
    if (sts < 0) {
        error("ERROR on binding");
    }

    /* get us ready to accept connection requests (max 5 clients) */
    sts = listen(si->parentfd, 5);
    if (sts < 0) {
        error("ERROR on listen");
    }
}


void client_connect(const server_t* si, client_t* ci) {
    /* wait for a connection request */
    socklen_t clientlen = sizeof(ci->clientaddr);
    ci->childfd = accept(si->parentfd, (struct sockaddr *)&ci->clientaddr,
                            &clientlen);
    if (ci->childfd < 0) {
        error("ERROR on accept");
    }

    /* open the child socket descriptor as a stream */
    ci->stream = fdopen(ci->childfd, "r+");
    if (ci->stream == NULL) {
        error("ERROR on fdopen");
    }
}


void client_disconnect(client_t* ci) {
    fclose(ci->stream);
    close(ci->childfd);
}


static void read_http_line(char* buf, size_t sz, FILE* stream) {
    fgets(buf, sz, stream);
}


void client_request_info(const client_t* ci, request_t* ri) {
    /* get the HTTP request line */
    read_http_line(ri->buf, BUFSIZE, ci->stream);
    sscanf(ri->buf, "%s %s %s\n", ri->method, ri->uri, ri->version);

    /* read (and ignore) the HTTP headers */
    do {
        read_http_line(ri->buf, BUFSIZE, ci->stream);
    } while(strcmp(ri->buf, "\r\n"));

    /* parse the uri [crufty] */
    strcpy(ri->filename, ".");
    strcat(ri->filename, ri->uri);
    if (ri->uri[strlen(ri->uri) - 1] == '/') {
        strcat(ri->filename, "index.html");

    }
}


int serve_static_content(client_t* ci, request_t* ri, char* buf, int len) {
    (void) ri;

    /* send response header and passed data */
    fprintf(ci->stream, "HTTP/1.1 200 OK\n");
    fprintf(ci->stream, "Server: Tiny Web Server\n");
    fprintf(ci->stream, "Content-length: %d\n", len);
    fprintf(ci->stream, "Content-type: text/html\n");
    fprintf(ci->stream, "\r\n");
    fwrite(buf, len, 1, ci->stream);
    fflush(ci->stream);
    return 0;
}
