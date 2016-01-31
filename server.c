#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PORT 776

#include "io.h"

static SOCKET sock;
static SOCKET csock;
static io_t io;

void* send_in(void *arg) {
	char string[STRING_SIZE] = "";
	while (1) {
		io_get(&io, string);
		send(csock, string, strlen(string)+1, 0);
	}
	pthread_exit(NULL);
}

void recv_out(pthread_t send_thread) {
	char string[STRING_SIZE] = "";
	int ret;
	while (1) {
		ret = recv(csock, string, STRING_SIZE, 0);
		if (ret == -1) {
			fprintf(stderr, "server: Recv error.\n");
			pthread_kill(send_thread, SIGKILL);
			exit(0);
		}
		else if (ret == 0) {
			pthread_kill(send_thread, SIGKILL);
			exit(0);
		}
		else
			io_send(&io, string);
	}
}

void close_sock() {
	printf("Quit\n");
	shutdown(csock, 2);
	close(sock);
	close(csock);
	io_close(&io);
}

int main(int argc, char* argv[]) {
	atexit(close_sock);
	
	// First, create the socket
	SOCKADDR_IN sin;
	socklen_t recsize = sizeof(sin);

	SOCKADDR_IN csin;
	socklen_t crecsize = sizeof(csin);

	sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock == INVALID_SOCKET) {
		perror("socket");
		close(sock);
		return EXIT_FAILURE;
	}

	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

	if(bind(sock, (SOCKADDR*)&sin, recsize) == SOCKET_ERROR) {
		perror("bind");
		close(sock);
		return EXIT_FAILURE;
	}

	if(listen(sock, 5) == SOCKET_ERROR) {
		perror("listen");
		close(sock);
		return EXIT_FAILURE;
	}
	
	// Create the pipes
	io_create(&io, argv[1]);
	
	while(1) {
		csock = accept(sock, (SOCKADDR*)&csin, &crecsize);

        	pthread_t thread;
		if (pthread_create(&thread, NULL, send_in, NULL) == -1) {
			perror("pthread_create");
			return EXIT_FAILURE;
		}
		recv_out(thread);
	}

	return EXIT_SUCCESS;
}

