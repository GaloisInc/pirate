#ifndef __PAL_RESOURCE_H
#define __PAL_RESOURCE_H

#include <stdbool.h>
#include <sys/types.h>
#include <yaml.h>

#include <pal/envelope.h>

#ifdef __cplusplus
extern "C" {
#endif

/* YAML document parsing
 *
 * Note: These functions are defined in yaml.c. They are declared here in
 * order for their signatures to be accessible to resource plugins.
 */

typedef struct pal_context pal_context_t;

typedef enum pal_yaml_result {
    PAL_YAML_OK = 0,
    PAL_YAML_NOT_FOUND,
    PAL_YAML_PARSER_ERROR,
    PAL_YAML_TYPE_ERROR,
} pal_yaml_result_t;

#define PAL_YAML_ENUM_END (pal_yaml_enum_schema_t){NULL, 0}

/* Array of name/value pairs for pal_config_flags and
 * pal_yaml_subdoc_find_enums.
 *
 * Final entry must be PAL_YAML_ENUM_END.
 */
typedef struct pal_yaml_enum_schema {
    char *name;
    int value;
} pal_yaml_enum_schema_t;

typedef struct pal_config_node pal_config_node_t;

/* Register an error message with the current context. After calling
 * this function, the context will be empty and must be repopulated
 * before pushing another error.
 */
void pal_error(pal_context_t *ctx, const pal_config_node_t* node, const char *fmt, ...);

/**
 * Write errors to stderr and zero out error count.
 */
void pal_context_flush_errors(pal_context_t *ctx);

/* Return the number of errors registered to a pal_context_t.
 */
size_t pal_error_count(pal_context_t *ctx);

/**
 * A pal boolean overridden to avoid undefined behavior in vararg functions
 * whose last fixed argument is a Boolean.
 */
typedef int pal_bool;

/**
 * Returns the node at the given path.
 */
pal_yaml_result_t pal_config_node(
        const pal_config_node_t** res,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);

/* Find a value of the specified type in pal_config_node_t. The
 * 'required' field indicates if the value is required, and the
 * following arguments are strings that identify fields to derefence.
 * The last argument must be 0 to indicate the end of arguments.
 *
 * E.g.:
 *
 * pal_config_string(&result, ctx, node, true, "my_list_of_maps", my_string_value", 0);
 *
 * On success, PAL_YAML_OK is returned. Any other return value signals
 * an error, in which case a textual description of the error is
 * logged to the pal_context_t. The accumulated errors can be
 * printed with pal_context_emit_errors(). Check how many errors
 * have been registered with pal_error_count().
 *
 * If a value is not required, `PAL_YAML_NOT_FOUND` is returned,
 * but no error is reported to `ctx`.
 */
pal_yaml_result_t pal_config_string(const char **str,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);
pal_yaml_result_t pal_config_static_string(char *str, size_t sz,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);
pal_yaml_result_t pal_config_int64(int64_t *val,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);
pal_yaml_result_t pal_config_int32(int32_t *val,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);
pal_yaml_result_t pal_config_int16(int16_t *val,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);
pal_yaml_result_t pal_config_uint64(uint64_t *val,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);
pal_yaml_result_t pal_config_uint32(uint32_t *val,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);
pal_yaml_result_t pal_config_uint16(uint16_t *val,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);
pal_yaml_result_t pal_config_double(double *val,
        pal_context_t *ctx,  const pal_config_node_t* root,
        pal_bool required, size_t count, ...);
pal_yaml_result_t pal_config_bool(bool *val,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);
pal_yaml_result_t pal_config_enum(int *val,
        pal_yaml_enum_schema_t *enums,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);
pal_yaml_result_t pal_config_flags(int *val,
        pal_yaml_enum_schema_t *enums,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);

/**
 * Find a sequence.
 */
pal_yaml_result_t pal_config_sequence(
        const pal_config_node_t*** seq, size_t *len,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);

pal_yaml_result_t pal_config_string_sequence(
        const char *** strseqp, size_t *lenp,
        pal_context_t *ctx, const pal_config_node_t* root,
        pal_bool required, size_t count, ...);

/* An app launched by pal.
 */
struct app {
    const char *name;
    int pipe_fd;
    bool hangup;
    pid_t pid;
};

/* A resource handler should inspect the supplied `pal_context_t` and
 * fill in `env`, which is guaranteed to point to a `pal_env_t` initialized
 * with `EMPTY_PAL_ENV(PAL_RESOURCE)`.
 *
 * The return value should be 0 if the environment was created successfully.
 * Otherwise, -1 should be returned, in which case `env` will not be
 * inspected.
 */
typedef int (pal_resource_handler_t)(pal_env_t *env,
        const struct app *app, pal_context_t *rsc, const pal_config_node_t* root);

#ifdef __cplusplus
}
#endif

#endif /* __PAL_RESOURCE_H */
