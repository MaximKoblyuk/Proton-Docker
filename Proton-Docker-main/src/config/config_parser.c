#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "proton.h"

/* Simple config parser - simplified version */

static void trim(char *str) {
    if (!str) return;
    
    /* Trim leading whitespace */
    char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    
    /* Trim trailing whitespace */
    size_t len = strlen(str);
    if (len == 0) return;
    
    char *end = str + len - 1;
    while (end >= str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

static int parse_int(const char *value, int default_value) {
    if (!value) return default_value;
    
    if (strcmp(value, "auto") == 0) {
        return 0; /* Auto-detect */
    }
    
    return atoi(value);
}

proton_config_t* proton_config_parse(const char *filename) {
    fprintf(stderr, "[CONFIG] Parsing: %s\n", filename);
    
    FILE *fp = fopen(filename, "r");
    fprintf(stderr, "[CONFIG] fopen result: %p\n", (void*)fp);
    
    if (!fp) {
        fprintf(stderr, "Failed to open config file: %s\n", filename);
        
        /* Return default config */
        proton_config_t *config = calloc(1, sizeof(proton_config_t));
        if (config) {
            config->worker_processes = 0; /* auto */
            config->worker_connections = 1024;
            config->listen_port = 8080;
            config->error_log = NULL;
            config->access_log = NULL;
            config->document_root = NULL;
        }
        return config;
    }
    
    fprintf(stderr, "[CONFIG] File opened, allocating config struct\n");
    
    proton_config_t *config = calloc(1, sizeof(proton_config_t));
    fprintf(stderr, "[CONFIG] calloc result: %p\n", (void*)config);
    
    if (!config) {
        fclose(fp);
        return NULL;
    }
    
    fprintf(stderr, "[CONFIG] Setting defaults\n");
    
    /* Set defaults */
    config->worker_processes = 0; /* auto */
    config->worker_connections = 1024;
    config->listen_port = 8080;
    
    /* Leave error_log, access_log, document_root as NULL initially */
    config->error_log = NULL;
    config->access_log = NULL;
    config->document_root = NULL;
    
    fprintf(stderr, "[CONFIG] Defaults set, starting parse loop\n");
    
    /* Parse line by line */
    char line[1024];
    int in_http = 0;
    int in_server = 0;
    
    fprintf(stderr, "[CONFIG] Entering while loop\n");
    
    while (fgets(line, sizeof(line), fp)) {
        trim(line);
        
        /* Skip empty lines and comments */
        if (line[0] == '\0' || line[0] == '#') continue;
        
        /* Parse directives */
        if (strncmp(line, "worker_processes", 16) == 0) {
            char *value = strchr(line, ' ');
            if (value) {
                value++;
                trim(value);
                /* Remove semicolon */
                char *semi = strchr(value, ';');
                if (semi) *semi = '\0';
                config->worker_processes = parse_int(value, 0);
            }
        }
        else if (strncmp(line, "worker_connections", 18) == 0) {
            char *value = strchr(line, ' ');
            if (value) {
                value++;
                trim(value);
                char *semi = strchr(value, ';');
                if (semi) *semi = '\0';
                config->worker_connections = atoi(value);
            }
        }
        else if (strncmp(line, "listen", 6) == 0 && in_server) {
            char *value = strchr(line, ' ');
            if (value) {
                value++;
                trim(value);
                char *semi = strchr(value, ';');
                if (semi) *semi = '\0';
                config->listen_port = atoi(value);
            }
        }
        else if (strncmp(line, "error_log", 9) == 0) {
            fprintf(stderr, "[CONFIG] Processing error_log directive\n");
            char *value = strchr(line, ' ');
            fprintf(stderr, "[CONFIG] strchr result: %p\n", (void*)value);
            if (value) {
                value++;
                fprintf(stderr, "[CONFIG] Before trim, value: '%s'\n", value);
                trim(value);
                fprintf(stderr, "[CONFIG] After trim, value: '%s'\n", value);
                char *semi = strchr(value, ';');
                fprintf(stderr, "[CONFIG] semicolon at: %p\n", (void*)semi);
                if (semi) *semi = '\0';
                fprintf(stderr, "[CONFIG] After removing semicolon, value: '%s'\n", value);
                
                if (config->error_log) {
                    fprintf(stderr, "[CONFIG] Freeing old error_log\n");
                    free(config->error_log);
                }
                fprintf(stderr, "[CONFIG] About to strdup value '%s'\n", value);
                fprintf(stderr, "[CONFIG] strlen(value) = %zu\n", strlen(value));
                
                // Try manual allocation instead of strdup
                size_t len = strlen(value);
                config->error_log = malloc(len + 1);
                fprintf(stderr, "[CONFIG] malloc returned: %p\n", (void*)config->error_log);
                if (config->error_log) {
                    strcpy(config->error_log, value);
                    fprintf(stderr, "[CONFIG] strcpy done\n");
                }
                
                fprintf(stderr, "[CONFIG] Set error_log to: %s (ptr: %p)\n", config->error_log, (void*)config->error_log);
            }
        }
        else if (strncmp(line, "root", 4) == 0 && in_server) {
            char *value = strchr(line, ' ');
            if (value) {
                value++;
                trim(value);
                char *semi = strchr(value, ';');
                if (semi) *semi = '\0';
                
                if (config->document_root) {
                    free(config->document_root);
                }
                size_t len = strlen(value);
                config->document_root = malloc(len + 1);
                if (config->document_root) strcpy(config->document_root, value);
            }
        }
        else if (strcmp(line, "http {") == 0) {
            in_http = 1;
        }
        else if (strcmp(line, "server {") == 0 && in_http) {
            in_server = 1;
        }
        else if (strcmp(line, "}") == 0) {
            if (in_server) in_server = 0;
            else if (in_http) in_http = 0;
        }
    }
    
    fprintf(stderr, "[CONFIG] Parse loop complete, closing file\n");
    fclose(fp);
    fprintf(stderr, "[CONFIG] File closed, returning config at %p\n", (void*)config);
    return config;
}

void proton_config_destroy(proton_config_t *config) {
    if (!config) return;
    
    free(config->error_log);
    free(config->access_log);
    free(config->document_root);
    free(config);
}
