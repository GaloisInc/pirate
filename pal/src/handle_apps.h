#pragma once

#include <pal/resource.h>

#include "launch.h"
#include "yaml.h"

/*
 * Handle messages from applications in a loop. Return when there are no
 * application fds remaining, or on an unrecoverable error.
 *
 * Return the number of application pipe fds remaining.
 *
 * This closes the pal context upon completion.
 */
int respond_to_apps(yaml_document_t* doc, struct app *apps, size_t apps_count, struct resource *rscs, size_t rscs_count);

/**
 * Prepend `base` to `path` with a separator if path is not absolute.
 *
 * Note.  This allocates memory with malloc that should be freed.
 * Returns null if malloc fails.
 */
const char* absolute_path(const char* path, const char* base);

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