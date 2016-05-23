
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ab_buf.h>


ab_buf_t *
ab_buf_malloc(int size) {
    
    ab_buf_t *buf;

    buf = (ab_buf_t *)malloc(sizeof(ab_buf_t));
    if (buf == NULL)
        return NULL;

    buf->data = (char *)malloc(size);
    if (buf->data == NULL) {
        free(buf);
        return NULL;
    }
    buf->last = buf->data + size;

    buf->start = buf->data;
    buf->pos = buf->data;
    
    buf->size = size;

    return buf;
}


void
ab_buf_free(ab_buf_t *buf) {

    free(buf->data);
    free(buf);
}


ab_buf_t *
ab_buf_realloc(ab_buf_t *buf, int size) {

    char *tmp;
    int n1, n2;

    /* do not shrink */
    if (size <= buf->size)
        return buf;

    n1 = buf->start - buf->data;
    n2 = buf->pos - buf->data;
    

    tmp = realloc(buf->data, size);
    if (tmp == NULL)
        return NULL;

    buf->data = tmp;
    buf->last = buf->data + size;

    buf->start = buf->data + n1;
    buf->pos = buf->data + n2;
    
    buf->size = size;

    return buf;
}


ab_buf_t *
ab_buf_enlarge(ab_buf_t *buf) {

    int size = buf->size * 2;
    return ab_buf_realloc(buf, size);
}


int
ab_buf_append(ab_buf_t *buf, char *data, int len) {

    int n;

    n = buf->last - buf->pos; /* n bytes available */
    if (n < len) {
        buf = ab_buf_enlarge(buf);
        if (buf == NULL)
            return -1;
    }

    memcpy(buf->pos, data, len);

    buf->pos = buf->pos + len;

    return 0;
}


int 
ab_buf_remove(ab_buf_t *buf, int n) {

    if (buf->start + n > buf->pos)
        return -1;

    buf->start += n;

    if (buf->start == buf->pos) {
        buf->start = buf->data;
        buf->pos = buf->data;
    }

    return 0;
}


int
ab_buf_empty(ab_buf_t *buf) {

    return buf->start == buf->pos ? 1 : 0;
}


void
ab_buf_reset(ab_buf_t *buf) {

    buf->start = buf->data;
    buf->pos = buf->data;
}
