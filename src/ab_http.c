
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ab_http.h>
#include <ab_param.h>
#include <ab_stat.h>
#include <ab_log.h>


char ab_http_request_data[AB_HTTP_REQUEST_BUF];
int ab_http_request_data_len;


void
ab_http_init_request() {
    snprintf(ab_http_request_data, AB_HTTP_REQUEST_BUF,
             "%s /%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: Keep-Alive\r\n\r\n", ab_request_method, ab_path, ab_hostname);
    ab_http_request_data[AB_HTTP_REQUEST_BUF-1] = '\0';
    ab_http_request_data_len = strlen(ab_http_request_data);

    ab_log_debug("REQUEST:\n%s", ab_http_request_data);
}


void
ab_http_request(ab_connection_t *c) {

    ab_http_parse_ctx_t *ctx;

    ab_stat_start_request();

    ab_buf_reset(c->rcv_buf);
    ab_buf_reset(c->snd_buf);

    ab_log_debug("start http request");
    
    if (c->data == NULL) {
        ctx = (ab_http_parse_ctx_t *)malloc(sizeof(*ctx));
        if (ctx == NULL) {
            c->close(c);
            return;
        }
        c->data = ctx;
    } else {
        ctx = c->data;
    }
    
    memset(ctx, 0, sizeof(ab_http_parse_ctx_t));

    ctx->state = AB_HTTP_PARSE_STATUS;
    ctx->c = c;
    //ctx->pos = c->rcv_buf->start;
    ctx->pos = 0;
        
    ab_buf_append(c->snd_buf, ab_http_request_data, ab_http_request_data_len);

    c->send(c);
    
    if (c->error) {
        ab_log_error("http request send error");
        c->close(c);
    }
}


void
ab_http_response(ab_connection_t *c) {

    ab_http_parse_ctx_t *ctx;

    ab_log_debug("start http response");
    
    c->recv(c);
    ctx = c->data;
    
    ab_http_parse(ctx);

    if (ctx->error) {
        ab_log_error("parse http response failed error=%d", ctx->error);
        ab_stat_request_failed();
        c->close(c);
        return;
    }
    
    if (ctx->state == AB_HTTP_PARSE_DONE) {
        ab_log_debug("http response parse done");
        ab_stat_request_complete();
        ab_stat.document_length = ctx->content_size;
        /*
        ab_stat_data_transferred(ctx->status_line_size + ctx->header_size
                                 + ctx->content_size);
        */
        ab_stat_data_transferred(ctx->transfer_size);
        ab_stat_html_transferred(ctx->content_size);

        if (c->closed) {
            /* server actively close connection to protect from attack */
            return;
        }
        if (ab_stat_requests_start() < ab_requests) {
            ab_http_request(c);
        } else {
            /* test task finished */
            c->closed = 1;
        }
    }
    
    if (c->closed) {
        if (ctx->state != AB_HTTP_PARSE_DONE) {
            ab_log_error("bad http response");
            ab_stat_request_failed();
        }
        c->close(c);
    }
}


void
ab_http_parse(ab_http_parse_ctx_t *ctx) {

    ab_connection_t *c = ctx->c;
    ab_buf_t *buf = c->rcv_buf;
    
    while(ctx->pos + buf->start < buf->pos) {
        switch(ctx->state) {
        case AB_HTTP_PARSE_STATUS:
            ab_http_parse_status_line(ctx); break;
        case AB_HTTP_PARSE_HEADER:
            ab_http_parse_header(ctx); break;
        case AB_HTTP_PARSE_CHUNK:
            ab_http_parse_chunk(ctx); break;
        case AB_HTTP_PARSE_IDENTITY:
            ab_http_parse_identity(ctx); break;
        case AB_HTTP_PARSE_LENGTH:
            ab_http_parse_length(ctx); break;
        case AB_HTTP_PARSE_DONE:
            break;
        default:
            ab_log_error("unknown http parse state %d", ctx->state);
            exit(0);
        }

        if (ctx->state == AB_HTTP_PARSE_LENGTH) {
            /* we only need to load all the data */
            break;
        }

        if (ctx->error) {
            return;
        }
    }
}


