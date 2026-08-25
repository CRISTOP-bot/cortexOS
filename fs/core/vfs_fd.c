#include "vfs_internal.h"
#include "process.h"
#include "console.h"
#include "tty.h"
#include "fs.h"
#include "kstring.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void vfs_fd_table_init(struct vfs_fd_table *table)
{
	if (!table) return;
	for (int i = 0; i < VFS_MAX_FDS; ++i) {
		table->fds[i].used = false;
		table->fds[i].node = -1;
		table->fds[i].offset = 0;
		table->fds[i].flags = 0;
		table->fds[i].pipe_id = -1;
		table->fds[i].pipe_read = false;
		table->fds[i].pipe_write = false;
		table->fds[i].tty = false;
		table->fds[i].tty_id = -1;
	}
}

int vfs_fd_table_clone(struct vfs_fd_table *dst,
		const struct vfs_fd_table *src)
{
	if (!dst || !src) return -1;
	*dst = *src;
	for (int i = 0; i < VFS_MAX_FDS; ++i) {
		if (!dst->fds[i].used || dst->fds[i].pipe_id < 0) continue;
		if (dst->fds[i].pipe_id >= VFS_MAX_PIPES ||
			!pipe_table[dst->fds[i].pipe_id].used) {
			vfs_fd_table_init(dst);
			return -1;
		}
		if (dst->fds[i].pipe_read) ++pipe_table[dst->fds[i].pipe_id].readers;
		if (dst->fds[i].pipe_write) ++pipe_table[dst->fds[i].pipe_id].writers;
	}
	return 0;
}

void vfs_fd_table_close_all(struct vfs_fd_table *table)
{
	if (!table) return;
	/* close through the table directly: this helper is also used while a
	 * zombie is being reaped, when it is not the current process. */
	for (int i = 3; i < VFS_MAX_FDS; ++i) {
		struct vfs_fd_state *state = &table->fds[i];
		if (!state->used) continue;
		if (state->pipe_id >= 0 && state->pipe_id < VFS_MAX_PIPES &&
			pipe_table[state->pipe_id].used) {
			if (state->pipe_read && pipe_table[state->pipe_id].readers > 0)
				--pipe_table[state->pipe_id].readers;
			if (state->pipe_write && pipe_table[state->pipe_id].writers > 0)
				--pipe_table[state->pipe_id].writers;
			if (!pipe_table[state->pipe_id].readers &&
				!pipe_table[state->pipe_id].writers)
				pipe_table[state->pipe_id].used = false;
		}
		state->used = false;
		state->node = -1;
		state->pipe_id = -1;
		state->tty = false;
		state->tty_id = -1;
	}
}
int vfs_open_fd(const char *path, int flags)
{
	int node;
	int fd = -1;

	if (!path) return -1;
	if (tty_is_device_path(path)) {
		for (int i = 3; i < VFS_MAX_FDS; ++i) if (!fd_table[i].used) {
			fd_table[i].used = true; fd_table[i].node = -1;
			fd_table[i].offset = 0; fd_table[i].flags = flags;
			fd_table[i].pipe_id = -1; fd_table[i].pipe_read = false;
			fd_table[i].pipe_write = false; fd_table[i].tty = true;
			fd_table[i].tty_id = CORTEXOS_TTY_CONSOLE; return i;
		}
		return -1;
	}
	node = vfs_resolve(path);
	if (node < 0 && (flags & VFS_OPEN_CREAT))
		node = vfs_create_node(path, false);
	if (node < 0 || nodes[node].is_dir)
		return -1;
	if ((flags & (VFS_OPEN_WRITE | VFS_OPEN_RDWR)) && nodes[node].read_only)
		return -1;
	if ((flags & VFS_OPEN_TRUNC) && (flags & (VFS_OPEN_WRITE | VFS_OPEN_RDWR))) {
		nodes[node].data = 0;
		nodes[node].size = 0;
	}
	for (int i = 3; i < VFS_MAX_FDS; ++i) {
		if (!fd_table[i].used) {
			fd = i;
			break;
		}
	}
	if (fd < 0)
		return -1;
	fd_table[fd].used = true;
	fd_table[fd].node = node;
	fd_table[fd].offset = 0;
	fd_table[fd].flags = flags;
	fd_table[fd].pipe_id = -1;
	fd_table[fd].pipe_read = false;
	fd_table[fd].pipe_write = false;
	fd_table[fd].tty = false; fd_table[fd].tty_id = -1;
	return fd;
}

