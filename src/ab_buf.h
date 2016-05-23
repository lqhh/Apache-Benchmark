
#ifndef __AB_BUF_H_INCLUDED__
#define __AB_BUF_H_INCLUDED__


typedef struct {
    char *data;  /* buffer begin */
    char *last;  /* buffer end */
    char *start; /* data begin */
    char *pos;   /* data end */
    int size;
} ab_buf_t;

ab_buf_t *ab_buf_malloc(int size);
void ab_buf_free(ab_buf_t *buf);
ab_buf_t *ab_buf_realloc(ab_buf_t *buf, int size);
ab_buf_t *ab_buf_enlarge(ab_buf_t *buf);
int ab_buf_append(ab_buf_t *buf, char *data, int len);
int ab_buf_remove(ab_buf_t *buf, int n);
int ab_buf_empty(ab_buf_t *buf);
void ab_buf_reset(ab_buf_t *buf);

#endif /* __AB_BUF_H_INCLUDED__ */
