#include "conn.h"
#include "skbuf.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define INITIAL_NEVENT 32
#define MAX_NEVENT 4096

static int set_non_blocking(int fd)
{
	int flag = fcntl(fd, F_GETFL, 0);
	if (flag == -1)
		return -1;
	return fcntl(fd, F_SETFL, flag | O_NONBLOCK);
}

static int connection_add(struct connection_pool *cp, int fd, struct connection *c)
{
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
	ev.data.fd = -1;
	ev.data.ptr = c;

	if (epoll_ctl(cp->epfd, EPOLL_CTL_ADD, fd, &ev) == -1)
		return -1;
	
	/* TODO lock */
	
	c->next = cp->conns;
	cp->conns->prev = c;
	cp->conns = c;
	c->prev = NULL;
	cp->n_conns++;

	/* TODO unlock */

	c->fd = fd;

	return 0;
}

static int connection_del(struct connection_pool *cp, int fd, struct connection *c)
{
	struct epoll_event ev;

	/* TODO lock */

	if (c->prev == NULL)
		cp->conns = c->next;
	else
		c->prev->next = c->next;
	if (c->next != NULL)
		c->next->prev = c->prev;
	cp->conns--;

	c->next = cp->free_conns;
	cp->free_conns->prev = c;
	cp->free_conns = c;
	c->prev = NULL;
	cp->n_free_conns++;

	/* TODO unlock */

	ev.events = 0;
	ev.data.ptr = NULL;
	if (epoll_ctl(cp->epfd, EPOLL_CTL_DEL, fd, &ev) == -1)
		return -1;
	return 0;
}

static struct connection *get_connection(struct connection_pool *cp)
{
	struct connection *c;

	/* TODO lock */

	if (cp->n_free_conns == 0)
	{
		c = NULL;
		goto done;
	}
	
	c = cp->free_conns;
	cp->free_conns = c->next;
	cp->free_conns->prev = NULL;
	c->next = NULL;
	cp->n_free_conns--;

done:
	/* TODO unlock */
	return c;
}

static void free_connection(struct connection_pool *cp, struct connection *c)
{
	/* TODO lock */

	c->next = cp->free_conns;
	cp->free_conns->prev = c;
	cp->free_conns = c;
	c->prev = NULL;
	cp->n_free_conns++;

	/* TODO unlock */
}

struct connection_pool *connection_pool_new(int max)
{
	int epfd;
	struct connection_pool *pool;
	struct connection *next;
	struct skbuf *buf;
	int i;

	if (max <= 0)
		goto failed;

	if ((epfd = epoll_create(0)) == -1)
		goto failed;
	
	if ((pool = malloc(sizeof(struct connection_pool))) == NULL)
		goto pool_failed;

	pool->epfd = epfd;

	pool->events = malloc(sizeof(struct epoll_event) * INITIAL_NEVENT);
	if (pool->events == NULL)
		goto events_failed;

	pool->nevents = INITIAL_NEVENT;

	pool->cs = malloc(sizeof(struct connection) * max);
		goto conn_failed;

	buf = n_skbuf_new(max * 2);
		goto buf_failed;

	pool->max = max;

	i = max;
	next = NULL;

	do
	{
		i--;

		/* double link */
		pool->cs[i].next = next;
		pool->cs[i].prev = NULL;
		if (next != NULL)
			next->prev = &pool->cs[i];

		pool->cs[i].fd = -1;
		pool->cs[i].recv_buf = &buf[i * 2];
		pool->cs[i].send_buf = &buf[i * 2 + 1];

		/* next */
		next = &pool->cs[i];

	} while (i);

	pool->free_conns = next;
	pool->n_free_conns = max;
	pool->conns = NULL;
	pool->n_conns = 0;

	return pool;

buf_failed:
	free(pool->cs);
conn_failed:
	free(pool->events);
events_failed:
	free(pool);
pool_failed:
	close(epfd);
failed:
	return NULL;
}

