#ifndef CORTEXOS_SYSCALL_H
#define CORTEXOS_SYSCALL_H

#define N_SYS_READ 0
#define N_SYS_WRITE 1
#define N_SYS_OPEN 2
#define N_SYS_CLOSE 3
#define N_SYS_EXIT 4
#define N_SYS_FORK 5
#define N_SYS_EXEC 6
#define N_SYS_WAIT 7
#define N_SYS_GETPID 8
#define N_SYS_SBRK 9
#define N_SYS_GETCWD 10
#define N_SYS_CHDIR 11
#define N_SYS_DUP2 14
#define N_SYS_ISATTY 15
#define N_SYS_PIPE 16
#define N_SYS_LSEEK 17
#define N_SYS_STAT 18
#define N_SYS_FCNTL 19
#define N_SYS_WAITPID 20
#define N_SYS_KILL 21
#define N_SYS_SIGACTION 22
#define N_SYS_SETSID 23
#define N_SYS_GETSID 24
#define N_SYS_SETPGID 25
#define N_SYS_GETPGRP 26
#define N_SYS_IOCTL 27
#define N_SYS_MOUNT 28
#define N_SYS_UMOUNT 29
#define N_SYS_MKDIR 30
#define N_SYS_UNLINK 31
#define N_SYS_ACCESS 32
#define N_SYS_GETUID 33
#define N_SYS_GETGID 34
#define N_SYS_SIGRETURN 35

static inline long __cortexos_syscall5(long number, long a1, long a2,
				      long a3, long a4, long a5)
{
	register long r10 __asm__("r10") = a4;
	register long r8 __asm__("r8") = a5;
	__asm__ volatile("int $0x80"
		: "+a"(number)
		: "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
		: "rcx", "r11", "memory");
	return number;
}

#define __cortexos_syscall0(n) __cortexos_syscall5((n), 0, 0, 0, 0, 0)
#define __cortexos_syscall1(n,a) __cortexos_syscall5((n), (long)(a), 0, 0, 0, 0)
#define __cortexos_syscall2(n,a,b) __cortexos_syscall5((n), (long)(a), (long)(b), 0, 0, 0)
#define __cortexos_syscall3(n,a,b,c) __cortexos_syscall5((n), (long)(a), (long)(b), (long)(c), 0, 0)
#define __cortexos_syscall4(n,a,b,c,d) __cortexos_syscall5((n), (long)(a), (long)(b), (long)(c), (long)(d), 0)

#endif

