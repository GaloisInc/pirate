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
 * Copyright 2020 Two Six Labs, LLC.  All rights reserved.
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <argp.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define OPT_INPUT_ADDR   (1000)
#define OPT_INPUT_PORT   (1100)
#define OPT_OUTPUT_ADDR  (1200)
#define OPT_OUTPUT_PORT  (1300)
#define OPT_MAX_PACKETS  (1400)
#define OPT_RECORD_RECV  (1500)
#define OPT_RECORD_SEND  (1600)

#define DATAPOINTS_ARRAY_SIZE (1 << 20)

static struct argp_option options[] = {
    { "src_addr",       OPT_INPUT_ADDR,  "ip",  0, "source address",             0 },
    { "src_port",       OPT_INPUT_PORT,  "num", 0, "source port",                0 },
    { "dst_addr",       OPT_OUTPUT_ADDR, "ip",  0, "destination address",        0 },
    { "dst_port",       OPT_OUTPUT_PORT, "num", 0, "destination port",           0 },
    { "num_packets",    OPT_MAX_PACKETS, "num", 0, "max number of packets",      0 },
    { "record_receive", OPT_RECORD_RECV, NULL,  0, "record packet receive time", 0 },
    { "record_send",    OPT_RECORD_SEND, NULL,  0, "record packet send time",    0 },
    { NULL,             0 ,              NULL,  0, NULL,                         0 },
};

typedef struct {
    uint32_t src_port;
    uint32_t dst_port;
    char src_addr[INET6_ADDRSTRLEN];
    char dst_addr[INET6_ADDRSTRLEN];
    struct addrinfo *src_addrinfo;
    struct addrinfo *dst_addrinfo;
    uint64_t num_packets;
    uint8_t record_receive;
    uint8_t record_send;
} options_t;

