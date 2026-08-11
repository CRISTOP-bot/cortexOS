#ifndef NUCLEOS_SYSCALL_H
#define NUCLEOS_SYSCALL_H

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

static inline long __nucleos_syscall5(long number, long a1, long a2,
				      long a3, long a4, long a5)
{
	register long r10 asm("r10") = a4;
	register long r8 asm("r8") = a5;
	asm volatile("int $0x80"
		: "+a"(number)
		: "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
		: "rcx", "r11", "memory");
	return number;
}

#define __nucleos_syscall0(n) __nucleos_syscall5((n), 0, 0, 0, 0, 0)
#define __nucleos_syscall1(n,a) __nucleos_syscall5((n), (long)(a), 0, 0, 0, 0)
#define __nucleos_syscall2(n,a,b) __nucleos_syscall5((n), (long)(a), (long)(b), 0, 0, 0)
#define __nucleos_syscall3(n,a,b,c) __nucleos_syscall5((n), (long)(a), (long)(b), (long)(c), 0, 0)

#endif
