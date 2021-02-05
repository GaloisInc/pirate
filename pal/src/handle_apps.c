#define _POSIX_C_SOURCE 200809L
#include <features.h>

#include <pal/envelope.h>

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "handle_apps.h"
#include "log.h"

#define min(_a, _b) ((_a) < (_b) ? (_a) : (_b))

int cstring_resource_handler(pal_env_t *env, const struct app *app, pal_context_t *sd, const pal_config_node_t* root)
{
    int ret = 0;
    const char *s = NULL;

    if (pal_config_string(&s, sd, root, true, 0))
        ret = -1;
    else if (pal_add_to_env(env, (void*) s, strlen(s)))
        ret = -1;

    free((void*) s);
    return ret;
}

int int64_resource_handler(pal_env_t *env, const struct app *app, pal_context_t *sd, const pal_config_node_t* root)
{
    int64_t n;

    if (pal_config_int64(&n, sd, root, true, 0))
        return -1;

    if(pal_add_to_env(env, &n, sizeof n))
        return -1;

    return 0;
}

int bool_resource_handler(pal_env_t *env, const struct app *app, pal_context_t *sd, const pal_config_node_t* root)
{
    bool b;

    if(pal_config_bool(&b, sd, root, true, 0))
        return -1;

    if(pal_add_to_env(env, &b, sizeof b))
        return -1;

    return 0;
}

int file_resource_handler(pal_env_t *env, const struct app *app, pal_context_t *sd, const pal_config_node_t* node)
{
    const char *path = NULL;
    int flags = O_RDWR;
    int ret = 0;
    int fd;

    pal_yaml_enum_schema_t fflags_schema[] = {
        {"O_RDONLY",    O_RDONLY},
        {"O_WRONLY",    O_WRONLY},
        {"O_RDWR",      O_RDWR},

        {"O_APPEND",    O_APPEND},
        {"O_ASYNC",     O_ASYNC},
        {"O_CLOEXEC",   O_CLOEXEC},
        {"O_CREAT",     O_CREAT},
        {"O_DIRECTORY", O_DIRECTORY},
        {"O_DSYNC",     O_DSYNC},
        {"O_EXCL",      O_EXCL},
        {"O_NOCTTY",    O_NOCTTY},
        {"O_NOFOLLOW",  O_NOFOLLOW},
        {"O_NONBLOCK",  O_NONBLOCK},
        {"O_SYNC",      O_SYNC},
        {"O_TRUNC",     O_TRUNC},
        PAL_YAML_ENUM_END
    };

    pal_config_string(&path, sd, node, true, 1, "file_path");
    pal_config_flags(&flags, fflags_schema, sd, node, true, 1, "file_flags");
    if(pal_error_count(sd) > 0)
        ret = -1;

    else if((fd = open(path, flags)) < 0)
        ret = -1;
    // TODO: Allow file creation modes to be set?
    // TODO: Allow paths relative to config?

    else if(pal_add_fd_to_env(env, fd))
        ret = -1;

    free((void*) path);
    return ret;
}

/* Maximum number of resource types
 */
#define HANDLER_TABLE_MAX 1024
#define HANDLER_TYPE_MAX 64

static struct handler_table_entry {
    char *type;
    pal_resource_handler_t *handler;
    void *dlhandle;
} handler_table[HANDLER_TABLE_MAX] = {
    { "boolean",        &bool_resource_handler,           NULL },
    { "file",           &file_resource_handler,           NULL },
    { "integer",        &int64_resource_handler,          NULL },
    { "string",         &cstring_resource_handler,        NULL },
    { NULL,             NULL,                             NULL },
};

