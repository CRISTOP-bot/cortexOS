.section .text.boot
.global _start
.type _start, %function

/* QEMU virt enters the image in EL1 with x0 containing the DTB address.
 * The first port stage only establishes a stack and writes diagnostics to the
 * PL011 UART. The DTB value is preserved for the next initialization stage. */
_start:
	msr daifset, #0xf
	ldr x1, =__stack_top
	mov sp, x1
	bl aarch64_early_main

1:
	wfe
	b 1b

.size _start, . - _start
