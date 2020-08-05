#include <libpirate.h>
#include <string.h>
#include <stdint.h>

#include "handlers.h"

// TODO: Smarter handler lookup, potentially.
resource_handler_t *lookup_handler(const char *type)
{
    size_t i;

    if(!type)
        return NULL;

    for(i = 0; i < HANDLER_TABLE_MAX && handler_table[i].type; ++i)
        if(!strcmp(type, handler_table[i].type))
            return handler_table[i].handler;

    return NULL;
}

int cstring_resource_handler(pal_env_t *env, const struct resource *rsc)
{
    const char *s = rsc->r_contents.cc_string_value;

    if(!s)
        return -1;

    if(pal_add_to_env(env, s, strlen(s)))
        return -1;

    return 0;
}

int int64_resource_handler(pal_env_t *env, const struct resource *rsc)
{
    const int64_t *n = rsc->r_contents.cc_integer_value;

    if(!n)
        return -1;

    if(pal_add_to_env(env, n, sizeof(*n)))
        return -1;

    return 0;
}

int bool_resource_handler(pal_env_t *env, const struct resource *rsc)
{
    const bool *b = rsc->r_contents.cc_boolean_value;

    if(!b)
        return -1;

    if(pal_add_to_env(env, b, sizeof(*b)))
        return -1;

    return 0;
}

int file_resource_handler(pal_env_t *env, const struct resource *rsc)
{
    const char *path = rsc->r_contents.cc_file_path;
    const int *flags = rsc->r_contents.cc_file_flags;
    int fd;

    if((fd = open(path, flags ? *flags : O_RDWR)) < 0)
        return -1;
    // TODO: Allow file creation modes to be set?
    // TODO: Allow paths relative to config?

    if(pal_add_fd_to_env(env, fd))
        return -1;

    return 0;
}

