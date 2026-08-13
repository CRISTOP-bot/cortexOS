#include "pmm.h"
#include "kstring.h"
#include "console.h"

#define MAX_PAGES (1024 * 1024)
#define BITMAP_SIZE (MAX_PAGES / 32)

extern char _kernel_start[];
extern char _kernel_end[];

static unsigned int pmm_bitmap[BITMAP_SIZE];
static unsigned long pmm_total_pages;
static unsigned long pmm_free_count;

#define PMM_MAX_RESERVED_RANGES 32
struct pmm_reserved_range {
	unsigned long start;
	unsigned long end;
};
static struct pmm_reserved_range pmm_reserved[PMM_MAX_RESERVED_RANGES];
static unsigned long pmm_reserved_count;

static inline void bitmap_set(unsigned long page)
{
	pmm_bitmap[page / 32] |= (1U << (page % 32));
}

static inline void bitmap_clear(unsigned long page)
{
	pmm_bitmap[page / 32] &= ~(1U << (page % 32));
}

static inline int bitmap_test(unsigned long page)
{
	return (pmm_bitmap[page / 32] >> (page % 32)) & 1;
}

void pmm_reserve_range(unsigned long start, unsigned long size)
{
	if (!size || pmm_reserved_count >= PMM_MAX_RESERVED_RANGES)
		return;
	unsigned long end = start + size;
	if (end < start)
		return;
	pmm_reserved[pmm_reserved_count].start = start;
	pmm_reserved[pmm_reserved_count].end = end;
	pmm_reserved_count++;
}

void pmm_init(unsigned long mem_lower, unsigned long mem_upper)
{
	(void)mem_lower;
	unsigned long total_ram = 1024 + mem_upper;
	pmm_total_pages = total_ram / PAGE_SIZE;
	if (pmm_total_pages > MAX_PAGES)
		pmm_total_pages = MAX_PAGES;

	pmm_free_count = 0;

	kmemset(pmm_bitmap, 0xFF, sizeof(pmm_bitmap));

	unsigned long kernel_end_page = ((unsigned long)_kernel_end + PAGE_SIZE - 1) / PAGE_SIZE;

	for (unsigned long i = 0; i < kernel_end_page; ++i)
		bitmap_set(i);

	for (unsigned long i = 0x9FC00 / PAGE_SIZE; i < 0x100000 / PAGE_SIZE; ++i)
		bitmap_set(i);

	for (unsigned long i = kernel_end_page; i < pmm_total_pages; ++i) {
		if (bitmap_test(i)) {
			bitmap_clear(i);
			pmm_free_count++;
		}
	}

	/* Apply bootloader reservations after making the rest of RAM available. */
	for (unsigned long r = 0; r < pmm_reserved_count; ++r) {
		unsigned long first = pmm_reserved[r].start / PAGE_SIZE;
		unsigned long last = (pmm_reserved[r].end + PAGE_SIZE - 1) / PAGE_SIZE;
		if (first >= pmm_total_pages)
			continue;
		if (last > pmm_total_pages)
			last = pmm_total_pages;
		for (unsigned long i = first; i < last; ++i) {
			if (!bitmap_test(i)) {
				bitmap_set(i);
				pmm_free_count--;
			}
		}
	}

	char buf[32];
	console_print("[PMM] Total RAM: ");
	kitoa(total_ram / 1024, buf, sizeof(buf));
	console_print(buf);
	console_print(" MB, ");
	kitoa(pmm_total_pages, buf, sizeof(buf));
	console_print(buf);
	console_print(" pages (4KB)\n");
}

void *pmm_alloc_pages(unsigned long count)
{
	if (!count || count > pmm_free_count)
		return 0;
	for (unsigned long start = 0; start + count <= pmm_total_pages; ++start) {
		unsigned long i;
		for (i = 0; i < count; ++i) {
			if (bitmap_test(start + i))
				break;
		}
		if (i != count)
			continue;
		for (i = 0; i < count; ++i) {
			bitmap_set(start + i);
			kmemset((void *)((start + i) * PAGE_SIZE), 0, PAGE_SIZE);
		}
		pmm_free_count -= count;
		return (void *)(start * PAGE_SIZE);
	}
	return 0;
}

void *pmm_alloc_page(void)
{
	return pmm_alloc_pages(1);
}

void pmm_free_page(void *page)
{
	if (!page)
		return;
	unsigned long addr = (unsigned long)page;
	unsigned long page_idx = addr / PAGE_SIZE;
	if (page_idx >= pmm_total_pages)
		return;
	if (bitmap_test(page_idx)) {
		bitmap_clear(page_idx);
		pmm_free_count++;
	}
}

unsigned long pmm_get_total_pages(void)
{
	return pmm_total_pages;
}

unsigned long pmm_get_free_pages(void)
{
	return pmm_free_count;
}

unsigned long pmm_get_used_pages(void)
{
	return pmm_total_pages - pmm_free_count;
}

unsigned long pmm_get_total_bytes(void)
{
	return pmm_total_pages * PAGE_SIZE;
}

unsigned long pmm_get_free_bytes(void)
{
	return pmm_free_count * PAGE_SIZE;
}