void connection_pool_free(struct connection_pool *cp)
{
	int i;
	struct connection *c;

	if (cp->cs != NULL)
	{
		for (i = 0; i < cp->max; i++)
		{
			c = &cp->cs[i];
			skbuf_free_all_chains(c->recv_buf->first);
			skbuf_free_all_chains(c->send_buf->first);
		}
		free(cp->cs);
	}
	if (cp->events != NULL)
		free(cp->events);
	if (cp->epfd >= 0)
		close(cp->epfd);

	free(cp);
}

static void accept_cb(struct connection_pool *cp, struct listening *l)
{
	struct sockaddr_in remote_addr;
	socklen_t len;
	struct connection *c;
	
	len = sizeof(struct sockaddr_in);

	/* system call */
	int client_fd = accept(l->fd, (struct sockaddr *)&remote_addr, &len);

	if (client_fd >= 0)
	{
		if (set_non_blocking(client_fd) == -1)
		{
			/* TODO log */
			goto failed;
		}

		c = get_connection(cp);
		if (c == NULL)
		{
			/* TODO log */
			goto failed;
		}

		if (connection_add(cp, client_fd, c) == -1)
		{
			free_connection(cp, c);
			goto failed;
		}
	}

	return;

failed:
	close(client_fd);
}

static void read_cb(struct connection_pool *cp, struct connection *c)
{
	ssize_t res;

	while (1)
	{
		res = skbuf_read(c->recv_buf, c->fd, -1);
		if (res > 0 || res == -2)
			break;
		else if (res == 0 || res == -3)
		{/*close fd*/
			if (connection_del(cp, c->fd, c) == -1)
			{
				/* TODO log */
			}
			break;
		}
		else if (res == -1)
			continue;
	}
}

static void write_cb(struct connection *c)
{

}

int connection_dispatch(struct connection_pool *cp, int timeout)
{
	int i, res;
	int fd;
	struct listening *l;
	struct connection *c;
	uint32_t ev;

	/* system call */
	res = epoll_wait(cp->epfd, cp->events, cp->nevents, timeout);

	if (res == -1)
		return -1;

	for (i = 0; i < res; ++i)
	{
		fd = cp->events[i].data.fd;

		if (fd != -1)
		{
			l = cp->events[i].data.ptr;
			accept_cb(cp, l);
		}
		else
		{
			c = cp->events[i].data.ptr;

			if (c->fd == -1)
				continue;

			ev = cp->events[i].events;

			if (ev & (EPOLLERR | EPOLLHUP) && (ev & (EPOLLIN | EPOLLOUT)) == 0)
				ev = (EPOLLIN | EPOLLOUT);

			if (ev & EPOLLIN)
				read_cb(cp, c);

			if (ev & EPOLLOUT)
				write_cb(c);
		}
	}

	/* expand the event queue */
	if (res == cp->nevents && cp->nevents < MAX_NEVENT)
	{
		int new_nevents = cp->nevents * 2;
		struct epoll_event *new_events;

		new_events = realloc(cp->events, sizeof(struct epoll_event) * new_nevents);
		if (new_events)
		{
			cp->events = new_events;
			cp->nevents = new_nevents;
		}
	}

	return 0;
}

#define DEFAULT_LISTEN_BACKLOG 20

struct listening *create_listening(struct connection_pool *cp, void* sockaddr, socklen_t socklen)
{
	struct listening *l;
	struct sockaddr *sa;

	l = malloc(sizeof(struct listening));
	if (l == NULL)
		return NULL;
	
	memset(l, 0, sizeof(struct listening));


	l->data = NULL;
	l->fd = -1;

	sa = malloc(sizeof(socklen));
	if (sa == NULL)
	{
		free(l);
		return NULL;
	}

	memcpy(sa, sockaddr, socklen);

	l->sockaddr = sa;
	l->socklen = socklen;

	l->backlog = DEFAULT_LISTEN_BACKLOG;
	l->conn = NULL;

	return l;
}

void close_listening_sockets(struct connection_pool *cp)
{
	struct listening *l;
	struct connection *c;

	l = cp->ls;

	while (l)
	{
		c = l->conn;
		if (c)
		{
			free_connection(cp, c);
			c->fd = -1;
		}

		close(l->fd);

		/* next */
		l = l->data;
	}
}
