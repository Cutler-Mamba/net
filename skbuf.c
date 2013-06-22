#include "skbuf.h"
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define MIN_BUFFER_SIZE 512

static struct skbuf_chain *skbuf_chain_new(size_t size)
{
	struct skbuf_chain *chain;
	size_t to_alloc;

	size += sizeof(struct skbuf_chain);

	/*get the next largest memory that can hold the buffer*/
	to_alloc = MIN_BUFFER_SIZE;
	while (to_alloc < size)
		to_alloc <<= 1;
	
	/*we get everything in one chunk*/
	if ((chain = malloc(to_alloc)) == NULL)
		return NULL;

	memset(chain, 0, sizeof(struct skbuf_chain));

	chain->buffer_len = to_alloc - sizeof(struct skbuf_chain);

	chain->buffer = (unsigned char *)((struct skbuf_chain *)chain + 1);

	return chain;
}

void skbuf_free_all_chains(struct skbuf_chain *chain)
{
	struct skbuf_chain *next;
	for (; chain; chain = next)
	{
		next = chain->next;
		free(chain);
	}
}

static void skbuf_chain_insert(struct skbuf *buf, struct skbuf_chain *chain)
{
	if (*buf->last_with_datap == NULL)
	{
		buf->first = buf->last = chain;
	}
	else
	{
		struct skbuf_chain **ch = buf->last_with_datap;
		while ((*ch) && (*ch)->off != 0)
			ch = &(*ch)->next;
		if (*ch == NULL)
		{
			buf->last->next = chain;
			if (chain->off)
				buf->last_with_datap = &buf->last->next;
		}
		else
		{
			skbuf_free_all_chains(*ch);
			*ch = chain;
		}
		buf->last = chain;
	}
	buf->total_len += chain->off;
}

static struct skbuf_chain *skbuf_chain_insert_new(struct skbuf *buf, size_t datlen)
{
	struct skbuf_chain *chain;
	if ((chain = skbuf_chain_new(datlen)) == NULL)
		return NULL;
	skbuf_chain_insert(buf, chain);
	return chain;
}

struct skbuf *skbuf_new()
{
	struct skbuf *buffer;
	buffer = malloc(sizeof(struct skbuf));
	if (buffer == NULL)
		return NULL;

	buffer->first = NULL;
	buffer->last = NULL;
	buffer->last_with_datap = &buffer->first;
	buffer->ref_count = 1;

	return buffer;
}

void skbuf_free(struct skbuf *buf)
{
	/*TODO lock*/
	struct skbuf_chain *chain, *next;

	if (--buf->ref_count > 0)
	{
		/*TODO unlock*/
		return;
	}

	for (chain = buf->first; chain != NULL; chain = next)
	{
		next = chain->next;
		free(chain);
	}

	/*TODO unlock*/
	free(buf);
}

struct skbuf *n_skbuf_new(size_t n)
{
	struct skbuf *buffer;
	size_t i;

	buffer = malloc(sizeof(struct skbuf) * n);
	if (buffer == NULL)
		return NULL;

	for (i = 0; i < n; i++)
	{
		buffer[i].first = NULL;
		buffer[i].last = NULL;
		buffer[i].last_with_datap = &buffer->first;
		buffer[i].ref_count = 1;
	}

	return buffer;
}

