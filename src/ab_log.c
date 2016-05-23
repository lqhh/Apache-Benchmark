
#include <stdio.h>
#include <stdarg.h>
#include <ab_log.h>
#include <ab_param.h>



void
ab_log_core(int v, char *fmt, ...) {
    
    static char ab_log_buf[AB_LOG_BUF];
    va_list args;

    if (v < ab_verbose)
        return;

    va_start(args, fmt);
    vsnprintf(ab_log_buf, AB_LOG_BUF, fmt, args);
    ab_log_buf[AB_LOG_BUF-1] = '\0';

    fprintf(stderr, "%s\n", ab_log_buf);
}
