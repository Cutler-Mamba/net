#ifndef CONNECTION_H_INCLUDED
#define CONNECTION_H_INCLUDED

#include <stddef.h>
#include <sys/socket.h>

/* declare forward */
struct connection;

struct listening
{
	/* single link */
	void *next;

	/* bound socket */
	struct sockaddr *sockaddr;
	socklen_t socklen;
	int fd;

	/* backlog */
	int backlog;

	/* handle stuff */
	struct connection *conn;
};

#define CONNECT_NONE 0
#define CONNECT_PENDING 1
#define CONNECT_SUCCESS 2

struct connecting
{
	/* single link */
	void *next;

	/* remote addr */
	struct sockaddr *sockaddr;
	socklen_t socklen;

	/* connect status */
	int status;

	/* handle stuff */
	struct connection *conn;
};

/* declare forward */
struct connection_pool;

typedef void (*conn_callback)(struct connection_pool *cp, struct connection *c);

/* declare forward */
struct skbuf;

struct connection
{
	/* associate */
	int fd;
	void *data;

	/* double link */
	struct connection *prev;
	struct connection *next;

	/* callback */
	conn_callback read_cb;
	conn_callback write_cb;

	/* buffer */
	struct skbuf *recv_buf;
	struct skbuf *send_buf;
};

/* forward declare */
struct heap;

struct connection_pool
{
	int shutdown;
	int max;

	/* epoll stuff */
	int epfd;
	struct epoll_event *events;
	int nevents;

	/* timer stuff */
	int timerfd;
	struct heap* timer_queue;

	/* all listening */
	struct listening *ls;
	
	/* all connecting */
	struct connecting *cis;
	
	/* all connection */
	struct connection *cs;

	/* all skbuf */
	struct skbuf *bufs;

	/* used connection */
	struct connection *conns;
	size_t n_conns;

	/* free connection */
	struct connection *free_conns;
	size_t n_free_conns;
};

/* connection pool */
struct connection_pool *connection_pool_new(int max);
void connection_pool_free(struct connection_pool *cp);
int connection_pool_dispatch(struct connection_pool *cp, int timeout);

/* listening */
struct listening *create_listening(struct connection_pool *cp, void* sockaddr, socklen_t socklen);
int open_listening_sockets(struct connection_pool *cp);
void close_listening_sockets(struct connection_pool *cp);

/* connecting */
struct connecting *create_connecting(struct connection_pool *cp, void* sockaddr, socklen_t socklen);
void connecting_peer(struct connection_pool* cp, struct connecting *ci);

/* send message */
int send_msg(struct connection_pool *cp, struct connection *c, char* msg, size_t sz);

#endif
