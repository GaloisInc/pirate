#ifndef _PIRATE_PAL_YAML_H
#define _PIRATE_PAL_YAML_H

#include <cyaml/cyaml.h>
#include <libpirate.h>
#include <stdbool.h>
#include <stdint.h>

#include "log.h"

struct enclave {
    char *enc_name;
    char *enc_path;
    char **enc_args;
    size_t enc_args_count;
    char **enc_env;
    size_t enc_env_count;
};

struct id {
    char *id_enclave;
    char *id_tag;
};

struct endpoint {
    char *ep_id;
    char *ep_dst_host;
    uint16_t ep_dst_port;
};

// mercury pirate_channel session parameters
struct session {
    uint32_t sess_level;
    uint32_t sess_src_id;
    uint32_t sess_dst_id;
    uint32_t *sess_messages;
    uint32_t sess_messages_count;
    uint32_t sess_id; // This cannot currently be set in libpirate. Remove it?
};

struct rsc_contents {

    /* pirate_ and fd_channel
     */
    channel_enum_t cc_channel_type;
    char *cc_path;              // device, pipe, unix_socket, shmem, udp_shmem, uio, serial
    uint32_t cc_min_tx_size;    // device, pipe, unix_socket, tcp_socket
    uint32_t cc_mtu;            // ALL
    uint32_t cc_buffer_size;    // unix_socket, tcp_socket, udp_socket, shmem, udp_shmem
    char *cc_host;              // tcp_socket, udp_socket, ge_eth
    uint16_t cc_port;           // tcp_socket, udp_socket, ge_eth
    uint32_t cc_max_tx_size;    // shmem, uio, serial
    size_t cc_packet_size;      // udp_shmem
    size_t cc_packet_count;     // udp_shmem
    uint16_t cc_region;         // uio
    speed_t cc_baud;            // serial
    struct session *cc_session; // mercury
    uint32_t cc_message_id;     // ge_eth

    /* Trivial resources
     */
    char *cc_string_value;
    int64_t *cc_integer_value;
    bool *cc_boolean_value;

    /* File resource
     */
    char *cc_file_path;
    int *cc_file_flags;
};

struct resource {
    char *r_name;
    char *r_type;
    char **r_ids;
    size_t r_ids_count;
    struct rsc_contents r_contents; // FIXME: Make this pluggable
};

struct config {
    enum log_level cfg_loglvl;
};

struct top_level {
    struct enclave *tl_encs;
    size_t tl_encs_count;
    struct resource *tl_rscs;
    size_t tl_rscs_count;
    struct config tl_cfg;
};

struct top_level *load_yaml(const char *fname);

void log_yaml_data(enum log_level lvl, struct top_level *yaml);

void free_yaml(struct top_level *tl);

#endif // _PIRATE_PAL_YAML_H
