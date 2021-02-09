#include <string.h>
#include <unistd.h>

#include "log.h"

enum log_level log_level = LOGLVL_DEFAULT;

static void vlog(const char *prefix, const char *fmt, va_list ap)
{
    char buf[4096];
    size_t pos = 0;

    if (prefix) {
        int r = snprintf(buf, sizeof(buf), "%s:\t", prefix);
        if (r < 0 || r >= sizeof(buf)) {
            const char* msg = "INTERNAL ERROR: Cannot render error.";
            write(STDERR_FILENO, msg, strlen(msg));
            return;
        }
        r = pos;
    }
    size_t sz = sizeof(buf) - pos - 2;
    int r = vsnprintf(buf + pos, sz, fmt, ap);
    if (r < 0 || (size_t) r >= sz) {
        const char* msg = "INTERNAL ERROR: Cannot render error.";
        write(STDERR_FILENO, msg, strlen(msg));
        return;
    }
    memcpy(buf+pos+r, "\n", 2);
    write(STDERR_FILENO, buf, pos+r+1);
}

void plog(enum log_level lvl, char *fmt, ...)
{
    if (lvl > log_level)
        return;
    const char* prefix=0;
    switch(lvl) {
    case LOGLVL_INFO:
        prefix = "INFO";
        break;
    case LOGLVL_DEBUG:
        prefix = "DEBUG";
        break;
    case LOGLVL_DEFAULT:
        break;
    }

    va_list ap;
    va_start(ap, fmt);
    vlog(prefix, fmt, ap);
    va_end(ap);
}

void fatal(char *fmt, ...)
{
    if (LOGLVL_DEFAULT <= log_level) {
        va_list ap;
        va_start(ap, fmt);
        vlog("ERROR", fmt, ap);
        va_end(ap);
    }

    exit(EXIT_FAILURE);
}

void error(char *fmt, ...)
{
    if (LOGLVL_DEFAULT > log_level) return;
    va_list ap;
    va_start(ap, fmt);
    vlog("ERROR", fmt, ap);
    va_end(ap);
}

void warn(char *fmt, ...)
{
    if (LOGLVL_DEFAULT > log_level)
        return;

    va_list ap;
    va_start(ap, fmt);
    vlog("WARN", fmt, ap);
    va_end(ap);
}
