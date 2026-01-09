#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "proton.h"
#include "event.h"
#include "http.h"
#include "module.h"

/* Global event loop reference */
extern proton_event_loop_t *event_loop;

static int http_read_handler(proton_event_t *ev);
static int http_write_handler(proton_event_t *ev);

proton_http_connection_t* proton_http_connection_create(int fd) {
    proton_http_connection_t *conn = calloc(1, sizeof(proton_http_connection_t));
    if (!conn) return NULL;
    
    conn->fd = fd;
    conn->pool = proton_pool_create(4096);
    conn->read_buf = proton_buffer_create(4096);
    conn->write_buf = proton_buffer_create(4096);
    conn->keep_alive = 1;
    
    if (!conn->pool || !conn->read_buf || !conn->write_buf) {
        proton_http_connection_close(conn);
        return NULL;
    }
    
    /* Create request and response */
    conn->request = proton_pool_alloc(conn->pool, sizeof(proton_http_request_t));
    if (!conn->request) {
        proton_http_connection_close(conn);
        return NULL;
    }
    memset(conn->request, 0, sizeof(proton_http_request_t));
    conn->request->pool = conn->pool;
    
    conn->response = proton_http_response_create();
    if (!conn->response) {
        proton_http_connection_close(conn);
        return NULL;
    }
    
    /* Create event */
    conn->event = proton_event_create(fd);
    if (!conn->event) {
        proton_http_connection_close(conn);
        return NULL;
    }
    
    conn->event->data = conn;
    conn->event->read_handler = http_read_handler;
    conn->event->write_handler = http_write_handler;
    
    return conn;
}

void proton_http_connection_close(proton_http_connection_t *conn) {
    if (!conn) return;
    
    if (conn->fd >= 0) {
        close(conn->fd);
    }
    
    if (conn->event) {
        proton_event_destroy(conn->event);
    }
    
    if (conn->response) {
        proton_http_response_destroy(conn->response);
    }
    
    if (conn->read_buf) {
        proton_buffer_destroy(conn->read_buf);
    }
    
    if (conn->write_buf) {
        proton_buffer_destroy(conn->write_buf);
    }
    
    if (conn->pool) {
        proton_pool_destroy(conn->pool);
    }
    
    free(conn);
}

static int http_read_handler(proton_event_t *ev) {
    proton_http_connection_t *conn = (proton_http_connection_t*)ev->data;
    
    /* Read data */
    char buf[4096];
    ssize_t n = read(conn->fd, buf, sizeof(buf));
    
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return PROTON_OK;
        }
        proton_log(LOG_ERROR, "Read error: %s", strerror(errno));
        proton_http_connection_close(conn);
        return PROTON_ERROR;
    }
    
    if (n == 0) {
        /* Connection closed */
        proton_http_connection_close(conn);
        return PROTON_OK;
    }
    
    /* Append to buffer */
    if (proton_buffer_append(conn->read_buf, buf, n) != PROTON_OK) {
        proton_http_connection_close(conn);
        return PROTON_ERROR;
    }
    
    /* Try to parse request */
    int ret = proton_http_parse_request(conn->read_buf, conn->request);
    if (ret == PROTON_AGAIN) {
        /* Need more data */
        return PROTON_OK;
    }
    
    if (ret != PROTON_OK) {
        /* Parse error */
        conn->response->status = HTTP_STATUS_BAD_REQUEST;
        proton_http_response_send(conn);
        return PROTON_OK;
    }
    
    /* Request parsed, handle it */
    proton_http_handle_request(conn);
    
    return PROTON_OK;
}

static int http_write_handler(proton_event_t *ev) {
    proton_http_connection_t *conn = (proton_http_connection_t*)ev->data;
    
    /* Write data from write buffer */
    if (conn->write_buf && conn->write_buf->len > 0) {
        ssize_t n = write(conn->fd, conn->write_buf->data, conn->write_buf->len);
        
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return PROTON_OK;
            }
            proton_log(LOG_ERROR, "Write error: %s", strerror(errno));
            proton_http_connection_close(conn);
            return PROTON_ERROR;
        }
        
        /* Remove written data */
        if ((size_t)n < conn->write_buf->len) {
            memmove(conn->write_buf->data, conn->write_buf->data + n, conn->write_buf->len - n);
            conn->write_buf->len -= n;
            return PROTON_OK;
        }
        
        conn->write_buf->len = 0;
    }
    
    /* All data written */
    if (conn->keep_alive) {
        /* Reset for next request */
        conn->read_buf->len = 0;
        conn->write_buf->len = 0;
        proton_pool_destroy(conn->pool);
        conn->pool = proton_pool_create(4096);
        
        conn->request = proton_pool_alloc(conn->pool, sizeof(proton_http_request_t));
        memset(conn->request, 0, sizeof(proton_http_request_t));
        conn->request->pool = conn->pool;
        
        if (conn->response) {
            proton_http_response_destroy(conn->response);
        }
        conn->response = proton_http_response_create();
    } else {
        /* Close connection */
        proton_http_connection_close(conn);
    }
    
    return PROTON_OK;
}

int proton_http_handle_request(proton_http_connection_t *conn) {
    /* Call module handlers */
    int ret = proton_modules_handle_request(conn);
    
    if (ret == PROTON_MODULE_DECLINED) {
        /* No module handled it */
        conn->response->status = HTTP_STATUS_NOT_FOUND;
        proton_http_response_write(conn->response, "404 Not Found\n", 14);
    }
    
    /* Send response */
    return proton_http_response_send(conn);
}
