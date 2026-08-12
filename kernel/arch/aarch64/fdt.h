#ifndef NUCLEOS_AARCH64_FDT_H
#define NUCLEOS_AARCH64_FDT_H

#include <stdint.h>

typedef struct {
	int valid;
	uint64_t uart_base;
	uint64_t ram_base;
	uint64_t ram_size;
	uint64_t gic_dist_base;
	uint64_t gic_redist_base;
	int has_uart;
	int has_memory;
	int has_gic;
} aarch64_fdt_info_t;

int aarch64_fdt_parse(uint64_t address, aarch64_fdt_info_t *info);

#endif
