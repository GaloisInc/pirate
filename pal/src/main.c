#include <errno.h>
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

int main(int argc, char **argv, char **envp)
{
    char *cfg_path;
    struct top_level *tlp;
    int err = 0;
    size_t i;

    if(argc != 2)
        fatal("Usage: %s path/to/config.yaml", argv[0]);
    cfg_path = argv[1];

    if(!(tlp = load_yaml(cfg_path)))
        fatal("Failed to load yaml from %s", argv[0]);

    if(tlp->tl_cfg.cfg_loglvl > log_level)
        log_level = tlp->tl_cfg.cfg_loglvl;

    plog(LOGLVL_DEBUG, "Read configuration from `%s'", cfg_path);

    size_t apps_count = tlp->tl_encs_count;
    struct app apps[apps_count];
    for(i = 0; i < apps_count; ++i) {
        if(launch(&apps[i], cfg_path, &tlp->tl_encs[i], envp)) {
            plog(LOGLVL_INFO, "Launching app %s failed",
                    tlp->tl_encs[i].enc_name);

            apps[i].pid = 0;
            err = -1;
        }
        plog(LOGLVL_INFO, "Launched app %s", tlp->tl_encs[i].enc_name);
    }

    if(err || handle_apps(apps, apps_count,
                tlp->tl_rscs, tlp->tl_rscs_count) < 0)
        error("Failed to start app handler");

    plog(LOGLVL_INFO, "Killing apps...");
    kill_apps(apps, apps_count);

    errno = 0;
    while(errno != ECHILD)
        wait(NULL);

    free_yaml(tlp);

    return -err;
}
