#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#define VFS_MAX_FDS 64
#define VFS_MAX_PIPES 16
#define VFS_PIPE_SIZE 4096

/* A descriptor table belongs to one process.  Pipe storage is shared by
 * descriptor entries, so fork/dup can preserve the required references. */
struct vfs_fd_state {
	bool used;
	int node;
	size_t offset;
	int flags;
	int pipe_id;
	bool pipe_read;
	bool pipe_write;
};
struct vfs_fd_table {
	struct vfs_fd_state fds[VFS_MAX_FDS];
};
void vfs_fd_table_init(struct vfs_fd_table *table);
int vfs_fd_table_clone(struct vfs_fd_table *dst,
		const struct vfs_fd_table *src);
void vfs_fd_table_close_all(struct vfs_fd_table *table);
bool vfs_init(void);
const char *vfs_pwd(void);
bool vfs_cd(const char *path);
bool vfs_list(const char *path);
bool vfs_cat(const char *path);
bool vfs_mkdir(const char *path);
bool vfs_touch(const char *path);
bool vfs_remove(const char *path);
bool vfs_rmdir(const char *path);
bool vfs_cp(const char *src, const char *dst);
bool vfs_mv(const char *src, const char *dst);
bool vfs_write(const char *path, const char *content, size_t length);
const void *vfs_read(const char *path);
size_t vfs_get_size(const char *path);
bool vfs_exists(const char *path);
bool vfs_is_dir(const char *path);
int vfs_get_children(const char *path, const char **names, int max_names);
int vfs_get_file_count(const char *path);
bool vfs_stat(const char *path);

/* Mount namespace scaffolding used by the early OpenRC ABI.  Only the
 * in-memory proc/sys registration is provided until a real VFS backend is
 * available; unsupported filesystems fail rather than claiming success. */
int vfs_mount(const char *source, const char *target, const char *fstype,
		unsigned long flags);
int vfs_umount(const char *target);
bool vfs_is_mounted(const char *target);

/* Kernel-side file descriptors used by the user ABI. */
#define VFS_OPEN_WRITE 0x0001
#define VFS_OPEN_RDWR  0x0002
#define VFS_OPEN_CREAT 0x0040
#define VFS_OPEN_TRUNC 0x0200
#define VFS_SEEK_SET 0
#define VFS_SEEK_CUR 1
#define VFS_SEEK_END 2
int vfs_open_fd(const char *path, int flags);
int vfs_read_fd(int fd, void *buffer, size_t count);
int vfs_write_fd(int fd, const void *buffer, size_t count);
int vfs_close_fd(int fd);
int vfs_dup_fd(int old_fd, int new_fd);
int vfs_pipe(int pipe_fds[2]);
int vfs_lseek_fd(int fd, long offset, int whence);
bool vfs_isatty_fd(int fd);
int vfs_get_fd_flags(int fd);
int vfs_set_fd_flags(int fd, int flags);
#endif /* VFS_H */

