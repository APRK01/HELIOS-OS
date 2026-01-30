#include "vfs.h"

fs_node_t *fs_root = NULL;

uint64_t vfs_read(fs_node_t *node, uint64_t offset, uint64_t size,
                  uint8_t *buffer) {
  if (node->read)
    return node->read(node, offset, size, buffer);
  return 0;
}

uint64_t vfs_write(fs_node_t *node, uint64_t offset, uint64_t size,
                   uint8_t *buffer) {
  if (node->write)
    return node->write(node, offset, size, buffer);
  return 0;
}

void vfs_open(fs_node_t *node, uint8_t read, uint8_t write) {
  if (node->open)
    node->open(node);
}

void vfs_close(fs_node_t *node) {
  if (node->close)
    node->close(node);
}

struct dirent *vfs_readdir(fs_node_t *node, uint32_t index) {
  if ((node->flags & 0x7) == FS_DIRECTORY && node->readdir)
    return node->readdir(node, index);
  return NULL;
}

fs_node_t *vfs_finddir(fs_node_t *node, char *name) {
  if ((node->flags & 0x7) == FS_DIRECTORY && node->finddir)
    return node->finddir(node, name);
  return NULL;
}

fs_node_t *vfs_create(fs_node_t *node, char *name) {
  if ((node->flags & 0x7) == FS_DIRECTORY && node->create)
    return node->create(node, name);
  return NULL;
}

fs_node_t *vfs_mkdir(fs_node_t *node, char *name) {
  if ((node->flags & 0x7) == FS_DIRECTORY && node->mkdir)
    return node->mkdir(node, name);
  return NULL;
}
