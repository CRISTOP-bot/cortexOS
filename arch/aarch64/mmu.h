#ifndef NUCLEOS_AARCH64_MMU_H
#define NUCLEOS_AARCH64_MMU_H

#include <stdint.h>

int aarch64_mmu_init(uint64_t ram_base, uint64_t ram_size);

#endif
