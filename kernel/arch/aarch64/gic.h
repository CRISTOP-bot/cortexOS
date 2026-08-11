#ifndef NUCLEOS_AARCH64_GIC_H
#define NUCLEOS_AARCH64_GIC_H

#include <stdint.h>

int aarch64_gicv3_init(uint64_t distributor, uint64_t redistributor);
void aarch64_irq_c(uint64_t interrupt_id);

#endif
