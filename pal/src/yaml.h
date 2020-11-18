#ifndef _PIRATE_PAL_YAML_H
#define _PIRATE_PAL_YAML_H

#include <stdint.h>

#include <pal/resource.h>

#include "log.h"

struct enclave {
    char *enc_name;
    char *enc_path;
    char *enc_directory;
    char **enc_args;
    size_t enc_args_count;
    char **enc_env;
    size_t enc_env_count;
};

struct session {
    uint64_t sess_level;
    uint64_t sess_src_id;
    uint64_t sess_dst_id;
    uint64_t *sess_messages;
    uint64_t sess_messages_count;
    uint64_t sess_id;
};

struct config {
    enum log_level cfg_loglvl;
    char *cfg_plugin_dir;
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
