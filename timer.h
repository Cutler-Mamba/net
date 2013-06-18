#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

typedef void (*timer_expire_handler)();

struct timer_handler_node
{
	struct timer_handler_node *next;
	timer_expire_handler handler;
};

struct timer_node
{
	int heap_idx;
	time_t timeout;
	struct timer_handler_node *head;
};

void get_ready_timers(struct timer_handler_node **n);

#endif
