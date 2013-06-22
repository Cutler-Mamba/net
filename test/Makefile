all:
	gcc -g -o timer -Wall test_timer.c ../skbuf.c ../conn.c ../timer.c
	gcc -g -o server -Wall test_server.c ../skbuf.c ../conn.c ../timer.c
	gcc -g -o client -Wall test_client.c ../skbuf.c ../conn.c ../timer.c
	#gcc -g -o skbuf -Wall test_skbuf.c ../skbuf.c
clean:
	rm -rf conn skbuf
