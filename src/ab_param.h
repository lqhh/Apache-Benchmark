
#ifndef __AB_PARAM_H_INCLUDED__
#define __AB_PARAM_H_INCLUDED__

#include <netinet/in.h>

#define AB_PARAM_BUF 64

#define AB_PARAM_CMD 0
#define AB_PARAM_VALUE 1

#define AB_PARAM_URI_SCHEME 0
#define AB_PARAM_URI_HOST 1
#define AB_PARAM_URI_PORT 2
#define AB_PARAM_URI_PATH 3

void ab_param_dump();
char *ab_param_error(char *fmt, ...);
int ab_param_check(char *cmd);
char *ab_param_parse(int argc, char *argv[]);
char *ab_param_parse_uri(char *uri);
char *ab_param_addr(char *hostname);


extern int ab_requests;
extern int ab_concurrency;
extern int ab_time_limit;
extern char *ab_request_method;
extern char *ab_uri;
extern int ab_verbose;

extern char *ab_scheme;
extern char *ab_hostname;
extern struct in_addr ab_addr;
extern short ab_port;
extern char *ab_path;

#endif /* __AB_PARAM_H_INCLUDED__ */
