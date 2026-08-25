#include "vfs.h"
#include "vfs_internal.h"
#include "process.h"
#include "console.h"
#include "tty.h"
#include "fs.h"
#include "kstring.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define VFS_MAX_NODES 80
#define VFS_NAME_SIZE 64
#define VFS_CHILDREN_MAX 16
#define VFS_DATA_STORE_SIZE 16384

vfs_node_t nodes[VFS_MAX_NODES];
char data_store[VFS_DATA_STORE_SIZE];
size_t data_used;
int root_node = -1;
int cwd_node = -1;


struct vfs_fd_table fallback_fd_table;
struct vfs_pipe_state pipe_table[VFS_MAX_PIPES];
struct vfs_mount_state mounts[VFS_MAX_MOUNTS];

struct vfs_fd_table *active_fd_table(void)
{
	struct process *process = process_current();
	return process ? &process->fd_table : &fallback_fd_table;
}
#define fd_table (active_fd_table()->fds)

static int vfs_new_node(void)
{
	for (int i = 0; i < VFS_MAX_NODES; ++i) {
		if (nodes[i].name[0] == '\0' && i != root_node)
			return i;
	}
	return -1;
}

void vfs_clear_node(int index)
{
	nodes[index].name[0] = '\0';
	nodes[index].is_dir = false;
	nodes[index].read_only = false;
	nodes[index].data = 0;
	nodes[index].size = 0;
	nodes[index].own_data = 0;
	nodes[index].own_size = 0;
	nodes[index].parent = -1;
	nodes[index].child_count = 0;
}

char *vfs_alloc_data(const char *data, size_t size)
{
	if (data_used + size >= VFS_DATA_STORE_SIZE)
		return 0;
	char *dest = data_store + data_used;
	for (size_t i = 0; i < size; ++i)
		dest[i] = data ? data[i] : 0;
	data_used += size;
	return dest;
}

int vfs_find_child(int dir, const char *name)
{
	if (dir < 0 || dir >= VFS_MAX_NODES || !nodes[dir].is_dir)
		return -1;
	for (int i = 0; i < nodes[dir].child_count; ++i) {
		int child = nodes[dir].children[i];
		if (kstrcmp(nodes[child].name, name) == 0)
			return child;
	}
	return -1;
}

int vfs_add_child(int parent, int child)
{
	if (parent < 0 || parent >= VFS_MAX_NODES)
		return -1;
	if (nodes[parent].child_count >= VFS_CHILDREN_MAX)
		return -1;
	nodes[parent].children[nodes[parent].child_count++] = child;
	nodes[child].parent = parent;
	return 0;
}

int vfs_resolve(const char *path)
{
	int current = cwd_node;
	if (!path || path[0] == '\0')
		return current;
	if (path[0] == '/') {
		current = root_node;
		++path;
	}
	if (current < 0)
		return -1;
	char component[VFS_NAME_SIZE];
	while (*path) {
		while (*path == '/')
			++path;
		if (!*path)
			break;
		size_t pos = 0;
		while (*path && *path != '/' && pos + 1 < VFS_NAME_SIZE)
			component[pos++] = *path++;
		component[pos] = '\0';
		if (kstrcmp(component, ".") == 0)
			continue;
		if (kstrcmp(component, "..") == 0) {
			if (nodes[current].parent >= 0)
				current = nodes[current].parent;
			continue;
		}
		int child = vfs_find_child(current, component);
		if (child < 0)
			return -1;
		current = child;
	}
	return current;
}

int vfs_resolve_parent(const char *path, char *basename)
{
	if (!path || path[0] == '\0')
		return -1;
	const char *slash = 0;
	const char *p = path;
	while (*p) {
		if (*p == '/')
			slash = p;
		++p;
	}
	if (!slash) {
		kstrcpy(basename, path, VFS_NAME_SIZE);
		return cwd_node;
	}
	size_t parent_len = (size_t)(slash - path);
	if (parent_len == 0) {
		kstrcpy(basename, slash + 1, VFS_NAME_SIZE);
		return root_node;
	}
	const char *name_start = slash + 1;
	while (*name_start == '/')
		++name_start;
	kstrcpy(basename, name_start, VFS_NAME_SIZE);
	char parent_path[128];
	size_t len = 0;
	const char *q = path;
	while (q < slash && len + 1 < sizeof(parent_path))
		parent_path[len++] = *q++;
	if (len > 1 && parent_path[len - 1] == '/')
		--len;
	parent_path[len] = '\0';
	if (len == 0)
		return root_node;
	return vfs_resolve(parent_path);
}

