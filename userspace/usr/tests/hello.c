#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv, char **envp)
{
	char buffer[32];
	int fd;
	(void)envp;
	printf("hello from CortexOS (argc=%d)\n", argc);
	if (argc > 1)
		printf("argv[1]=%s\n", argv[1]);
	fd = open("/etc/hostname", 0);
	if (fd < 0)
		return 2;
	if (read(fd, buffer, sizeof(buffer) - 1) > 0) {
		buffer[sizeof(buffer) - 1] = '\0';
		write(1, buffer, strlen(buffer));
	}
	close(fd);
	return 0;
}
