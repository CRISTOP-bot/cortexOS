#include "pmm.h"

/* Bitmap capacity: one GiB of 4 KiB pages. QEMU virt CI currently uses 128 MiB. */
#define PMM_MAX_PAGES (1UL << 18)
#define PMM_BITMAP_BYTES (PMM_MAX_PAGES / 8)

static uint8_t page_bitmap[PMM_BITMAP_BYTES];
static uint64_t pmm_base;
static uint64_t pmm_pages;
static uint64_t free_pages;
static uint64_t next_page;

static uint64_t align_down(uint64_t value)
{
	return value & ~(AARCH64_PAGE_SIZE - 1);
}

static uint64_t align_up(uint64_t value)
{
	return (value + AARCH64_PAGE_SIZE - 1) & ~(AARCH64_PAGE_SIZE - 1);
}

static void mark_used(uint64_t index)
{
	uint8_t mask;
	if (index >= pmm_pages)
		return;
	mask = (uint8_t)(1U << (index & 7));
	if (!(page_bitmap[index >> 3] & mask)) {
		page_bitmap[index >> 3] |= mask;
		if (free_pages)
			free_pages--;
	}
}

static void mark_free(uint64_t index)
{
	uint8_t mask;
	if (index >= pmm_pages)
		return;
	mask = (uint8_t)(1U << (index & 7));
	if (page_bitmap[index >> 3] & mask) {
		page_bitmap[index >> 3] &= (uint8_t)~mask;
		free_pages++;
	}
}

void aarch64_pmm_reserve(uint64_t start, uint64_t size)
{
	uint64_t first;
	uint64_t last;
	uint64_t index;

	if (!size || start + size < start)
		return;
	first = start < pmm_base ? pmm_base : align_down(start);
	last = align_up(start + size);
	if (last < first)
		return;
	for (index = first; index < last; index += AARCH64_PAGE_SIZE) {
		if (index >= pmm_base && index - pmm_base < pmm_pages * AARCH64_PAGE_SIZE)
			mark_used((index - pmm_base) / AARCH64_PAGE_SIZE);
	}
}

int aarch64_pmm_init(uint64_t ram_base, uint64_t ram_size,
                     uint64_t kernel_start, uint64_t kernel_end,
                     uint64_t dtb, uint64_t dtb_size)
{
	uint64_t i;
	uint64_t max_bytes = PMM_MAX_PAGES * AARCH64_PAGE_SIZE;

	if (!ram_size || ram_base + ram_size < ram_base || ram_size > max_bytes)
		return -1;
	pmm_base = align_up(ram_base);
	pmm_pages = (ram_size - (pmm_base - ram_base)) / AARCH64_PAGE_SIZE;
	if (!pmm_pages)
		return -1;

	for (i = 0; i < PMM_BITMAP_BYTES; i++)
		page_bitmap[i] = 0xff;
	free_pages = 0;
	for (i = 0; i < pmm_pages; i++)
		mark_free(i);
	next_page = 0;

	/* The image includes the bitmap, early stack and all static data. */
	aarch64_pmm_reserve(kernel_start, kernel_end - kernel_start);
	if (dtb_size)
		aarch64_pmm_reserve(dtb, dtb_size);
	return 0;
}

uint64_t aarch64_pmm_alloc_page(void)
{
	uint64_t scanned;
	for (scanned = 0; scanned < pmm_pages; scanned++) {
		uint64_t index = (next_page + scanned) % pmm_pages;
		if (!(page_bitmap[index >> 3] & (1U << (index & 7)))) {
			mark_used(index);
			next_page = (index + 1) % pmm_pages;
			return pmm_base + index * AARCH64_PAGE_SIZE;
		}
	}
	return 0;
}

uint64_t aarch64_pmm_free_pages(void)
{
	return free_pages;
}
