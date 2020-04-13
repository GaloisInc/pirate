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

struct rsc_contents {

    /* gaps_ and fd_channel
     */
    channel_enum_t cc_type;
    struct endpoint *cc_left;
    struct endpoint *cc_right;
    char *cc_path;
    size_t cc_buffer;
    size_t cc_packet_size;
    size_t cc_iov_length;
    uint16_t cc_rate;

    /* Trivial resources
     */
    char *cc_string_value;
    int64_t *cc_integer_value;
    bool *cc_boolean_value;
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
