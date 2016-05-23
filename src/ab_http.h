
#ifndef __AB_HTTP_H_INCLUDED__
#define __AB_HTTP_H_INCLUDED__

#include <ab_connection.h>
#include <ab_buf.h>

#define AB_HTTP_REQUEST_BUF 4096
#define AB_HTTP_TEXT_BUF 4096

#define AB_HTTP_PARSE_STATUS_ERROR 1
#define AB_HTTP_PARSE_HEADER_ERROR 2
#define AB_HTTP_PARSE_CHUNK_ERROR 3
#define AB_HTTP_PARSE_IDENTITY_ERROR 4
#define AB_HTTP_PARSE_CONTENT_ERROR 5

#define AB_HTTP_PARSE_STATUS 0
#define AB_HTTP_PARSE_HEADER 1
#define AB_HTTP_PARSE_CHUNK 2
#define AB_HTTP_PARSE_IDENTITY 3
#define AB_HTTP_PARSE_LENGTH 4
#define AB_HTTP_PARSE_DONE 5

#define AB_HTTP_STATUS_LINE_START 0
#define AB_HTTP_STATUS_LINE_H 1
#define AB_HTTP_STATUS_LINE_HT 2
#define AB_HTTP_STATUS_LINE_HTT 3
#define AB_HTTP_STATUS_LINE_HTTP 4
#define AB_HTTP_STATUS_LINE_BACKSLASH 5
#define AB_HTTP_STATUS_LINE_MAJOR 6
#define AB_HTTP_STATUS_LINE_DOT 7
#define AB_HTTP_STATUS_LINE_MINOR 8
#define AB_HTTP_STATUS_LINE_SPACE_BEFORE_STATUS 9
#define AB_HTTP_STATUS_LINE_STATUS 10
#define AB_HTTP_STATUS_LINE_SPACE_AFTER_STATUS 11
#define AB_HTTP_STATUS_LINE_STATUS_TEXT 12
#define AB_HTTP_STATUS_LINE_ALMOST_DONE 13


#define AB_HTTP_HEADER_START 0
#define AB_HTTP_HEADER_NAME 1
#define AB_HTTP_HEADER_SPACE_BEFORE_VALUE 2
#define AB_HTTP_HEADER_VALUE 3
#define AB_HTTP_HEADER_LINE_ALMOST_DONE 4
#define AB_HTTP_HEADER_ALMOST_DONE 5


#define AB_HTTP_CHUNK_START 0
#define AB_HTTP_CHUNK_SIZE 1
#define AB_HTTP_CHUNK_SIZE_ALMOST_DONE 2
#define AB_HTTP_CHUNK_SIZE_DONE 3
#define AB_HTTP_CHUNK_DATA 4
#define AB_HTTP_CHUNK_DATA_ALMOST_DONE 5
#define AB_HTTP_CHUNK_ALMOST_DONE 6

typedef struct {
    int text;
    int last;
} ab_http_text_t;


typedef struct {
    int minor;
    int major;
    int status;
    ab_http_text_t status_text;
} ab_http_status_t;


typedef struct {
    int chunked:1;
    int identity:1;
    int length:1;
    int content_length;
    ab_http_text_t server;
    ab_http_text_t content_type;
} ab_http_header_t;


typedef struct {
    ab_connection_t *c;
    int state;
    int substate;
    int error;
    int pos;
    int status_line_size;
    int header_size;
    int header_name_start;
    int header_name_last;
    int header_value_start;
    int header_value_last;
    int content_size;
    int chunk_size;
    int chunk_start;
    int transfer_start;
    int transfer_size;
    ab_http_status_t status_line;
    ab_http_header_t header;
} ab_http_parse_ctx_t;


void ab_http_init_request();
void ab_http_request(ab_connection_t *c);
void ab_http_response(ab_connection_t *c);

void ab_http_parse(ab_http_parse_ctx_t *ctx);
void ab_http_parse_status_line(ab_http_parse_ctx_t *ctx);
void ab_http_parse_header(ab_http_parse_ctx_t *ctx);
int ab_http_parse_header_line(ab_http_parse_ctx_t *ctx);
void ab_http_parse_chunk(ab_http_parse_ctx_t *ctx);
void ab_http_parse_identity(ab_http_parse_ctx_t *ctx);
void ab_http_parse_length(ab_http_parse_ctx_t *ctx);

void ab_http_text_fill(ab_http_text_t *text, int start, int len);
void ab_http_text_fill_range(ab_http_text_t *text, int start, int end);
void ab_http_text_fill_header_value(ab_http_text_t *text, ab_http_parse_ctx_t *ctx);

int ab_http_header_name_cmp(ab_http_parse_ctx_t *ctx, char *str);
int ab_http_header_value_cmp(ab_http_parse_ctx_t *ctx, char *str);
int ab_http_header_val2int(ab_http_parse_ctx_t *ctx);

void ab_http_debug_text(char *base, char *prefix, ab_http_text_t *text);
void ab_http_debug_str(char *prefix, char *start, char *last);
void ab_http_debug_header_line(ab_http_parse_ctx_t *ctx);
void ab_http_debug_status_line(ab_http_parse_ctx_t *ctx);

#endif /* __AB_HTTP_H_INCLUDED__ */
