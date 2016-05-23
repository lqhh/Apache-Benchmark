
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <ab_param.h>
#include <ab_connection.h>
#include <ab_http.h>
#include <ab_log.h>
#include <ab_stat.h>


ab_connection_t *ab_connections;

int ab_connection_closed = 0;


int
ab_connection_init() {
    
    ab_connections = (ab_connection_t *)malloc(
        sizeof(ab_connection_t) * ab_concurrency);
    if (ab_connections == NULL)
        return -1;

    return 0;
}


ab_connection_t *
ab_connection_create(int idx) {

    int flags;
    int rc;
    struct sockaddr_in addr;
    ab_connection_t *c = &ab_connections[idx];

    c->idx = idx;
    
    c->fd = socket(AF_INET, SOCK_STREAM, 0);
    if (c->fd < 0) {
        printf("socket() failed\n");
        return NULL;
    }

    if ((flags = fcntl(c->fd, F_GETFL, 0)) < 0) {
        printf("fcntl(GET) failed\n");
        goto failed;
    }
    if (fcntl(c->fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        printf("fcntl(SET) failed\n");
        goto failed;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(ab_port);
    addr.sin_addr.s_addr = ab_addr.s_addr;
    rc = connect(c->fd, (void *)&addr, sizeof(addr));
    if (rc < 0) {
        if (errno != EINPROGRESS) {
            printf("nonblock connect() failed: %s", strerror(errno));
            goto failed;
        }
        c->state = AB_CONNECTION_CONNECTING;
    } else
        c->state = AB_CONNECTION_CONNECTED;

    c->event_read = 0;
    c->event_write = 0;
    c->event_error = 0;
    c->closed = 0;
    c->error = 0;
        
    c->rcv_buf = ab_buf_malloc(AB_CONNECTION_BUF);
    if (c->rcv_buf == NULL)
        goto failed;
    c->snd_buf = ab_buf_malloc(AB_CONNECTION_BUF);
    if (c->snd_buf == NULL)
        goto failed;

    c->send = ab_connection_send;
    c->recv = ab_connection_recv;
    c->close = ab_connection_close;
    c->data = NULL;

    return c;

failed:
    if (c->fd != -1) {
        close(c->fd);
        c->fd = -1;
    }
    if (c->rcv_buf)
        ab_buf_free(c->rcv_buf);
    ab_stat_connect_fail();
    return NULL;
}


void
ab_connection_close(ab_connection_t *c) {

    if (c->fd == -1)
        return;

    /* automaticly removed from epoll */
    close(c->fd);
    ab_buf_free(c->rcv_buf);
    ab_buf_free(c->snd_buf);
    if (c->data)
        free(c->data);
    
    ab_connection_closed++;
}


void
ab_connection_handler(ab_connection_t *c) {
    
    if (c->event_error) {
        ab_connection_close(c);
        return;
    }

    if (c->event_write) {
        if (c->state == AB_CONNECTION_CONNECTING) {
            c->state = AB_CONNECTION_CONNECTED;
            ab_http_request(c);
        } else if (!ab_buf_empty(c->snd_buf)) {
            ab_connection_send(c);
            if (c->error)
                ab_connection_close(c);
        }
    }

    if (c->event_read) {
        ab_http_response(c);
    }
}


void
ab_connection_send(ab_connection_t *c) {

    ab_buf_t *buf;
    int rc, n;

    buf = c->snd_buf;
    while (1) {
        n = buf->pos - buf->start;
        if (n <= 0)
            break;
        
        rc = send(c->fd, buf->start, n, 0);
        if (rc > 0) {
            ab_buf_remove(buf, rc);
            if (rc < n) {
                /* tcp send buffer is full */
                break;
            }
        }

        if (rc == 0) {
            printf("send() return zero\n");
            break;
        }
        if (rc < 0) {
            if (errno != EAGAIN && rc != EINTR) {
                ab_stat_write_errors();
                printf("send(%d) failed\n", c->fd);
                c->error = 1;
            }
            break;
        }
    }
}


void
ab_connection_recv(ab_connection_t *c) {

    ab_buf_t *buf;
    int rc, n;

    buf = c->rcv_buf;
    while (1) {
        n = buf->last - buf->pos;
        if (n <= 0) {
            buf = ab_buf_enlarge(buf);
            if (buf == NULL) {
                ab_log_error("ab_buf_enlarge() failed %d", c->idx);
                c->error = 1;
                break;
            }
            /* recalucate available space */
            n = buf->last - buf->pos;
        }

        ab_log_debug("recv buf %d", n);
        rc = recv(c->fd, buf->pos, n, 0);
        
        if (rc == 0) {
            ab_log_info("server close connection %d", c->idx);
            c->closed = 1;
            break;
        }
        
        if (rc < 0) {
            if (errno != EAGAIN && errno != EINTR) {
                ab_log_error("recv(%d) failed", c->fd);
                c->error = 1;
            }
            break;
        }

        ab_log_debug("recv %d bytes", rc);

        /* rc > 0 */
        buf->pos += rc;
    }
}
