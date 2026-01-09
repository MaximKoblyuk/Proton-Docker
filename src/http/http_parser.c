#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "proton.h"
#include "http.h"

/* Parse HTTP method */
static int parse_method(const char *line, proton_http_request_t *req) {
    if (strncmp(line, "GET ", 4) == 0) {
        req->method = HTTP_GET;
        return 4;
    } else if (strncmp(line, "POST ", 5) == 0) {
        req->method = HTTP_POST;
        return 5;
    } else if (strncmp(line, "HEAD ", 5) == 0) {
        req->method = HTTP_HEAD;
        return 5;
    } else if (strncmp(line, "PUT ", 4) == 0) {
        req->method = HTTP_PUT;
        return 4;
    } else if (strncmp(line, "DELETE ", 7) == 0) {
        req->method = HTTP_DELETE;
        return 7;
    }
    return -1;
}

/* Parse URI and query string */
static int parse_uri(const char *line, int offset, proton_http_request_t *req) {
    const char *start = line + offset;
    const char *end = strchr(start, ' ');
    if (!end) return -1;
    
    int uri_len = end - start;
    req->uri = proton_pool_alloc(req->pool, uri_len + 1);
    if (!req->uri) return -1;
    
    memcpy(req->uri, start, uri_len);
    req->uri[uri_len] = '\0';
    
    /* Check for query string */
    char *query = strchr(req->uri, '?');
    if (query) {
        *query = '\0';
        req->query_string = query + 1;
    }
    
    return end - line + 1;
}

/* Parse HTTP version */
static int parse_version(const char *line, int offset, proton_http_request_t *req) {
    const char *ver = line + offset;
    
    if (strncmp(ver, "HTTP/1.1", 8) == 0) {
        req->version = HTTP_VERSION_11;
        return 0;
    } else if (strncmp(ver, "HTTP/1.0", 8) == 0) {
        req->version = HTTP_VERSION_10;
        return 0;
    }
    
    return -1;
}

/* Parse request line: GET /path HTTP/1.1 */
static int parse_request_line(const char *line, proton_http_request_t *req) {
    int offset = parse_method(line, req);
    if (offset < 0) return PROTON_ERROR;
    
    offset = parse_uri(line, offset, req);
    if (offset < 0) return PROTON_ERROR;
    
    if (parse_version(line, offset, req) < 0) return PROTON_ERROR;
    
    return PROTON_OK;
}

/* Parse header line: Name: Value */
static int parse_header(const char *line, proton_http_request_t *req) {
    const char *colon = strchr(line, ':');
    if (!colon) return PROTON_ERROR;
    
    /* Allocate header */
    proton_http_header_t *header = proton_pool_alloc(req->pool, sizeof(proton_http_header_t));
    if (!header) return PROTON_ERROR;
    
    /* Parse name */
    int name_len = colon - line;
    header->name = proton_pool_alloc(req->pool, name_len + 1);
    if (!header->name) return PROTON_ERROR;
    memcpy(header->name, line, name_len);
    header->name[name_len] = '\0';
    
    /* Parse value (skip leading whitespace) */
    const char *value_start = colon + 1;
    while (*value_start == ' ' || *value_start == '\t') value_start++;
    
    int value_len = strlen(value_start);
    /* Trim trailing whitespace */
    while (value_len > 0 && (value_start[value_len-1] == ' ' || value_start[value_len-1] == '\t')) {
        value_len--;
    }
    
    header->value = proton_pool_alloc(req->pool, value_len + 1);
    if (!header->value) return PROTON_ERROR;
    memcpy(header->value, value_start, value_len);
    header->value[value_len] = '\0';
    
    /* Add to list */
    header->next = req->headers;
    req->headers = header;
    
    return PROTON_OK;
}

int proton_http_parse_request(proton_buffer_t *buf, proton_http_request_t *req) {
    if (!buf || !req || !buf->data) return PROTON_ERROR;
    
    /* Find end of headers (\r\n\r\n) */
    char *headers_end = strstr(buf->data, "\r\n\r\n");
    if (!headers_end) return PROTON_AGAIN; /* Need more data */
    
    /* Create pool for request */
    if (!req->pool) {
        req->pool = proton_pool_create(4096);
        if (!req->pool) return PROTON_ERROR;
    }
    
    /* Parse line by line */
    char *line = buf->data;
    char *line_end;
    int first_line = 1;
    
    while (line < headers_end) {
        line_end = strstr(line, "\r\n");
        if (!line_end) break;
        
        /* Null-terminate line */
        *line_end = '\0';
        
        if (first_line) {
            /* Parse request line */
            if (parse_request_line(line, req) != PROTON_OK) {
                return PROTON_ERROR;
            }
            first_line = 0;
        } else if (*line != '\0') {
            /* Parse header */
            if (parse_header(line, req) != PROTON_OK) {
                return PROTON_ERROR;
            }
        }
        
        line = line_end + 2; /* Skip \r\n */
    }
    
    /* TODO: Parse body if Content-Length present */
    
    return PROTON_OK;
}

const char* proton_http_get_header(proton_http_request_t *req, const char *name) {
    if (!req || !name) return NULL;
    
    for (proton_http_header_t *h = req->headers; h; h = h->next) {
        if (strcasecmp(h->name, name) == 0) {
            return h->value;
        }
    }
    
    return NULL;
}
