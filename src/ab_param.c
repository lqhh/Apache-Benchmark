
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ab_param.h>
#include <ab_log.h>
#include <ab_stat.h>

int ab_verbose = AB_LOG_VERBOSE_NOLOG;
int ab_requests = 1;
int ab_concurrency = 1;
int ab_time_limit = 50000;
char *ab_request_method = "GET";

char *ab_uri = NULL;
static char ab_uri_parse[AB_PARAM_BUF];
char *ab_scheme = NULL;
char *ab_hostname = NULL;
struct in_addr ab_addr;
static char *ab_port2 = NULL;
short ab_port = 80;
char *ab_path = NULL;


void
ab_param_dump() {
    ab_log_error("-n %d", ab_requests);
    ab_log_error("-c %d", ab_concurrency);
    ab_log_error("-t 0x%x", ab_time_limit);
    ab_log_error("-i %s", ab_request_method);
    ab_log_error("-v %d", ab_verbose);
    ab_log_error("URI %s", ab_uri);
    ab_log_error("scheme %s", ab_scheme);
    ab_log_error("hostname %s", ab_hostname);
    ab_log_error("addr %s", inet_ntoa(ab_addr));
    ab_log_error("port %hu", ab_port);
    ab_log_error("path %s", ab_path);
}


char *
ab_param_error(char *fmt, ...) {

    static char error[AB_PARAM_BUF];
    va_list args;

    va_start(args, fmt);
    vsnprintf(error, AB_PARAM_BUF, fmt, args);
    va_end(args);
    error[AB_PARAM_BUF-1] = '\0';
    
    return error;
}


int
ab_param_check(char *cmd){

    if (cmd[0] != '-' || cmd[1] == '\0' || cmd[2] != '\0')
        return 0;
    
    return 1;
}


char *
ab_param_parse(int argc, char *argv[]) {

    int i, state;
    char cmd;

    /*
      skip argv[0] and argv[argc-1]
      argv[0] is program name, argv[argc-1] is URI
    */
    for (i = 1, state = AB_PARAM_CMD; i < argc-1; i++) {
        
        if (state == AB_PARAM_CMD) {
            if (ab_param_check(argv[i]) == 0)
                return ab_param_error("bad command \"%s\"", argv[i]);
            
            cmd = argv[i][1];
            /* handle command without argument */
            switch(cmd) {
            case 'i':
                ab_request_method = "HEAD";
                break;
            default:
                state = AB_PARAM_VALUE;
                break;
            }
            continue;
        }

        /* set command value */
        switch(cmd) {
        case 'n':
            sscanf(argv[i], "%d", &ab_requests);
            break;
        case 'c':
            sscanf(argv[i], "%d", &ab_concurrency);
            break;
        case 't':
            sscanf(argv[i], "%d", &ab_time_limit);
            break;
        case 'v':
            sscanf(argv[i], "%d", &ab_verbose);
            break;
        default:
            return ab_param_error("unknown command \"%c\"", cmd);
        }

        state = AB_PARAM_CMD;
    }

    if (state == AB_PARAM_VALUE)
        return ab_param_error("absent command value");

    ab_uri = argv[argc-1];
    strcpy(ab_uri_parse, ab_uri);

    return ab_param_parse_uri(ab_uri_parse);
}


char *
ab_param_parse_uri(char *uri) {
    
    int i;
    int state;
    
    state = AB_PARAM_URI_SCHEME;
    for (i = 0; uri[i] != '\0'; i++) {
        switch(state) {
        case AB_PARAM_URI_SCHEME:
            if (ab_scheme == NULL)
                ab_scheme = &uri[i];
            if (i-2 >= 0 && uri[i] == '/' && uri[i-1] == '/' && uri[i-2] == ':') {
                uri[i-2] = '\0';
                state = AB_PARAM_URI_HOST;
            }
            break;
    
        case AB_PARAM_URI_HOST:
            if (ab_hostname == NULL)
                ab_hostname = &uri[i];
            
            if (uri[i] == ':') {
                uri[i] = '\0';
                state = AB_PARAM_URI_PORT;
            }

            if (uri[i] == '/') {
                uri[i] = '\0';
                state = AB_PARAM_URI_PATH;
            }
            
            break;

        case AB_PARAM_URI_PORT:
            if (ab_port2 == NULL)
                ab_port2 = &uri[i];
            
            if (uri[i] == '/') {
                uri[i] = '\0';
                state = AB_PARAM_URI_PATH;
            }

            break;
            
        case AB_PARAM_URI_PATH:
            if (ab_path == NULL)
                ab_path = &uri[i];
            
        default:
            break;
        }
    }

    if (state != AB_PARAM_URI_PATH &&
        state != AB_PARAM_URI_PORT &&
        state != AB_PARAM_URI_HOST) 
        return ab_param_error("bad uri \"%s\"", ab_uri);

    if (ab_port2)
        sscanf(ab_port2, "%hu", &ab_port);

    if (strcmp(ab_scheme, "http") != 0)
        return "only http supported";

    if (ab_hostname == NULL)
        return "hostname expected";

    if (ab_path == NULL)
        ab_path = "/";

    return ab_param_addr(ab_hostname);
}


char *
ab_param_addr(char *hostname) {

    struct hostent *h;

    h = gethostbyname(hostname);
    if (h == NULL || h->h_addr_list[0] == NULL)
        return "host not found";
    /* one addr is enough */
    ab_addr.s_addr = *(in_addr_t *)(h->h_addr_list[0]);
    
    return NULL;
}

