#include "conn.h"

int main(int argc, char **argv)
{
	struct connection_pool *cp = connection_pool_new(5120);
	if (cp == NULL)
		return -1;

	return 0;
}
