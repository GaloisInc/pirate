#include <pal/envelope.h>

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "handle_apps.h"
#include "handlers.h"
#include "log.h"

static int make_epfd(struct app *apps, size_t apps_count)
{
    size_t i;
    int epfd;

    if((epfd = epoll_create(apps_count)) < 0) {
        plog(LOGLVL_INFO, "Failed to create epoll instance: %s",
                strerror(errno));
        return -1;
    }

    for(i = 0; i < apps_count; ++i) {
        struct app *app = &apps[i];
        struct epoll_event event = {
            .data = { .ptr = app },
            .events = EPOLLIN,
        };

        if(epoll_ctl(epfd, EPOLL_CTL_ADD, app->pipe_fd, &event)) {
            plog(LOGLVL_INFO,
                    "Failed to add fd for %s to epoll instance: %s",
                    apps[i].name, strerror(errno));
            close(epfd);
            return -1;
        }
    }

    return epfd;
}

static struct resource *lookup_resource(char *app_name, char *rsc_name,
        struct resource *rscs, size_t rscs_count)
{
    size_t id_size = strlen(app_name) + 1 + strlen(rsc_name) + 1;
    char id[id_size];
    size_t i;

    snprintf(id, id_size, "%s/%s", app_name, rsc_name);

    for(i = 0; i < rscs_count; ++i) {
        size_t j;
        struct resource *rsc = &rscs[i];

        for(j = 0; j < rsc->r_ids_count; ++j)
            if(!strcmp(id, rsc->r_ids[j]))
                return rsc;
    }

    return NULL;
}

static resource_handler_t *lookup_handler(const char *type)
{
    size_t i;

    for(i = 0; i < HANDLER_TABLE_MAX && handler_table[i].type; ++i)
        if(!strcmp(type, handler_table[i].type))
            return handler_table[i].handler;

    return NULL;
}

/* Handle an event received from `epoll_wait`. If that event indicates that
 * data can be read from the pipe, interpret the message and send a response,
 * if appropriate.
 *
 * Return 0 on success if the fd corresponding to the request could still
 * potentially be used. If an empty message is received, indicating the other
 * end has closed the connection, return -1.
 */
static int handle_event(struct epoll_event *event,
        struct resource *rscs, size_t rscs_count)
{
    struct app *app = (struct app *)event->data.ptr;
    resource_handler_t *handle;

    if(event->events & EPOLLHUP) {
        app->hangup = true;
        plog(LOGLVL_INFO, "Received hangup from %s", app->name);
    }

    if(event->events & EPOLLERR)
        plog(LOGLVL_DEFAULT, "Encountered an error on fd for %s", app->name);

    if(event->events & EPOLLIN) {
        char *name = NULL, *type = NULL;
        struct resource *rsc;
        pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);
        int err;

        err = pal_recv_resource_request(app->pipe_fd, &type, &name,
                MSG_DONTWAIT);
        if(err == PAL_ERR_EMPTY && app->hangup)
            return -1;

        plog(LOGLVL_INFO, "Received a resource request from %s", app->name);

        if(err)
            error("Encountered an error reading resource request from %s: %s",
                    app->name, err > 0 ? pal_strerror(err) : strerror(-err));

        else if(!(rsc = lookup_resource(app->name, name, rscs, rscs_count)))
            error("Received request for unknown resource named %s "
                    "of type %s from %s", name, type, app->name);

        else if(strcmp(rsc->r_type, type))
            error("Type %s of resource %s requested by %s does not match "
                    "config (%s)", type, name, app->name, rsc->r_type);

        else if(!(handle = lookup_handler(type)))
            error("Received request for resource named %s of unknown type %s "
                    "from %s", name, type, app->name);

        else if(handle(&env, app, rsc))
            error("Handler failed for resource named %s of type %s requested "
                    "by %s", name, type, app->name);

        else if((err = pal_send_env(app->pipe_fd, &env, MSG_DONTWAIT)))
            error("Failed to send resource named %s of type %s to %s: %s",
                    name, type, app->name, strerror(-err));

        free(type);
        free(name);
        pal_free_env(&env);
    }

    return 0;
}

int handle_apps(struct app *apps, size_t apps_count,
        struct resource *rscs, size_t rscs_count)
{
    int epfd;
    struct epoll_event event;

    if((epfd = make_epfd(apps, apps_count)) < 0)
        return -1;

    while(apps_count && epoll_wait(epfd, &event, 1, -1) != -1)
        if(handle_event(&event, rscs, rscs_count)) {
            struct app *app = event.data.ptr;

            if(epoll_ctl(epfd, EPOLL_CTL_DEL, app->pipe_fd, NULL))
                error("Failed to deregister pipe fd for %s: %s",
                        app->name, strerror(errno));
            app->pipe_fd = -1;
            --apps_count;
        }

    return apps_count;
}
