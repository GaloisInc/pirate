#ifndef _PIRATE_PAL_HANDLERS_H
#define _PIRATE_PAL_HANDLERS_H

#include <pal/envelope.h>

#include "launch.h"
#include "yaml.h"

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

int cstring_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc);
int int64_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc);
int bool_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc);
int file_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc);
int pirate_channel_resource_handler(pal_env_t *env,
        const struct app *app, struct resource *rsc);

#endif // _PIRATE_PAL_HANDLERS_H
