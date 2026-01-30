#ifndef TMPFS_H
#define TMPFS_H

#include "vfs.h"

// Initialize TmpFS and return the root node
fs_node_t *tmpfs_init(void);

// Create a file in TmpFS
fs_node_t *tmpfs_create_file(fs_node_t *parent, char *name);

// Create a directory in TmpFS
fs_node_t *tmpfs_create_dir(fs_node_t *parent, char *name);

#endif
