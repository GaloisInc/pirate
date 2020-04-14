#ifndef _PIRATE_PAL_HANDLERS_H
#define _PIRATE_PAL_HANDLERS_H

#include <pal/pal.h>

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
        const struct app *app, const struct resource *rsc);

struct handler_table_entry {
    const char *type;
    resource_handler_t *handler;
};

extern struct handler_table_entry handler_table[HANDLER_TABLE_MAX];

#endif // _PIRATE_PAL_HANDLERS_H
