#include "../conn.h"
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char **argv)
{
	struct connection_pool *cp;
	struct sockaddr_in l_addr;
	struct listening *l;
	struct connecting *ci;

	cp = connection_pool_new(2048);
	if (cp == NULL)
		return -1;

	memset(&l_addr, 0, sizeof(struct sockaddr_in));
	l_addr.sin_family = AF_INET;
	l_addr.sin_port = htons(8888);
	l_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	ci = create_connecting(cp, &l_addr, sizeof(struct sockaddr_in));
	if (ci == NULL)
		return -1;

	connecting_peer(cp, ci);

	while (1)
	{
		connection_pool_dispatch(cp, -1);
	}

	connection_pool_free(cp);

	return 0;
}
