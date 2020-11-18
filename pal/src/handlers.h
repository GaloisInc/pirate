#ifndef _PIRATE_PAL_HANDLERS_H
#define _PIRATE_PAL_HANDLERS_H

#include <pal/envelope.h>

#include "launch.h"
#include "yaml.h"

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
