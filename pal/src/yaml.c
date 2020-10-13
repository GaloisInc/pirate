#include "yaml.h"

static const cyaml_schema_value_t string_ptr_schema = {
    CYAML_VALUE_STRING(CYAML_FLAG_POINTER, char, 0, CYAML_UNLIMITED),
};

/*
 * struct enclave schemas
 */

static const cyaml_schema_field_t enclave_field_schema[] = {
    CYAML_FIELD_STRING_PTR("name", CYAML_FLAG_POINTER,
            struct enclave, enc_name, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("path", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
            struct enclave, enc_path, 0, CYAML_UNLIMITED),
    CYAML_FIELD_SEQUENCE("arguments", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
            struct enclave, enc_args, &string_ptr_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_SEQUENCE("environment", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
            struct enclave, enc_env, &string_ptr_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_END,
};

static const cyaml_schema_value_t enclave_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
            struct enclave, enclave_field_schema),
};

/*
 * resource config schemas
 */

static const cyaml_strval_t channel_type_strings[] = {
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
};

static const cyaml_schema_field_t __attribute__((unused))
endpoint_field_schema[] = {
    CYAML_FIELD_STRING_PTR("id",
            CYAML_FLAG_POINTER_NULL | CYAML_FLAG_OPTIONAL,
            struct endpoint, ep_id, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("host",
            CYAML_FLAG_POINTER_NULL | CYAML_FLAG_OPTIONAL,
            struct endpoint, ep_dst_host, 0, INET_ADDRSTRLEN),
    CYAML_FIELD_UINT("port", CYAML_FLAG_OPTIONAL,
            struct endpoint, ep_dst_port),
    CYAML_FIELD_END,
};

static const cyaml_schema_value_t message_schema = {
    CYAML_VALUE_UINT(CYAML_FLAG_DEFAULT, uint32_t),
};

static const cyaml_schema_field_t session_field_schema[] = {
    CYAML_FIELD_UINT("level", CYAML_FLAG_OPTIONAL,
            struct session, sess_level),
    CYAML_FIELD_UINT("src_id", CYAML_FLAG_OPTIONAL,
            struct session, sess_src_id),
    CYAML_FIELD_UINT("dst_id", CYAML_FLAG_OPTIONAL,
            struct session, sess_dst_id),
    CYAML_FIELD_SEQUENCE("messages",
            CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER_NULL,
            struct session, sess_messages, &message_schema,
            1, PIRATE_MERCURY_MESSAGE_TABLE_LEN),
    CYAML_FIELD_UINT("id", CYAML_FLAG_OPTIONAL,
            struct session, sess_id),
    CYAML_FIELD_END,
};

static const cyaml_strval_t open_flags_strings[] = {
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
};

// FIXME: Make this pluggable
static const cyaml_schema_field_t rsc_contents_field_schema[] = {

    /* pirate_ and fd_channel
     */
    CYAML_FIELD_ENUM("channel_type", CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_channel_type,
            channel_type_strings, CYAML_ARRAY_LEN(channel_type_strings)),
    CYAML_FIELD_STRING_PTR("path",
            CYAML_FLAG_POINTER_NULL | CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_path, 1, PIRATE_LEN_NAME),
    CYAML_FIELD_UINT("min_tx_size", CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_min_tx_size),
    CYAML_FIELD_UINT("mtu", CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_mtu),
    CYAML_FIELD_UINT("buffer_size", CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_buffer_size),
    CYAML_FIELD_STRING_PTR("reader_host",
            CYAML_FLAG_POINTER_NULL | CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_reader_host, 1, INET_ADDRSTRLEN),
    CYAML_FIELD_UINT("reader_port", CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_reader_port),
    CYAML_FIELD_STRING_PTR("writer_host",
            CYAML_FLAG_POINTER_NULL | CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_writer_host, 1, INET_ADDRSTRLEN),
    CYAML_FIELD_UINT("writer_port", CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_writer_port),
    CYAML_FIELD_UINT("max_tx_size", CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_max_tx_size),
    CYAML_FIELD_UINT("packet_size", CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_packet_size),
    CYAML_FIELD_UINT("packet_count", CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_packet_count),
    CYAML_FIELD_UINT("region", CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_region),
    CYAML_FIELD_UINT("baud", CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_baud),
    CYAML_FIELD_MAPPING_PTR("session",
            CYAML_FLAG_POINTER_NULL | CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_session, session_field_schema),
    CYAML_FIELD_UINT("message_id", CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_message_id),

    /* Trivial resources
     */
    CYAML_FIELD_STRING_PTR("string_value",
            CYAML_FLAG_POINTER_NULL | CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_string_value, 0, CYAML_UNLIMITED),
    CYAML_FIELD_INT_PTR("integer_value",
            CYAML_FLAG_POINTER_NULL | CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_integer_value),
    CYAML_FIELD_BOOL_PTR("boolean_value",
            CYAML_FLAG_POINTER_NULL | CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_boolean_value),

    /* File resource
     */
    CYAML_FIELD_STRING_PTR("file_path",
            CYAML_FLAG_POINTER_NULL | CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_file_path, 0, CYAML_UNLIMITED),
    CYAML_FIELD_FLAGS_PTR("file_flags",
            CYAML_FLAG_POINTER_NULL | CYAML_FLAG_OPTIONAL,
            struct rsc_contents, cc_file_flags, open_flags_strings,
            CYAML_ARRAY_LEN(open_flags_strings)),

    CYAML_FIELD_END,
};

/*
 * struct resource schemas
 */

static const cyaml_schema_field_t resource_field_schema[] = {
    CYAML_FIELD_STRING_PTR("name", CYAML_FLAG_POINTER,
            struct resource, r_name, 0, CYAML_UNLIMITED),
    CYAML_FIELD_STRING_PTR("type", CYAML_FLAG_POINTER,
            struct resource, r_type, 0, CYAML_UNLIMITED),
    CYAML_FIELD_SEQUENCE("ids", CYAML_FLAG_POINTER,
            struct resource, r_ids, &string_ptr_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_MAPPING("contents", CYAML_FLAG_DEFAULT,
            struct resource, r_contents, rsc_contents_field_schema), // FIXME: Make this pluggable
    CYAML_FIELD_END,
};

static const cyaml_schema_value_t resource_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
            struct resource, resource_field_schema),
};

/*
 * struct config schemas
 */

static const cyaml_strval_t log_level_strings[] = {
    {"default", LOGLVL_DEFAULT},
    {"info",    LOGLVL_INFO},
    {"debug",   LOGLVL_DEBUG},
};

static const cyaml_schema_field_t config_field_schema[] = {
    CYAML_FIELD_ENUM("log_level", CYAML_FLAG_OPTIONAL,
            struct config, cfg_loglvl,
            log_level_strings, CYAML_ARRAY_LEN(log_level_strings)),
    CYAML_FIELD_END,
};

/*
 * struct top_level schemas
 */

static const cyaml_schema_field_t top_level_field_schema[] = {
    CYAML_FIELD_SEQUENCE("enclaves", CYAML_FLAG_POINTER,
            struct top_level, tl_encs,
            &enclave_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_SEQUENCE("resources",
            CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
            struct top_level, tl_rscs,
            &resource_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_MAPPING("config", CYAML_FLAG_OPTIONAL,
            struct top_level, tl_cfg, config_field_schema),
    CYAML_FIELD_END,
};

static const cyaml_schema_value_t top_level_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
            struct top_level, top_level_field_schema),
};

/*
 * cyaml configuration
 */

static const cyaml_config_t cyaml_config = {
    .log_fn = cyaml_log, //FIXME: Use project logging function
    .log_ctx = NULL,
    .mem_fn = cyaml_mem,
    .mem_ctx = NULL,
    .log_level = CYAML_LOG_ERROR, //FIXME: Yeah?
    .flags = CYAML_CFG_DEFAULT,
};

struct top_level *load_yaml(const char *fname)
{
    cyaml_err_t err;
    struct top_level *res = NULL;

    err = cyaml_load_file(fname, &cyaml_config, &top_level_schema,
            (void **)&res, 0);
    if(err != CYAML_OK)
        fatal("Unable to parse %s: %s", fname, cyaml_strerror(err));

    return res;
}

void free_yaml(struct top_level *tl)
{
    cyaml_free(&cyaml_config, &top_level_schema, tl, 0);
}
