#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "launch.h"

#define PARENT_END (0)
#define CHILD_END (1)

// TODO: Make things const when possible

/* Make the path to the executable for exec:
 *   * If app_path is absolute, just use that
 *   * Otherwise, use a path relative to the location of the config file
 *
 * `path` should point to at least `strlen(cfg_path) + strlen(app_path) + 1`
 * bytes. Otherwise, the path may be truncated.
 */
static void make_path(char *path, size_t path_len,
        char *cfg_path, char *app_path)
{
    char *last_slash;

    if(app_path[0] == '/' // app_path is absolute
            || !(last_slash = strrchr(cfg_path, '/'))) {
        snprintf(path, path_len, "%s", app_path);
    } else {
        snprintf(path, path_len, "%.*s%s",
                (int)(last_slash - cfg_path) + 1, cfg_path, app_path);
    }
}

/* Count the number of non-NULL strings in a NULL-terminated vector of
 * strings.
 */
static size_t count_envp(char **envp)
{
    size_t i;

    for(i = 0; envp[i]; ++i)
        ;

    return i;
}

/* Set up an environment vector for the launched executable, containing
 * `env_count` strings from `env`, `old_envp_count` strings from
 * `old_envp`, and a final NULL terminator.
 *
 * The `new_envp` vector should contain space for at least
 * `env_count + old_envp_count + 2` strings.
 */
static void make_envp(char **new_envp, char *pal_fd_env,
        char **env, size_t env_count,
        char **old_envp, size_t old_envp_count)
{
    size_t i;

    new_envp[0] = pal_fd_env;

    for(i = 0; i < env_count; ++i)
        new_envp[i + 1] = env[i];

    for(i = 0; i < old_envp_count; ++i)
        new_envp[i + env_count + 1] = old_envp[i];

    new_envp[env_count + old_envp_count + 1] = NULL;
}

/* Set up an argument vector for the launched executable and put in
 * `new_argv`.
 *
 * The `new_argv` vector should contain space for at least
 * `args_count + 2` strings.
 */
static void make_argv(char **new_argv, char *path,
        char **args, size_t args_count)
{
    size_t i;

    new_argv[0] = path;
    for(i = 0; i < args_count; ++i)
        new_argv[i + 1] = args[i];
    new_argv[args_count + 1] = NULL;
}

/* Set `FD_CLOEXEC` on a file descriptor.
 *
 * Return 0 on success. Otherwise, return -1 and leave `errno` set.
 */
static int set_cloexec(int fd)
{
    int flags;

    if((flags = fcntl(fd, F_GETFD)) < 0)
        return -1;

    if(fcntl(fd, F_SETFD, flags | FD_CLOEXEC))
        return -1;

    return 0;
}

#define PERROR(_op) error("Failed to %s: %s", _op, strerror(errno))

int launch(struct app *app, char *cfg_path,
        struct enclave *enc, char **envp)
{
    int fds[2];
    size_t envp_count = count_envp(envp);
    char *enc_path = enc->enc_path ? enc->enc_path : enc->enc_name;
    char *new_argv[1 /*progname*/ + enc->enc_args_count + 1 /*NULL*/];
    char *new_envp[1 /*PAL_FD*/ + enc->enc_env_count + envp_count + 1 /*NULL*/];
    char path[strlen(cfg_path) + strlen(enc_path) + 1 /*\0*/];
    char pal_fd_env[32]; // Large enough for "PAL_FD=XXXX"
    int cwd = -1;

    // Open pipe to child
    if(socketpair(AF_LOCAL, SOCK_STREAM, 0, fds)) {
        PERROR("create pipe");
        return -1;
    }
    app->pipe_fd = fds[PARENT_END];
    app->hangup = false;
    snprintf(pal_fd_env, sizeof pal_fd_env, "PAL_FD=%d", fds[CHILD_END]);

    // Set up path, argument vector, and environment vector for app
    make_path(path, sizeof path, cfg_path, enc_path);
    make_argv(new_argv, path, enc->enc_args, enc->enc_args_count);
    make_envp(new_envp, pal_fd_env,
            enc->enc_env, enc->enc_env_count,
            envp, envp_count);
    app->name = enc->enc_name;

    if (enc->enc_directory) {
        cwd = open(".", O_RDONLY | O_CLOEXEC);
        if (cwd < 0)
            PERROR("open current directory");
        if (chdir(enc->enc_directory))
            PERROR("change directory");
    }

    if(set_cloexec(fds[PARENT_END]))
        PERROR("set FD_CLOEXEC on pipe");
    else if(posix_spawn(&app->pid, path, NULL, NULL, new_argv, new_envp))
        PERROR("spawn process");

    close(fds[CHILD_END]);
    if (cwd >= 0) {
        if (fchdir(cwd))
            PERROR("revert directory");
        if (close(cwd))
            PERROR("close current directory");
    }
    return 0;
}