void
ab_http_parse_status_line(ab_http_parse_ctx_t *ctx) {

    ab_connection_t *c = ctx->c;
    ab_buf_t *buf = c->rcv_buf;
    char *p;

    ab_log_debug("parse http response status line");
    
    for (p = ctx->pos + buf->start; p < buf->pos; p++) {
        switch(ctx->substate) {
        case AB_HTTP_STATUS_LINE_START:
            if (*p != 'H') {
                ab_log_error("status line start error");
                goto failed;
            }
            ctx->transfer_start = p - buf->start;
            ctx->substate = AB_HTTP_STATUS_LINE_H;
            break;
            
        case AB_HTTP_STATUS_LINE_H:
            if (*p != 'T') {
                ab_log_error("status line H error");
                goto failed;
            }
                
            ctx->substate = AB_HTTP_STATUS_LINE_HT;
            break;
            
        case AB_HTTP_STATUS_LINE_HT:
            if (*p != 'T') {
                ab_log_error("status line HT error");
                goto failed;
            }
                
            ctx->substate = AB_HTTP_STATUS_LINE_HTT;
            break;
            
        case AB_HTTP_STATUS_LINE_HTT:
            if (*p != 'P') {
                ab_log_error("status line HTT error");
                goto failed;
            }
                
            ctx->substate = AB_HTTP_STATUS_LINE_HTTP;
            break;
            
        case AB_HTTP_STATUS_LINE_HTTP:
            if (*p != '/') {
                ab_log_error("status line HTTP error");
                goto failed;
            }
            ctx->substate = AB_HTTP_STATUS_LINE_BACKSLASH;
            break;

        case AB_HTTP_STATUS_LINE_BACKSLASH:
            if (*p < '1' || *p > '9') {
                ab_log_error("status line backslash error");
                goto failed;
            }
            ctx->status_line.major = *p - '0';
            ctx->substate = AB_HTTP_STATUS_LINE_MAJOR;
            break;

        case AB_HTTP_STATUS_LINE_MAJOR:
            if (*p != '.') {
                ab_log_error("status line major error");
                goto failed;
            }
                
            ctx->substate = AB_HTTP_STATUS_LINE_DOT;
            break;

        case AB_HTTP_STATUS_LINE_DOT:
            if (*p < '0' || *p > '9') {
                ab_log_error("status line dot error");
                goto failed;
            }

            ctx->substate = AB_HTTP_STATUS_LINE_MINOR;
            ctx->status_line.minor = *p - '0';
            break;

        case AB_HTTP_STATUS_LINE_MINOR:
            if (*p != ' ') {
                ab_log_error("status line minor error");
                goto failed;
            }
            ctx->substate = AB_HTTP_STATUS_LINE_SPACE_BEFORE_STATUS;
            break;

        case AB_HTTP_STATUS_LINE_SPACE_BEFORE_STATUS:
            if (*p < '0' || *p > '9') {
                ab_log_error("status line space before status error");
                goto failed;
            }
            ctx->status_line.status = ctx->status_line.status * 10 + *p - '0';
            ctx->substate = AB_HTTP_STATUS_LINE_STATUS;
            break;

        case AB_HTTP_STATUS_LINE_STATUS:
            if (*p == ' ') {
                ctx->substate = AB_HTTP_STATUS_LINE_SPACE_AFTER_STATUS;
                break;
            }
            if (*p < '0' || *p > '9') {
                ab_log_error("status line status error");
                goto failed;
            }
            ctx->status_line.status = ctx->status_line.status * 10 + *p - '0';
            break;

        case AB_HTTP_STATUS_LINE_SPACE_AFTER_STATUS:
            ctx->status_line.status_text.text = p - buf->start;
            ctx->substate = AB_HTTP_STATUS_LINE_STATUS_TEXT;
            break;

        case AB_HTTP_STATUS_LINE_STATUS_TEXT:
            if (*p == '\r') {
                ctx->status_line.status_text.last = p - buf->start;
                ctx->substate = AB_HTTP_STATUS_LINE_ALMOST_DONE;
                break;
            }
            break;
            
        case AB_HTTP_STATUS_LINE_ALMOST_DONE:
            if (*p != '\n') {
                ab_log_error("status almost done error");
                goto failed;
            }

            ab_http_debug_status_line(ctx);
            
            ctx->state = AB_HTTP_PARSE_HEADER;
            ctx->substate = AB_HTTP_HEADER_START;
            ctx->status_line_size++;
            goto status_line_done;
        }

        ctx->status_line_size++;
    }
    
    ctx->pos = p - buf->start;
    return;

status_line_done:
    ctx->pos = p - buf->start + 1;
    return;

failed:
    ab_log_debug("parse status line ctx '%c'", *p);
    ctx->error = AB_HTTP_PARSE_STATUS_ERROR;
}


