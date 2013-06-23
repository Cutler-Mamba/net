#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include <time.h>

/* forward declare */
struct connection_pool;

typedef void (*timer_expire_handler)(struct connection_pool *cp, void *data);

struct timer_handler_node
{
	struct timer_handler_node *next;
	void *data;
	timer_expire_handler handler;
};

struct timer_node
{
	int heap_idx;
	time_t timeout;
	void* data;
	timer_expire_handler handler;
};

int timer_queue_init(struct connection_pool *cp);
void timer_queue_destroy(struct connection_pool *cp);
int add_timer(struct connection_pool *cp, time_t timeout, timer_expire_handler handler, void* data);
long wait_duration_usec(struct connection_pool *cp, long max_duration);
void get_ready_timers(struct connection_pool *cp, struct timer_handler_node **n);
void get_all_timers(struct connection_pool *cp, struct timer_handler_node **n);

#endif
