#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "proton.h"
#include "http.h"
#include "module.h"

static char *document_root = NULL;

static const char* get_mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    
    ext++; /* Skip the dot */
    
    if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0) return "text/html";
    if (strcmp(ext, "css") == 0) return "text/css";
    if (strcmp(ext, "js") == 0) return "application/javascript";
    if (strcmp(ext, "json") == 0) return "application/json";
    if (strcmp(ext, "png") == 0) return "image/png";
    if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, "gif") == 0) return "image/gif";
    if (strcmp(ext, "svg") == 0) return "image/svg+xml";
    if (strcmp(ext, "txt") == 0) return "text/plain";
    if (strcmp(ext, "xml") == 0) return "application/xml";
    
    return "application/octet-stream";
}

static int mod_static_init(proton_config_t *config) {
    if (config->document_root) {
        size_t len = strlen(config->document_root);
        document_root = malloc(len + 1);
        if (document_root) strcpy(document_root, config->document_root);
        proton_log(LOG_INFO, "Static file module initialized (root=%s)", document_root);
    } else {
        document_root = malloc(2);
        if (document_root) strcpy(document_root, ".");
        proton_log(LOG_INFO, "Static file module initialized (root=.)");
    }
    return PROTON_OK;
}

static int mod_static_handler(proton_http_connection_t *conn) {
    if (!conn || !conn->request) return PROTON_MODULE_ERROR;
    
    proton_http_request_t *req = conn->request;
    proton_http_response_t *res = conn->response;
    
    /* Only handle GET and HEAD */
    if (req->method != HTTP_GET && req->method != HTTP_HEAD) {
        return PROTON_MODULE_DECLINED;
    }
    
    /* Build file path */
    char filepath[4096];
    snprintf(filepath, sizeof(filepath), "%s%s", document_root, req->uri);
    
    /* Prevent directory traversal */
    if (strstr(filepath, "..")) {
        res->status = HTTP_STATUS_BAD_REQUEST;
        proton_http_response_write(res, "400 Bad Request\n", 16);
        return PROTON_MODULE_HANDLED;
    }
    
    /* Check if directory - append index.html */
    struct stat st;
    if (stat(filepath, &st) == 0 && S_ISDIR(st.st_mode)) {
        snprintf(filepath, sizeof(filepath), "%s%s/index.html", document_root, req->uri);
    }
    
    /* Try to open file */
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        if (errno == ENOENT) {
            return PROTON_MODULE_DECLINED; /* Let other modules try */
        }
        
        proton_log(LOG_ERROR, "Failed to open %s: %s", filepath, strerror(errno));
        res->status = HTTP_STATUS_INTERNAL_ERROR;
        proton_http_response_write(res, "500 Internal Server Error\n", 26);
        return PROTON_MODULE_HANDLED;
    }
    
    /* Get file size */
    if (fstat(fd, &st) < 0) {
        close(fd);
        res->status = HTTP_STATUS_INTERNAL_ERROR;
        proton_http_response_write(res, "500 Internal Server Error\n", 26);
        return PROTON_MODULE_HANDLED;
    }
    
    /* Set response headers */
    res->status = HTTP_STATUS_OK;
    proton_http_response_add_header(res, "Content-Type", get_mime_type(filepath));
    
    /* Read and send file */
    if (req->method == HTTP_GET) {
        char buf[4096];
        ssize_t n;
        
        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            proton_http_response_write(res, buf, n);
        }
    }
    
    close(fd);
    
    proton_log(LOG_INFO, "Served static file: %s (%ld bytes)", filepath, st.st_size);
    
    return PROTON_MODULE_HANDLED;
}

static void mod_static_cleanup(void) {
    if (document_root) {
        free(document_root);
        document_root = NULL;
    }
}

proton_module_t mod_static = {
    .name = "static",
    .init = mod_static_init,
    .handler = mod_static_handler,
    .cleanup = mod_static_cleanup
};
