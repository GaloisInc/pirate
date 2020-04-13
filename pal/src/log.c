#include "log.h"

enum log_level log_level = LOGLVL_DEBUG;
// FIXME: ^ Set this to default and make configurable.

static void vlog(enum log_level lvl, const char *fmt, va_list ap)
{
    if(lvl > log_level)
        return;

    switch(lvl) {
    case LOGLVL_INFO:
        fputs("INFO:\t", stderr);
        break;
    case LOGLVL_DEBUG:
        fputs("DEBUG:\t", stderr);
        break;
    case LOGLVL_DEFAULT:
        break;
    }

    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
}

void plog(enum log_level lvl, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vlog(lvl, fmt, ap);
    va_end(ap);
}

void fatal(char *fmt, ...)
{
    va_list ap;

    fputs("ERROR:\t", stderr);
    va_start(ap, fmt);
    vlog(LOGLVL_DEFAULT, fmt, ap);
    va_end(ap);

    exit(EXIT_FAILURE);
}

void error(char *fmt, ...)
{
    va_list ap;

    fputs("ERROR:\t", stderr);
    va_start(ap, fmt);
    vlog(LOGLVL_DEFAULT, fmt, ap);
    va_end(ap);
}

void warn(char *fmt, ...)
{
    va_list ap;

    fputs("WARNING:\t", stderr);
    va_start(ap, fmt);
    vlog(LOGLVL_DEFAULT, fmt, ap);
    va_end(ap);
}
