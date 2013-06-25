#include "timer.h"
#include "conn.h"
#include <stddef.h>
#include <stdlib.h>

struct heap
{
	struct timer_node **p;
	size_t n, a;
};

inline static int greater(struct timer_node *a, struct timer_node *b)
{
	if (a->timeout > b->timeout)
		return 1;
	return 0;
}

inline static void heap_init(struct heap *h)
{
	h->p = NULL;
	h->n = 0;
	h->a = 0;
}

inline static void heap_destroy(struct heap *h)
{
	if (h->p)
		free(h->p);
}

inline static int heap_empty(struct heap *h)
{
	return h->n == 0;
}

inline static size_t heap_size(struct heap *h)
{
	return h->n;
}

static void heap_shift_up(struct heap *h, size_t idx, struct timer_node *tn)
{
	size_t parent = (idx - 1) / 2;
	while (idx && greater(h->p[parent], tn))
	{
		h->p[idx] = h->p[parent];
		h->p[idx]->heap_idx = idx;
		idx = parent;
		parent = (idx - 1) / 2;
	}
	h->p[idx] = tn;
	tn->heap_idx = idx;
}

static void heap_shift_down(struct heap *h, size_t idx, struct timer_node *tn)
{
	size_t min_child = 2 * (idx + 1);
	while (min_child <= h->n)
	{
		if (min_child == h->n)
			min_child -= 1;
		else if (greater(h->p[min_child], h->p[min_child - 1]))
			min_child -= 1;
		if (!(greater(tn, h->p[min_child])))
			break;
		h->p[idx] = h->p[min_child];
		h->p[idx]->heap_idx = idx;
		idx = min_child;
		min_child = 2 * (idx + 1);
	}
	h->p[idx] = tn;
	h->p[idx]->heap_idx = idx;
}


static int heap_reserve(struct heap *h, size_t n)
{
	struct timer_node **p;
	size_t a;

	if (h->a < n)
	{
		a = h->a ? h->a * 2 : 8;
		if (a < n)
			a = n;
		if (!(p = (struct timer_node **)realloc(h->p, a * sizeof(struct timer_node *))))
			return -1;
		h->p = p;
		h->a = a;
	}
	return 0;
}

static int heap_push(struct heap *h, struct timer_node *tn)
{
	if (heap_reserve(h, h->n + 1))
		return -1;
	heap_shift_up(h, h->n++, tn);
	return 0;
}

static struct timer_node *heap_pop(struct heap *h)
{
	if (h->n)
	{
		struct timer_node *tn = *h->p;
		heap_shift_down(h, 0, h->p[--h->n]);
		tn->heap_idx = -1;
		return tn;
	}
	return NULL;
}

static int heap_erase(struct heap *h, struct timer_node *tn)
{
	if (tn->heap_idx != -1)
	{
		struct timer_node *last = h->p[--h->n];
		size_t parent = (tn->heap_idx - 1) / 2;
		if (tn->heap_idx > 0 && greater(h->p[parent], last))
			heap_shift_up(h, tn->heap_idx, last);
		else
			heap_shift_down(h, tn->heap_idx, last);
		tn->heap_idx = -1;
		return 0;
	}
	return -1;
}

int timer_queue_init(struct connection_pool *cp)
{
	cp->timer_queue = malloc(sizeof(struct heap));
	if (cp->timer_queue == NULL)
	{
		/* TODO log */
		return -1;
	}

	heap_init(cp->timer_queue);
	return 0;
}

void timer_queue_destroy(struct connection_pool *cp)
{
	heap_destroy(cp->timer_queue);
}

int add_timer(struct connection_pool *cp, time_t timeout, timer_expire_handler handler, void* data)
{
	struct timer_node* tn;

	tn = malloc(sizeof(struct timer_node));
	if (tn == NULL)
		return -1;
	
	tn->heap_idx = -1;
	tn->timeout = time(NULL) + timeout;
	tn->data = data;
	tn->handler = handler;
	if (heap_push(cp->timer_queue, tn) == -1)
	{
		free(tn);
		return -1;
	}
	return 0;
}

long wait_duration_usec(struct connection_pool *cp, long max_duration)
{
	long duration;

	if (heap_empty(cp->timer_queue))
		return max_duration;
	
	duration = cp->timer_queue->p[0]->timeout - time(NULL);
	if (duration < 0)
		duration = 0;
	else if (duration > max_duration)
		duration = max_duration;
	return duration;
}

void get_ready_timers(struct connection_pool *cp, struct timer_handler_node **n)
{
	*n = NULL;
}

void get_all_timers(struct connection_pool *cp, struct timer_handler_node **n)
{

}
