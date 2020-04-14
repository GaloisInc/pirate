#ifndef _PIRATE_PAL_HANDLE_APPS_H
#define _PIRATE_PAL_HANDLE_APPS_H

#include "launch.h"
#include "yaml.h"

#include <pal/pal.h>

/* Handle messages from applications in a loop. Return when there are no
 * application fds remaining, or on an unrecoverable error.
 *
 * Return the number of application pipe fds remaining.
 */
int handle_apps(struct app *apps, size_t apps_count,
        struct resource *rscs, size_t rscs_count);

#endif // _PIRATE_PAL_HANDLE_APPS_H
