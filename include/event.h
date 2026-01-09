#ifndef PROTON_EVENT_H
#define PROTON_EVENT_H

#include "proton.h"

/* Event types */
#define PROTON_EVENT_READ   0x01
#define PROTON_EVENT_WRITE  0x02
#define PROTON_EVENT_ERROR  0x04
#define PROTON_EVENT_CLOSE  0x08

/* Forward declarations */
typedef struct proton_event_s proton_event_t;
typedef struct proton_event_loop_s proton_event_loop_t;
typedef int (*proton_event_handler_t)(proton_event_t *ev);

/* Event structure */
struct proton_event_s {
    int fd;
    int events;
    void *data;
    proton_event_handler_t read_handler;
    proton_event_handler_t write_handler;
};

/* Event loop */
struct proton_event_loop_s {
    int epfd;
    int max_events;
    proton_event_t **events;
};

/* Event loop operations */
proton_event_loop_t* proton_event_loop_create(int max_events);
int proton_event_add(proton_event_loop_t *loop, proton_event_t *ev, int events);
int proton_event_del(proton_event_loop_t *loop, proton_event_t *ev);
int proton_event_process(proton_event_loop_t *loop, int timeout);
void proton_event_loop_destroy(proton_event_loop_t *loop);

/* Event helpers */
proton_event_t* proton_event_create(int fd);
void proton_event_destroy(proton_event_t *ev);

#endif /* PROTON_EVENT_H */
