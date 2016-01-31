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
static io_t io;

void* send_in(void *arg) {
	char string[STRING_SIZE] = "";
	while (1) {
		io_get(&io, string);
		send(sock, string, strlen(string)+1, 0);
	}
	pthread_exit(NULL);
}

void recv_out(pthread_t send_thread) {
	char string[STRING_SIZE] = "";
	int ret;
	while (1) {
		ret = recv(sock, string, STRING_SIZE, 0);
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
	close(sock);
	io_close(&io);
}


int main(int argc, char *argv[]) {
	atexit(close_sock);
	
	// First, open the stocket
	sock = socket(AF_INET, SOCK_STREAM, 0);

	char adress[] = "127.0.0.1";
	SOCKADDR_IN sin;
	sin.sin_addr.s_addr = inet_addr(adress);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(PORT);

	if(connect(sock, (SOCKADDR*)&sin, sizeof(sin)) == -1) {
		perror("Sorry, the connexion failed: connect");
		return EXIT_FAILURE;
	}
	
	// Create the pipes
	io_create(&io, argv[1]);
	
	// Run the input/output functions	
	pthread_t thread;
	if (pthread_create(&thread, NULL, send_in, NULL) == -1) {
		perror("pthread_create");
		return EXIT_FAILURE;
	}
	recv_out(thread);
	
	return EXIT_SUCCESS;
}

