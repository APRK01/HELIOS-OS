#ifndef TMPFS_H
#define TMPFS_H

#include "vfs.h"

 
fs_node_t *tmpfs_init(void);

 
fs_node_t *tmpfs_create_file(fs_node_t *parent, char *name);

 
fs_node_t *tmpfs_create_dir(fs_node_t *parent, char *name);

#endif
