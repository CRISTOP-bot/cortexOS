#ifndef VFS_INTERNAL_H
#define VFS_INTERNAL_H
#include "vfs.h"
#include <stdbool.h>
#include <stddef.h>
#define VFS_MAX_NODES 80
#define VFS_NAME_SIZE 64
#define VFS_CHILDREN_MAX 16
#define VFS_DATA_STORE_SIZE 16384
struct vfs_node {
 char name[VFS_NAME_SIZE]; bool is_dir; bool read_only; const char *data;
 size_t size; char *own_data; size_t own_size; int parent;
 int child_count; int children[VFS_CHILDREN_MAX];
};
typedef struct vfs_node vfs_node_t;
struct vfs_pipe_state { bool used; unsigned char data[VFS_PIPE_SIZE];
 size_t read_offset, write_offset; int readers, writers; };
extern vfs_node_t nodes[VFS_MAX_NODES];
extern char data_store[VFS_DATA_STORE_SIZE];
extern size_t data_used; extern int root_node, cwd_node;
extern struct vfs_fd_table fallback_fd_table;
extern struct vfs_pipe_state pipe_table[VFS_MAX_PIPES];
struct vfs_fd_table *active_fd_table(void);
#define fd_table (active_fd_table()->fds)
char *vfs_alloc_data(const char *data, size_t size);
int vfs_resolve(const char *path);
int vfs_create_node(const char *path, bool dir);
#endif
