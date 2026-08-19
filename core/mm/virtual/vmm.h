#ifndef VMM_H
#define VMM_H

#include <stdbool.h>
#include <stdint.h>

void vmm_init(unsigned long mem_lower, unsigned long mem_upper);
void vmm_map_page(unsigned long phys, unsigned long virt, unsigned int flags);
void vmm_unmap_page(unsigned long virt);
bool vmm_is_page_mapped(unsigned long virt);
unsigned long vmm_get_phys(unsigned long virt);

/* Address-space operations used by the process manager.  The returned CR3
 * values are physical addresses, identity mapped by the kernel. */
uint64_t vmm_create_address_space(void);
void vmm_destroy_address_space(uint64_t cr3);
void vmm_switch_address_space(uint64_t cr3);
uint64_t vmm_current_address_space(void);
bool vmm_map_page_in(uint64_t cr3, unsigned long phys, unsigned long virt,
                    unsigned int flags);
void vmm_unmap_page_in(uint64_t cr3, unsigned long virt);
bool vmm_is_page_mapped_in(uint64_t cr3, unsigned long virt);
unsigned long vmm_get_phys_in(uint64_t cr3, unsigned long virt);
unsigned int vmm_get_flags_in(uint64_t cr3, unsigned long virt);
bool vmm_user_range_valid(uint64_t cr3, uint64_t address, uint64_t length,
                          bool write);

#define VMM_PRESENT       0x01
#define VMM_WRITE         0x02
#define VMM_USER          0x04
#define VMM_WRITE_THROUGH 0x08
#define VMM_CACHE_DISABLE 0x10

#endif /* VMM_H */
