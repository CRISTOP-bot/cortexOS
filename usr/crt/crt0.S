.section .text
.global _start
.type _start,@function
_start:
	/* CortexOS builds the normal SysV initial stack: argc, argv[], envp[]. */
	mov (%rsp), %rdi
	lea 8(%rsp), %rsi
	lea 8(%rsi,%rdi,8), %rdx
	call main
	mov %eax, %edi
	mov $4, %eax
	int $0x80
1:
	hlt
	jmp 1b
.size _start, .- _start
