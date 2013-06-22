#include "../conn.h"
#include <string.h>

int main(int argc, char **argv)
{
	struct connection_pool *cp;

	cp = connection_pool_new(2048);
	if (cp == NULL)
		return -1;

	while (1)
	{
		connection_pool_dispatch(cp, -1);
	}

	connection_pool_free(cp);

	return 0;
}