int vfs_create_node(const char *path, bool dir)
{
	char name[VFS_NAME_SIZE];
	int parent = vfs_resolve_parent(path, name);
	if (parent < 0)
		return -1;
	if (!nodes[parent].is_dir)
		return -1;
	if (name[0] == '\0')
		return -1;
	int existing = vfs_find_child(parent, name);
	if (existing >= 0)
		return -1;
	int node = -1;
	for (int i = 0; i < VFS_MAX_NODES; ++i) {
		if (nodes[i].name[0] == '\0' && (i != root_node || root_node < 0)) {
			node = i;
			break;
		}
	}
	if (node < 0)
		return -1;
	vfs_clear_node(node);
	kstrcpy(nodes[node].name, name, VFS_NAME_SIZE);
	nodes[node].is_dir = dir;
	nodes[node].read_only = false;
	nodes[node].parent = parent;
	nodes[node].child_count = 0;
	if (vfs_add_child(parent, node) < 0)
		return -1;
	return node;
}

void vfs_build_path(int node, char *out, size_t max_len)
{
	if (node < 0) {
		out[0] = '\0';
		return;
	}
	if (nodes[node].parent < 0) {
		if (max_len > 1) {
			out[0] = '/';
			out[1] = '\0';
		} else if (max_len > 0) {
			out[0] = '\0';
		}
		return;
	}
	char tmp[256];
	vfs_build_path(nodes[node].parent, tmp, sizeof(tmp));
	size_t len = kstrlen(tmp);
	if (len > 0 && tmp[len - 1] != '/' && len + 1 < max_len) {
		if (len + 1 >= sizeof(tmp))
			return;
		tmp[len++] = '/';
		tmp[len] = '\0';
	}
	size_t i = 0;
	while (i < len && i + 1 < max_len) {
		out[i] = tmp[i];
		++i;
	}
	const char *name = nodes[node].name;
	while (*name && i + 1 < max_len)
		out[i++] = *name++;
	out[i] = '\0';
}


bool vfs_init(void)
{
	for (int i = 0; i < VFS_MAX_NODES; ++i) {
		nodes[i].name[0] = '\0';
		nodes[i].parent = -1;
		nodes[i].child_count = 0;
		nodes[i].own_data = 0;
	}
	data_used = 0;
	vfs_fd_table_init(&fallback_fd_table);
	for (int i = 0; i < VFS_MAX_PIPES; ++i) {
		pipe_table[i].used = false;
		pipe_table[i].read_offset = 0;
		pipe_table[i].write_offset = 0;
		pipe_table[i].readers = 0;
		pipe_table[i].writers = 0;
	}
	root_node = 0;
	vfs_clear_node(root_node);
	nodes[root_node].is_dir = true;
	nodes[root_node].read_only = true;
	nodes[root_node].parent = -1;
	cwd_node = root_node;

	size_t total = fs_file_count();
	for (size_t i = 0; i < total; ++i) {
		const struct fs_file *file = fs_file_at(i);
		if (!file || !file->name)
			continue;
		const char *path = file->name;
		int current = root_node;
		char buffer[VFS_NAME_SIZE];
		const char *p = path;
		while (*p) {
			while (*p == '/')
				++p;
			if (!*p)
				break;
			size_t pos = 0;
			while (*p && *p != '/' && pos + 1 < VFS_NAME_SIZE)
				buffer[pos++] = *p++;
			buffer[pos] = '\0';
			int child = vfs_find_child(current, buffer);
			if (child < 0) {
				int node = -1;
				if (*p != '\0') {
					node = vfs_new_node();
					if (node >= 0) {
						vfs_clear_node(node);
						kstrcpy(nodes[node].name, buffer, VFS_NAME_SIZE);
						nodes[node].is_dir = true;
						nodes[node].read_only = true;
						nodes[node].parent = current;
						nodes[node].child_count = 0;
						if (vfs_add_child(current, node) < 0)
							continue;
					}
				} else {
					node = vfs_new_node();
					if (node >= 0) {
						vfs_clear_node(node);
						kstrcpy(nodes[node].name, buffer, VFS_NAME_SIZE);
						nodes[node].is_dir = false;
						nodes[node].read_only = true;
						nodes[node].data = (const char *)file->data;
						nodes[node].size = file->size;
						nodes[node].parent = current;
						nodes[node].child_count = 0;
						if (vfs_add_child(current, node) < 0)
							continue;
					}
				}
				if (node < 0)
					break;
				child = node;
			}
			current = child;
		}
	}

	const char *std_dirs[] = {
		"proc", "sys", "dev", "tmp", "etc", "usr", "var",
		"mnt", "opt", "srv", "run", "home"
	};
	for (size_t i = 0; i < sizeof(std_dirs) / sizeof(std_dirs[0]); ++i) {
		int existing = vfs_find_child(root_node, std_dirs[i]);
		if (existing < 0) {
			int node = vfs_new_node();
			if (node >= 0) {
				vfs_clear_node(node);
				kstrcpy(nodes[node].name, std_dirs[i], VFS_NAME_SIZE);
				nodes[node].is_dir = true;
				nodes[node].read_only = true;
				nodes[node].parent = root_node;
				nodes[node].child_count = 0;
				vfs_add_child(root_node, node);
			}
		}
	}
	for (int i = 0; i < VFS_MAX_MOUNTS; ++i) mounts[i].used = false;
	return true;
}

/* User-space descriptor operations. Standard input/output/error remain
 * terminal descriptors owned by syscall.c; this table stores regular files. */
