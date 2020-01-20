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


/* cerror - returns an error message to the client */
void cerror(FILE *s, char *cause, int statuscode, char *shortmsg) {
    fprintf(s, "HTTP/1.1 %d %s\r\n", statuscode, shortmsg);
    fprintf(s, "Content-type: text/html\r\n");
    fprintf(s, "\r\n");
    fprintf(s, "<html>");
    fprintf(s, "<body bgcolor=ffffff>\n");
    fprintf(s, "%s %d: %s\n", cause, statuscode, shortmsg);
    fprintf(s, "<hr><em>The Tiny Web server</em>\n");
    fprintf(s, "</body></html>");
}

static char* socket_err = "socket";
static char* setsockopt_err = "setsockopt";
static char* bind_err = "bind";
static char* listen_err = "listen";

char *server_connect(server_t* si, int port) {
    si->portno = port;

    /* open socket descriptor */
    si->parentfd = socket(AF_INET, SOCK_STREAM, 0);
    if (si->parentfd < 0) {
        return socket_err;
    }

    /* allows us to restart server immediately */
    int optval = 1;
    int sts = setsockopt(si->parentfd, SOL_SOCKET, SO_REUSEADDR,
                        (const void *)&optval, sizeof(int));
    if (sts != 0) {
        return setsockopt_err;
    }

    /* bind port to socket */
    bzero((char *)&si->serveraddr, sizeof(si->serveraddr));
    si->serveraddr.sin_family = AF_INET;
    si->serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    si->serveraddr.sin_port = htons((unsigned short)si->portno);
    sts = bind(si->parentfd, (struct sockaddr *)&si->serveraddr,
                sizeof(si->serveraddr));
    if (sts < 0) {
        return bind_err;
    }

    /* get us ready to accept connection requests (max 5 clients) */
    sts = listen(si->parentfd, 5);
    if (sts < 0) {
        return listen_err;
    }

    return NULL;
}

static char* close_server_err = "close child socket";

char *server_disconnect(server_t* si) {
    if (si->parentfd) {
        if (close(si->parentfd)) {
            return close_server_err;
        }
    }
    return NULL;
}

static char* accept_err = "accept";
static char* fdopen_err = "fdopen";

char *client_connect(const server_t* si, client_t* ci) {
    /* wait for a connection request */
    socklen_t clientlen = sizeof(ci->clientaddr);
    ci->childfd = accept(si->parentfd, (struct sockaddr *)&ci->clientaddr,
                            &clientlen);
    if (ci->childfd < 0) {
        return accept_err;
    }

    /* open the child socket descriptor as a stream */
    ci->stream = fdopen(ci->childfd, "r+");
    if (ci->stream == NULL) {
        return fdopen_err;
    }

    return NULL;
}

static char* fclose_err = "fclose";

char *client_disconnect(client_t* ci) {
    if (ci->stream) {
        if (fclose(ci->stream)) {
            return fclose_err;
        }
    }
    return NULL;
}


static void read_http_line(char* buf, size_t sz, FILE* stream) {
    char *ignored = fgets(buf, sz, stream);
    (void) ignored;
}


void client_request_info(const client_t* ci, request_t* ri) {
    /* get the HTTP request line */
    read_http_line(ri->buf, BUFSIZE, ci->stream);
    sscanf(ri->buf, "%s %s %s\n", ri->method, ri->uri, ri->version);

    /* read (and ignore) the HTTP headers */
    do {
        read_http_line(ri->buf, BUFSIZE, ci->stream);
    } while(strncmp(ri->buf, "\r\n", 3));

    /* parse the uri [crufty] */
    const char *const filepart =
      strnlen(ri->uri, 2) < 2 ? "/index.html" : ri->uri;

    snprintf(ri->filename, sizeof(ri->filename), ".%s", filepart);
}


int serve_static_content(client_t* ci, request_t* ri, char* buf, int len) {
    (void) ri;

    /* send response header and passed data */
    fprintf(ci->stream, "HTTP/1.1 200 OK\r\n");
    fprintf(ci->stream, "Server: Tiny Web Server\r\n");
    fprintf(ci->stream, "Content-length: %d\r\n", len);
    fprintf(ci->stream, "Content-type: text/html\r\n");
    fprintf(ci->stream, "\r\n");
    fwrite(buf, len, 1, ci->stream);
    fflush(ci->stream);
    return 0;
}
