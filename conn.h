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

#define CONNECT_NONE 0
#define CONNECT_PENDING 1
#define CONNECT_SUCCESS 2

struct connecting
{
	void *data;
	struct sockaddr *sockaddr;
	socklen_t socklen;
	int status;
	struct connection *conn;
};

/* declare forward */
struct connection_pool;

typedef void (*conn_callback)(struct connection_pool *cp, struct connection *c);

/* declare forward */
struct skbuf;

struct connection
{
	int fd;
	struct connection *prev;
	struct connection *next;
	void *data;
	conn_callback read_cb;
	conn_callback write_cb;
	struct skbuf *recv_buf;
	struct skbuf *send_buf;
};

/* forward declare */
struct heap;

struct connection_pool
{
	int shutdown;
	int max;
	int epfd;
	int timerfd;
	struct epoll_event *events;
	int nevents;
	struct heap* timer_queue;
	struct listening *ls;
	struct connecting *cis;
	struct connection *cs;
	struct connection *conns;
	size_t n_conns;
	struct connection *free_conns;
	size_t n_free_conns;
};

struct connection_pool *connection_pool_new(int max);
void connection_pool_free(struct connection_pool *cp);
int connection_pool_dispatch(struct connection_pool *cp, int timeout);
struct listening *create_listening(struct connection_pool *cp, void* sockaddr, socklen_t socklen);
int open_listening_sockets(struct connection_pool *cp);
void close_listening_sockets(struct connection_pool *cp);
struct connecting *create_connecting(struct connection_pool *cp, void* sockaddr, socklen_t socklen);
void do_connecting_all(struct connection_pool *cp);

#endif
