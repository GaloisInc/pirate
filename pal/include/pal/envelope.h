#ifndef _PIRATE_PAL_ENVELOPE_H
#define _PIRATE_PAL_ENVELOPE_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t pal_env_type_t;
typedef uint32_t pal_env_size_t;

#define PAL_NO_TYPE          ((pal_env_type_t)3)
#define PAL_RESOURCE_REQUEST ((pal_env_type_t)1)
#define PAL_RESOURCE         ((pal_env_type_t)2)
#define PAL_REQUEST_FAILED   ((pal_env_type_t)3)
#define _PAL_NUM_TYPES       (3)
// ^ What other message types do we want? Errors?

#define PAL_ERR_SUCCESS (0)
#define PAL_ERR_BADREQ  (1) // An incorrectly formatted message was received
#define PAL_ERR_EMPTY   (2) // A zero-length message was received
#define PAL_ERR_TOOBIG  (3) // Message size exceeds PAL_MSG_MAX
#define PAL_ERR_FTOOBIG (4) // File descriptor count exceeds PAL_FDS_MAX
#define PAL_ERR_BADCTRL (5) // Received control message too big
#define _PAL_ERR_MAX    (6)

#define PAL_FDS_MAX (32)
#define PAL_MSG_MAX (8192)
// ^ Maximum buffer to allocate on the receiving end

typedef struct {
    pal_env_type_t type; // Message type
    pal_env_size_t size; // Size of message payload
    char *buf;
    size_t buf_size;
    int fds[PAL_FDS_MAX];
    pal_env_size_t fds_count; // File descriptor count
} pal_env_t;

/* Initializer for an empty `pal_env_t`.
 */
#define EMPTY_PAL_ENV(_type) { \
    .type = (_type), \
    .size = 0, \
    .buf = NULL, \
    .buf_size = 0, \
    .fds = {0}, \
    .fds_count = 0, \
}

/* Print an informative string about a `PAL_ERR_*` value.
 */
const char *pal_strerror(int err);

/* Add data to a `pal_env_t`. The `pal_env_t` should be initialized
 * with `EMPTY_PAL_ENV(<type>)` before the first data is added.
 *
 * Returns 0 on success. Returns `PAL_ERR_TOOBIG` if adding to the envelope
 * would grow it larger than `PAL_MSG_MAX` bytes.
 */
int pal_add_to_env(pal_env_t *env,
        const void *data, pal_env_size_t data_size);

/* Add a file descriptor to a `pal_env_t`. The `pal_env_t` should be
 * initialized with `EMPTY_PAL_ENV(<type>)` before the first file descriptor
 * is added.
 *
 * Returns 0 on success. Returns `PAL_ERR_TOOBIG` if adding the file
 * desctiptor would exceed `PAL_FDS_MAX`. Otherwise, returns a negative
 * `errno` value.
 */
int pal_add_fd_to_env(pal_env_t *env, int fd);

/* Free the buffer allocated to a `pal_env_t`. This should be called on
 * any buffer to which data has been allocated using `pal_add_to_env` or
 * `pal_recv_env`, and is safe to call on an empty envelope.
 */
void pal_free_env(pal_env_t *env);

/* Close the file descriptors that have been added to a `pal_env_t`. Any file
 * descriptor that has been set to a value less than zero will be ignored.
 * This is safe to call on an empty envelope.
 */
void pal_close_env_fds(pal_env_t *env);

/* Send a `pal_env_t` on an existing socket. The `flags` field is passed
 * directly to `sendmsg`.
 *
 * Returns 0 on success. Otherwise, returns a negative errno value.
 */
int pal_send_env(int sock, pal_env_t *env, int flags);

/* Receive a `pal_env_t` on an existing socket. The `flags` field is passed
 * directly to `recvmsg`.
 *
 * Returns 0 on success. Returns `PAL_ERR_TOOBIG` if the message length field
 * exceeds `PAL_MSG_MAX` or `PAL_ERR_EMPTY` if a zero-length message is
 * received. Otherwise, returns a negative `errno` value. If the call fails,
 * `env` is left unchanged.
 */
int pal_recv_env(int sock, pal_env_t *env, int flags);

/* Request a resource on an existing socket. The `flags` field is passed
 * directly to `sendmsg`.
 *
 * Return 0 on success. Otherwise, return a negative errno value.
 */
int pal_send_resource_request(int sock,
        const char *type, const char *name, int flags);

/* Receive a resource request on an existing socket. The `flags` field is
 * passed directly to `recvmsg`. If the call returns successfully, `*type`
 * and `*name` point to memory that must be deallocated with `free()`.
 *
 * Return 0 on success. Return `PAL_ERR_BADREQ` if the resource request
 * appears to be incorrectly formatted, `PAL_ERR_TOOBIG` if the length field
 * of the received message exceeds `PAL_MSG_MAX`, or `PAL_ERR_EMPTY` if a
 * zero-length message is received. Otherwise, return a negative `errno`
 * value. If the call fails, `typep` and `namep` are left unchanged.
 */
int pal_recv_resource_request(int sock,
        char **typep, char **namep, int flags);

/*
 * Helper functions for iterating through the data elements in a
 * `pal_env_t`.
 */

typedef void *pal_env_iterator_t;

pal_env_iterator_t pal_env_iterator_start(pal_env_t *env);

pal_env_iterator_t pal_env_iterator_end(pal_env_t *env);

pal_env_size_t pal_env_iterator_size(pal_env_iterator_t it);

void * pal_env_iterator_data(pal_env_iterator_t it);

pal_env_iterator_t pal_env_iterator_next(pal_env_iterator_t it);

#ifdef __cplusplus
}
#endif

#endif // _PIRATE_PAL_ENVELOPE_H
