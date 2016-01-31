#include <wordexp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include "io.h"

/* This file is based on the "pipex_posix.c" and "io.c" files of Polyglot (Winboard protocol to UCI protocol adapter). */

void io_create(io_t *io, const char *command) {
	int from_child[2], to_child[2];
	int i,ret;
	char * argv[256];
	int argc;
	wordexp_t p;
	
	if (command == NULL) {
		io->pid = 0;
		
		io->in_fd = STDIN_FILENO;
		io->out_fd = STDOUT_FILENO;
		// attach standard error to standard output
		dup2(STDOUT_FILENO,STDERR_FILENO);
	}
	else {
		ret = wordexp(command, &p, 0);
		if (ret!=0) {
			perror("io_create(): Unable to parse command.\n");
			exit(EXIT_FAILURE);
		}

		argc = p.we_wordc;
		if (argc>=256-2) {
			perror("io_create(): Too many arguments.\n");
			exit(EXIT_FAILURE);
		}

		for (i=0;i<argc;i++)
			argv[i] = p.we_wordv[i];

		argv[argc] = NULL;
		// create the pipes
		if (pipe(from_child) == -1) {
			perror("io_create(): pipe()");
			exit(EXIT_FAILURE);
		}
		if (pipe(to_child) == -1) {
			perror("io_create(): pipe()");
			exit(EXIT_FAILURE);
		}

		// create the child process 
		io->pid = fork();

		if (io->pid == -1) {
			perror("io_create(): fork()");
			exit(EXIT_FAILURE);
		}
		else if (io->pid == 0) {
			// child

			// close unused pipe descriptors to avoid deadlocks
			close(from_child[0]);
			close(to_child[1]);

			// attach the pipe to standard input
			dup2(to_child[0], STDIN_FILENO);
			close(to_child[0]);

			// attach the pipe to standard output
			dup2(from_child[1], STDOUT_FILENO);
			close(from_child[1]);

			// attach standard error to standard output
			// commenting this out gives error messages on the console
			dup2(STDOUT_FILENO, STDERR_FILENO);

			// launch the new executable file
			execvp(argv[0],&argv[0]);
			

			while(fgets(NULL, STRING_SIZE, stdin));
			exit(EXIT_SUCCESS);
		}
		else {
			// parent 

			// close unused pipe descriptors to avoid deadlocks
			close(from_child[1]);
			close(to_child[0]);

			// copy values in the descriptor param
			io->in_fd = from_child[0];
			io->out_fd = to_child[1];
			
			io->in_size = 0;
			io->out_size = 0;
		}
	}
}

void io_close(io_t *io){
	int status;
	int ret = 0;
	
	if (io->pid != 0) { // Only if a new process has been created !
		// Terminate the child
		if (waitpid(io->pid, &status, WNOHANG) == 0) {
			printf("Child is runing. Terminating it.\n");
			kill(io->pid, SIGKILL);
			waitpid(io->pid, &status, 0);	
		}
	
		if (WIFEXITED(status))
			printf("Child exited with status %d.\n", WEXITSTATUS(status));
		else if (WIFSIGNALED(status))
			printf("Child terminated with signal %d.\n", WTERMSIG(status));
		else
			printf("Child terminated with signal %d.\n", WTERMSIG(status));
	
		// Close in_fd and out_fd
		close(io->in_fd);
		close(io->out_fd);
	}
}

void io_get_update(io_t *io) {
	int pos, size;
	int n;
	
	while (1) {
		// init
		pos = io->in_size;
		size = BUFFER_SIZE - pos;

		if (size <= 0) {
			printf("io_get_update(): buffer overflow\n");
			exit(EXIT_FAILURE);
		}

		// read as many data as possible
		n = read(io->in_fd, &io->in_buffer[pos], size);
		
		if (n > 0) { // at least one character was read
			// update buffer size
			io->in_size += n;
		}
		else // EOF
			break;
		if (memchr(io->in_buffer, '\n', io->in_size) != NULL) // buffer contains '\n'
			break;
	}
}

void io_get(io_t * io, char string[]) {
	int src, dst;
	char c;

	src = 0;
	dst = 0;
	
	io_get_update(io); // first, call io_get_update

	while (1) {
		// test for end of buffer
		if (src >= io->in_size) {
			string[0] = '\0';
			return;
		}

		// test for end of string
		if (dst >= STRING_SIZE) {
			printf("io_get_line(): buffer overflow\n");
			exit(EXIT_FAILURE);
		}

		// copy the next character
		c = io->in_buffer[src++];

		if (c == '\n') { // '\n' => line complete
			string[dst] = '\0';
			break;
		}
		else if (c != '\r') { // skip '\r's
			string[dst++] = c;
		}
	}

	// shift the buffer
	io->in_size -= src;
	memmove(&io->in_buffer[0], &io->in_buffer[src], io->in_size);
}

void io_send(io_t *io, char string[]) {
	int len;

	// append string to buffer
	len = strlen(string);
	if (io->out_size + len > BUFFER_SIZE-2) {
		printf("io_send(): buffer overflow\n");
		exit(EXIT_FAILURE);
	}

	memcpy(&io->out_buffer[io->out_size], string, len);
	io->out_size += len;

	io->out_buffer[io->out_size] = '\0';

	// append EOL to buffer
	io->out_buffer[io->out_size++] = '\n';

	// flush buffer
	write(io->out_fd, io->out_buffer, io->out_size);

	io->out_size = 0;
}
