
#include <stdio.h>
#include <time.h>
#include <ab_test.h>
#include <ab_param.h>
#include <ab_connection.h>
#include <ab_event.h>
#include <ab_http.h>
#include <ab_stat.h>
#include <ab_log.h>


static void ab_test_connection(int idx);
static void ab_test_loop();


void
ab_test_start() {

    int i;

    ab_http_init_request();
    ab_stat_start();

    if (ab_connection_init() < 0) {
        printf("ab_connect_init() failed\n");
        return;
    }

    if (ab_event_init() < 0) {
        printf("ab_event_init() failed\n");
        return;
    }
    
    for (i = 0; i < ab_concurrency; i++)
        ab_test_connection(i);
    ab_test_loop();
}


void
ab_test_result() {

    ab_stat_shipout();
}


static void
ab_test_connection(int idx) {

    ab_connection_t *c;

    if ((c = ab_connection_create(idx)) == NULL) {
        printf("ab_connection_create(%d) failed\n", idx);
        goto failed;
    }

    if (ab_event_add_connection(c) < 0) {
        printf("ab_event_add_connection(%d) failed\n", idx);
        goto failed;
    }

    return;

failed:
    if (c)
        ab_connection_close(c);
}


static void
ab_test_loop() {

    time_t t;
    int timeout;
    int rc;

    timeout = ab_time_limit + time(&t);
    
    ab_stat_progress_start_shipout();
    
    while(time(&t) < timeout) {
        
        if (ab_connection_closed >= ab_concurrency) {
            ab_log_debug("all connection was closed");
            break;
        }
        
        rc = ab_event_poll(500);
        if (rc == -1)
            break;
        
        ab_stat_progress_shipout(2, 0);
    }
    ab_stat_progress_shipout(2, 1);
}
