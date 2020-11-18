#ifndef _PIRATE_PAL_HANDLE_APPS_H
#define _PIRATE_PAL_HANDLE_APPS_H

#include <pal/envelope.h>

#include "launch.h"
#include "yaml.h"

/* Maximum number of resource types
 */
#define HANDLER_TABLE_MAX 1024

/* A resource handler should inspect the supplied `struct resource` and
 * fill in `env`, which is guaranteed to point to a `pal_env_t` initialized
 * with `EMPTY_PAL_ENV(PAL_RESOURCE)`.
 *
 * The return value should be 0 if the environment was created successfully.
 * Otherwise, -1 should be returned, in which case `env` will not be
 * inspected.
 */
typedef int (resource_handler_t)(pal_env_t *env,
        const struct app *app, struct resource *rsc);

void load_resource_plugins(const char *dirpath);
void free_resource_plugins(void);

/* Handle messages from applications in a loop. Return when there are no
 * application fds remaining, or on an unrecoverable error.
 *
 * Return the number of application pipe fds remaining.
 */
int handle_apps(struct app *apps, size_t apps_count,
        struct resource *rscs, size_t rscs_count);

#endif // _PIRATE_PAL_HANDLE_APPS_H
