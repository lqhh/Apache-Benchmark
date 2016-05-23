
#ifndef __AB_LOG_H_INCLUDED__
#define __AB_LOG_H_INCLUDED__

#include <ab_param.h>

#define AB_LOG_BUF 4096

#define AB_LOG_VERBOSE_DEBUG 0
#define AB_LOG_VERBOSE_INFO 1
#define AB_LOG_VERBOSE_ERROR 2
#define AB_LOG_VERBOSE_CRIT 3
#define AB_LOG_VERBOSE_NOLOG 4

void ab_log_core(int v, char *fmt, ...);

#define ab_log_debug(fmt, ...)                                          \
    ab_log_core(AB_LOG_VERBOSE_DEBUG, "%s "fmt, "[DEBUG]", ##__VA_ARGS__)

#define ab_log_info(fmt, ...)                                           \
    ab_log_core(AB_LOG_VERBOSE_INFO, "%s "fmt, "[INFO]", ##__VA_ARGS__)

#define ab_log_error(fmt, ...)                                          \
    ab_log_core(AB_LOG_VERBOSE_ERROR, "%s "fmt, "[ERROR]", ##__VA_ARGS__)

#define ab_log_crit(fmt, ...)                                           \
    ab_log_core(AB_LOG_VERBOSE_CRIT, "%s "fmt, "[CRIT]", ##__VA_ARGS__)

#define ab_log(fmt, ...)                                    \
    ab_log_core(AB_LOG_VERBOSE_NOLOG, fmt, ##__VA_ARGS__)

#endif /* __AB_LOG_H_INCLUDED__ */
