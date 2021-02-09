#include <pal/resource.h>
#include <libpirate.h>

#define min(_a, _b) ((_a) < (_b) ? (_a) : (_b))

static void handle_device(pirate_channel_param_t *params,
        pal_context_t *sd, const pal_config_node_t* root)
{
    pal_config_static_string(params->channel.device.path, PIRATE_LEN_NAME, sd, root, true, 1, "path");
    pal_config_uint32(&params->channel.device.min_tx, sd, root, false, 1, "min_tx_size");
    pal_config_uint32(&params->channel.device.mtu,    sd, root, false, 1, "mtu");
}

static void handle_pipe(pirate_channel_param_t *params, pal_context_t *sd, const pal_config_node_t* node)
{
    pal_config_static_string(params->channel.pipe.path, PIRATE_LEN_NAME, sd, node, true, 1, "path");
    pal_config_uint32(&params->channel.pipe.min_tx, sd, node, false, 1, "min_tx_size");
    pal_config_uint32(&params->channel.pipe.mtu, sd, node, false, 1, "mtu");
}

static void handle_unix_socket(pirate_channel_param_t *params,
        pal_context_t *sd, const pal_config_node_t* node)
{
    pirate_unix_socket_param_t* skt = &params->channel.unix_socket;
    pal_config_static_string(skt->path, PIRATE_LEN_NAME, sd, node, true, 1, "path");
    pal_config_uint32(&skt->min_tx, sd, node, false, 1, "min_tx_size");
    pal_config_uint32(&skt->mtu, sd, node, false, 1, "mtu");
    pal_config_uint32(&skt->buffer_size, sd, node, false, 1, "buffer_size");
}

static void handle_tcp_socket(pirate_channel_param_t *params,
        pal_context_t *sd, const pal_config_node_t* node)
{
    pirate_tcp_socket_param_t* skt = &params->channel.tcp_socket;
    pal_config_static_string(skt->reader_addr, INET_ADDRSTRLEN, sd, node, true, 1, "reader_addr");
    pal_config_uint16(&skt->reader_port, sd, node, true, 1, "reader_port");
    pal_config_static_string(skt->writer_addr, INET_ADDRSTRLEN, sd, node, true, 1, "writer_addr");
    pal_config_uint16( &skt->writer_port, sd, node, true, 1, "writer_port");
    pal_config_uint32(&skt->min_tx, sd, node, false, 1, "min_tx_size");
    pal_config_uint32(&skt->mtu, sd, node, false, 1, "mtu");
    pal_config_uint32(&skt->buffer_size, sd, node, false, 1, "buffer_size");
}

static void handle_udp_socket(pirate_channel_param_t *params,
        pal_context_t *sd, const pal_config_node_t* node)
{
    pirate_udp_socket_param_t* skt = &params->channel.udp_socket;
    pal_config_static_string(skt->reader_addr, INET_ADDRSTRLEN, sd, node, true, 1, "reader_addr");
    pal_config_uint16(&skt->reader_port, sd, node, true, 1, "reader_port");
    pal_config_static_string(skt->writer_addr, INET_ADDRSTRLEN, sd, node, true, 1, "writer_addr");
    pal_config_uint16(&params->channel.udp_socket.writer_port, sd, node, true, 1, "writer_port");
    pal_config_uint32(&params->channel.udp_socket.mtu, sd, node, false, 1, "mtu");
    pal_config_uint32(&params->channel.udp_socket.buffer_size, sd, node, false, 1, "buffer_size");
}

static
void handle_shmem(pirate_channel_param_t *params, pal_context_t *sd, const pal_config_node_t* node)
{
    pirate_shmem_param_t* c = &params->channel.shmem;
    pal_config_static_string(c->path, PIRATE_LEN_NAME, sd, node, true, 1, "path");
    pal_config_uint32(&c->max_tx, sd, node, false, 1, "max_tx_size");
    pal_config_uint32(&c->mtu, sd, node, false, 1, "mtu");
    pal_config_uint32(&c->buffer_size, sd, node, false, 1, "buffer_size");
}

static void handle_udp_shmem(pirate_channel_param_t *params,
        pal_context_t *sd, const pal_config_node_t* node)
{
    pirate_udp_shmem_param_t* c = &params->channel.udp_shmem;
    pal_config_static_string(c->path, PIRATE_LEN_NAME, sd, node, true, 1, "path");
    pal_config_uint32(&c->mtu, sd, node, false, 1, "mtu");
    pal_config_uint32(&c->buffer_size, sd, node, false, 1, "buffer_size");
    pal_config_uint64(&c->packet_size, sd, node, false, 1, "max_tx_size");
    pal_config_uint64(&c->packet_count, sd, node, false, 1, "max_tx_size");
}

