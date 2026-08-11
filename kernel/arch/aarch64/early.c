#include <stdint.h>

#define PL011_BASE 0x09000000UL
#define UART_DR    (*(volatile uint32_t *)(PL011_BASE + 0x00))
#define UART_FR    (*(volatile uint32_t *)(PL011_BASE + 0x18))
#define UART_FR_TXFF (1U << 5)

static void uart_putc(char c)
{
	while (UART_FR & UART_FR_TXFF)
		;
	UART_DR = (uint32_t)c;
}

static void uart_puts(const char *text)
{
	while (*text) {
		if (*text == '\n')
			uart_putc('\r');
		uart_putc(*text++);
	}
}

void aarch64_early_main(void)
{
	uart_puts("NucleOS AArch64 early boot\n");
	uart_puts("EL1 stack and PL011 console initialized\n");
	uart_puts("Next: DTB, exception vectors, MMU and timer\n");
}
