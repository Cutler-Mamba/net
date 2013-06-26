#ifndef SKBUF_H_INCLUDED
#define SKBUF_H_INCLUDED

#include <stddef.h>
#include <sys/types.h>

/* forward declare */
struct skbuf_chain;

struct skbuf
{
	struct skbuf_chain *first;
	struct skbuf_chain *last;
	struct skbuf_chain **last_with_datap;
	size_t total_len;
	size_t n_add_for_cb;
	size_t n_del_for_cb;
	void *data;
	int ref_count;
};

struct skbuf_chain
{
	struct skbuf_chain *next;
	size_t buffer_len;
	size_t misalign;
	size_t off;
	unsigned char *buffer;
};

void skbuf_init(struct skbuf *buf);
void skbuf_destroy(struct skbuf *buf);

struct skbuf *skbuf_new();
void skbuf_free(struct skbuf *buf);
struct skbuf *n_skbuf_new(size_t n);
void skbuf_free_all_chains(struct skbuf_chain *chain);
ssize_t skbuf_read(struct skbuf *buf, int fd, int howmuch);
ssize_t skbuf_write(struct skbuf *buf, int fd);

#endif