unsigned char *skbuf_pullup(struct skbuf *buf, int size)
{
	struct skbuf_chain *chain, *next, *tmp, *last_with_data;
	unsigned char *buffer, *result = NULL;
	int remaining;
	int removed_last_with_data = 0;
	int removed_last_with_datap = 0;

	chain = buf->first;

	if (size < 0)
		size = buf->total_len;

	if (size == 0 || (size_t)size > buf->total_len)
		goto done;

	/* no need to pull up */
	if (chain->off >= (size_t)size)
	{
		result = chain->buffer + chain->misalign;
		goto done;
	}

	remaining = size - chain->off;
	for (tmp = chain->next; tmp; tmp = tmp->next)
	{
		if (tmp->off >= (size_t)remaining)
			break;
		remaining -= tmp->off;
	}

	if (chain->buffer_len - chain->misalign >= (size_t)size)
	{
		/* already have enough space in the first chain */
		size_t old_off = chain->off;
		buffer = chain->buffer + chain->misalign + chain->off;
		tmp = chain;
		tmp->off = size;
		size -= old_off;
		chain = chain->next;
	}
	else
	{
		if ((tmp = skbuf_chain_new(size)) == NULL)
			goto done;
		buffer = tmp->buffer;
		tmp->off = size;
		buf->first = tmp;
	}

	last_with_data = *buf->last_with_datap;
	for (; chain != NULL && (size_t)size >= chain->off; chain = next)
	{
		next = chain->next;

		memcpy(buffer, chain->buffer + chain->misalign, chain->off);
		size -= chain->off;
		buffer += chain->off;
		if (chain == last_with_data)
			removed_last_with_data = 1;
		if (&chain->next == buf->last_with_datap)
			removed_last_with_datap = 1;

		free(chain);
	}

	if (chain != NULL)
	{
		memcpy(buffer, chain->buffer + chain->misalign, size);
		chain->misalign += size;
		chain->off -= size;
	}
	else
	{
		buf->last = tmp;
	}

	tmp->next = chain;

	if (removed_last_with_data)
		buf->last_with_datap = &buf->first;
	else if (removed_last_with_datap)
	{
		if (buf->first->next && buf->first->next->off)
			buf->last_with_datap = &buf->first->next;
		else
			buf->last_with_datap = &buf->first;
	}

	result = (tmp->buffer + tmp->misalign);

done:
	return result;
}

int skbuf_drain(struct skbuf *buf, size_t len)
{
	struct skbuf_chain *chain, *next;
	size_t remaining, old_len;
	int result = 0;

	old_len = buf->total_len;

	if (old_len == 0)
		goto done;

	if (len >= old_len)
		len = old_len;
	
	buf->total_len -= len;
	remaining = len;
	for (chain = buf->first; remaining >= chain->off; chain = next)
	{
		next = chain->next;
		remaining -= chain->off;

		if (chain == *buf->last_with_datap)
			buf->last_with_datap = &buf->first;

		if (&chain->next == buf->last_with_datap)
			buf->last_with_datap = &buf->first;

		free(chain);
	}

	buf->first = chain;
	if (chain)
	{
		chain->misalign += remaining;
		chain->off -= remaining;
	}

	buf->n_del_for_cb += len;

	/* callback */

done:
	return result;
}

static void skbuf_chain_align(struct skbuf_chain *chain)
{
	memmove(chain->buffer, chain->buffer + chain->misalign, chain->off);
	chain->misalign = 0;
}

#define MAX_TO_COPY_IN_EXPAND 4096
#define MAX_TO_REALIGN_IN_EXPAND 2048

static int skbuf_chain_should_realign(struct skbuf_chain *chain, size_t datlen)
{
	return chain->buffer_len - chain->off >= datlen &&
		(chain->off < chain->buffer_len / 2) &&
		(chain->off <= MAX_TO_REALIGN_IN_EXPAND);
}

#define CHAIN_SPACE_LEN(ch) ((ch)->buffer_len - ((ch)->misalign +(ch)->off))

static struct skbuf_chain *skbuf_expand_singlechain(struct skbuf *buf, size_t datlen)
{
	struct skbuf_chain *chain, **chainp;
	struct skbuf_chain *result = NULL;

	chainp = buf->last_with_datap;

	if (*chainp && CHAIN_SPACE_LEN(*chainp) == 0)
		chainp = &(*chainp)->next;

	chain = *chainp;

	if (chain == NULL)
		goto insert_new;

	if (CHAIN_SPACE_LEN(chain) >= datlen)
	{
		result = chain;
		goto ok;
	}

	if (chain->off == 0)
		goto insert_new;
	
	if (skbuf_chain_should_realign(chain, datlen))
	{
		skbuf_chain_align(chain);
		result = chain;
		goto ok;
	}