const char* absolute_path(const char* path, const char* base) {
    // If path starts with '/' then it is already absolute.
    if (path[0] == '/') {
        return strdup(path);
    }
    size_t baseLen = strlen(base);
    if (baseLen == 0) return 0;
    size_t pathLen = strlen(path);
    bool lastBaseCharIsSep = base[baseLen - 1] == '/';

    if (lastBaseCharIsSep) {
        char* r = malloc(baseLen + pathLen + 1);
        if (!r) return 0;
        memcpy(r, base, baseLen);
        memcpy(r + baseLen, path, pathLen + 1);
        return r;
    } else {
        char* r = malloc(baseLen + pathLen + 2);
        if (!r) return 0;
        memcpy(r, base, baseLen);
        r[baseLen] = '/';
        memcpy(r + baseLen + 1, path, pathLen + 1);
        return r;
    }
}

void load_resource_plugins(const char *dirpath)
{
    DIR *dir = opendir(dirpath);
    if(!dir) {
        fatal("Failed to open plugin directory `%s'.", dirpath);
        return;
    }

    struct dirent *ent = NULL;
    while((ent = readdir(dir))) {
        const char *suffix = strrchr(ent->d_name, '.');
        if(!suffix || strcmp(suffix, ".so"))
            continue;

        char name[strlen(ent->d_name) + 1 /*\0*/];
        snprintf(name, sizeof name, "%.*s",
                (int)(strlen(ent->d_name) - 3 /*.so*/), ent->d_name);

        char path[strlen(dirpath) + 1 + strlen(ent->d_name) + 1 /*\0*/];
        snprintf(path, sizeof path, "%s/%s", dirpath, ent->d_name);
        plog(LOGLVL_DEBUG, "Found plugin for %s: %s", name, path);

        void *dlhandle = dlopen(path, RTLD_NOW);
        if(!dlhandle) {
            fatal("Failed to open plugin for %s from %s", name, path);
            continue;
        }

        char hname[strlen(name) + sizeof("_resource_handler")];
        snprintf(hname, sizeof hname, "%s_resource_handler", name);

        pal_resource_handler_t *handler =
            (pal_resource_handler_t*)(unsigned long)dlsym(dlhandle, hname);
        if(!handler) {
            warn("Failed to find expected handler function %s in %s",
                    hname, path);
            dlclose(dlhandle);
            continue;
        }

        size_t i = 0;
        while(i < HANDLER_TABLE_MAX-1 && handler_table[i].type)
            ++i;
        if(i == HANDLER_TABLE_MAX-1) {
            warn("Out of space in handler table");
        } else {
            handler_table[i].type = strdup(name);
            handler_table[i].handler = handler;
            handler_table[i].dlhandle = dlhandle;

            handler_table[i+1].type = NULL;
            handler_table[i+1].handler = NULL;
            handler_table[i+1].dlhandle = NULL;
        }
    }

    closedir(dir);
}

void free_resource_plugins(void)
{
    for(size_t i = 0; i < HANDLER_TABLE_MAX && handler_table[i].type; ++i)
        if(handler_table[i].dlhandle) {
            dlclose(handler_table[i].dlhandle);
            free(handler_table[i].type);
        }
}

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

static struct resource *lookup_resource(const char *app_name, char *rsc_name,
        struct resource *rscs, size_t rscs_count)
{
    size_t id_size = strlen(app_name) + 1 + strlen(rsc_name) + 1;
    char id[id_size];
    snprintf(id, id_size, "%s/%s", app_name, rsc_name);

    for(size_t i = 0; i < rscs_count; ++i) {
        
        struct resource *rsc = &rscs[i];

        for(size_t j = 0; j < rsc->r_ids_count; ++j) {
            if (!strcmp(id, rsc->r_ids[j])) {
                return rsc;
            }
        }
    }

    return NULL;
}

/* Look up a resource handler from `handler_table` by type name.
 *
 * Return a pointer to the handler function if the type is found, or NULL,
 * otherwise.
 */
static pal_resource_handler_t *lookup_handler(const char *type)
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
 *
 * NB: Errors that indicate errors in the config file are treated as fatal,
 * and will cause PAL to hang up on the current app. This include an app
 * requesting an unknown resource or a resource of an incorrect or mismatched
 * type, as well as failures in the called resource handler.
 */