int pirate_channel_resource_handler(pal_env_t *env, const struct resource *rsc)
{
    pirate_channel_param_t params = {0};

    params.channel_type = rsc->r_contents.cc_channel_type;
    switch(params.channel_type) {
        case DEVICE:
            if(rsc->r_contents.cc_path)
                strncpy(params.channel.device.path,
                        rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.device.min_tx
                    = rsc->r_contents.cc_min_tx_size;
            params.channel.device.mtu
                    = rsc->r_contents.cc_mtu;
            break;
        case PIPE:
            if(rsc->r_contents.cc_path)
                strncpy(params.channel.pipe.path,
                        rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.pipe.min_tx
                    = rsc->r_contents.cc_min_tx_size;
            params.channel.pipe.mtu
                    = rsc->r_contents.cc_mtu;
            break;
        case UNIX_SOCKET:
            if(rsc->r_contents.cc_path)
                strncpy(params.channel.unix_socket.path,
                        rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.unix_socket.min_tx
                    = rsc->r_contents.cc_min_tx_size;
            params.channel.unix_socket.mtu
                    = rsc->r_contents.cc_mtu;
            params.channel.unix_socket.buffer_size
                    = rsc->r_contents.cc_buffer_size;
            break;
        case TCP_SOCKET:
            if(rsc->r_contents.cc_host)
                strncpy(params.channel.tcp_socket.addr,
                        rsc->r_contents.cc_host, INET_ADDRSTRLEN);
            params.channel.tcp_socket.port
                    = rsc->r_contents.cc_port;
            params.channel.tcp_socket.min_tx
                    = rsc->r_contents.cc_min_tx_size;
            params.channel.tcp_socket.mtu
                    = rsc->r_contents.cc_mtu;
            params.channel.tcp_socket.buffer_size
                    = rsc->r_contents.cc_buffer_size;
            break;
        case UDP_SOCKET:
            if(rsc->r_contents.cc_host)
                strncpy(params.channel.udp_socket.addr,
                        rsc->r_contents.cc_host, INET_ADDRSTRLEN);
            params.channel.udp_socket.port
                    = rsc->r_contents.cc_port;
            params.channel.udp_socket.mtu
                    = rsc->r_contents.cc_mtu;
            params.channel.udp_socket.buffer_size
                    = rsc->r_contents.cc_buffer_size;
            break;
        case SHMEM:
            strncpy(params.channel.shmem.path,
                    rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.shmem.mtu
                    = rsc->r_contents.cc_mtu;
            params.channel.shmem.max_tx
                    = rsc->r_contents.cc_max_tx_size;
            params.channel.shmem.buffer_size
                    = rsc->r_contents.cc_buffer_size;
            break;
        case UDP_SHMEM:
            if(rsc->r_contents.cc_path)
                strncpy(params.channel.udp_shmem.path,
                        rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.shmem.mtu
                    = rsc->r_contents.cc_mtu;
            params.channel.udp_shmem.buffer_size
                    = rsc->r_contents.cc_buffer_size;
            params.channel.udp_shmem.packet_size
                    = rsc->r_contents.cc_packet_size;
            params.channel.udp_shmem.packet_count
                    = rsc->r_contents.cc_packet_count;
            break;
        case UIO_DEVICE:
            if(rsc->r_contents.cc_path)
                strncpy(params.channel.uio.path,
                        rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.uio.mtu
                    = rsc->r_contents.cc_mtu;
            params.channel.uio.max_tx
                    = rsc->r_contents.cc_max_tx_size;
            params.channel.uio.region
                    = rsc->r_contents.cc_region;
            break;
        case SERIAL:
            if(rsc->r_contents.cc_path)
                strncpy(params.channel.serial.path,
                        rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.serial.mtu
                    = rsc->r_contents.cc_mtu;
            params.channel.serial.max_tx
                    = rsc->r_contents.cc_max_tx_size;
            params.channel.serial.baud
                    = rsc->r_contents.cc_baud;
            break;
        case MERCURY:
            params.channel.mercury.mtu
                    = rsc->r_contents.cc_mtu;
            params.channel.mercury.session.level
                    = rsc->r_contents.cc_session->sess_level;
            params.channel.mercury.session.source_id
                    = rsc->r_contents.cc_session->sess_src_id;
            params.channel.mercury.session.destination_id
                    = rsc->r_contents.cc_session->sess_dst_id;
            params.channel.mercury.session.message_count
                    = rsc->r_contents.cc_session->sess_messages_count;
            if(rsc->r_contents.cc_session->sess_messages)
                memcpy(params.channel.mercury.session.messages,
                        rsc->r_contents.cc_session->sess_messages,
                        rsc->r_contents.cc_session->sess_messages_count
                            * sizeof(uint32_t));
            params.channel.mercury.session.id
                    = rsc->r_contents.cc_session->sess_id;
            break;
        case GE_ETH:
            if(rsc->r_contents.cc_host)
                strncpy(params.channel.ge_eth.addr,
                        rsc->r_contents.cc_host, INET_ADDRSTRLEN);
            params.channel.ge_eth.mtu
                    = rsc->r_contents.cc_mtu;
            params.channel.ge_eth.port
                    = rsc->r_contents.cc_port;
            params.channel.ge_eth.message_id
                    = rsc->r_contents.cc_message_id;
            break;
        default:
            return -1;
    }

    size_t pstr_len = pirate_unparse_channel_param(&params, NULL, 0);
    if(pstr_len <= 0)
        return -1;

    char pstr[pstr_len + 1];
    if(pirate_unparse_channel_param(&params, pstr, sizeof pstr) != pstr_len)
        return -1;

    if(pal_add_to_env(env, pstr, pstr_len))
        return -1;

    return 0;
}

struct handler_table_entry handler_table[HANDLER_TABLE_MAX] = {
    { "boolean",        &bool_resource_handler },
    { "file",           &file_resource_handler },
    { "integer",        &int64_resource_handler },
    { "pirate_channel", &pirate_channel_resource_handler },
    { "string",         &cstring_resource_handler },
    { NULL,             NULL },
};
