#ifndef NUCLEOS_AARCH64_FDT_H
#define NUCLEOS_AARCH64_FDT_H

#include <stdint.h>

#define AARCH64_FDT_MAX_RAM_RANGES 8

typedef struct {
	uint64_t base;
	uint64_t size;
} aarch64_fdt_ram_range_t;

typedef struct {
	int valid;
	uint64_t dtb_size;
	uint64_t uart_base;
	uint64_t ram_base;
	uint64_t ram_size;
	uint32_t ram_range_count;
	aarch64_fdt_ram_range_t ram_ranges[AARCH64_FDT_MAX_RAM_RANGES];
	uint64_t gic_dist_base;
	uint64_t gic_redist_base;
	int has_uart;
	int has_memory;
	int has_gic;
} aarch64_fdt_info_t;

int aarch64_fdt_parse(uint64_t address, aarch64_fdt_info_t *info);

#endif
