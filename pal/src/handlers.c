#include <libpirate.h>
#include <string.h>
#include <stdint.h>

#include "handlers.h"

int cstring_resource_handler(pal_env_t *env,
        const struct app *app, const struct resource *rsc)
{
    const char *s = rsc->r_contents.cc_string_value;

    if(!s)
        return -1;

    if(pal_add_to_env(env, s, strlen(s)))
        return -1;

    return 0;
}

int int64_resource_handler(pal_env_t *env,
        const struct app *app, const struct resource *rsc)
{
    const int64_t *n = rsc->r_contents.cc_integer_value;

    if(!n)
        return -1;

    if(pal_add_to_env(env, n, sizeof(*n)))
        return -1;

    return 0;
}

int bool_resource_handler(pal_env_t *env,
        const struct app *app, const struct resource *rsc)
{
    const bool *b = rsc->r_contents.cc_boolean_value;

    if(!b)
        return -1;

    if(pal_add_to_env(env, b, sizeof(*b)))
        return -1;

    return 0;
}

int file_resource_handler(pal_env_t *env,
        const struct app *app, const struct resource *rsc)
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

int pirate_channel_resource_handler(pal_env_t *env,
        const struct app *app, const struct resource *rsc)
{
    char *pstr;
    ssize_t pstr_len;
    pirate_channel_param_t params = {0};

    params.channel_type = rsc->r_contents.cc_channel_type;
    switch(params.channel_type) {
        case DEVICE:
            strncpy(params.channel.pipe.path,
                    rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.pipe.iov_len
                    = rsc->r_contents.cc_iov_length;
            break;
        case PIPE:
            strncpy(params.channel.device.path,
                    rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.device.iov_len
                    = rsc->r_contents.cc_iov_length;
            break;
        case UNIX_SOCKET:
            strncpy(params.channel.unix_socket.path,
                    rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.unix_socket.iov_len
                    = rsc->r_contents.cc_iov_length;
            params.channel.unix_socket.buffer_size
                    = rsc->r_contents.cc_buffer_size;
            break;
        case TCP_SOCKET:
            strncpy(params.channel.tcp_socket.addr,
                    rsc->r_contents.cc_host, INET_ADDRSTRLEN);
            params.channel.tcp_socket.port
                    = rsc->r_contents.cc_port;
            params.channel.tcp_socket.iov_len
                    = rsc->r_contents.cc_iov_length;
            params.channel.tcp_socket.buffer_size
                    = rsc->r_contents.cc_buffer_size;
            break;
        case UDP_SOCKET:
            strncpy(params.channel.udp_socket.addr,
                    rsc->r_contents.cc_host, INET_ADDRSTRLEN);
            params.channel.udp_socket.port
                    = rsc->r_contents.cc_port;
            params.channel.udp_socket.iov_len
                    = rsc->r_contents.cc_iov_length;
            params.channel.udp_socket.buffer_size
                    = rsc->r_contents.cc_buffer_size;
            break;
        case SHMEM:
            strncpy(params.channel.shmem.path,
                    rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.shmem.buffer_size
                    = rsc->r_contents.cc_buffer_size;
            break;
        case UDP_SHMEM:
            strncpy(params.channel.udp_shmem.path,
                    rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.udp_shmem.buffer_size
                    = rsc->r_contents.cc_buffer_size;
            params.channel.udp_shmem.packet_size
                    = rsc->r_contents.cc_packet_size;
            params.channel.udp_shmem.packet_count
                    = rsc->r_contents.cc_packet_count;
            break;
        case UIO_DEVICE:
            strncpy(params.channel.uio.path,
                    rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.uio.region
                    = rsc->r_contents.cc_region;
            break;
        case SERIAL:
            strncpy(params.channel.serial.path,
                    rsc->r_contents.cc_path, PIRATE_LEN_NAME);
            params.channel.serial.baud
                    = rsc->r_contents.cc_baud;
            params.channel.serial.mtu
                    = rsc->r_contents.cc_mtu;
            break;
        case MERCURY:
            params.channel.mercury.session.level
                    = rsc->r_contents.cc_session->sess_level;
            params.channel.mercury.session.source_id
                    = rsc->r_contents.cc_session->sess_src_id;
            params.channel.mercury.session.destination_id
                    = rsc->r_contents.cc_session->sess_dst_id;
            params.channel.mercury.session.message_count
                    = rsc->r_contents.cc_session->sess_messages_count;
            memcpy(params.channel.mercury.session.messages,
                    rsc->r_contents.cc_session->sess_messages,
                    PIRATE_MERCURY_MESSAGE_TABLE_LEN * sizeof(uint32_t));
            params.channel.mercury.session.id
                    = rsc->r_contents.cc_session->sess_id;
            params.channel.mercury.mtu
                    = rsc->r_contents.cc_mtu;
            break;
        case GE_ETH:
            strncpy(params.channel.ge_eth.addr,
                    rsc->r_contents.cc_host, INET_ADDRSTRLEN);
            params.channel.ge_eth.port
                    = rsc->r_contents.cc_port;
            params.channel.ge_eth.message_id
                    = rsc->r_contents.cc_message_id;
            params.channel.ge_eth.mtu
                    = rsc->r_contents.cc_mtu;
            break;
        default:
            fatal("Pirate channel %s has unknown channel type: %d",
                    rsc->r_name, params.channel_type);
    }

    if((pstr_len = pirate_unparse_channel_param(&params, NULL, 0)) < 0)
        return -1;
    if(!(pstr = malloc(pstr_len + 1)))
        return -1;
    if(pirate_unparse_channel_param(&params, pstr, pstr_len + 1) != pstr_len)
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
