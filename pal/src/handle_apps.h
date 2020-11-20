#ifndef _PIRATE_PAL_HANDLE_APPS_H
#define _PIRATE_PAL_HANDLE_APPS_H

#include <pal/resource.h>

#include "launch.h"
#include "yaml.h"

#define DEFAULT_PLUGIN_DIR "/usr/local/lib/pirate/pal/plugins"

/* Handle messages from applications in a loop. Return when there are no
 * application fds remaining, or on an unrecoverable error.
 *
 * Return the number of application pipe fds remaining.
 */
int handle_apps(struct app *apps, size_t apps_count,
        struct resource *rscs, size_t rscs_count);

/* Return the path to the directory we should look for resource plugins in.
 * The caller is responsible for calling free on the returned pointer.
 *
 * Returns a non-NULL pointer on success. If memory cannot be allocated,
 * returns NULL.
 */
char *get_plugin_dir(const char *cfg_path, struct top_level *tlp);

/* Load all resource plugins from dirpath. Plugins are expected to be called
 * <resource_type>.so and to contain a function function called
 * <resource_type>_resource_handler().
 *
 * A non-existent plugin directory or invalid plugin files result in warnings.
 */
void load_resource_plugins(const char *dirpath);

/* Close resource plugins opened with dlopen().
 */
void free_resource_plugins(void);

#endif // _PIRATE_PAL_HANDLE_APPS_H
