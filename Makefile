all:
	gcc -g -o conn -Wall test_conn.c skbuf.c conn.c
	gcc -g -o skbuf -Wall test_skbuf.c skbuf.c
	gcc -g -o timer -Wall timer.c

clean:
	rm -rf conn skbuf timer
