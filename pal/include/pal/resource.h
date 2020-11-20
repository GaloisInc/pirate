#ifndef __PAL_RESOURCE_H
#define __PAL_RESOURCE_H

#include <libpirate.h>
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

enum pal_yaml_elem_type {
    PAL_YAML_ELEM_MAP,
    PAL_YAML_ELEM_SEQ,
};

struct pal_yaml_elem {
    enum pal_yaml_elem_type type;
    union {
        size_t seq_elem;
        const char *map_elem;
    };
};

typedef struct pal_yaml_elem pal_yaml_elem_t;

#define PAL_MAP_FIELD(_str) \
    (struct pal_yaml_elem){ .type = PAL_YAML_ELEM_MAP, .map_elem = (_str) }
#define PAL_SEQ_IDX(_num) \
    (struct pal_yaml_elem){ .type = PAL_YAML_ELEM_SEQ, .seq_elem = (_num) }

struct pal_yaml_error_stack {
    char err[1024];
    struct pal_yaml_error_stack *next;
};

typedef struct pal_yaml_subdoc {
    yaml_document_t doc;
    size_t *doc_ref_count;
    yaml_node_t *node;
    char filename[1024];

    /* Each stack is a single error, with the context entries being
     * below it on the stack. Error context is pushed to
     * errors[error_count] until an error is pushed, at which point,
     * error_count is incremented.
     */
    struct pal_yaml_error_stack *errors[512];
    size_t error_count;

    /* Error context inherited from parent subdoc. This is appended
     * to the end of the error stack for each error when the errors
     * are logged.
     */
    struct pal_yaml_error_stack *context;
} pal_yaml_subdoc_t;

typedef enum pal_yaml_result {
    PAL_YAML_OK = 0,
    PAL_YAML_NOT_FOUND,
    PAL_YAML_PARSER_ERROR,
    PAL_YAML_TYPE_ERROR,
} pal_yaml_result_t;

#define PAL_YAML_ENUM_END (pal_yaml_enum_schema_t){NULL, 0}

/* Array of name/value pairs for pal_yaml_subdoc_find_flags and
 * pal_yaml_subdoc_find_enums.
 *
 * Final entry must be PAL_YAML_ENUM_END.
 */
typedef struct pal_yaml_enum_schema {
    char *name;
    int value;
} pal_yaml_enum_schema_t;

/* Add to the context of the current error. E.g., this is used
 * to add lines such as "In field X of mapping at Y:Z".
 */
void pal_yaml_subdoc_error_context_push(pal_yaml_subdoc_t *sd,
        const char *fmt, ...);

/* Clear the context of the current error.
 */
void pal_yaml_subdoc_error_context_clear(pal_yaml_subdoc_t *sd);

/* Register an error message with the current context. After calling
 * this function, the context will be empty and must be repopulated
 * before pushing another error.
 */
void pal_yaml_subdoc_error_push(pal_yaml_subdoc_t *sd,
        const char *fmt, ...);

/* Remove the last error registered to a pal_yaml_subdoc_t.
 */
void pal_yaml_subdoc_error_pop(pal_yaml_subdoc_t *sd);

/* Clear all errors and error context from a pal_yaml_subdoc_t.
 */
void pal_yaml_subdoc_clear_errors(pal_yaml_subdoc_t *sd);

/* Return the number of errors registered to a pal_yaml_subdoc_t.
 */
size_t pal_yaml_subdoc_error_count(pal_yaml_subdoc_t *sd);

/* Log all errors registered to a pal_yaml_subdoc_t. It is an error to
 * call this without first calling pal_yaml_subdoc_open() on sd.
 */
void pal_yaml_subdoc_log_errors(pal_yaml_subdoc_t *sd);

/* Open a yaml document and place it at the root of a yaml_subdoc_t,
 * for use by the pal_yaml_subdoc_find_* family of functions.
 *
 * On success, returns PAL_YAML_OK. If the file could not be opened,
 * returns PAL_YAML_NOT_FOUND. Otherwise, returns
 * PAL_YAML_PARSER_ERROR if the document was empty or could not be
 * parsed.
 */
pal_yaml_result_t pal_yaml_subdoc_open(pal_yaml_subdoc_t *sd,
        const char *fname);

