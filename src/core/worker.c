#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include "proton.h"
#include "event.h"
#include "http.h"
#include "module.h"

static int listen_fd = -1;
proton_event_loop_t *event_loop = NULL;  /* Global for event system */
static proton_config_t *worker_config = NULL;

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int create_listen_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        proton_log(LOG_ERROR, "Failed to create socket: %s", strerror(errno));
        return -1;
    }
    
    /* Set SO_REUSEADDR and SO_REUSEPORT for multi-worker support */
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        proton_log(LOG_ERROR, "Failed to set SO_REUSEADDR: %s", strerror(errno));
        close(fd);
        return -1;
    }
    
    #ifdef SO_REUSEPORT
    opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        proton_log(LOG_ERROR, "Failed to set SO_REUSEPORT: %s", strerror(errno));
        close(fd);
        return -1;
    }
    #endif
    
    /* Bind */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        proton_log(LOG_ERROR, "Failed to bind to port %d: %s", port, strerror(errno));
        close(fd);
        return -1;
    }
    
    /* Listen */
    if (listen(fd, 128) < 0) {
        proton_log(LOG_ERROR, "Failed to listen: %s", strerror(errno));
        close(fd);
        return -1;
    }
    
    /* Set non-blocking */
    if (set_nonblocking(fd) < 0) {
        proton_log(LOG_ERROR, "Failed to set non-blocking: %s", strerror(errno));
        close(fd);
        return -1;
    }
    
    return fd;
}

static int accept_handler(proton_event_t *ev) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    while (1) {
        int client_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; /* No more connections */
            }
            proton_log(LOG_ERROR, "Accept failed: %s", strerror(errno));
            break;
        }
        
        /* Set non-blocking */
        set_nonblocking(client_fd);
        
        /* Create HTTP connection */
        proton_http_connection_t *conn = proton_http_connection_create(client_fd);
        if (!conn) {
            proton_log(LOG_ERROR, "Failed to create HTTP connection");
            close(client_fd);
            continue;
        }
        
        /* Add to event loop */
        proton_event_add(event_loop, conn->event, PROTON_EVENT_READ);
    }
    
    return PROTON_OK;
}

int proton_worker_process(proton_config_t *config) {
    worker_config = config;
    
    /* Create listen socket (shared across workers via SO_REUSEPORT or inherited) */
    int port = config->listen_port;
    listen_fd = create_listen_socket(port);
    if (listen_fd < 0) {
        return 1;
    }
    
    /* Create event loop */
    int max_conns = config->worker_connections;
    event_loop = proton_event_loop_create(max_conns);
    if (!event_loop) {
        proton_log(LOG_ERROR, "Failed to create event loop");
        close(listen_fd);
        return 1;
    }
    
    /* Add listen socket to event loop */
    proton_event_t *listen_event = proton_event_create(listen_fd);
    if (!listen_event) {
        proton_event_loop_destroy(event_loop);
        close(listen_fd);
        return 1;
    }
    listen_event->read_handler = accept_handler;
    
    if (proton_event_add(event_loop, listen_event, PROTON_EVENT_READ) != PROTON_OK) {
        proton_event_destroy(listen_event);
        proton_event_loop_destroy(event_loop);
        close(listen_fd);
        return 1;
    }
    
    proton_log(LOG_INFO, "Worker ready, listening on port %d", port);
    
    /* Event loop */
    while (!proton_quit) {
        int ret = proton_event_process(event_loop, 1000); /* 1 second timeout */
        if (ret < 0 && errno != EINTR) {
            proton_log(LOG_ERROR, "Event processing error: %s", strerror(errno));
            break;
        }
    }
    
    proton_log(LOG_INFO, "Worker shutting down");
    
    /* Cleanup */
    proton_event_destroy(listen_event);
    proton_event_loop_destroy(event_loop);
    close(listen_fd);
    
    return 0;
}