static error_t parseOpt(int key, char* arg, struct argp_state* state) {
    options_t *opt = (options_t*) (state->input);
    char *endptr = NULL;
 
    switch (key) {
        case OPT_INPUT_ADDR:
            strncpy(opt->src_addr, arg, INET6_ADDRSTRLEN);
            break;
        case OPT_INPUT_PORT:
            opt->src_port = strtoul(arg, &endptr, 10);
            if (*endptr) {
                argp_error(state, "source port '%s' cannot be parsed", arg);
            }
            break;
        case OPT_OUTPUT_ADDR:
            strncpy(opt->dst_addr, arg, INET6_ADDRSTRLEN);
            break;
        case OPT_OUTPUT_PORT:
            opt->dst_port = strtoul(arg, &endptr, 10);
            if (*endptr) {
                argp_error(state, "destination port '%s' cannot be parsed", arg);
            }
            break;
        case OPT_MAX_PACKETS:
            opt->num_packets = strtoull(arg, &endptr, 10);
            if (*endptr) {
                argp_error(state, "max number of packets '%s' cannot be parsed", arg);
            }
            break;
        case OPT_RECORD_RECV:
            opt->record_receive = 1;
            break;
        case OPT_RECORD_SEND:
            opt->record_send = 1;
            break;
        default:
          return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static void parseArgs(int argc, char * argv[], options_t* opt)
{
    struct argp argp;
    argp.options = options;
    argp.parser = parseOpt;
    argp.args_doc = "";
    argp.doc = "UDP packet reflector";
    argp.children = NULL;
    argp.help_filter = NULL;
    argp.argp_domain = NULL;

    memset(opt, 0, sizeof(options_t));
    argp_parse(&argp, argc, argv, 0, 0, opt);
}

static int validateArgs(options_t* opt) {
    struct addrinfo hints;
    int rv, src_addr, dst_addr;
    struct sockaddr_in* ip4_addr;
    struct sockaddr_in6* ip6_addr;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_NUMERICHOST;

    if (opt->src_port == 0) {
        fprintf(stderr, "source port argument is required\n");
        return 1;
    }
    if (opt->src_port > UINT16_MAX) {
        fprintf(stderr, "source port argument '%d' is too large\n", opt->src_port);
        return 1;
    }
    if (opt->dst_port > UINT16_MAX) {
        fprintf(stderr, "destination port argument '%d' is too large\n", opt->dst_port);
        return 1;
    }

    src_addr = strnlen(opt->src_addr, 1) > 0;
    dst_addr = strnlen(opt->dst_addr, 1) > 0;

    if (dst_addr ^ (opt->dst_port > 0)) {
        fprintf(stderr, "destination port and destination address must be used together\n");
        return 1;
    }
    if (dst_addr) {
        rv = getaddrinfo(opt->dst_addr, NULL, &hints, &opt->dst_addrinfo);
        if (rv != 0) {
            fprintf(stderr, "unable to parse destination ip address '%s'\n", opt->dst_addr);
            return 1;
        }
    }
    if (!src_addr) {
        if (dst_addr && (opt->dst_addrinfo->ai_family == AF_INET6)) {
            strncpy(opt->src_addr, "0:0:0:0:0:0:0:0", INET6_ADDRSTRLEN);
        } else {
            strncpy(opt->src_addr, "0.0.0.0", INET6_ADDRSTRLEN);
        }
    }
    rv = getaddrinfo(opt->src_addr, NULL, &hints, &opt->src_addrinfo);
    if (rv != 0) {
        fprintf(stderr, "unable to parse source ip address '%s'\n", opt->src_addr);
        return 1;
    }

    switch (opt->src_addrinfo->ai_family) {
        case AF_INET:
            ip4_addr = ((struct sockaddr_in*) opt->src_addrinfo->ai_addr);
            ip4_addr->sin_port = htons(opt->src_port);
            break;
        case AF_INET6:
            ip6_addr = ((struct sockaddr_in6*) opt->src_addrinfo->ai_addr);
            ip6_addr->sin6_port = htons(opt->src_port);
            break;
        default:
            fprintf(stderr, "unexpected address family %d\n", opt->src_addrinfo->ai_family);
            return 1;
    }

    if (opt->num_packets == 0) {
        opt->num_packets = UINT64_MAX;
    }
    return 0;
}

static uint64_t to_nanos(struct timespec *tp) {
    uint64_t retval = tp->tv_sec;
    retval *= 1000000000;
    retval += tp->tv_nsec;
    return retval;
}

static void print_datapoints(uint64_t *datapoints, size_t end, char* label) {
    size_t start = ((end - 1) / DATAPOINTS_ARRAY_SIZE) * DATAPOINTS_ARRAY_SIZE;
    for(size_t i = start; i < end; i++) {
        printf("%ld\t%s\t%ld\n", i, label, datapoints[i % DATAPOINTS_ARRAY_SIZE]);
    }
}

int main(int argc, char *argv[]) {
    options_t options;
    int rv = 0;
    uint64_t npackets = 0;
    int recvSock = -1, sendSock = -1;
    unsigned char *data = (unsigned char*) calloc(UINT16_MAX, 1);
    uint64_t *recv_nanos = NULL, *send_nanos = NULL;
    size_t recv_idx = 0, send_idx = 0;

    parseArgs(argc, argv, &options);
    rv = validateArgs(&options);
    if (rv) {
        goto cleanup;
    }

    recvSock = socket(options.src_addrinfo->ai_family, SOCK_DGRAM, 0);
    if (recvSock < 0) {
        perror("unable to open socket for receiving");
        rv = -1;
        goto cleanup;
    }
    rv = bind(recvSock, options.src_addrinfo->ai_addr, options.src_addrinfo->ai_addrlen);
    if (rv < 0) {
        perror("unable to bind source socket");
        goto cleanup;
    }
    if (options.dst_addrinfo != NULL) {
        sendSock = socket(options.dst_addrinfo->ai_family, SOCK_DGRAM, 0);
        if (sendSock < 0) {
            perror("unable to open socket for sending");
            rv = -1;
            goto cleanup;
        }
        rv = connect(sendSock, options.dst_addrinfo->ai_addr, options.dst_addrinfo->ai_addrlen);
        if (rv < 0) {
            perror("unable to connect destination socket");
            goto cleanup;
        }
    }

    recv_nanos = (uint64_t*) calloc(DATAPOINTS_ARRAY_SIZE, sizeof(uint64_t));
    send_nanos = (uint64_t*) calloc(DATAPOINTS_ARRAY_SIZE, sizeof(uint64_t));

    while (npackets < options.num_packets) {
        struct timespec tspec;        
        uint64_t nanos;
        ssize_t nbytes = recv(recvSock, data, UINT16_MAX, 0);
        if (nbytes < 0) {
            perror("receive error");
            goto cleanup;
        }
        if (options.record_receive) {
            clock_gettime(CLOCK_MONOTONIC, &tspec);
            nanos = to_nanos(&tspec);
            recv_nanos[recv_idx % DATAPOINTS_ARRAY_SIZE] = nanos;
            recv_idx++;
            if ((recv_idx % DATAPOINTS_ARRAY_SIZE) == 0) {
                print_datapoints(recv_nanos, recv_idx, "recv");
            }
        }
        if (sendSock >= 0) {
            nbytes = send(sendSock, data, (size_t) nbytes, 0);
            if (nbytes < 0) {
                perror("send error");
                goto cleanup;
            }
            if (options.record_send) {
                clock_gettime(CLOCK_MONOTONIC, &tspec);
                nanos = to_nanos(&tspec);
                send_nanos[send_idx % DATAPOINTS_ARRAY_SIZE] = nanos;
                send_idx++;
                if ((send_idx % DATAPOINTS_ARRAY_SIZE) == 0) {
                    print_datapoints(send_nanos, send_idx, "send");
                }
            }
        }
        npackets++;
    }
    if (recv_idx > 0) {
        print_datapoints(recv_nanos, recv_idx, "recv");        
    }
    if (send_idx > 0) {
        print_datapoints(send_nanos, send_idx, "send");        
    }

cleanup:
    if (send_nanos != NULL) {
        free(send_nanos);
    }
    if (recv_nanos != NULL) {
        free(recv_nanos);
    }
    if (recvSock >= 0) {
        close(recvSock);
    }
    if (sendSock >= 0) {
        close(sendSock);
    }
    if (options.dst_addrinfo != NULL) {
        freeaddrinfo(options.dst_addrinfo);
    }
    if (options.src_addrinfo != NULL) {
        freeaddrinfo(options.src_addrinfo);
    }
    free(data);
    return rv;
}