/* Clean up a pal_yaml_subdoc_t. It is an error to call this without
 * first calling pal_yaml_subdoc_open() on sd.
 */
void pal_yaml_subdoc_close(pal_yaml_subdoc_t *sd);

/* Return a static string corresponding to a pal_yaml_result_t.
 */
const char *pal_yaml_strerror(pal_yaml_result_t res);

/* Find a value of the specified type in a pal_yaml_subdoc_t. The
 * depth argument specifies the length of the path from the root of
 * the subdoc to the value, and the variadic arguments are a sequence
 * of PAL_MAP_FIELD() and PAL_SEQ_IDX() specifying the path to follow.
 * E.g.:
 *
 * pal_yaml_subdoc_find_string(&result, &sd,
 *          3, // depth of value from root
 *          PAL_MAP_FIELD("my_list_of_maps"),
 *          PAL_SEQ_IDX(3),
 *          PAL_MAP_FIELD("my_string_value"));
 *
 * On success, PAL_YAML_OK is returned. Any other return value signals
 * an error, in which case a textual description of the error is
 * logged to the pal_yaml_subdoc_t. The accumulated errors can be
 * printed with pal_yaml_subdoc_log_errors(). Check how many errors
 * have been registered with pal_yaml_subdoc_error_count().
 *
 * If a value is optional, you can check for the return value
 * PAL_YAML_NOT_FOUND, and call pal_yaml_subdoc_error_pop() to ignore
 * the last error.
 *
 * NB: pal_yaml_subdoc_t values retrieved using the
 * pal_yaml_subdoc_find_* functions refer to memory allocated by
 * pal_yaml_subdoc_open(&sd) and stored in sd. Do not call
 * pal_yaml_subdoc_close() on sd until you are done with them.
 *
 * NB2: The string value retrieved using pal_yaml_subdoc_find_string
 * contains memory allocated on the heap. The caller is responsible
 * for freeing this memory.
 */
pal_yaml_result_t pal_yaml_subdoc_find_string(char **str,
        pal_yaml_subdoc_t *sd, bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_static_string(char *str,
        size_t sz, pal_yaml_subdoc_t *sd, bool required,
        size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_int64(int64_t *val,
        pal_yaml_subdoc_t *sd, bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_int32(int32_t *val,
        pal_yaml_subdoc_t *sd, bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_int16(int16_t *val,
        pal_yaml_subdoc_t *sd, bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_int8(int8_t *val,
        pal_yaml_subdoc_t *sd, bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_uint64(uint64_t *val,
        pal_yaml_subdoc_t *sd, bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_uint32(uint32_t *val,
        pal_yaml_subdoc_t *sd, bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_uint16(uint16_t *val,
        pal_yaml_subdoc_t *sd, bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_uint8(uint8_t *val,
        pal_yaml_subdoc_t *sd, bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_double(double *val,
        pal_yaml_subdoc_t *sd, bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_bool(bool *val,
        pal_yaml_subdoc_t *sd, bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_enum(int *val,
        pal_yaml_enum_schema_t *enums, pal_yaml_subdoc_t *sd,
        bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_flags(int *val,
        pal_yaml_enum_schema_t *enums, pal_yaml_subdoc_t *sd,
        bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_sequence(
        pal_yaml_subdoc_t *seq, size_t *len, pal_yaml_subdoc_t *sd,
        bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_string_sequence(
        char ***strseqp, size_t *lenp, pal_yaml_subdoc_t *sd,
        bool required, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_subdoc(pal_yaml_subdoc_t *dst,
        pal_yaml_subdoc_t *sd, bool required, size_t depth, ...);

/* An app launched by pal.
 */
struct app {
    char *name;
    int pipe_fd;
    bool hangup;
    pid_t pid;
};

/* A resource handler should inspect the supplied `pal_yaml_subdoc_t` and
 * fill in `env`, which is guaranteed to point to a `pal_env_t` initialized
 * with `EMPTY_PAL_ENV(PAL_RESOURCE)`.
 *
 * The return value should be 0 if the environment was created successfully.
 * Otherwise, -1 should be returned, in which case `env` will not be
 * inspected.
 */
typedef int (pal_resource_handler_t)(pal_env_t *env,
        const struct app *app, pal_yaml_subdoc_t *rsc);

#ifdef __cplusplus
}
#endif

#endif /* __PAL_RESOURCE_H */
