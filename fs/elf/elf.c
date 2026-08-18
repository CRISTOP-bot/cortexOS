#include "elf.h"

#include <stdint.h>

static bool elf_range_ok(uint64_t offset, uint64_t length, uint64_t total)
{
	return offset <= total && length <= total - offset;
}

static void elf_copy(unsigned char *dst, const unsigned char *src, uint64_t size)
{
	for (uint64_t i = 0; i < size; ++i)
		dst[i] = src[i];
}

static void elf_zero(unsigned char *dst, uint64_t size)
{
	for (uint64_t i = 0; i < size; ++i)
		dst[i] = 0;
}

bool elf_load_image(const void *image, size_t image_size,
			uint64_t mapped_start, uint64_t mapped_end,
			uint64_t *entry_point)
{
	const unsigned char *bytes = (const unsigned char *)image;
	const struct elf64_header *header;
	uint64_t highest_loaded = 0;
	bool loaded = false;

	if (!image || image_size < sizeof(struct elf64_header) || !entry_point)
		return false;

	header = (const struct elf64_header *)image;
	if (header->ident[0] != 0x7f || header->ident[1] != 'E' ||
	    header->ident[2] != 'L' || header->ident[3] != 'F' ||
	    header->ident[4] != ELF_CLASS_64 || header->ident[5] != ELF_DATA_LSB ||
	    header->ident[6] != ELF_VERSION_CURRENT ||
	    header->type != ELF_TYPE_EXEC ||
	    header->machine != ELF_MACHINE_X86_64 ||
	    header->version != ELF_VERSION_CURRENT)
		return false;

	if (header->header_size < sizeof(struct elf64_header) ||
	    header->program_header_size < sizeof(struct elf64_program_header) ||
	    header->program_header_count == 0 ||
	    !elf_range_ok(header->program_header_offset,
			(uint64_t)header->program_header_size * header->program_header_count,
			image_size))
		return false;

	for (uint16_t i = 0; i < header->program_header_count; ++i) {
		uint64_t ph_offset = header->program_header_offset +
			(uint64_t)i * header->program_header_size;
		const struct elf64_program_header *ph =
			(const struct elf64_program_header *)(bytes + ph_offset);
		uint64_t segment_end;
		unsigned char *destination;

		/* CortexOS currently has no dynamic linker.  Refuse PT_INTERP
		 * explicitly instead of silently loading an unusable image. */
		if (ph->type == ELF_PROGRAM_INTERP)
			return false;
		if (ph->type != ELF_PROGRAM_LOAD)
			continue;
		if (ph->file_size == 0 && ph->memory_size == 0)
			continue;
		if (ph->alignment > 1 && (ph->alignment & (ph->alignment - 1)) != 0)
			return false;
		if (ph->alignment > 1 &&
			(ph->virtual_address % ph->alignment) != (ph->offset % ph->alignment))
			return false;
		if (ph->memory_size < ph->file_size ||
		    !elf_range_ok(ph->offset, ph->file_size, image_size))
			return false;
		if (ph->virtual_address < mapped_start ||
		    ph->virtual_address >= mapped_end ||
		    ph->memory_size > mapped_end - ph->virtual_address)
			return false;

		segment_end = ph->virtual_address + ph->memory_size;
		destination = (unsigned char *)(uintptr_t)ph->virtual_address;
		elf_zero(destination, ph->memory_size);
		elf_copy(destination, bytes + ph->offset, ph->file_size);
		if (segment_end > highest_loaded)
			highest_loaded = segment_end;
		loaded = true;
	}

	if (!loaded || header->entry < mapped_start || header->entry >= highest_loaded)
		return false;
	*entry_point = header->entry;
	return true;
}
