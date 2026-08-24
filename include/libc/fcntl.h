#ifndef CORTEXOS_FCNTL_H
#define CORTEXOS_FCNTL_H

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 64
#define O_TRUNC 512
#define O_APPEND 1024
#define F_GETFL 3
#define F_SETFL 4
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

int open(const char *path, int flags, ...);
int fcntl(int fd, int command, ...);

#endif

