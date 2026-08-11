#ifndef NUCLEOS_SYS_STAT_H
#define NUCLEOS_SYS_STAT_H

#include <stdint.h>

#define S_IFMT  0170000U
#define S_IFREG 0100000U
#define S_IFDIR 0040000U
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)

struct stat {
	uint64_t st_size;
	uint32_t st_mode;
	uint32_t st_nlink;
	uint32_t st_uid;
	uint32_t st_gid;
};

int stat(const char *path, struct stat *buffer);

#endif
