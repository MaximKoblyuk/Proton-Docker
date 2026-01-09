#ifndef PROTON_HTTP_H
#define PROTON_HTTP_H

#include "proton.h"
#include "event.h"

/* HTTP methods */
#define HTTP_GET     0
#define HTTP_POST    1
#define HTTP_HEAD    2
#define HTTP_PUT     3
#define HTTP_DELETE  4

/* HTTP versions */
#define HTTP_VERSION_10  0
#define HTTP_VERSION_11  1

/* HTTP status codes */
#define HTTP_STATUS_OK                  200
#define HTTP_STATUS_BAD_REQUEST         400
#define HTTP_STATUS_NOT_FOUND           404
#define HTTP_STATUS_INTERNAL_ERROR      500
#define HTTP_STATUS_NOT_IMPLEMENTED     501

/* Forward declarations */
typedef struct proton_http_request_s proton_http_request_t;
typedef struct proton_http_response_s proton_http_response_t;
typedef struct proton_http_header_s proton_http_header_t;
typedef struct proton_http_connection_s proton_http_connection_t;

/* HTTP header */
struct proton_http_header_s {
    char *name;
    char *value;
    proton_http_header_t *next;
};

/* HTTP request */
struct proton_http_request_s {
    int method;
    int version;
    char *uri;
    char *query_string;
    proton_http_header_t *headers;
    char *body;
    size_t body_len;
    proton_pool_t *pool;
};

/* HTTP response */
struct proton_http_response_s {
    int status;
    proton_http_header_t *headers;
    proton_buffer_t *body;
    int headers_sent;
};

/* HTTP connection */
struct proton_http_connection_s {
    int fd;
    proton_event_t *event;
    proton_http_request_t *request;
    proton_http_response_t *response;
    proton_buffer_t *read_buf;
    proton_buffer_t *write_buf;
    proton_pool_t *pool;
    int keep_alive;
};

/* HTTP request parsing */
int proton_http_parse_request(proton_buffer_t *buf, proton_http_request_t *req);

/* HTTP response building */
proton_http_response_t* proton_http_response_create(void);
int proton_http_response_set_status(proton_http_response_t *res, int status);
int proton_http_response_add_header(proton_http_response_t *res, const char *name, const char *value);
int proton_http_response_write(proton_http_response_t *res, const char *data, size_t len);
int proton_http_response_send(proton_http_connection_t *conn);
void proton_http_response_destroy(proton_http_response_t *res);

/* HTTP connection handling */
proton_http_connection_t* proton_http_connection_create(int fd);
int proton_http_handle_request(proton_http_connection_t *conn);
void proton_http_connection_close(proton_http_connection_t *conn);

/* HTTP header helpers */
const char* proton_http_get_header(proton_http_request_t *req, const char *name);
const char* proton_http_status_string(int status);

#endif /* PROTON_HTTP_H */
