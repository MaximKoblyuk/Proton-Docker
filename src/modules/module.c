#include <stdio.h>
#include "proton.h"
#include "module.h"

/* External module declarations */
extern proton_module_t mod_static;

/* Module registry */
proton_module_t *proton_modules[] = {
    &mod_static,
    NULL
};

int proton_modules_init(proton_config_t *config) {
    for (int i = 0; proton_modules[i] != NULL; i++) {
        proton_module_t *mod = proton_modules[i];
        
        if (mod->init) {
            int ret = mod->init(config);
            if (ret != PROTON_OK) {
                proton_log(LOG_ERROR, "Failed to initialize module: %s", mod->name);
                return ret;
            }
            proton_log(LOG_INFO, "Module loaded: %s", mod->name);
        }
    }
    
    return PROTON_OK;
}

int proton_modules_handle_request(proton_http_connection_t *conn) {
    for (int i = 0; proton_modules[i] != NULL; i++) {
        proton_module_t *mod = proton_modules[i];
        
        if (mod->handler) {
            int ret = mod->handler(conn);
            
            if (ret == PROTON_MODULE_HANDLED) {
                return PROTON_MODULE_HANDLED;
            }
            
            if (ret == PROTON_MODULE_ERROR) {
                proton_log(LOG_ERROR, "Module %s returned error", mod->name);
                return PROTON_MODULE_ERROR;
            }
            
            /* PROTON_MODULE_DECLINED - continue to next module */
        }
    }
    
    return PROTON_MODULE_DECLINED;
}

void proton_modules_cleanup(void) {
    for (int i = 0; proton_modules[i] != NULL; i++) {
        proton_module_t *mod = proton_modules[i];
        
        if (mod->cleanup) {
            mod->cleanup();
            proton_log(LOG_INFO, "Module cleaned up: %s", mod->name);
        }
    }
}
