#ifndef CONNECTION_H_INCLUDED
#define CONNECTION_H_INCLUDED

#include <stddef.h>
#include <sys/socket.h>

/* declare forward */
struct connection;

struct listening
{
	void *data;
	int fd;
	struct sockaddr *sockaddr;
	socklen_t socklen;
	int backlog;
	struct connection *conn;
};

/* declare forward */
struct skbuf;

struct connection
{
	int fd;
	struct connection *prev;
	struct connection *next;
	struct skbuf *recv_buf;
	struct skbuf *send_buf;
};

struct connection_pool
{
	int max;
	int epfd;
	struct epoll_event *events;
	int nevents;
	struct listening *ls;
	struct connection *cs;
	struct connection *conns;
	size_t n_conns;
	struct connection *free_conns;
	size_t n_free_conns;
	int timerfd;
};

struct connection_pool *connection_pool_new(int max);
void connection_pool_free(struct connection_pool *cp);
int connection_dispatch(struct connection_pool *cp, int timeout);
struct listening *create_listening(struct connection_pool *cp, void* sockaddr, socklen_t socklen);
void close_listening_sockets();

#endif
