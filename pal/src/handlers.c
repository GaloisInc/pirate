#include <libpirate.h>
#include <string.h>
#include <stdint.h>

#include "handlers.h"
#include "log.h"
#include "yaml.h"

#define min(_a, _b) ((_a) < (_b) ? (_a) : (_b))

int cstring_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc)
{
    pal_yaml_subdoc_t *sd = &rsc->r_yaml;
    int ret = 0;
    char *s = NULL;

    if(pal_yaml_subdoc_find_string(&s, sd,
                true, 1, PAL_MAP_FIELD("string_value")))
        ret = -1;

    else if(pal_add_to_env(env, s, strlen(s)))
        ret = -1;

    free(s);
    return ret;
}

int int64_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc)
{
    pal_yaml_subdoc_t *sd = &rsc->r_yaml;
    int64_t n;

    if(pal_yaml_subdoc_find_int64(&n, sd,
                true, 1, PAL_MAP_FIELD("integer_value")))
        return -1;

    if(pal_add_to_env(env, &n, sizeof n))
        return -1;

    return 0;
}

int bool_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc)
{
    pal_yaml_subdoc_t *sd = &rsc->r_yaml;
    bool b;

    if(pal_yaml_subdoc_find_bool(&b, sd,
                true, 1, PAL_MAP_FIELD("boolean_value")))
        return -1;

    if(pal_add_to_env(env, &b, sizeof b))
        return -1;

    return 0;
}

int file_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc)
{
    pal_yaml_subdoc_t *sd = &rsc->r_yaml;
    char *path = NULL;
    int flags = O_RDWR;
    int ret = 0;
    int fd;

    pal_yaml_enum_schema_t fflags_schema[] = {
        {"O_RDONLY",    O_RDONLY},
        {"O_WRONLY",    O_WRONLY},
        {"O_RDWR",      O_RDWR},

        {"O_APPEND",    O_APPEND},
        {"O_ASYNC",     O_ASYNC},
        {"O_CLOEXEC",   O_CLOEXEC},
        {"O_CREAT",     O_CREAT},
        {"O_DIRECTORY", O_DIRECTORY},
        {"O_DSYNC",     O_DSYNC},
        {"O_EXCL",      O_EXCL},
        {"O_NOCTTY",    O_NOCTTY},
        {"O_NOFOLLOW",  O_NOFOLLOW},
        {"O_NONBLOCK",  O_NONBLOCK},
        {"O_SYNC",      O_SYNC},
        {"O_TRUNC",     O_TRUNC},
        PAL_YAML_ENUM_END
    };

    pal_yaml_subdoc_find_string(&path, sd,
                true, 1, PAL_MAP_FIELD("file_path"));
    pal_yaml_subdoc_find_flags(&flags, fflags_schema, sd,
                true, 1, PAL_MAP_FIELD("file_flags"));
    if(pal_yaml_subdoc_error_count(sd) > 0)
        ret = -1;

    else if((fd = open(path, flags)) < 0)
        ret = -1;
    // TODO: Allow file creation modes to be set?
    // TODO: Allow paths relative to config?

    else if(pal_add_fd_to_env(env, fd))
        ret = -1;

    free(path);
    return ret;
}

static void handle_device(pirate_channel_param_t *params,
        pal_yaml_subdoc_t *sd)
{
    pal_yaml_subdoc_find_static_string(params->channel.device.path,
            PIRATE_LEN_NAME, sd,
            true, 1, PAL_MAP_FIELD("path"));
    pal_yaml_subdoc_find_uint32(&params->channel.device.min_tx, sd,
            false, 1, PAL_MAP_FIELD("min_tx_size"));
    pal_yaml_subdoc_find_uint32(&params->channel.device.mtu, sd,
            false, 1, PAL_MAP_FIELD("mtu"));
}