void
ab_http_parse_header(ab_http_parse_ctx_t *ctx) {

    ab_connection_t *c = ctx->c;
    ab_buf_t *buf = c->rcv_buf;
    char *p;

    ab_log_debug("start parse http response header");
    
    for (p = ctx->pos + buf->start; p < buf->pos; p++) {
        
        switch(ctx->substate) {
            
        case AB_HTTP_HEADER_START:
            if (*p == '\r') {
                ctx->substate = AB_HTTP_HEADER_ALMOST_DONE;
                break;
            }
            ctx->header_name_start = p - buf->start;
            ctx->substate = AB_HTTP_HEADER_NAME;
            break;

        case AB_HTTP_HEADER_NAME:
            if (*p == ':') {
                ctx->header_name_last = p - buf->start;
                ctx->substate = AB_HTTP_HEADER_SPACE_BEFORE_VALUE;
            }
            break;

        case AB_HTTP_HEADER_SPACE_BEFORE_VALUE:
            if (*p != ' ') {
                ctx->header_value_start = p - buf->start;
                ctx->substate = AB_HTTP_HEADER_VALUE;
            }
            break;

        case AB_HTTP_HEADER_VALUE:
            if (*p == '\r') {
                ctx->header_value_last = p - buf->start;
                ctx->substate = AB_HTTP_HEADER_LINE_ALMOST_DONE;
                if (ab_http_parse_header_line(ctx) < 0)
                    goto failed;
            }
            break;

        case AB_HTTP_HEADER_LINE_ALMOST_DONE:
            if (*p != '\n')
                goto failed;
            ctx->substate = AB_HTTP_HEADER_START;
            break;
            
        case AB_HTTP_HEADER_ALMOST_DONE:
            if (*p != '\n')
                goto failed;

            ab_log_debug("http response header parse done");
            if (ctx->header.chunked) {
                ctx->state = AB_HTTP_PARSE_CHUNK;
                ctx->substate = AB_HTTP_CHUNK_START;
            } else if (ctx->header.identity)
                ctx->state = AB_HTTP_PARSE_IDENTITY;
            else if (ctx->header.length)
                ctx->state = AB_HTTP_PARSE_LENGTH;
            else
                goto failed;
            
            goto header_done;
        }

        ctx->header_size++;
    }

    ctx->pos = p - buf->start;
    return;

header_done:
    ctx->pos = p - buf->start + 1;
    return;

failed:
    ctx->error = AB_HTTP_PARSE_HEADER_ERROR;
}


int
ab_http_parse_header_line(ab_http_parse_ctx_t *ctx) {

    ab_connection_t *c = ctx->c;
    ab_buf_t *buf = c->rcv_buf;

    if (ctx->header_name_start == ctx->header_name_last ||
        ctx->header_value_start == ctx->header_name_last)
        return -1;

    ab_http_debug_header_line(ctx);

    if (ab_http_header_name_cmp(ctx, "Server") == 0) {
        ab_http_text_fill_header_value(&ctx->header.server, ctx);
        ab_stat_server(buf->start + ctx->header_value_start,
                       ctx->header_value_last - ctx->header_value_start);
        return 0;
    }

    if (ab_http_header_name_cmp(ctx, "Content-Type") == 0) {
        ab_http_text_fill_header_value(&ctx->header.content_type, ctx);
        return 0;
    }

    if (ab_http_header_name_cmp(ctx, "Content-Length") == 0) {
        ctx->header.length = 1;
        ctx->header.content_length = ab_http_header_val2int(ctx);
        if (ctx->header.content_length < 0)
            return -1;
        return 0;
    }

    if (ab_http_header_name_cmp(ctx, "Transfer-Encoding") == 0) {
        if (ab_http_header_value_cmp(ctx, "chunked") == 0) {
            ctx->header.chunked = 1;
        } else if (ab_http_header_value_cmp(ctx, "identity") == 0) {
            ctx->header.identity = 1;
        } else {
            return -1;
        }
        return 0;
    }

    return 0;
}


