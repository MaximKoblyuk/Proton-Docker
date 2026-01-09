#ifndef PROTON_MODULE_H
#define PROTON_MODULE_H

#include "proton.h"
#include "http.h"

/* Module return codes */
#define PROTON_MODULE_HANDLED   0
#define PROTON_MODULE_DECLINED -1
#define PROTON_MODULE_ERROR    -2

/* Module structure */
typedef struct proton_module_s {
    const char *name;
    int (*init)(proton_config_t *config);
    int (*handler)(proton_http_connection_t *conn);
    void (*cleanup)(void);
} proton_module_t;

/* Module registration */
extern proton_module_t *proton_modules[];

/* Module lifecycle */
int proton_modules_init(proton_config_t *config);
int proton_modules_handle_request(proton_http_connection_t *conn);
void proton_modules_cleanup(void);

#endif /* PROTON_MODULE_H */