static void handle_pipe(pirate_channel_param_t *params,
        pal_yaml_subdoc_t *sd)
{
    pal_yaml_subdoc_find_static_string(
            params->channel.pipe.path, PIRATE_LEN_NAME, sd,
            true, 1, PAL_MAP_FIELD("path"));
    pal_yaml_subdoc_find_uint32(&params->channel.pipe.min_tx, sd,
            false, 1, PAL_MAP_FIELD("min_tx_size"));
    pal_yaml_subdoc_find_uint32(&params->channel.pipe.mtu, sd,
            false, 1, PAL_MAP_FIELD("mtu"));
}

static void handle_unix_socket(pirate_channel_param_t *params,
        pal_yaml_subdoc_t *sd)
{
    pal_yaml_subdoc_find_static_string(
            params->channel.unix_socket.path, PIRATE_LEN_NAME, sd,
            true, 1, PAL_MAP_FIELD("path"));
    pal_yaml_subdoc_find_uint32(&params->channel.unix_socket.min_tx, sd,
            false, 1, PAL_MAP_FIELD("min_tx_size"));
    pal_yaml_subdoc_find_uint32(&params->channel.unix_socket.mtu, sd,
            false, 1, PAL_MAP_FIELD("mtu"));
    pal_yaml_subdoc_find_uint32(
            &params->channel.unix_socket.buffer_size, sd,
            false, 1, PAL_MAP_FIELD("buffer_size"));
}

static void handle_tcp_socket(pirate_channel_param_t *params,
        pal_yaml_subdoc_t *sd)
{
    pal_yaml_subdoc_find_static_string(
            params->channel.tcp_socket.reader_addr, INET_ADDRSTRLEN, sd,
            true, 1, PAL_MAP_FIELD("reader_addr"));
    pal_yaml_subdoc_find_int16(
            &params->channel.tcp_socket.reader_port, sd,
            true, 1, PAL_MAP_FIELD("reader_port"));
    pal_yaml_subdoc_find_static_string(
            params->channel.tcp_socket.writer_addr, INET_ADDRSTRLEN, sd,
            true, 1, PAL_MAP_FIELD("writer_addr"));
    pal_yaml_subdoc_find_int16(&params->channel.tcp_socket.writer_port, sd,
            true, 1, PAL_MAP_FIELD("writer_port"));
    pal_yaml_subdoc_find_uint32(&params->channel.tcp_socket.min_tx, sd,
            false, 1, PAL_MAP_FIELD("min_tx_size"));
    pal_yaml_subdoc_find_uint32(&params->channel.tcp_socket.mtu, sd,
            false, 1, PAL_MAP_FIELD("mtu"));
    pal_yaml_subdoc_find_uint32(&params->channel.tcp_socket.buffer_size, sd,
            false, 1, PAL_MAP_FIELD("buffer_size"));
}

static void handle_udp_socket(pirate_channel_param_t *params,
        pal_yaml_subdoc_t *sd)
{
    pal_yaml_subdoc_find_static_string(
            params->channel.udp_socket.reader_addr, INET_ADDRSTRLEN, sd,
            true, 1, PAL_MAP_FIELD("reader_addr"));
    pal_yaml_subdoc_find_int16(&params->channel.udp_socket.reader_port, sd,
            true, 1, PAL_MAP_FIELD("reader_port"));
    pal_yaml_subdoc_find_static_string(
            params->channel.udp_socket.writer_addr, INET_ADDRSTRLEN, sd,
            true, 1, PAL_MAP_FIELD("writer_addr"));
    pal_yaml_subdoc_find_int16(&params->channel.udp_socket.writer_port, sd,
            true, 1, PAL_MAP_FIELD("writer_port"));
    pal_yaml_subdoc_find_uint32(&params->channel.udp_socket.mtu, sd,
            false, 1, PAL_MAP_FIELD("mtu"));
    pal_yaml_subdoc_find_uint32(&params->channel.udp_socket.buffer_size, sd,
            false, 1, PAL_MAP_FIELD("buffer_size"));
}