void
ab_http_parse_chunk(ab_http_parse_ctx_t *ctx) {

    ab_connection_t *c = ctx->c;
    ab_buf_t *buf = c->rcv_buf;
    char *p;

    for (p = ctx->pos + buf->start; p < buf->pos; p++) {

        switch(ctx->substate) {

        case AB_HTTP_CHUNK_START:
            if (*p == '\r') {
                ctx->substate = AB_HTTP_CHUNK_ALMOST_DONE;
                break;
            }
            if (*p >= '0' && *p <= '9') {
                ctx->chunk_size = *p - '0';
                
            } else if ( *p >= 'a' && *p <= 'f') {
                ctx->chunk_size = *p - 'a' + 10;
            } else {
                ab_log_debug("chunk substate start error");
                goto failed;
            }
            ctx->substate = AB_HTTP_CHUNK_SIZE;
            break;

        case AB_HTTP_CHUNK_SIZE:
            if (*p == '\r') {
                ab_log_debug("chunck size %d", ctx->chunk_size);
                ctx->substate = AB_HTTP_CHUNK_SIZE_ALMOST_DONE;
                break;
            }
            if (*p >= '0' && *p <= '9') {
                ctx->chunk_size = ctx->chunk_size * 16 + *p - '0';
            } else if (*p >= 'a' && *p <= 'f') {
                ctx->chunk_size = ctx->chunk_size * 16 + *p - 'a' + 10;
            } else {
                ab_log_debug("chunk substate chunk size error");
                goto failed;
            }
            break;

        case AB_HTTP_CHUNK_ALMOST_DONE:
            if (*p != '\n') {
                ab_log_debug("chunk substate chunk almost done error");
                break;
            }
            ctx->state = AB_HTTP_PARSE_DONE;
            goto chunk_done;

        case AB_HTTP_CHUNK_SIZE_ALMOST_DONE:
            if (*p != '\n') {
                ab_log_debug("chunk substate chunck size almost done error");
                goto failed;
            }
                
            if (ctx->chunk_size) {
                ctx->content_size += ctx->chunk_size;
                ctx->substate = AB_HTTP_CHUNK_SIZE_DONE;
            } else {
                ctx->substate = AB_HTTP_CHUNK_START;
            }
            break;

        case AB_HTTP_CHUNK_SIZE_DONE:
            ctx->chunk_start = p - buf->start;
            ctx->chunk_size--;
            ctx->substate = AB_HTTP_CHUNK_DATA;
            break;
            
        case AB_HTTP_CHUNK_DATA:
            if (ctx->chunk_size < 0) {
                ab_log_debug("chunk substate chunk data error ctx '%c'", *p);
                goto failed;
            }
            
            if (ctx->chunk_size == 0 && *p == '\r') {
                ab_http_debug_str("chuncked: ", buf->start + ctx->chunk_start, p);
                ctx->substate = AB_HTTP_CHUNK_DATA_ALMOST_DONE;
                break;
            }
            ctx->chunk_size--;
            break;

        case AB_HTTP_CHUNK_DATA_ALMOST_DONE:
            if (*p != '\n') {
                ab_log_debug("chunk substate chunk data almost done error: '%c'", *p);
                goto failed;
            }
            ctx->substate = AB_HTTP_CHUNK_START;
            break;
        }
    }

    ctx->pos = p - buf->start;
    return;

chunk_done:
    if (p+1 != buf->pos)
        goto failed;
    ctx->pos = p - buf->start + 1;
    ctx->transfer_size = ctx->pos - ctx->transfer_start;
    return;

failed:
    ctx->error = AB_HTTP_PARSE_CHUNK_ERROR;
}


void
ab_http_parse_identity(ab_http_parse_ctx_t *ctx) {

    ab_log_crit("Transfer-Encoding: identity is not supported");
    ctx->error = AB_HTTP_PARSE_HEADER_ERROR;
}


void
ab_http_parse_length(ab_http_parse_ctx_t *ctx) {

    ab_connection_t *c = ctx->c;
    ab_buf_t *buf = c->rcv_buf;
    int len;

    len = ctx->header.content_length;

    if (buf->start + ctx->pos + len == buf->pos) {
        ctx->state = AB_HTTP_PARSE_DONE;
        ctx->content_size = len;
        ab_http_debug_str("content: ", buf->start + ctx->pos, buf->pos);
        ctx->pos += len;
        ctx->transfer_size = ctx->pos - ctx->transfer_start;
    } else if (buf->start + ctx->pos + len < buf->pos) {
        ctx->error = AB_HTTP_PARSE_CONTENT_ERROR;
    }
}


void
ab_http_text_fill(ab_http_text_t *text, int start, int len) {

    ab_http_text_fill_range(text, start, start + len);
}


