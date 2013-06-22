#include "../skbuf.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	int fd = open("string.txt", O_RDONLY);
	if (fd == -1)
		return -1;

	struct skbuf *buf = skbuf_new();
	if (buf == NULL)
		return -1;

	skbuf_read(buf, fd, -1);

	skbuf_free(buf);

	return 0;
}