static void handle_shmem(pirate_channel_param_t *params,
        pal_yaml_subdoc_t *sd)
{
    pal_yaml_subdoc_find_static_string(
            params->channel.shmem.path, PIRATE_LEN_NAME, sd,
            true, 1, PAL_MAP_FIELD("path"));
    pal_yaml_subdoc_find_uint32(&params->channel.shmem.max_tx, sd,
            false, 1, PAL_MAP_FIELD("max_tx_size"));
    pal_yaml_subdoc_find_uint32(&params->channel.shmem.mtu, sd,
            false, 1, PAL_MAP_FIELD("mtu"));
    pal_yaml_subdoc_find_uint32(&params->channel.shmem.buffer_size, sd,
            false, 1, PAL_MAP_FIELD("buffer_size"));
}

static void handle_udp_shmem(pirate_channel_param_t *params,
        pal_yaml_subdoc_t *sd)
{
    pal_yaml_subdoc_find_static_string(
            params->channel.udp_shmem.path, PIRATE_LEN_NAME, sd,
            true, 1, PAL_MAP_FIELD("path"));
    pal_yaml_subdoc_find_uint32(&params->channel.udp_shmem.mtu, sd,
            false, 1, PAL_MAP_FIELD("mtu"));
    pal_yaml_subdoc_find_uint32(&params->channel.udp_shmem.buffer_size, sd,
            false, 1, PAL_MAP_FIELD("buffer_size"));
    pal_yaml_subdoc_find_uint64(&params->channel.udp_shmem.packet_size, sd,
            false, 1, PAL_MAP_FIELD("max_tx_size"));
    pal_yaml_subdoc_find_uint64(&params->channel.udp_shmem.packet_count, sd,
            false, 1, PAL_MAP_FIELD("max_tx_size"));
}

static void handle_uio(pirate_channel_param_t *params,
        pal_yaml_subdoc_t *sd)
{
    pal_yaml_subdoc_find_static_string(
            params->channel.uio.path, PIRATE_LEN_NAME, sd,
            false, 1, PAL_MAP_FIELD("path"));
    pal_yaml_subdoc_find_uint32(&params->channel.uio.max_tx, sd,
            false, 1, PAL_MAP_FIELD("max_tx_size"));
    pal_yaml_subdoc_find_uint32(&params->channel.uio.mtu, sd,
            false, 1, PAL_MAP_FIELD("mtu"));
    pal_yaml_subdoc_find_uint16(&params->channel.uio.region, sd,
            false, 1, PAL_MAP_FIELD("region"));
}

static void handle_serial(pirate_channel_param_t *params,
        pal_yaml_subdoc_t *sd)
{
    pal_yaml_subdoc_find_static_string(params->channel.serial.path,
            PIRATE_LEN_NAME, sd,
            true, 1, PAL_MAP_FIELD("path"));
    pal_yaml_subdoc_find_uint32(&params->channel.serial.max_tx, sd,
            false, 1, PAL_MAP_FIELD("max_tx_size"));
    pal_yaml_subdoc_find_uint32(&params->channel.serial.mtu, sd,
            false, 1, PAL_MAP_FIELD("mtu"));
    pal_yaml_subdoc_find_uint32(&params->channel.serial.baud, sd,
            false, 1, PAL_MAP_FIELD("baud"));
}

static void handle_mercury(pirate_channel_param_t *params,
        pal_yaml_subdoc_t *sd)
{
    pal_yaml_subdoc_find_uint32(&params->channel.mercury.session.level, sd,
            true, 2, PAL_MAP_FIELD("session"), PAL_MAP_FIELD("level"));
    pal_yaml_subdoc_find_uint32(
            &params->channel.mercury.session.source_id, sd,
            true, 2, PAL_MAP_FIELD("session"), PAL_MAP_FIELD("source_id"));
    pal_yaml_subdoc_find_uint32(
            &params->channel.mercury.session.destination_id, sd,
            true, 2, PAL_MAP_FIELD("session"),
                     PAL_MAP_FIELD("destination_id"));
    {
        pal_yaml_subdoc_t seq;
        size_t count;
        if(pal_yaml_subdoc_find_sequence(&seq, &count, sd,
                    false, 2, PAL_MAP_FIELD("session"),
                              PAL_MAP_FIELD("messages"))
                == PAL_YAML_OK) {
            count = min(count, PIRATE_MERCURY_MESSAGE_TABLE_LEN);
            params->channel.mercury.session.message_count = count;
            for(size_t i = 0; i < count; ++i)
                pal_yaml_subdoc_find_uint32(
                        &params->channel.mercury.session.messages[i], &seq,
                        true, 1, PAL_SEQ_IDX(i));
        }
    }
    pal_yaml_subdoc_find_uint32(&params->channel.mercury.session.id, sd,
            false, 1, PAL_MAP_FIELD("id"));
    pal_yaml_subdoc_find_uint32(&params->channel.mercury.mtu, sd,
            false, 1, PAL_MAP_FIELD("mtu"));
}