int vfs_read_fd(int fd, void *buffer, size_t count)
{
	struct vfs_fd_state *state;
	const vfs_node_t *node;
	size_t available;

	if (!buffer) return -1;
	if (fd >= 0 && fd <= 2) return tty_read(CORTEXOS_TTY_CONSOLE, buffer, count);
	if (fd < 3 || fd >= VFS_MAX_FDS || !fd_table[fd].used) return -1;
	state = &fd_table[fd];
	if (state->tty) return tty_read(state->tty_id, buffer, count);
	if (state->pipe_id >= 0) {
		struct vfs_pipe_state *pipe = &pipe_table[state->pipe_id];
		size_t available;
		if (!state->pipe_read || !pipe->used)
			return -1;
		available = pipe->write_offset - pipe->read_offset;
		if (count > available)
			count = available;
		for (size_t i = 0; i < count; ++i)
			((unsigned char *)buffer)[i] = pipe->data[pipe->read_offset + i];
		pipe->read_offset += count;
		if (pipe->read_offset == pipe->write_offset)
			pipe->read_offset = pipe->write_offset = 0;
		return (int)count;
	}
	node = &nodes[state->node];
	if (node->is_dir || state->offset >= node->size)
		return 0;
	available = node->size - state->offset;
	if (count > available)
		count = available;
	for (size_t i = 0; i < count; ++i)
		((unsigned char *)buffer)[i] = ((const unsigned char *)node->data)[state->offset + i];
	state->offset += count;
	return (int)count;
}

int vfs_write_fd(int fd, const void *buffer, size_t count)
{
	struct vfs_fd_state *state;
	vfs_node_t *node;
	size_t new_size;
	char *storage;

	if (!buffer && count) return -1;
	if (fd >= 0 && fd <= 2) return tty_write(CORTEXOS_TTY_CONSOLE, buffer, count);
	if (fd < 3 || fd >= VFS_MAX_FDS || !fd_table[fd].used) return -1;
	state = &fd_table[fd];
	if (state->tty) return tty_write(state->tty_id, buffer, count);
	if (state->pipe_id >= 0) {
		struct vfs_pipe_state *pipe = &pipe_table[state->pipe_id];
		size_t available;
		if (!state->pipe_write || !pipe->used || pipe->readers <= 0)
			return -1;
		available = VFS_PIPE_SIZE - pipe->write_offset;
		if (count > available)
			count = available;
		for (size_t i = 0; i < count; ++i)
			pipe->data[pipe->write_offset + i] = ((const unsigned char *)buffer)[i];
		pipe->write_offset += count;
		return (int)count;
	}
	if (!(state->flags & (VFS_OPEN_WRITE | VFS_OPEN_RDWR)))
		return -1;
	node = &nodes[state->node];
	if (node->is_dir || node->read_only || count > (size_t)-1 - state->offset)
		return -1;
	new_size = node->size;
	if (state->offset + count > new_size)
		new_size = state->offset + count;
	storage = vfs_alloc_data(0, new_size);
	if (!storage)
		return -1;
	for (size_t i = 0; i < node->size; ++i)
		storage[i] = node->data ? node->data[i] : 0;
	for (size_t i = 0; i < count; ++i)
		storage[state->offset + i] = ((const unsigned char *)buffer)[i];
	node->data = storage;
	node->size = new_size;
	node->own_data = storage;
	node->own_size = new_size;
	state->offset += count;
	return (int)count;
}

int vfs_close_fd(int fd)
{
	struct vfs_fd_state *state;
	if (fd >= 0 && fd <= 2) return 0;
	if (fd < 3 || fd >= VFS_MAX_FDS || !fd_table[fd].used) return -1;
	state = &fd_table[fd];
	if (state->pipe_id >= 0 && pipe_table[state->pipe_id].used) {
		if (state->pipe_read && pipe_table[state->pipe_id].readers > 0)
			--pipe_table[state->pipe_id].readers;
		if (state->pipe_write && pipe_table[state->pipe_id].writers > 0)
			--pipe_table[state->pipe_id].writers;
		if (pipe_table[state->pipe_id].readers == 0 &&
		    pipe_table[state->pipe_id].writers == 0)
			pipe_table[state->pipe_id].used = false;
	}
	state->used = false;
	state->node = -1;
	state->offset = 0;
	state->flags = 0;
	state->pipe_id = -1;
	state->pipe_read = false;
	state->pipe_write = false;
	state->tty = false;
	state->tty_id = -1;
	return 0;
}

