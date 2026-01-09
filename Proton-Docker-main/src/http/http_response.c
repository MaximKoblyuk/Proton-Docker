#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "proton.h"
#include "http.h"

const char* proton_http_status_string(int status) {
    switch (status) {
        case 200: return "OK";
        case 400: return "Bad Request";
        case 404: return "Not Found";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        default: return "Unknown";
    }
}

proton_http_response_t* proton_http_response_create(void) {
    proton_http_response_t *res = calloc(1, sizeof(proton_http_response_t));
    if (!res) return NULL;
    
    res->status = HTTP_STATUS_OK;
    res->headers = NULL;
    res->body = proton_buffer_create(4096);
    res->headers_sent = 0;
    
    if (!res->body) {
        free(res);
        return NULL;
    }
    
    return res;
}

int proton_http_response_set_status(proton_http_response_t *res, int status) {
    if (!res) return PROTON_ERROR;
    res->status = status;
    return PROTON_OK;
}

int proton_http_response_add_header(proton_http_response_t *res, const char *name, const char *value) {
    if (!res || !name || !value) return PROTON_ERROR;
    
    proton_http_header_t *header = malloc(sizeof(proton_http_header_t));
    if (!header) return PROTON_ERROR;
    
    /* Use malloc+strcpy instead of strdup for Alpine compatibility */
    header->name = malloc(strlen(name) + 1);
    header->value = malloc(strlen(value) + 1);
    
    if (!header->name || !header->value) {
        free(header->name);
        free(header->value);
        free(header);
        return PROTON_ERROR;
    }
    
    strcpy(header->name, name);
    strcpy(header->value, value);
    
    header->next = res->headers;
    res->headers = header;
    
    return PROTON_OK;
}

int proton_http_response_write(proton_http_response_t *res, const char *data, size_t len) {
    if (!res || !data || len == 0) return PROTON_ERROR;
    return proton_buffer_append(res->body, data, len);
}

int proton_http_response_send(proton_http_connection_t *conn) {
    if (!conn || !conn->response) return PROTON_ERROR;
    
    proton_http_response_t *res = conn->response;
    proton_buffer_t *buf = conn->write_buf;
    
    /* Build status line */
    char status_line[256];
    snprintf(status_line, sizeof(status_line), "HTTP/1.1 %d %s\r\n", 
             res->status, proton_http_status_string(res->status));
    proton_buffer_append(buf, status_line, strlen(status_line));
    
    /* Add Server header */
    proton_buffer_append(buf, "Server: Proton/", 15);
    proton_buffer_append(buf, PROTON_VERSION, strlen(PROTON_VERSION));
    proton_buffer_append(buf, "\r\n", 2);
    
    /* Add Content-Length */
    char content_length[64];
    snprintf(content_length, sizeof(content_length), "Content-Length: %zu\r\n", res->body->len);
    proton_buffer_append(buf, content_length, strlen(content_length));
    
    /* Add custom headers */
    for (proton_http_header_t *h = res->headers; h; h = h->next) {
        proton_buffer_append(buf, h->name, strlen(h->name));
        proton_buffer_append(buf, ": ", 2);
        proton_buffer_append(buf, h->value, strlen(h->value));
        proton_buffer_append(buf, "\r\n", 2);
    }
    
    /* End of headers */
    proton_buffer_append(buf, "\r\n", 2);
    
    /* Add body */
    if (res->body->len > 0) {
        proton_buffer_append(buf, res->body->data, res->body->len);
    }
    
    /* Trigger write */
    extern proton_event_loop_t *event_loop;
    if (event_loop) {
        proton_event_add(event_loop, conn->event, PROTON_EVENT_READ | PROTON_EVENT_WRITE);
    }
    
    return PROTON_OK;
}

void proton_http_response_destroy(proton_http_response_t *res) {
    if (!res) return;
    
    /* Free headers */
    proton_http_header_t *h = res->headers;
    while (h) {
        proton_http_header_t *next = h->next;
        free(h->name);
        free(h->value);
        free(h);
        h = next;
    }
    
    if (res->body) {
        proton_buffer_destroy(res->body);
    }
    
    free(res);
}
