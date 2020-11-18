#ifndef _PIRATE_PAL_HANDLE_APPS_H
#define _PIRATE_PAL_HANDLE_APPS_H

#include <pal/resource.h>

#include "launch.h"
#include "yaml.h"

/* Handle messages from applications in a loop. Return when there are no
 * application fds remaining, or on an unrecoverable error.
 *
 * Return the number of application pipe fds remaining.
 */
int handle_apps(struct app *apps, size_t apps_count,
        struct resource *rscs, size_t rscs_count);

/* Load all resource plugins from dirpath. Plugins are expected to be called
 * <resource_name>.so and to contain a function function called
 * <resource_name>_resource_handler().
 *
 * A non-existent plugin directory or invalid plugin files result in warnings.
 */
void load_resource_plugins(const char *dirpath);

/* Close resource plugins opened with dlopen().
 */
void free_resource_plugins(void);

#endif // _PIRATE_PAL_HANDLE_APPS_H