int vfs_dup_fd(int old_fd, int new_fd)
{
	if (new_fd < 3 || new_fd >= VFS_MAX_FDS) return -1;
	if (old_fd >= 0 && old_fd <= 2) {
		if (fd_table[new_fd].used) vfs_close_fd(new_fd);
		fd_table[new_fd].used = true; fd_table[new_fd].node = -1;
		fd_table[new_fd].offset = 0; fd_table[new_fd].flags = 0;
		fd_table[new_fd].pipe_id = -1; fd_table[new_fd].pipe_read = false;
		fd_table[new_fd].pipe_write = false; fd_table[new_fd].tty = true;
		fd_table[new_fd].tty_id = CORTEXOS_TTY_CONSOLE; return new_fd;
	}
	if (old_fd < 3 || old_fd >= VFS_MAX_FDS || !fd_table[old_fd].used) return -1;
	if (old_fd == new_fd) return new_fd;
	if (fd_table[new_fd].used) vfs_close_fd(new_fd);
	fd_table[new_fd] = fd_table[old_fd];
	fd_table[new_fd].used = true;
	if (fd_table[new_fd].pipe_id >= 0) {
		if (fd_table[new_fd].pipe_read)
			++pipe_table[fd_table[new_fd].pipe_id].readers;
		if (fd_table[new_fd].pipe_write)
			++pipe_table[fd_table[new_fd].pipe_id].writers;
	}
	return new_fd;
}

int vfs_pipe(int pipe_fds[2])
{
	int pipe_id = -1;
	int read_fd = -1;
	int write_fd = -1;
	if (!pipe_fds)
		return -1;
	for (int i = 0; i < VFS_MAX_PIPES; ++i) {
		if (!pipe_table[i].used) {
			pipe_id = i;
			break;
		}
	}
	for (int i = 3; i < VFS_MAX_FDS; ++i) {
		if (!fd_table[i].used) {
			read_fd = i;
			break;
		}
	}
	for (int i = read_fd + 1; i < VFS_MAX_FDS; ++i) {
		if (!fd_table[i].used) {
			write_fd = i;
			break;
		}
	}
	if (pipe_id < 0 || read_fd < 0 || write_fd < 0)
		return -1;
	pipe_table[pipe_id].used = true;
	pipe_table[pipe_id].read_offset = 0;
	pipe_table[pipe_id].write_offset = 0;
	pipe_table[pipe_id].readers = 1;
	pipe_table[pipe_id].writers = 1;
	fd_table[read_fd].used = true;
	fd_table[read_fd].node = -1;
	fd_table[read_fd].offset = 0;
	fd_table[read_fd].flags = 0;
	fd_table[read_fd].pipe_id = pipe_id;
	fd_table[read_fd].pipe_read = true;
	fd_table[read_fd].pipe_write = false;
	fd_table[write_fd] = fd_table[read_fd];
	fd_table[write_fd].pipe_read = false;
	fd_table[write_fd].pipe_write = true;
	pipe_fds[0] = read_fd;
	pipe_fds[1] = write_fd;
	return 0;
}

int vfs_lseek_fd(int fd, long offset, int whence)
{
	struct vfs_fd_state *state;
	long base;
	long next;
	if (fd < 3 || fd >= VFS_MAX_FDS || !fd_table[fd].used ||
	    fd_table[fd].pipe_id >= 0 || fd_table[fd].tty)
		return -1;
	state = &fd_table[fd];
	if (whence == VFS_SEEK_SET)
		base = 0;
	else if (whence == VFS_SEEK_CUR)
		base = (long)state->offset;
	else if (whence == VFS_SEEK_END)
		base = (long)nodes[state->node].size;
	else
		return -1;
	next = base + offset;
	if (next < 0)
		return -1;
	state->offset = (size_t)next;
	return (int)state->offset;
}

bool vfs_isatty_fd(int fd)
{
	if (fd >= 0 && fd <= 2) return true;
	return fd >= 3 && fd < VFS_MAX_FDS && fd_table[fd].used && fd_table[fd].tty;
}

int vfs_get_fd_flags(int fd)
{
	if (fd >= 0 && fd <= 2)
		return 0;
	if (fd < 3 || fd >= VFS_MAX_FDS || !fd_table[fd].used)
		return -1;
	return fd_table[fd].flags;
}

int vfs_set_fd_flags(int fd, int flags)
{
	if (fd < 3 || fd >= VFS_MAX_FDS || !fd_table[fd].used)
		return -1;
	fd_table[fd].flags = flags;
	return 0;
}

