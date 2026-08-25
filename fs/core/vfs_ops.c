#include "vfs_internal.h"
#include "console.h"
#include "tty.h"
#include "fs.h"
#include "kstring.h"
#include <stddef.h>
#include <stdbool.h>

const char *vfs_pwd(void)
{
	static char buffer[256];
	vfs_build_path(cwd_node, buffer, sizeof(buffer));
	return buffer;
}

bool vfs_cd(const char *path)
{
	int node = vfs_resolve(path);
	if (node < 0 || !nodes[node].is_dir)
		return false;
	cwd_node = node;
	return true;
}

bool vfs_list(const char *path)
{
	int node = path && path[0] ? vfs_resolve(path) : cwd_node;
	if (node < 0 || !nodes[node].is_dir)
		return false;
	for (int i = 0; i < nodes[node].child_count; ++i) {
		int child = nodes[node].children[i];
		if (nodes[child].is_dir)
			console_print("/ ");
		else
			console_print("  ");
		console_print(nodes[child].name);
		console_print("\n");
	}
	return true;
}

bool vfs_cat(const char *path)
{
	int node = vfs_resolve(path);
	if (node < 0 || nodes[node].is_dir)
		return false;
	const char *data = nodes[node].data;
	size_t size = nodes[node].size;
	for (size_t i = 0; i < size; ++i) {
		char c = data[i];
		console_putchar(c ? c : '\n');
	}
	console_print("\n");
	return true;
}

bool vfs_mkdir(const char *path)
{
	return vfs_create_node(path, true) >= 0;
}

bool vfs_touch(const char *path)
{
	int existing = vfs_resolve(path);
	if (existing >= 0)
		return true;
	int node = vfs_create_node(path, false);
	if (node < 0)
		return false;
	nodes[node].data = "";
	nodes[node].size = 0;
	return true;
}

static bool vfs_remove_node(int node)
{
	if (node < 0 || nodes[node].read_only)
		return false;
	if (nodes[node].is_dir && nodes[node].child_count > 0)
		return false;
	int parent = nodes[node].parent;
	if (parent >= 0) {
		int write = 0;
		for (int i = 0; i < nodes[parent].child_count; ++i) {
			if (nodes[parent].children[i] == node)
				continue;
			nodes[parent].children[write++] = nodes[parent].children[i];
		}
		nodes[parent].child_count = write;
	}
	vfs_clear_node(node);
	return true;
}

bool vfs_remove(const char *path)
{
	int node = vfs_resolve(path);
	if (node < 0 || nodes[node].is_dir)
		return false;
	return vfs_remove_node(node);
}

bool vfs_rmdir(const char *path)
{
	int node = vfs_resolve(path);
	if (node < 0 || !nodes[node].is_dir)
		return false;
	return vfs_remove_node(node);
}

bool vfs_cp(const char *src, const char *dst)
{
	int node = vfs_resolve(src);
	if (node < 0 || nodes[node].is_dir)
		return false;
	int target = vfs_resolve(dst);
	if (target >= 0)
		return false;
	char name[VFS_NAME_SIZE];
	int parent = vfs_resolve_parent(dst, name);
	if (parent < 0 || !nodes[parent].is_dir)
		return false;
	int child = vfs_create_node(dst, false);
	if (child < 0)
		return false;
	char *storage = vfs_alloc_data(nodes[node].data, nodes[node].size);
	if (!storage) {
		vfs_remove_node(child);
		return false;
	}
	nodes[child].data = storage;
	nodes[child].size = nodes[node].size;
	nodes[child].own_data = storage;
	nodes[child].own_size = nodes[node].size;
	return true;
}

bool vfs_mv(const char *src, const char *dst)
{
	int node = vfs_resolve(src);
	if (node < 0 || nodes[node].read_only)
		return false;
	char name[VFS_NAME_SIZE];
	int parent = vfs_resolve_parent(dst, name);
	if (parent < 0 || !nodes[parent].is_dir)
		return false;
	int existing = vfs_find_child(parent, name);
	if (existing >= 0)
		return false;
	int old_parent = nodes[node].parent;
	if (old_parent >= 0) {
		int write = 0;
		for (int i = 0; i < nodes[old_parent].child_count; ++i) {
			if (nodes[old_parent].children[i] == node)
				continue;
			nodes[old_parent].children[write++] = nodes[old_parent].children[i];
		}
		nodes[old_parent].child_count = write;
	}
	kstrcpy(nodes[node].name, name, VFS_NAME_SIZE);
	vfs_add_child(parent, node);
	return true;
}

bool vfs_write(const char *path, const char *content, size_t length)
{
	int node = vfs_resolve(path);
	if (node >= 0 && nodes[node].read_only)
		return false;
	if (node < 0) {
		node = vfs_create_node(path, false);
		if (node < 0)
			return false;
	}
	if (nodes[node].is_dir)
		return false;
	char *storage = vfs_alloc_data(content, length);
	if (!storage)
		return false;
	nodes[node].data = storage;
	nodes[node].size = length;
	nodes[node].own_data = storage;
	nodes[node].own_size = length;
	return true;
}

const void *vfs_read(const char *path)
{
	int node = vfs_resolve(path);
	if (node < 0 || nodes[node].is_dir)
		return 0;
	return nodes[node].data;
}

size_t vfs_get_size(const char *path)
{
	int node;
	if (tty_is_device_path(path)) return 0;
	node = vfs_resolve(path);
	if (node < 0 || nodes[node].is_dir)
		return 0;
	return nodes[node].size;
}

bool vfs_exists(const char *path)
{
	return tty_is_device_path(path) || vfs_resolve(path) >= 0;
}

bool vfs_is_dir(const char *path)
{
	int node;
	if (tty_is_device_path(path)) return false;
	node = vfs_resolve(path);
	if (node < 0)
		return false;
	return nodes[node].is_dir;
}

int vfs_get_children(const char *path, const char **names, int max_names)
{
	int node = vfs_resolve(path);
	if (node < 0 || !nodes[node].is_dir)
		return 0;
	int count = 0;
	for (int i = 0; i < nodes[node].child_count && count < max_names; ++i) {
		int child = nodes[node].children[i];
		if (child >= 0) {
			names[count] = nodes[child].name;
			count++;
		}
	}
	return count;
}

int vfs_get_file_count(const char *path)
{
	int node = vfs_resolve(path);
	if (node < 0)
		return 0;
	return nodes[node].child_count;
}

bool vfs_stat(const char *path)
{
	int node = vfs_resolve(path);
	if (node < 0)
		return false;
	console_print("Name:   ");
	console_print(nodes[node].name);
	console_print("\nType:   ");
	if (nodes[node].is_dir) {
		console_print_color("directory\n", VGA_ATTR(VGA_CYAN, VGA_BLACK));
	} else if (nodes[node].read_only) {
		console_print_color("read-only file\n", VGA_ATTR(VGA_YELLOW, VGA_BLACK));
	} else {
		console_print_color("file\n", VGA_ATTR(VGA_WHITE, VGA_BLACK));
	}
	console_print("Size:   ");
	char buf[16];
	kitoa(nodes[node].size, buf, sizeof(buf));
	console_print(buf);
	console_print(" bytes\n");
	if (nodes[node].is_dir) {
		console_print("Children: ");
		kitoa(nodes[node].child_count, buf, sizeof(buf));
		console_print(buf);
		console_print("\n");
	}
	return true;
}

