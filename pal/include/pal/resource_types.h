#ifndef _PIRATE_PAL_RESOURCE_TYPES_H
#define _PIRATE_PAL_RESOURCE_TYPES_H

#include <stdbool.h>
#include <stdint.h>

/* Search environment for "PAL_FD=XXXX", where "XXXX" is a valid file
 * descriptor number.
 *
 * Return "XXXX" if found. Otherwise, return -1;
 */
int get_pal_fd();

/* Get a resource of type "boolean" from the application launcher.
 *
 * Return 0 on success. Return 1 if the format of the received resource
 * is incorrect. Otherwise, return a negative errno value.
 */
int get_boolean_res(int fd, const char *name, bool *outp);

/* Get a resource of type "integer" from the application launcher.
 *
 * Return 0 on success. Return 1 if the format of the received resource
 * is incorrect. Otherwise, return a negative errno value.
 */
int get_integer_res(int fd, const char *name, int64_t *outp);

/* Get a resource of type "string" from the application launcher. The
 * contents of `*outp` must be freed on successful return.
 *
 * Return 0 on success. Return 1 if the format of the received resource
 * is incorrect. Otherwise, return a negative errno value.
 */
int get_string_res(int fd, const char *name, char **outp);

/* Get a resource of type "file" from the application launcher.
 *
 * Return 0 on success. Return 1 if the format of the received resource
 * is incorrect. Otherwise, return a negative errno value.
 */
int get_file_res(int fd, const char *name, int *outp);

#endif
