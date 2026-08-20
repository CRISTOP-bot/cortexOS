#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>

#define SYSCALL_READ    0
#define SYSCALL_WRITE   1
#define SYSCALL_OPEN    2
#define SYSCALL_CLOSE   3
#define SYSCALL_EXIT    4
#define SYSCALL_FORK    5
#define SYSCALL_EXEC    6
#define SYSCALL_WAIT    7
#define SYSCALL_GETPID  8
#define SYSCALL_SBRK    9
#define SYSCALL_GETCWD  10
#define SYSCALL_CHDIR   11
#define SYSCALL_PS      12
#define SYSCALL_TICKS   13
#define SYSCALL_DUP2    14
#define SYSCALL_ISATTY  15
#define SYSCALL_PIPE    16
#define SYSCALL_LSEEK   17
#define SYSCALL_STAT    18
#define SYSCALL_FCNTL   19
#define SYSCALL_WAITPID 20
#define SYSCALL_KILL    21
#define SYSCALL_SIGACTION 22
#define SYSCALL_SETSID  23
#define SYSCALL_GETSID  24
#define SYSCALL_SETPGID 25
#define SYSCALL_GETPGRP 26
#define SYSCALL_IOCTL   27
#define SYSCALL_MOUNT   28
#define SYSCALL_UMOUNT  29
#define SYSCALL_MKDIR   30
#define SYSCALL_UNLINK  31
#define SYSCALL_ACCESS  32
#define SYSCALL_GETUID  33
#define SYSCALL_GETGID  34
#define SYSCALL_MAX     36

void syscall_init(void);
int64_t syscall_handler(uint64_t rdi, uint64_t rsi, uint64_t rdx,
			uint64_t r10, uint64_t r8, uint64_t rax);

#endif

