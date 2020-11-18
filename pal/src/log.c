#include <unistd.h>

#include "log.h"

//enum log_level log_level = LOGLVL_DEFAULT;
enum log_level log_level = LOGLVL_DEBUG;

static void vlog(const char *prefix, enum log_level lvl,
        const char *fmt, va_list ap)
{
    if(lvl > log_level)
        return;

    char buf[4096];
    size_t pos = 0;

    if(prefix)
        pos += snprintf(buf + pos, sizeof(buf) - pos, "%s:\t", prefix);

    switch(lvl) {
    case LOGLVL_INFO:
        pos += snprintf(buf + pos, sizeof(buf) - pos, "INFO:\t");
        break;
    case LOGLVL_DEBUG:
        pos += snprintf(buf + pos, sizeof(buf) - pos, "DEBUG:\t");
        break;
    case LOGLVL_DEFAULT:
        break;
    }

    pos += vsnprintf(buf + pos, sizeof(buf) - pos, fmt, ap);
    pos += snprintf(buf + pos, sizeof(buf) - pos, "\n");

    write(STDERR_FILENO, buf, pos);
}

void plog(enum log_level lvl, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vlog(NULL, lvl, fmt, ap);
    va_end(ap);
}

void fatal(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vlog("ERROR", LOGLVL_DEFAULT, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

void error(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vlog("ERROR", LOGLVL_DEFAULT, fmt, ap);
    va_end(ap);
}

void warn(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vlog("WARN", LOGLVL_DEFAULT, fmt, ap);
    va_end(ap);
}
