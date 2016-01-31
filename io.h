#define STRING_SIZE 4096
#define BUFFER_SIZE 16384

typedef struct {
	int out_fd;
	int in_fd;

	int out_size;
	int in_size;

	char in_buffer[BUFFER_SIZE];
	char out_buffer[BUFFER_SIZE];

	pid_t pid;
} io_t;

void io_create(io_t *io, const char *command);
void io_close(io_t *io);
void in_get_update(io_t *io);
void io_get(io_t *io, char string[]);
void io_send(io_t *io, char string[]);
