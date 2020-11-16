#ifndef _PIRATE_PAL_YAML_H
#define _PIRATE_PAL_YAML_H

#include <cyaml/cyaml.h>
#include <libpirate.h>
#include <stdbool.h>
#include <stdint.h>
#include <yaml.h>

#include "log.h"

/* YAML document parsing
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
    yaml_node_t *node;
    char filename[1024];
    bool is_master; // Whether freeing should close yaml_document_t

    /* Each stack is a single error, with the context entries being
     * below it on the stack. Error context is pushed to
     * errors[error_count] until an error is pushed, at which point,
     * error_count is incremented.
     */
    struct pal_yaml_error_stack *errors[512];
    size_t error_count;
    struct pal_yaml_error_stack *context;
} pal_yaml_subdoc_t;

typedef enum pal_yaml_result {
    PAL_YAML_OK = 0,
    PAL_YAML_NOT_FOUND,
    PAL_YAML_PARSER_ERROR,
    PAL_YAML_TYPE_ERROR,
} pal_yaml_result_t;

#define PAL_YAML_ENUM_END (pal_yaml_enum_schema_t){NULL, 0}

typedef struct pal_yaml_enum_schema {
    char *name;
    int value;
} pal_yaml_enum_schema_t;

void pal_yaml_subdoc_error_push(pal_yaml_subdoc_t *sd,
        const char *fmt, ...);
void pal_yaml_subdoc_error_context_push(pal_yaml_subdoc_t *sd,
        const char *fmt, ...);
void pal_yaml_subdoc_error_context_clear(pal_yaml_subdoc_t *sd);

/* Remove the last error registered to a pal_yaml_subdoc_t.
 */
void pal_yaml_subdoc_error_pop(pal_yaml_subdoc_t *sd);

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
        pal_yaml_subdoc_t *sd, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_signed(int64_t *val,
        pal_yaml_subdoc_t *sd, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_unsigned(uint64_t *val,
        pal_yaml_subdoc_t *sd, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_floating(double *val,
        pal_yaml_subdoc_t *sd, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_boolean(bool *val,
        pal_yaml_subdoc_t *sd, size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_enum(int *val,
        pal_yaml_enum_schema_t *enums, pal_yaml_subdoc_t *sd,
        size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_flags(int *val,
        pal_yaml_enum_schema_t *enums, pal_yaml_subdoc_t *sd,
        size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_sequence(
        pal_yaml_subdoc_t *seq, size_t *len, pal_yaml_subdoc_t *sd,
        size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_string_sequence(
        char ***strseqp, size_t *lenp, pal_yaml_subdoc_t *sd,
        size_t depth, ...);
pal_yaml_result_t pal_yaml_subdoc_find_subdoc(pal_yaml_subdoc_t *dst,
        pal_yaml_subdoc_t *sd, size_t depth, ...);

/* Config file data structure
 */

struct enclave {
    char *enc_name;
    char *enc_path;
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

struct rsc_contents {

    /* pirate_ and fd_channel
     */
    channel_enum_t cc_channel_type;
    char *cc_path;              // device, pipe, unix_socket, shmem, udp_shmem, uio, serial
<<<<<<< HEAD
    uint64_t cc_min_tx_size;    // device, pipe, unix_socket, tcp_socket
    uint64_t cc_mtu;            // ALL
    uint64_t cc_buffer_size;    // unix_socket, tcp_socket, udp_socket, shmem, udp_shmem
    char *cc_host;              // tcp_socket, udp_socket, ge_eth
    uint64_t cc_port;           // tcp_socket, udp_socket, ge_eth
    uint64_t cc_max_tx_size;    // shmem, uio, serial
    uint64_t cc_packet_size;      // udp_shmem
    uint64_t cc_packet_count;     // udp_shmem
    uint64_t cc_region;         // uio
    uint64_t cc_baud;            // serial
=======
    uint32_t cc_min_tx_size;    // device, pipe, unix_socket, tcp_socket
    uint32_t cc_mtu;            // ALL
    uint32_t cc_buffer_size;    // unix_socket, tcp_socket, udp_socket, shmem, udp_shmem
    char *cc_reader_host;       // tcp_socket, udp_socket, ge_eth
    uint16_t cc_reader_port;    // tcp_socket, udp_socket, ge_eth
    char *cc_writer_host;       // tcp_socket, udp_socket, ge_eth
    uint16_t cc_writer_port;    // tcp_socket, udp_socket, ge_eth
    uint32_t cc_max_tx_size;    // shmem, uio, serial
    size_t cc_packet_size;      // udp_shmem
    size_t cc_packet_count;     // udp_shmem
    uint16_t cc_region;         // uio
    speed_t cc_baud;            // serial
>>>>>>> 2ad9265ad610350ed653acf69c31db845604be19
    struct session *cc_session; // mercury
    uint64_t cc_message_id;     // ge_eth

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
    pal_yaml_subdoc_t r_yaml;
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
    pal_yaml_subdoc_t tl_yaml;
};

struct top_level *load_yaml(const char *fname);

void log_yaml_data(enum log_level lvl, struct top_level *yaml);

void free_yaml(struct top_level *tl);

#endif // _PIRATE_PAL_YAML_H