static int handle_event(yaml_document_t* doc, struct epoll_event *event, struct resource *rscs, size_t rscs_count)
{
    struct app *app = (struct app *)event->data.ptr;
    pal_resource_handler_t *handle;

    pal_context_t* ctx = malloc(sizeof(pal_context_t));
    if (!ctx) {
        error("Failed to allocate context.");
        return -1;
    }
    ctx->doc = doc;
    ctx->error_count = 0;

    plog(LOGLVL_DEBUG, "Received an epoll event from %s", app->name);

    if (event->events & EPOLLHUP) {
        app->hangup = true;
        plog(LOGLVL_INFO, "Received hangup from %s", app->name);
    }

    if(event->events & EPOLLERR)
        plog(LOGLVL_DEFAULT, "Encountered an error on fd for %s", app->name);

    if (event->events & EPOLLIN) {
        char *name = NULL, *type = NULL;
        struct resource *rsc = NULL;
        pal_env_t env = EMPTY_PAL_ENV(PAL_RESOURCE);
        int err;


        err = pal_recv_resource_request(app->pipe_fd, &type, &name, MSG_DONTWAIT);
        if(err == PAL_ERR_EMPTY) {
            plog(LOGLVL_INFO, "Received connection-terminating empty message "
                    "from %s", app->name);
            return -1;
        } else if(err) {
            plog(LOGLVL_INFO, "Encountered an error parsing resource request "
                    "from %s: %s", app->name,
                    err < 0 ? strerror(-err) : pal_strerror(err));
            return 0;
        }

        plog(LOGLVL_INFO, "Received request for resource %s of type %s "
                "from %s", name, type, app->name);

        bool fatal = true;

        if(err) {
            error("Encountered an error reading resource request from %s: %s",
                    app->name, err > 0 ? pal_strerror(err) : strerror(-err));
            goto exit;
        }

        if (!(rsc = lookup_resource(app->name, name, rscs, rscs_count))) {
            error("Unknown resource named %s of type %s from %s",
                    name, type, app->name);
            goto exit;
        }

        if (strcmp(rsc->r_type, type)) {
            error("Type %s of resource %s requested by %s does not match "
                    "config (%s)", type, name, app->name, rsc->r_type);
            goto exit;
        }

        if (!(handle = lookup_handler(type))) {
            error("Received request for resource named %s of unknown type %s "
                    "from %s", name, type, app->name);
            goto exit;
        }

        if (handle(&env, app, ctx, rsc->r_config)) {
            error("Handler failed for resource named %s of type %s requested "
                    "by %s", name, type, app->name);
            pal_context_flush_errors(ctx);
            goto exit;
        }
        pal_context_flush_errors(ctx);

        if ((err = pal_send_env(app->pipe_fd, &env, MSG_DONTWAIT))) {
            error("Failed to send resource named %s of type %s to %s: %s",
                    name, type, app->name, strerror(-err));
            goto exit;
        }

        plog(LOGLVL_INFO, "Sent a %lu-byte envelope with %lu fds to %s",
                env.size, env.fds_count, app->name);
        fatal = false;


        exit:
            free(type);
            free(name);
            pal_free_env(&env);
            if (fatal)
                return -1;
    }

    return 0;
}

int respond_to_apps(yaml_document_t* doc, struct app *apps, size_t apps_count, struct resource *rscs, size_t rscs_count)
{
    int epfd;
    struct epoll_event event;

    if ((epfd = make_epfd(apps, apps_count)) < 0) {
        return -1;
    }

    while(apps_count && epoll_wait(epfd, &event, 1, -1) != -1) {
        if (handle_event(doc, &event, rscs, rscs_count)) {
            struct app *app = event.data.ptr;

            if(epoll_ctl(epfd, EPOLL_CTL_DEL, app->pipe_fd, NULL))
                error("Failed to deregister pipe fd for %s: %s", app->name, strerror(errno));
            else
                plog(LOGLVL_DEBUG, "Deregistered pipe fd for %s", app->name);
            app->pipe_fd = -1;
            --apps_count;
        }
    }
    return apps_count;
}
