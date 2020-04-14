#ifndef _PIRATE_PAL_LOG_H
#define _PIRATE_PAL_LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define DO_AT_LOGLVL(_lvl) \
    if(log_level >= (_lvl))

enum log_level {
    LOGLVL_DEFAULT,
    LOGLVL_INFO,
    LOGLVL_DEBUG,
};

extern enum log_level log_level;

void plog(enum log_level lvl, char *fmt, ...);

/* Output a LOGLVL_DEFAULT message and exit.
 */
void fatal(char *fmt, ...);

/* Output a LOGLVL_DEFAULT message.
 */
void error(char *fmt, ...);

/* Output a LOGLVL_DEFAULT message.
 */
void warn(char *fmt, ...);

#endif // _PIRATE_PAL_LOG_H
