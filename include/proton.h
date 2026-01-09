#ifndef PROTON_H
#define PROTON_H

#include <stddef.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>

/* Version */
#define PROTON_VERSION "0.1.0"

/* Return codes */
#define PROTON_OK        0
#define PROTON_ERROR    -1
#define PROTON_AGAIN    -2
#define PROTON_DECLINED -3

/* Log levels */
#define LOG_DEBUG   0
#define LOG_INFO    1
#define LOG_WARN    2
#define LOG_ERROR   3

/* Forward declarations */
typedef struct proton_pool_s proton_pool_t;
typedef struct proton_buffer_s proton_buffer_t;
typedef struct proton_config_s proton_config_t;
typedef struct proton_connection_s proton_connection_t;

/* Memory pool */
struct proton_pool_s {
    size_t size;
    size_t used;
    void *data;
    proton_pool_t *next;
};

proton_pool_t* proton_pool_create(size_t size);
void* proton_pool_alloc(proton_pool_t *pool, size_t size);
void proton_pool_destroy(proton_pool_t *pool);

/* Buffer chain */
struct proton_buffer_s {
    char *data;
    size_t len;
    size_t capacity;
    proton_buffer_t *next;
};

proton_buffer_t* proton_buffer_create(size_t size);
int proton_buffer_append(proton_buffer_t *buf, const char *data, size_t len);
void proton_buffer_destroy(proton_buffer_t *buf);

/* Logging */
void proton_log_init(const char *filename, int level);
void proton_log(int level, const char *fmt, ...);
void proton_log_close(void);

/* Configuration */
struct proton_config_s {
    int worker_processes;
    int worker_connections;
    int listen_port;
    char *error_log;
    char *access_log;
    char *document_root;
};

proton_config_t* proton_config_parse(const char *filename);
void proton_config_destroy(proton_config_t *config);

/* Master process */
int proton_master_process(proton_config_t *config);

/* Worker process */
int proton_worker_process(proton_config_t *config);

/* Global state */
extern volatile sig_atomic_t proton_quit;
extern volatile sig_atomic_t proton_reload;
extern pid_t proton_pid;

#endif /* PROTON_H */
