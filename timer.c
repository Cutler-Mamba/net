#include "timer.h"
#include <stddef.h>
#include <stdlib.h>

struct heap
{
	struct timer_node **p;
	size_t n, a;
};

static inline void heap_init(struct heap *h)
{
	h->p = NULL;
	h->n = 0;
	h->a = 0;
}

static inline void heap_uninit(struct heap *h)
{
	if (h->p)
		free(h->p);
}

static void heap_shift_up(struct heap *h, size_t hole_index, struct timer_node *tn)
{
	size_t parent = (hole_index - 1) / 2;
	while (hole_index && heap_elem_greater(h->p[parent], tn))
	{
		(h->p[hole_index] = h->p[parent])->heap_idx = hole_index;
		hole_index = parent;
		parent = (hole_index - 1) / 2;
	}
	(h->p[hole_index] = tn)->heap_idx = hole_index;
}

static void heap_shift_down(struct heap *h, size_t hole_index, struct timer_node *tn)
{
	size_t min_child = 2 * (hole_index + 1);
	while (min_child <= h->n)
	{
		min_child -= min_child == h->n || heap_elem_greater(h->p[min_child], h->p[min_child - 1]);
		if (!(heap_elem_greater(tn, h->p[min_child])))
			break;
		(h->p[hole_index] = h->p[min_child])->heap_idx = hole_index;
		hole_index = min_child;
		min_child = 2 * (hole_index + 1);
	}
	(h->p[hole_index] = tn)->heap_idx = hole_index;
}

static int heap_elem_greater(struct timer_node *a, struct timer_node *b)
{
	
}

static int heap_reserve(struct heap *h, size_t n)
{
	struct timer_node **p;
	size_t a;

	if (h->a < n)
	{
		a = h->a ? s->a * 2 : 8;
		if (a < n)
			a = n;
		if (!(p = (struct timer_node **)realloc(h->p, a * sizeof(struct timer_node *))))
			return -1;
		s->p = p;
		s->a = a;
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
		heap_shift_down(h, 0u, h->p[--h->n]);
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
		if (tn->heap_idx > 0 && heap_elem_greater(h->p[parent], last))
			heap_shift_up(h, tn->heap_idx, last);
		else
			heap_shift_down(h, tn->heap_idx, last);
		tn->heap_idx = -1;
		return 0;
	}
	return -1;
}
