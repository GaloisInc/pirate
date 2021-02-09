#pragma once

#include <stdint.h>

#include <pal/resource.h>

#include "log.h"

struct pal_context {
    yaml_document_t* doc;

    /* Each stack is a single error, with the context entries being
     * below it on the stack. Error context is pushed to
     * errors[error_count] until an error is pushed, at which point,
     * error_count is incremented.
     */
    //struct pal_error *errors[512];

    size_t error_count;
};

/**
 * A node in the configuration file.
 *
 * May be a string value, map or object.
 */
struct pal_config_node {
    const yaml_node_t yaml;
};

inline static
const pal_config_node_t* mk_node(const yaml_node_t* n) {
    return (const pal_config_node_t*) n;
}

/**
 * Configuration of an enclave.
 */
struct enclave {
    const char *enc_name;
    const char *enc_path;
    const char *enc_directory;
    /**
     * Arguments to passed to enclave
     * N.B. The strings are owned by the Yaml document, and do not need to be freed
     */
    const char **enc_args;    
    size_t enc_args_count;
    /**
     * Environment parameters to passed to enclave
     * N.B. The strings are owned by the Yaml document, and do not need to be freed
     */
    const char **enc_env;
    size_t enc_env_count;
};

struct resource {
    const char *r_name;
    const char *r_type;
    const char **r_ids;
    size_t r_ids_count;
    const pal_config_node_t* r_config;
};

struct config {
    enum log_level cfg_loglvl;
    const char *cfg_plugin_dir;
};

struct top_level {
    struct enclave *tl_encs;
    size_t tl_encs_count;
    struct resource *tl_rscs;
    size_t tl_rscs_count;
    struct config tl_cfg;
};

/**
 * Load the resource yaml.
 *
 */
struct top_level *load_yaml(yaml_document_t* doc, pal_context_t* sd, const char *fname);

void log_yaml_data(enum log_level lvl, struct top_level *yaml);

void free_yaml(struct top_level *tl);