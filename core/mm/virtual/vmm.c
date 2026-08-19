#include "vmm.h"
#include "pmm.h"
#include "kstring.h"
#include "console.h"
#include <stdint.h>

#define PRESENT (1ULL << 0)
#define WRITE   (1ULL << 1)
#define USER    (1ULL << 2)
#define PS      (1ULL << 7)
#define ENTRY_FLAGS 0x1FULL
#define PAGE_MASK (~0xFFFULL)

static inline uint64_t *current_pml4(void)
{
	uint64_t cr3;
	__asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
	return (uint64_t *)(cr3 & PAGE_MASK);
}

static inline uint64_t current_cr3(void)
{
	uint64_t cr3;
	__asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
	return cr3 & PAGE_MASK;
}

static inline void invlpg(unsigned long addr)
{
	__asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

static uint64_t *table_for(uint64_t cr3)
{
	return (uint64_t *)(uintptr_t)(cr3 & PAGE_MASK);
}

static bool indices(unsigned long virt, unsigned int *pml4, unsigned int *pdp,
			unsigned int *pd, unsigned int *pt)
{
	/* Reject non-canonical addresses instead of truncating them into a
	 * different process mapping. CortexOS currently uses the low half. */
	if (virt >> 47)
		return false;
	*pml4 = (virt >> 39) & 0x1FF;
	*pdp = (virt >> 30) & 0x1FF;
	*pd = (virt >> 21) & 0x1FF;
	*pt = (virt >> 12) & 0x1FF;
	return true;
}

static uint64_t *alloc_table(void)
{
	uint64_t *table = (uint64_t *)pmm_alloc_page();
	if (table)
		kmemset(table, 0, PAGE_SIZE);
	return table;
}

static bool map_page_root(uint64_t cr3, unsigned long phys, unsigned long virt,
			  unsigned int flags)
{
	unsigned int pml4_idx, pdp_idx, pd_idx, pt_idx;
	uint64_t *pml4, *pdp, *pd, *pt;
	uint64_t entry_flags;

	if (!indices(virt, &pml4_idx, &pdp_idx, &pd_idx, &pt_idx))
		return false;
	pml4 = table_for(cr3);
	entry_flags = (uint64_t)(flags & ENTRY_FLAGS) | PRESENT;
	if (!(pml4[pml4_idx] & PRESENT)) {
		pdp = alloc_table();
		if (!pdp) return false;
		pml4[pml4_idx] = (uint64_t)(uintptr_t)pdp | entry_flags;
	}
	pdp = (uint64_t *)(uintptr_t)(pml4[pml4_idx] & PAGE_MASK);
	if (!(pdp[pdp_idx] & PRESENT)) {
		pd = alloc_table();
		if (!pd) return false;
		pdp[pdp_idx] = (uint64_t)(uintptr_t)pd | entry_flags;
	}
	pd = (uint64_t *)(uintptr_t)(pdp[pdp_idx] & PAGE_MASK);
	if (pd[pd_idx] & PS) {
		uint64_t base_phys = pd[pd_idx] & ~0x3FFFFFULL;
		uint64_t base_flags = pd[pd_idx] & ENTRY_FLAGS;
		pt = alloc_table();
		if (!pt) return false;
		for (unsigned int i = 0; i < 512; ++i)
			pt[i] = (base_phys + (uint64_t)i * PAGE_SIZE) | base_flags;
		pd[pd_idx] = (uint64_t)(uintptr_t)pt | (base_flags & ~PS);
	}
	if (!(pd[pd_idx] & PRESENT)) {
		pt = alloc_table();
		if (!pt) return false;
		pd[pd_idx] = (uint64_t)(uintptr_t)pt | entry_flags;
	}
	pt = (uint64_t *)(uintptr_t)(pd[pd_idx] & PAGE_MASK);
	pt[pt_idx] = (phys & PAGE_MASK) | entry_flags;
	if (cr3 == current_cr3())
		invlpg(virt);
	return true;
}

static uint64_t lookup_entry(uint64_t cr3, unsigned long virt,
			     unsigned int *flags_out)
{
	unsigned int pml4_idx, pdp_idx, pd_idx, pt_idx;
	uint64_t *pml4, *pdp, *pd, *pt, entry;
	if (!indices(virt, &pml4_idx, &pdp_idx, &pd_idx, &pt_idx)) return 0;
	pml4 = table_for(cr3);
	if (!(pml4[pml4_idx] & PRESENT)) return 0;
	pdp = (uint64_t *)(uintptr_t)(pml4[pml4_idx] & PAGE_MASK);
	if (!(pdp[pdp_idx] & PRESENT)) return 0;
	pd = (uint64_t *)(uintptr_t)(pdp[pdp_idx] & PAGE_MASK);
	if (!(pd[pd_idx] & PRESENT)) return 0;
	if (pd[pd_idx] & PS) {
		entry = pd[pd_idx];
		if (flags_out) *flags_out = (unsigned int)(entry & ENTRY_FLAGS);
		return (entry & ~0x3FFFFFULL) | (virt & 0x3FFFFFULL);
	}
	pt = (uint64_t *)(uintptr_t)(pd[pd_idx] & PAGE_MASK);
	entry = pt[pt_idx];
	if (!(entry & PRESENT)) return 0;
	if (flags_out) *flags_out = (unsigned int)(entry & ENTRY_FLAGS);
	return (entry & PAGE_MASK) | (virt & 0xFFFULL);
}

void vmm_init(unsigned long mem_lower, unsigned long mem_upper)
{
	(void)mem_lower;
	unsigned long total_bytes = (1024 + mem_upper) * 1024;
	uint64_t *pml4 = current_pml4();
	if (!(pml4[0] & PRESENT)) {
		console_print("[VMM] ERROR: PML4[0] not present\n");
		return;
	}
	uint64_t *pdp = (uint64_t *)(uintptr_t)(pml4[0] & PAGE_MASK);
	if (!(pdp[0] & PRESENT)) {
		console_print("[VMM] ERROR: PDPT[0] not present\n");
		return;
	}
	uint64_t *pd = (uint64_t *)(uintptr_t)(pdp[0] & PAGE_MASK);
	unsigned long num_2mb = (total_bytes + 0x1FFFFF) / 0x200000;
	if (num_2mb > 512) num_2mb = 512;
	for (unsigned long i = 8; i < num_2mb; ++i)
		if (!(pd[i] & PRESENT)) pd[i] = (i * 0x200000) | 0x83;
	char buf[32];
	console_print("[VMM] Extended identity map to ");
	kitoa((long)(num_2mb * 2), buf, sizeof(buf));
	console_print(buf);
	console_print(" MB\n");
}

uint64_t vmm_create_address_space(void)
{
	uint64_t *pml4 = alloc_table();
	uint64_t *pdp = alloc_table();
	uint64_t *pd = alloc_table();
	unsigned long total_bytes = pmm_get_total_bytes();
	unsigned long num_2mb = (total_bytes + 0x1FFFFF) / 0x200000;
	if (!pml4 || !pdp || !pd) return 0;
	if (num_2mb > 512) num_2mb = 512;
	pml4[0] = (uint64_t)(uintptr_t)pdp | PRESENT | WRITE;
	pdp[0] = (uint64_t)(uintptr_t)pd | PRESENT | WRITE;
	for (unsigned long i = 0; i < num_2mb; ++i)
		pd[i] = (i * 0x200000ULL) | PRESENT | WRITE | PS;
	return (uint64_t)(uintptr_t)pml4;
}

static void destroy_pd(uint64_t *pd)
{
	for (unsigned int i = 0; i < 512; ++i) {
		if (!(pd[i] & PRESENT) || (pd[i] & PS)) continue;
		uint64_t *pt = (uint64_t *)(uintptr_t)(pd[i] & PAGE_MASK);
		for (unsigned int j = 0; j < 512; ++j) {
			if ((pt[j] & (PRESENT | USER)) == (PRESENT | USER))
				pmm_free_page((void *)(uintptr_t)(pt[j] & PAGE_MASK));
		}
		pmm_free_page(pt);
	}
	pmm_free_page(pd);
}

void vmm_destroy_address_space(uint64_t cr3)
{
	if (!cr3 || cr3 == current_cr3()) return;
	uint64_t *pml4 = table_for(cr3);
	for (unsigned int i = 0; i < 512; ++i) {
		if (!(pml4[i] & PRESENT)) continue;
		uint64_t *pdp = (uint64_t *)(uintptr_t)(pml4[i] & PAGE_MASK);
		for (unsigned int j = 0; j < 512; ++j) {
			if (!(pdp[j] & PRESENT)) continue;
			uint64_t *pd = (uint64_t *)(uintptr_t)(pdp[j] & PAGE_MASK);
			destroy_pd(pd);
		}
		pmm_free_page(pdp);
	}
	pmm_free_page(pml4);
}

void vmm_switch_address_space(uint64_t cr3)
{
	if (!cr3 || cr3 == current_cr3()) return;
	__asm__ volatile("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

uint64_t vmm_current_address_space(void) { return current_cr3(); }

bool vmm_map_page_in(uint64_t cr3, unsigned long phys, unsigned long virt,
			     unsigned int flags)
{ return cr3 && map_page_root(cr3, phys, virt, flags); }

void vmm_map_page(unsigned long phys, unsigned long virt, unsigned int flags)
{ (void)map_page_root(current_cr3(), phys, virt, flags); }

void vmm_unmap_page_in(uint64_t cr3, unsigned long virt)
{
	unsigned int pml4_idx, pdp_idx, pd_idx, pt_idx;
	uint64_t *pml4, *pdp, *pd, *pt;
	if (!cr3 || !indices(virt, &pml4_idx, &pdp_idx, &pd_idx, &pt_idx)) return;
	pml4 = table_for(cr3);
	if (!(pml4[pml4_idx] & PRESENT)) return;
	pdp = (uint64_t *)(uintptr_t)(pml4[pml4_idx] & PAGE_MASK);
	if (!(pdp[pdp_idx] & PRESENT)) return;
	pd = (uint64_t *)(uintptr_t)(pdp[pdp_idx] & PAGE_MASK);
	if (!(pd[pd_idx] & PRESENT) || (pd[pd_idx] & PS)) return;
	pt = (uint64_t *)(uintptr_t)(pd[pd_idx] & PAGE_MASK);
	pt[pt_idx] = 0;
	if (cr3 == current_cr3()) invlpg(virt);
}

void vmm_unmap_page(unsigned long virt)
{ vmm_unmap_page_in(current_cr3(), virt); }

bool vmm_is_page_mapped_in(uint64_t cr3, unsigned long virt)
{ return lookup_entry(cr3, virt, 0) != 0; }

bool vmm_is_page_mapped(unsigned long virt)
{ return vmm_is_page_mapped_in(current_cr3(), virt); }

unsigned long vmm_get_phys_in(uint64_t cr3, unsigned long virt)
{ return (unsigned long)lookup_entry(cr3, virt, 0); }

unsigned long vmm_get_phys(unsigned long virt)
{ return vmm_get_phys_in(current_cr3(), virt); }

unsigned int vmm_get_flags_in(uint64_t cr3, unsigned long virt)
{
	unsigned int flags = 0;
	(void)lookup_entry(cr3, virt, &flags);
	return flags;
}

bool vmm_user_range_valid(uint64_t cr3, uint64_t address, uint64_t length,
			      bool write)
{
	uint64_t end;
	if (!cr3 || !length || address >= (1ULL << 47) ||
		length - 1 > UINT64_MAX - address)
		return false;
	end = address + length - 1;
	if (end >= (1ULL << 47)) return false;
	for (uint64_t page = address & PAGE_MASK;; page += PAGE_SIZE) {
		unsigned int flags = vmm_get_flags_in(cr3, (unsigned long)page);
		if ((flags & (VMM_PRESENT | VMM_USER)) != (VMM_PRESENT | VMM_USER) ||
		    (write && !(flags & VMM_WRITE))) return false;
		if (page >= (end & PAGE_MASK)) break;
		if (page > UINT64_MAX - PAGE_SIZE) return false;
	}
	return true;
}