void
ab_http_text_fill_range(ab_http_text_t *text, int start, int end) {

    text->text = start;
    text->last = end;
}


void
ab_http_text_fill_header_value(ab_http_text_t *text, ab_http_parse_ctx_t *ctx) {

    ab_http_text_fill_range(text, ctx->header_value_start,
                            ctx->header_value_last);
}


int
ab_http_header_name_cmp(ab_http_parse_ctx_t *ctx, char *str) {

    ab_connection_t *c = ctx->c;
    ab_buf_t *buf = c->rcv_buf;
    
    return strncmp(str, ctx->header_name_start + buf->start,
                   ctx->header_name_last - ctx->header_name_start);
}


int
ab_http_header_value_cmp(ab_http_parse_ctx_t *ctx, char *str) {

    ab_connection_t *c = ctx->c;
    ab_buf_t *buf = c->rcv_buf;
    
    return strncmp(str, ctx->header_value_start + buf->start,
                   ctx->header_value_last - ctx->header_value_start);
}


int
ab_http_header_val2int(ab_http_parse_ctx_t *ctx) {

    ab_connection_t *c = ctx->c;
    ab_buf_t *buf = c->rcv_buf;
    int val;
    char *p;

    for (val = 0, p = ctx->header_value_start + buf->start;
         p < ctx->header_value_last + buf->start; p++) {
        
        if (*p < '0' || *p > '9')
            return -1;
        val = val * 10 + *p - '0';
    }

    return val;
}


void
ab_http_debug_status_line(ab_http_parse_ctx_t *ctx) {

    ab_http_status_t *status = &ctx->status_line;
    ab_http_text_t *text = &status->status_text;
    ab_connection_t *c = ctx->c;
    ab_buf_t *buf = c->rcv_buf;
    
    static char line[AB_HTTP_TEXT_BUF];
    static char status_text[AB_HTTP_TEXT_BUF];

    if (ab_verbose == AB_LOG_VERBOSE_DEBUG) {
        strncpy(status_text, buf->start + text->text, text->last - text->text);
        status_text[AB_HTTP_TEXT_BUF-1] = '\0';

        snprintf(line, AB_HTTP_TEXT_BUF, "status: HTTP/%d.%d %d %s",
                 status->major, status->minor, status->status, status_text);

        line[AB_HTTP_TEXT_BUF-1] = '\0';

        ab_log_debug("%s", line);
    }
}


void
ab_http_debug_text(char *base, char *prefix, ab_http_text_t *text) {

    static char buf[AB_HTTP_TEXT_BUF];
    int len;

    if (ab_verbose == AB_LOG_VERBOSE_DEBUG) {
        len = text->last - text->text;
        strncpy(buf, base + text->text, len);
        if (len >= AB_HTTP_TEXT_BUF)
            len = AB_HTTP_TEXT_BUF-1;
        buf[len] = '\0';
        ab_log_debug("%s%s", prefix, buf);
    }
}


void
ab_http_debug_str(char *prefix, char *start, char *last) {

    static char buf[AB_HTTP_TEXT_BUF];
    int len = last - start;

    if (ab_verbose == AB_LOG_VERBOSE_DEBUG) {
        if (len >= AB_HTTP_TEXT_BUF)
            len = AB_HTTP_TEXT_BUF -1;
        strncpy(buf, start, len);
        buf[len] = '\0';
        ab_log_debug("%s%s", prefix, buf);
    }
}


void
ab_http_debug_header_line(ab_http_parse_ctx_t *ctx) {

    ab_connection_t *c = ctx->c;
    ab_buf_t *buf = c->rcv_buf;
    static char name[AB_HTTP_TEXT_BUF];
    static char value[AB_HTTP_TEXT_BUF];
    int len;

    if (ab_verbose == AB_LOG_VERBOSE_DEBUG) {
        len = ctx->header_name_last - ctx->header_name_start;
        if (len >= AB_HTTP_TEXT_BUF)
            len = AB_HTTP_TEXT_BUF-1;
        strncpy(name, ctx->header_name_start + buf->start, len);
        name[len] = '\0';

        len = ctx->header_value_last - ctx->header_value_start;
        strncpy(value, ctx->header_value_start + buf->start, len);
        if (len >= AB_HTTP_TEXT_BUF)
            len = AB_HTTP_TEXT_BUF-1;
        value[len] = '\0';
        ab_log_debug("header: %s=%s", name, value);
    } 
}
