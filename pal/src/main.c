#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "yaml.h"
#include "log.h"
#include "launch.h"
#include "handle_apps.h"

void kill_apps(struct app *apps, size_t apps_count)
{
    size_t i;

    for(i = 0; i < apps_count; ++i)
        if(apps[i].pid)
            kill(apps[i].pid, SIGTERM);
}

struct Args {
    const char* plugin_dir;
    const char* config_path;
    bool showHelp;
};

void showUsage(const char* exe) {
    fatal("Usage: %s [--plugins <path>] config.yaml", exe);
}

void parseArgs(struct Args* args, int argc, const char** argv) {
    memset(args, 0, sizeof(struct Args));
    int i = 1;
    while (i < argc) {
        const char* a = argv[i];
        if (strcmp(a, "--help") == 0) {
            showUsage(argv[0]);
        } else if (strcmp(a, "--plugins") == 0) {
            if (i + 1 == argc) {
                showUsage(argv[0]);
            }
            args->plugin_dir = argv[i+1];
            i += 2;
        } else {
            if (args->config_path != 0) {
                showUsage(argv[0]);
            }
            args->config_path = argv[i];
            ++i;
        }
    }
    if (args->config_path == 0) {
        showUsage(argv[0]);
    }
}

int main(int argc, const char **argv, char **envp) {
    struct top_level *tlp;

    struct Args args;
    parseArgs(&args, argc, argv);

    yaml_document_t doc;
    pal_context_t ctx;
    if (!(tlp = load_yaml(&doc, &ctx, args.config_path)))
        fatal("Failed to load yaml from %s.", args.config_path);

    if (tlp->tl_cfg.cfg_loglvl > log_level)
        log_level = tlp->tl_cfg.cfg_loglvl;

    plog(LOGLVL_DEBUG, "Read configuration from `%s'", args.config_path);

    const char* plugin_dir;
    if (args.plugin_dir) {
        plugin_dir = strdup(args.plugin_dir);
    } else if (tlp->tl_cfg.cfg_plugin_dir) {
        plugin_dir = absolute_path(tlp->tl_cfg.cfg_plugin_dir, args.config_path);
        if (!plugin_dir) fatal("Memory allocation failed.");
    } else {
        char* exe_dir = strdup(argv[0]);
        if (!exe_dir) fatal("Memory allocation failed.");
        dirname(exe_dir); // Drop filename
        dirname(exe_dir); // Drop /bin
        plugin_dir = absolute_path("lib/pirate/pal/plugins", exe_dir);
        if (!plugin_dir) fatal("Memory allocation failed.");
        free(exe_dir);
    }

    load_resource_plugins(plugin_dir);
    free((void*) plugin_dir);

    size_t apps_count = tlp->tl_encs_count;
    struct app apps[apps_count];
    int err = 0;
    for(size_t i = 0; i < apps_count; ++i) {
        if (launch(&apps[i], args.config_path, &tlp->tl_encs[i], envp)) {
            error("Launching app %s failed", tlp->tl_encs[i].enc_name);

            apps[i].pid = 0;
            err = -1;
        } else {
            plog(LOGLVL_INFO, "Launched app %s", tlp->tl_encs[i].enc_name);
        }
    }

    if (err || respond_to_apps(&doc, apps, apps_count, tlp->tl_rscs, tlp->tl_rscs_count) < 0)
        error("Failed to start app handler");
    free_yaml(tlp);

    plog(LOGLVL_INFO, "Killing apps...");
    kill_apps(apps, apps_count);

    errno = 0;
    while (errno != ECHILD)
        wait(NULL);

    free_resource_plugins();
    return -err;
}
