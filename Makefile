CC=gcc
CFLAGS=-lpthread

all:
	$(CC) client.c io.c -o client $(CFLAGS)
	$(CC) server.c io.c -o server $(CFLAGS)
client:
	$(CC) client.c io.c -o client $(CFLAGS)
server:
	$(CC) server.c io.c -o server $(CFLAGS)
clean:
	rm server client
