
#ifndef __AB_EVENT_H_INCLUDED__
#define __AB_EVENT_H_INCLUDED__

#include <ab_connection.h>

int ab_event_init();
int ab_event_add_connection(ab_connection_t *c);
int ab_event_del_connection(ab_connection_t *c);
int ab_event_poll(int timer);

#endif /* __AB_EVENT_H_INCLUDED__ */
