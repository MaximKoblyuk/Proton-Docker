#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "proton.h"

#define BUFFER_DEFAULT_SIZE 4096

proton_buffer_t* proton_buffer_create(size_t size) {
    if (size == 0) size = BUFFER_DEFAULT_SIZE;
    
    proton_buffer_t *buf = malloc(sizeof(proton_buffer_t));
    if (!buf) return NULL;
    
    buf->data = malloc(size);
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    
    buf->len = 0;
    buf->capacity = size;
    buf->next = NULL;
    
    return buf;
}

int proton_buffer_append(proton_buffer_t *buf, const char *data, size_t len) {
    if (!buf || !data || len == 0) return PROTON_ERROR;
    
    /* Check if we need to resize */
    if (buf->len + len > buf->capacity) {
        size_t new_capacity = buf->capacity * 2;
        while (new_capacity < buf->len + len) {
            new_capacity *= 2;
        }
        
        char *new_data = realloc(buf->data, new_capacity);
        if (!new_data) return PROTON_ERROR;
        
        buf->data = new_data;
        buf->capacity = new_capacity;
    }
    
    /* Append data */
    memcpy(buf->data + buf->len, data, len);
    buf->len += len;
    
    return PROTON_OK;
}

void proton_buffer_destroy(proton_buffer_t *buf) {
    while (buf) {
        proton_buffer_t *next = buf->next;
        free(buf->data);
        free(buf);
        buf = next;
    }
}
