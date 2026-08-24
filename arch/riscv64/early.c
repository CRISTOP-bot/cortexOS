#include <stdint.h>

#define UART0 0x10000000UL

static void uart_putc(char c)
{
	volatile uint8_t *uart = (volatile uint8_t *)(uintptr_t)UART0;
	while (!(uart[5] & 0x20)) {}
	uart[0] = (uint8_t)c;
}

static void uart_print(const char *text)
{
	while (*text) {
		if (*text == '\n') uart_putc('\r');
		uart_putc(*text++);
	}
}

void riscv64_early_main(void)
{
	uart_print("CortexOS RISC-V64 early boot\n");
	uart_print("UART: initialized\n");
	uart_print("RISC-V64 early stage complete\n");
}
