.section .text.user, "ax"
.global aarch64_user_program
.type aarch64_user_program, %function

aarch64_user_program:
	adr x0, aarch64_user_message
	mov x1, #19
	mov x8, #1              /* SYS_write */
	svc #0
	mov x8, #2              /* SYS_ticks */
	svc #0
1:
	wfe
	b 1b

.size aarch64_user_program, . - aarch64_user_program

.section .rodata.user, "a"
aarch64_user_message:
	.asciz "CortexOS EL0 online\n"
