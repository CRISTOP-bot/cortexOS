#ifndef CORTEXOS_AARCH64_PMM_H
#define CORTEXOS_AARCH64_PMM_H

#include <stdint.h>

#define AARCH64_PAGE_SIZE 4096UL

int aarch64_pmm_init(uint64_t ram_base, uint64_t ram_size,
                     uint64_t kernel_start, uint64_t kernel_end,
                     uint64_t dtb, uint64_t dtb_size);
uint64_t aarch64_pmm_alloc_page(void);
void aarch64_pmm_reserve(uint64_t start, uint64_t size);
uint64_t aarch64_pmm_free_pages(void);

#endif
