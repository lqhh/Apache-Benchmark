
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <ab_stat.h>
#include <ab_param.h>
#include <ab_log.h>

ab_stat_t ab_stat;


void
ab_stat_gettimeofday(ab_time_t *t) {

    struct timeval tv;

    if (gettimeofday(&tv, NULL) < 0) {
        printf("gettimeofday() failed\n");
        exit(0);
    }

    t->tv_sec = tv.tv_sec;
    t->tv_msec = tv.tv_usec / 1000;
}


double
ab_stat_time_interval() {

    double interval;
    ab_time_t tv;
    
    ab_stat_gettimeofday(&tv);
    interval = tv.tv_sec - ab_stat.start_time.tv_sec;
    interval += (tv.tv_msec - ab_stat.start_time.tv_msec) / 1000.0;

    return interval;
}


void
ab_stat_start() {

    memset(&ab_stat, 0, sizeof(ab_stat));
    
    strcpy(ab_stat.hostname, ab_hostname);
    ab_stat.port = ab_port;
    if (ab_path[0] != '/') {
        ab_stat.document_path[0] = '/';
        strcpy(ab_stat.document_path + 1, ab_path);
    } else {
        strcpy(ab_stat.document_path, ab_path);
    }
    ab_stat.concurrency_level = ab_concurrency;
    ab_stat_gettimeofday(&ab_stat.start_time);
}


inline void
ab_stat_server(char *server, int len) {

    if (ab_stat.server[0] == '\0')
        strncpy(ab_stat.server, server, len);
}


inline void
ab_stat_document_length(int len) {

    ab_stat.document_length = len;
}


inline void
ab_stat_request_complete() {
    
    ab_stat.complete_requests++;
}


inline void
ab_stat_request_failed() {

    ab_stat.failed_requests++;
}


inline void
ab_stat_write_errors() {

    ab_stat.write_errors++;
}


inline void
ab_stat_data_transferred(int n) {

    ab_stat.total_transferred += n;
}


inline void
ab_stat_html_transferred(int n) {

    ab_stat.html_transferred += n;
}


inline void
ab_stat_start_request() {

    ab_stat.start_requests++;
}


inline int
ab_stat_requests_start() {
    return ab_stat.start_requests;
}


void
ab_stat_shipout() {

    double interval;

    interval = ab_stat_time_interval();

    printf("\n");
    
    printf("Server Software:\t%s\n", ab_stat.server);
    printf("Server Hostname:\t%s\n", ab_stat.hostname);
    printf("Server Port:\t%hu\n", ab_stat.port);
    printf("\n");
    
    printf("Document Path:\t%s\n", ab_stat.document_path);
    printf("Document Length:\t%d bytes\n", ab_stat.document_length);
    printf("\n");
    
    printf("Concurrency Level:\t%d\n", ab_stat.concurrency_level);
    printf("Time taken for tests:\t%.3f seconds\n", interval);
    printf("Complete requests:\t%d\n", ab_stat.complete_requests);
    printf("Failed requests:\t%d\n", ab_stat.failed_requests);
    printf("Write errors:\t%d\n", ab_stat.write_errors);
    printf("Total transferred:\t%d bytes\n", ab_stat.total_transferred);
    printf("HTML transferred:\t%d bytes\n", ab_stat.html_transferred);
    printf("Requests per second:\t%.3f[#/sec] (mean)\n",
           ab_stat.complete_requests / interval);
    printf("Time per request:\t%.3f [ms] (mean)\n",
           interval * 1000 / (ab_stat.complete_requests / ab_stat.concurrency_level));
    printf("Time per request:\t%.3f [ms] (mean, across all concurrent requests)\n",
           interval * 1000 / ab_stat.complete_requests);
    printf("Transfer rate:\t%.3f [Kbytes/sec] received\n",
           ab_stat.total_transferred / interval / 1024);

    printf("\n");
    printf("Connection failed:\t%d\n", ab_stat.connect_failed);
}


inline void
ab_stat_progress_start_shipout() {

    ab_log("Benchmarking localhost (be patient)");
}


inline void
ab_stat_progress_shipout(int intval, int last) {

    static int prev = 0;
    int now;

    now = time(NULL);
    if (now - prev >= intval || last) {
        ab_log("Completed %d requests", ab_stat.complete_requests);
        prev = now;
    }
}

/* other stat */

inline void
ab_stat_connect_fail() {

    ab_stat.connect_failed++;
}