static void handle_ge_eth(pirate_channel_param_t *params,
        pal_yaml_subdoc_t *sd)
{
    pal_yaml_subdoc_find_static_string(
            params->channel.ge_eth.reader_addr, INET_ADDRSTRLEN, sd,
            true, 1, PAL_MAP_FIELD("reader_addr"));
    pal_yaml_subdoc_find_int16(&params->channel.ge_eth.reader_port, sd,
            true, 1, PAL_MAP_FIELD("reader_port"));
    pal_yaml_subdoc_find_static_string(
            params->channel.ge_eth.writer_addr, INET_ADDRSTRLEN, sd,
            true, 1, PAL_MAP_FIELD("writer_addr"));
    pal_yaml_subdoc_find_int16(&params->channel.ge_eth.writer_port, sd,
            true, 1, PAL_MAP_FIELD("writer_port"));
    pal_yaml_subdoc_find_uint32(&params->channel.ge_eth.message_id, sd,
            false, 1, PAL_MAP_FIELD("message_id"));
    pal_yaml_subdoc_find_uint32(&params->channel.ge_eth.mtu, sd,
            false, 1, PAL_MAP_FIELD("mtu"));
}

int pirate_channel_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc)
{
    pal_yaml_subdoc_t *sd = &rsc->r_yaml;

    pirate_channel_param_t params = {0};

    pal_yaml_enum_schema_t channel_type_schema[] = {
        {"device",      DEVICE},
        {"pipe",        PIPE},
        {"unix_socket", UNIX_SOCKET},
        {"tcp_socket",  TCP_SOCKET},
        {"udp_socket",  UDP_SOCKET},
        {"shmem",       SHMEM},
        {"udp_shmem",   UDP_SHMEM},
        {"uio",         UIO_DEVICE},
        {"serial",      SERIAL},
        {"mercury",     MERCURY},
        {"ge_eth",      GE_ETH},
        PAL_YAML_ENUM_END
    };

    pal_yaml_subdoc_find_enum((int*)&params.channel_type, channel_type_schema, sd,
                true, 1, PAL_MAP_FIELD("channel_type"));

    switch(params.channel_type) {
        case DEVICE:
            handle_device(&params, sd);
            break;
        case PIPE:
            handle_pipe(&params, sd);
            break;
        case UNIX_SOCKET:
            handle_unix_socket(&params, sd);
            break;
        case TCP_SOCKET:
            handle_tcp_socket(&params, sd);
            break;
        case UDP_SOCKET:
            handle_udp_socket(&params, sd);
            break;
        case SHMEM:
            handle_shmem(&params, sd);
            break;
        case UDP_SHMEM:
            handle_udp_shmem(&params, sd);
            break;
        case UIO_DEVICE:
            handle_uio(&params, sd);
            break;
        case SERIAL:
            handle_serial(&params, sd);
            break;
        case MERCURY:
            handle_mercury(&params, sd);
            break;
        case GE_ETH:
            handle_ge_eth(&params, sd);
            break;
        default:
            fatal("Pirate channel %s has unknown channel type: %d",
                    rsc->r_name, params.channel_type);
    }

    if(pal_yaml_subdoc_error_count(sd) > 0)
        return -1;

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
