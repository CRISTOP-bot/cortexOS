#ifndef ELF_H
#define ELF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ELF64 constants needed by the CortexOS user-program loader. */
#define ELF_IDENT_SIZE 16
#define ELF_CLASS_64 2
#define ELF_DATA_LSB 1
#define ELF_VERSION_CURRENT 1
#define ELF_TYPE_EXEC 2
#define ELF_TYPE_DYN 3
#define ELF_MACHINE_X86_64 62
#define ELF_PROGRAM_LOAD 1
#define ELF_PROGRAM_INTERP 3
#define ELF_PF_X 1
#define ELF_PF_W 2
#define ELF_PF_R 4

struct elf64_header {
	unsigned char ident[ELF_IDENT_SIZE];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint64_t entry;
	uint64_t program_header_offset;
	uint64_t section_header_offset;
	uint32_t flags;
	uint16_t header_size;
	uint16_t program_header_size;
	uint16_t program_header_count;
	uint16_t section_header_size;
	uint16_t section_header_count;
	uint16_t section_name_index;
} __attribute__((packed));

struct elf64_program_header {
	uint32_t type;
	uint32_t flags;
	uint64_t offset;
	uint64_t virtual_address;
	uint64_t physical_address;
	uint64_t file_size;
	uint64_t memory_size;
	uint64_t alignment;
} __attribute__((packed));

/* Load a static ET_EXEC image into its already mapped user address range. */
bool elf_load_image(const void *image, size_t image_size,
			uint64_t mapped_start, uint64_t mapped_end,
			uint64_t *entry_point);

#endif
