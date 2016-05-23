
#ifndef __AB_STAT_H_INCLUDED__
#define __AB_STAT_H_INCLUDED__

#define AB_STAT_BUF 1024


typedef struct {
    int tv_sec;
    int tv_msec;
} ab_time_t;

typedef struct {
    char server[AB_STAT_BUF];
    char hostname[AB_STAT_BUF];
    short port;
    char document_path[AB_STAT_BUF];
    int document_length;
    int concurrency_level;
    ab_time_t start_time;
    int complete_requests;
    int failed_requests;
    int start_requests;
    int write_errors;
    int total_transferred;
    int html_transferred;
    /* other stat */
    int connect_failed;
} ab_stat_t;


void ab_stat_gettimeofday(ab_time_t *t);
double ab_stat_time_interval();

void ab_stat_start();
void ab_stat_server(char *server, int len);
void ab_stat_document_length(int len);
void ab_stat_request_complete();
void ab_stat_request_failed();
void ab_stat_start_request();
int ab_stat_requests_start();
void ab_stat_write_errors();
void ab_stat_data_transferred(int n);
void ab_stat_html_transferred(int n);

void ab_stat_shipout();
void ab_stat_progress_start_shipout();
void ab_stat_progress_shipout(int intval, int last);

/* other stat */
void ab_stat_connect_fail();

extern ab_stat_t ab_stat;

#endif /* __AB_STAT_H_INCLUDED__ */
