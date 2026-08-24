#include "cortex_package.h"
#include "shell.h"
#include "vfs.h"
#include "console.h"
#include "kstring.h"
#include <stddef.h>
#include <stdint.h>

#define CORTEX_PACKAGE_MAGIC "CORTEXPK"
#define CORTEX_PACKAGE_HEADER 18u
#define CORTEX_PACKAGE_VERSION 1u
#define CORTEX_PACKAGE_MAX_MANIFEST 4096u
#define CORTEX_PACKAGE_MAX_COMMANDS 32

static uint16_t package_u16(const unsigned char *p)
{
	return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8);
}

static uint32_t package_u32(const unsigned char *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
		((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int package_line(char *line, size_t size, const char **cursor)
{
	const char *p = *cursor;
	size_t n = 0;
	while (*p && *p != '\n' && n + 1 < size)
		line[n++] = *p++;
	line[n] = '\0';
	if (*p == '\n')
		++p;
	*cursor = p;
	return n != 0;
}

static int package_command(const char *line)
{
	const char *value = line;
	if (kstrlen(line) > 8 && line[0] == 'c' && line[1] == 'o' &&
		line[2] == 'm' && line[3] == 'm' && line[4] == 'a' && line[5] == 'n' &&
		line[6] == 'd' && line[7] == '=') {
		value = line + 8;
	} else {
		return 0;
	}
	while (*value == ' ' || *value == '\t')
		++value;
	if (!*value)
		return 0;
	return shell_execute_line(value);
}

int cortex_package_run(const char *path)
{
	const unsigned char *data = (const unsigned char *)vfs_read(path);
	size_t size = vfs_get_size(path);
	char manifest[CORTEX_PACKAGE_MAX_MANIFEST + 1];
	const char *cursor;
	char line[256];
	uint32_t manifest_size, payload_size;
	uint16_t version;
	int commands = 0;
	int result = 0;

	if (!data || size < CORTEX_PACKAGE_HEADER || !path)
		return -1;
	for (unsigned int i = 0; i < 8; ++i)
		if (data[i] != CORTEX_PACKAGE_MAGIC[i])
			return -2;
	version = package_u16(data + 8);
	manifest_size = package_u32(data + 10);
	payload_size = package_u32(data + 14);
	(void)payload_size;
	if (version != CORTEX_PACKAGE_VERSION || manifest_size > CORTEX_PACKAGE_MAX_MANIFEST ||
		(size < 18u) || (uint64_t)18u + manifest_size + payload_size > size)
		return -3;
	for (uint32_t i = 0; i < manifest_size; ++i)
		manifest[i] = (char)data[18u + i];
	manifest[manifest_size] = '\0';
	cursor = manifest;
	while (*cursor && commands < CORTEX_PACKAGE_MAX_COMMANDS) {
		if (!package_line(line, sizeof(line), &cursor))
			continue;
		if (line[0] == '#' || line[0] == '\r')
			continue;
		if (package_command(line) < 0)
			result = -4;
		++commands;
	}
	if (*cursor)
		return -5;
	return result;
}

void cortex_package_help(void)
{
	console_print("Usage: pkg run <file.cortex>\n");
	console_print("Format: CORTEXPK v1, manifest commands, optional payload\n");
}
