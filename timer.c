#include "timer.h"
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

int add_timer(time_t timeout, timer_expire_handler handler, void* data)
{
	return -1;
}

long wait_duration_usec(long max_duration)
{
	return max_duration;
}

void get_ready_timers(struct timer_handler_node **n)
{

}

void get_all_timers(struct timer_handler_node **n)
{

}