static void handle_uio(pirate_channel_param_t *params,
        pal_context_t *sd, const pal_config_node_t* node)
{
    pirate_uio_param_t* c = &params->channel.uio;
    pal_config_static_string(c->path, PIRATE_LEN_NAME, sd, node, false, 1, "path");
    pal_config_uint32(&c->max_tx,                      sd, node, false, 1, "max_tx_size");
    pal_config_uint32(&c->mtu,                         sd, node, false, 1, "mtu");
    pal_config_uint16(&c->region,                      sd, node, false, 1, "region");
}

static void handle_serial(pirate_channel_param_t *params,
        pal_context_t *sd, const pal_config_node_t* node)
{
    pirate_serial_param_t* c = &params->channel.serial;
    pal_config_static_string(c->path, PIRATE_LEN_NAME, sd, node, true,  1, "path");
    pal_config_uint32(&c->max_tx,                      sd, node, false, 1, "max_tx_size");
    pal_config_uint32(&c->mtu,                         sd, node, false, 1, "mtu");
    pal_config_uint32(&c->baud,                        sd, node, false, 1, "baud");
}

static void handle_mercury(pirate_channel_param_t *params, pal_context_t *sd, pal_config_node_t* root)
{
    pirate_mercury_param_t* c = &params->channel.mercury;
    pal_config_uint32(&c->session.level, sd, root, true, 2, "session", "level");
    pal_config_uint32(&c->session.source_id, sd, root, true, 2, "session", "source_id");
    pal_config_uint32(&c->session.destination_id, sd, root, true, 2, "session", "destination_id");
    {
        const pal_config_node_t** seq = 0;
        size_t count = 0;
        if (pal_config_sequence(&seq, &count, sd, root, false, 2, "session", "messages")
                == PAL_YAML_OK) {
            count = min(count, PIRATE_MERCURY_MESSAGE_TABLE_LEN);
            params->channel.mercury.session.message_count = count;
            for(size_t i = 0; i < count; ++i)
                pal_config_uint32(&params->channel.mercury.session.messages[i], sd, seq[i], true, 0);
        }
    }
    pal_config_uint32(&params->channel.mercury.session.id, sd, root, false, 1, "id");
    pal_config_uint32(&params->channel.mercury.mtu, sd, root, false, 1, "mtu");
}

static void handle_ge_eth(pirate_channel_param_t *params,
        pal_context_t *sd, pal_config_node_t* node)
{
    pirate_ge_eth_param_t* c = &params->channel.ge_eth;
    pal_config_static_string(c->reader_addr, INET_ADDRSTRLEN, sd, node, true, 1, "reader_addr");
    pal_config_uint16(&c->reader_port, sd, node, true, 1, "reader_port");
    pal_config_static_string(c->writer_addr, INET_ADDRSTRLEN, sd, node, true, 1, "writer_addr");
    pal_config_uint16(&c->writer_port, sd, node, true, 1, "writer_port");
    pal_config_uint32(&c->message_id, sd, node, false, 1, "message_id");
    pal_config_uint32(&c->mtu, sd, node, false, 1, "mtu");
}

int pirate_channel_resource_handler(pal_env_t *env, const struct app *app, pal_context_t *ctx, pal_config_node_t* root)
{
    size_t init_error_count = pal_error_count(ctx);

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

    pal_config_enum((int*)&params.channel_type, channel_type_schema, ctx, root, true, 1, "channel_type");

    switch(params.channel_type) {
        case DEVICE:
            handle_device(&params, ctx, root);
            break;
        case PIPE:
            handle_pipe(&params, ctx, root);
            break;
        case UNIX_SOCKET:
            handle_unix_socket(&params, ctx, root);
            break;
        case TCP_SOCKET:
            handle_tcp_socket(&params, ctx, root);
            break;
        case UDP_SOCKET:
            handle_udp_socket(&params, ctx, root);
            break;
        case SHMEM:
            handle_shmem(&params, ctx, root);
            break;
        case UDP_SHMEM:
            handle_udp_shmem(&params, ctx, root);
            break;
        case UIO_DEVICE:
            handle_uio(&params, ctx, root);
            break;
        case SERIAL:
            handle_serial(&params, ctx, root);
            break;
        case MERCURY:
            handle_mercury(&params, ctx, root);
            break;
        case GE_ETH:
            handle_ge_eth(&params, ctx, root);
            break;
        default:
            return -1;
    }

    if (pal_error_count(ctx) > init_error_count)
        return -1;

    size_t pstr_len = pirate_unparse_channel_param(&params, NULL, 0);
    if (pstr_len <= 0)
        return -1;

    char pstr[pstr_len + 1];
    if (pirate_unparse_channel_param(&params, pstr, sizeof pstr) != pstr_len)
        return -1;

    if (pal_add_to_env(env, pstr, pstr_len))
        return -1;

    return 0;
}
