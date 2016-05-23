
#ifndef __AB_CONNECTION_H_INCLUDED__
#define __AB_CONNECTION_H_INCLUDED__

#include <ab_buf.h>

/* It's enough to receive http header */
#define AB_CONNECTION_BUF 1024

#define AB_CONNECTION_CONNECTING 0
#define AB_CONNECTION_CONNECTED 1
#define AB_CONNECTION_CLOSED 2

struct ab_connection_s;
typedef struct ab_connection_s ab_connection_t;
typedef void (ab_connection_handler_pt)(ab_connection_t *);

struct ab_connection_s {
    int fd;
    int idx;
    int state;
    unsigned int event_write:1;
    unsigned int event_read:1;
    unsigned int event_error:1;
    unsigned int closed:1;
    unsigned int error:1;
    ab_buf_t *rcv_buf;
    ab_buf_t *snd_buf;
    ab_connection_handler_pt *send;
    ab_connection_handler_pt *recv;
    ab_connection_handler_pt *close;
    void *data;
};


int ab_connection_init();
ab_connection_t *ab_connection_create(int idx);
void ab_connection_handler(ab_connection_t *c);

void ab_connection_send(ab_connection_t *c);
void ab_connection_recv(ab_connection_t *c);
void ab_connection_close(ab_connection_t *c);

extern int ab_connection_closed;

#endif /* __AB_CONNECTION_H_INCLUDED__ */