	if (CHAIN_SPACE_LEN(chain) < chain->buffer_len / 8 ||
		chain->off > MAX_TO_COPY_IN_EXPAND)
	{
		if (chain->next && CHAIN_SPACE_LEN(chain->next) >= datlen)
		{
			result = chain->next;
			goto ok;
		}
		else
		{
			goto insert_new;
		}
	}
	else
	{
		size_t length = chain->off + datlen;
		struct skbuf_chain *tmp = skbuf_chain_new(length);
		if (tmp == NULL)
			goto err;

		tmp->off = chain->off;
		memcpy(tmp->buffer, chain->buffer + chain->misalign, chain->off);

		result = *chainp = tmp;

		if (buf->last == chain)
			buf->last = tmp;

		tmp->next = chain->next;
		free(chain);
		goto ok;
	}

insert_new:
	result = skbuf_chain_insert_new(buf, datlen);
	if (!result)
		goto err;
ok:
err:
	return result;
}

#define SKBUF_MAX_READ 4096

static int get_n_bytes_readable_on_socket(int fd)
{
	int n = SKBUF_MAX_READ;
	if (ioctl(fd, FIONREAD, &n) < 0)
		return -1;
	return n;
}

static int advance_last_with_data(struct skbuf *buf)
{
	int n = 0;
	if (!*buf->last_with_datap)
		return 0;

	while ((*buf->last_with_datap)->next && (*buf->last_with_datap)->next->off)
	{
		buf->last_with_datap = &(*buf->last_with_datap)->next;
		++n;
	}
	return n;
}

ssize_t skbuf_read(struct skbuf *buf, int fd, int howmuch)
{
	ssize_t err;
	ssize_t n, result;

	struct skbuf_chain *chain;
	unsigned char *p;

	n = get_n_bytes_readable_on_socket(fd);
	if (n <= 0 || n > SKBUF_MAX_READ)
		n = SKBUF_MAX_READ;
	if (howmuch < 0 || howmuch > n)
		howmuch = n;

	if ((chain = skbuf_expand_singlechain(buf, howmuch)) == NULL)
	{
		result = -1;
		goto done;
	}

	/* we can append new data at this point */
	p = chain->buffer + chain->misalign + chain->off;

	while(1)
	{
		/* system call */
		n = recv(fd, p, howmuch, 0);

		/* client close */
		if (n == 0)
		{
			close(fd);
			result = n;
			goto done;
		}
		else if (n > 0)
		{
			chain->off += n;
			advance_last_with_data(buf);

			buf->total_len += n;
			buf->n_add_for_cb += n;

			/* callback */

			if (n < howmuch)
				result = n;
			else
				result = -1;
			goto done;
		}

		/* err occurs */
		err = errno;

		if (err != EINTR)
		{
			if (err == EAGAIN)
				result = -2;
			else
			{
				result = -3;
				close(fd);
			}
			goto done;
		}
	}

done:
	return result;
}

static ssize_t skbuf_write_atmost(struct skbuf *buffer, int fd, int howmuch)
{
	ssize_t err;
	ssize_t n, result;

	if (howmuch < 0 || (size_t)howmuch > buffer->total_len)
		howmuch = buffer->total_len;

	if (howmuch > 0)
	{
		void *p = skbuf_pullup(buffer, howmuch);

		while (1)
		{
			/* system call */
			n = send(fd, p, howmuch, 0);

			if (n > 0)
			{
				skbuf_drain(buffer, n);

				if (n < (ssize_t)howmuch)
					result = n;
				else
					result = -1;
				goto done;
			}

			err = errno;

			if (n == 0)
			{
				result = n;
				goto done;
			}
			
			if (err != EINTR)
			{
				if (err == EAGAIN)
				{
					result = -2;
				}
				else
				{
					result = -3;
					close(fd);
				}
				goto done;
			}
		}
	}
	else
		result = -4;
done:
	return result;
}

ssize_t skbuf_write(struct skbuf *buf, int fd)
{
	return skbuf_write_atmost(buf, fd, -1);
}
