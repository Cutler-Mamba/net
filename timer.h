#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include <time.h>

typedef void (*timer_expire_handler)(void *data);

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
	struct timer_handler_node *head;
};

int add_timer(time_t timeout, timer_expire_handler handler, void* data);
long wait_duration_usec(long max_duration);
void get_ready_timers(struct timer_handler_node **n);
void get_all_timers(struct timer_handler_node **n);

#endif
