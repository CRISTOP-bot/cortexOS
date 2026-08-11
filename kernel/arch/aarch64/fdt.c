#include "fdt.h"

#define FDT_MAGIC       0xd00dfeedU
#define FDT_BEGIN_NODE  1U
#define FDT_END_NODE    2U
#define FDT_PROP        3U
#define FDT_NOP         4U
#define FDT_END         9U

struct fdt_header {
	uint32_t magic;
	uint32_t totalsize;
	uint32_t off_dt_struct;
	uint32_t off_dt_strings;
	uint32_t off_mem_rsvmap;
	uint32_t version;
	uint32_t last_comp_version;
	uint32_t boot_cpuid_phys;
	uint32_t size_dt_strings;
	uint32_t size_dt_struct;
};

static uint32_t be32(const void *ptr)
{
	const uint8_t *p = (const uint8_t *)ptr;
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	       ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint64_t be64(const void *ptr)
{
	const uint8_t *p = (const uint8_t *)ptr;
	return ((uint64_t)be32(p) << 32) | be32(p + 4);
}

static uint32_t align4(uint32_t value)
{
	return (value + 3U) & ~3U;
}

static int bounded(uint32_t offset, uint32_t length, uint32_t limit)
{
	return offset <= limit && length <= limit - offset;
}

static int string_has(const uint8_t *data, uint32_t length, const char *needle)
{
	uint32_t i;
	uint32_t n = 0;

	while (needle[n])
		n++;
	if (!n)
		return 1;
	for (i = 0; i + n <= length; i++) {
		uint32_t j;
		for (j = 0; j < n && data[i + j] == (uint8_t)needle[j]; j++)
			;
		if (j == n)
			return 1;
	}
	return 0;
}

static int node_is(const char *name, const char *prefix)
{
	uint32_t i = 0;
	while (prefix[i] && name[i] == prefix[i])
		i++;
	return prefix[i] == '\0';
}

int aarch64_fdt_parse(uint64_t address, aarch64_fdt_info_t *info)
{
	const struct fdt_header *header = (const struct fdt_header *)(uintptr_t)address;
	const uint8_t *structure;
	const uint8_t *strings;
	uint32_t total;
	uint32_t struct_offset;
	uint32_t struct_size;
	uint32_t strings_offset;
	uint32_t strings_size;
	uint32_t cursor;
	uint32_t end;
	char node_name[96];
	int compatible_uart = 0;
	int compatible_gic = 0;
	int pending_reg = 0;
	uint64_t pending_reg_base = 0;
	uint64_t pending_reg_size = 0;
	uint64_t pending_reg_extra = 0;

	info->valid = 0;
	info->has_uart = 0;
	info->has_memory = 0;
	info->has_gic = 0;
	info->uart_base = 0;
	info->ram_base = 0;
	info->ram_size = 0;
	info->gic_dist_base = 0;
	info->gic_redist_base = 0;

	if (!header || be32(&header->magic) != FDT_MAGIC)
		return -1;

	total = be32(&header->totalsize);
	struct_offset = be32(&header->off_dt_struct);
	struct_size = be32(&header->size_dt_struct);
	strings_offset = be32(&header->off_dt_strings);
	strings_size = be32(&header->size_dt_strings);
	if (total < sizeof(*header) ||
	    !bounded(struct_offset, struct_size, total) ||
	    !bounded(strings_offset, strings_size, total))
		return -1;

	structure = (const uint8_t *)(uintptr_t)(address + struct_offset);
	strings = (const uint8_t *)(uintptr_t)(address + strings_offset);
	cursor = 0;
	end = struct_size;
	node_name[0] = '\0';

	while (cursor + 4 <= end) {
		uint32_t token = be32(structure + cursor);
		cursor += 4;

		if (token == FDT_BEGIN_NODE) {
			uint32_t start = cursor;
			uint32_t length = 0;
			while (cursor < end && structure[cursor]) {
				cursor++;
				length++;
			}
			if (cursor >= end || length >= sizeof(node_name))
				return -1;
			for (uint32_t i = 0; i < length; i++)
				node_name[i] = (char)structure[start + i];
			node_name[length] = '\0';
			cursor = align4(cursor + 1);
			compatible_uart = 0;
			compatible_gic = 0;
			pending_reg = 0;
			continue;
		}

		if (token == FDT_END_NODE) {
			node_name[0] = '\0';
			compatible_uart = 0;
			compatible_gic = 0;
			continue;
		}

		if (token == FDT_NOP)
			continue;
		if (token == FDT_END)
			break;
		if (token != FDT_PROP || cursor + 8 > end)
			return -1;

		{
			uint32_t length = be32(structure + cursor);
			uint32_t name_offset = be32(structure + cursor + 4);
			const uint8_t *value = structure + cursor + 8;
			const char *property;
			if (!bounded(cursor + 8 - 0, length, end))
				return -1;
			if (name_offset >= strings_size)
				return -1;
			property = (const char *)(strings + name_offset);
			cursor = align4(cursor + 8 + length);

			if (property[0] == 'c' && property[1] == 'o' &&
			    property[2] == 'm' && property[3] == 'p' &&
			    property[4] == 'a' && property[5] == 't' &&
			    property[6] == 'i' && property[7] == 'b' &&
			    property[8] == 'l' && property[9] == 'e') {
				compatible_uart = string_has(value, length, "arm,pl011");
				compatible_gic = string_has(value, length, "arm,gic-v3") ||
				                string_has(value, length, "arm,gic-400");
				if (pending_reg && compatible_uart) {
					info->uart_base = pending_reg_base;
					info->has_uart = 1;
				}
				if (pending_reg && compatible_gic) {
					info->gic_dist_base = pending_reg_base;
					info->gic_redist_base = pending_reg_extra;
					info->has_gic = 1;
				}
			}

			if (property[0] == 'r' && property[1] == 'e' &&
			    property[2] == 'g' && length >= 16) {
				pending_reg = 1;
				pending_reg_base = be64(value);
				pending_reg_size = be64(value + 8);
				pending_reg_extra = length >= 32 ? be64(value + 16) : 0;
				if (compatible_uart) {
					info->uart_base = pending_reg_base;
					info->has_uart = 1;
				} else if (compatible_gic) {
					info->gic_dist_base = pending_reg_base;
					info->gic_redist_base = pending_reg_extra;
					info->has_gic = 1;
				} else if (node_is(node_name, "memory")) {
					info->ram_base = pending_reg_base;
					info->ram_size = pending_reg_size;
					info->has_memory = 1;
				}
			}
		}
	}

	info->valid = 1;
	return 0;
}
