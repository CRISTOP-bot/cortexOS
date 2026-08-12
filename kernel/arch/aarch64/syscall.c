#include <stdint.h>

#include "gic.h"

#define SYS_WRITE 1
#define SYS_TICKS 2

void aarch64_uart_write(const char *text, uint64_t length);

uint64_t aarch64_syscall_c(uint64_t number, uint64_t arg0,
                           uint64_t arg1, uint64_t arg2)
{
	(void)arg2;
	if (number == SYS_WRITE) {
		if (arg1 > 256)
			arg1 = 256;
		aarch64_uart_write((const char *)(uintptr_t)arg0, arg1);
		return arg1;
	}
	if (number == SYS_TICKS)
		return aarch64_ticks();
	return (uint64_t)-38; /* ENOSYS */
}
