#include "tmpfs.h"
#include "console.h"
#include "heap.h"
#include "string.h"

// TmpFS Operations
static uint64_t tmpfs_read(fs_node_t *node, uint64_t offset, uint64_t size,
                           uint8_t *buffer);
static uint64_t tmpfs_write(fs_node_t *node, uint64_t offset, uint64_t size,
                            uint8_t *buffer);
static struct dirent *tmpfs_readdir(fs_node_t *node, uint32_t index);
static fs_node_t *tmpfs_finddir(fs_node_t *node, char *name);

// Forward declarations for create functions
fs_node_t *tmpfs_create_file(fs_node_t *parent, char *name);
fs_node_t *tmpfs_create_dir(fs_node_t *parent, char *name);

// Helper to create a new generic node
static fs_node_t *create_node(char *name, uint32_t flags) {
  fs_node_t *node = (fs_node_t *)malloc(sizeof(fs_node_t));
  k_memset(node, 0, sizeof(fs_node_t));
  k_strcpy(node->name, name);
  node->flags = flags;
  node->open = NULL;
  node->close = NULL;
  node->read = tmpfs_read;
  node->write = tmpfs_write;
  node->readdir = tmpfs_readdir;
  node->finddir = tmpfs_finddir;
  node->create = tmpfs_create_file;
  node->mkdir = tmpfs_create_dir;
  return node;
}

fs_node_t *tmpfs_init(void) { return tmpfs_create_dir(NULL, "root"); }

fs_node_t *tmpfs_create_file(fs_node_t *parent, char *name) {
  fs_node_t *node = create_node(name, FS_FILE);
  // ptr will hold data content (char*)
  node->ptr = NULL;
  node->length = 0;

  if (parent) {
    // Add to parent's list
    node->next = parent->ptr;
    parent->ptr = node;
  }
  return node;
}

fs_node_t *tmpfs_create_dir(fs_node_t *parent, char *name) {
  fs_node_t *node = create_node(name, FS_DIRECTORY);
  // ptr will hold head of children list (fs_node_t*)
  node->ptr = NULL;

  if (parent) {
    node->next = parent->ptr;
    parent->ptr = node;
  }
  return node;
}

static uint64_t tmpfs_read(fs_node_t *node, uint64_t offset, uint64_t size,
                           uint8_t *buffer) {
  if ((node->flags & 0x7) != FS_FILE)
    return 0;
  if (offset >= node->length)
    return 0;
  if (offset + size > node->length)
    size = node->length - offset;

  char *data = (char *)node->ptr;
  if (!data)
    return 0;

  k_memcpy(buffer, data + offset, size);
  return size;
}

static uint64_t tmpfs_write(fs_node_t *node, uint64_t offset, uint64_t size,
                            uint8_t *buffer) {
  if ((node->flags & 0x7) != FS_FILE)
    return 0;

  // Simple append/overwrite logic relying on realloc... wait, we don't have
  // realloc. For TmpFS simplistic: always allocate large enough OR create a new
  // buffer. We will implement a simple re-allocate logic: new_buffer = malloc;
  // memcpy; free old.

  size_t new_end = offset + size;
  if (new_end > node->length) {
    // Resize
    char *new_data = (char *)malloc(new_end + 1); // +1 safety
    if (!new_data)
      return 0;
    k_memset(new_data, 0, new_end + 1);

    if (node->ptr) {
      k_memcpy(new_data, node->ptr, node->length);
      free(node->ptr);
    }
    node->ptr = (struct fs_node *)new_data; // Casting for storage
    node->length = new_end;
  }

  char *data = (char *)node->ptr;
  k_memcpy(data + offset, buffer, size);
  return size;
}

static struct dirent dir_entry; // Static buffer for readdir

static struct dirent *tmpfs_readdir(fs_node_t *node, uint32_t index) {
  if ((node->flags & 0x7) != FS_DIRECTORY)
    return NULL;

  // ptr points to the first child (head of list)
  fs_node_t *child = (fs_node_t *)node->ptr;
  uint32_t i = 0;
  while (child != NULL) {
    if (i == index) {
      k_strcpy(dir_entry.name, child->name);
      dir_entry.inode = 0;
      return &dir_entry;
    }
    child = child->next;
    i++;
  }
  return NULL;
}

static fs_node_t *tmpfs_finddir(fs_node_t *node, char *name) {
  if ((node->flags & 0x7) != FS_DIRECTORY)
    return NULL;

  fs_node_t *child = (fs_node_t *)node->ptr;
  while (child != NULL) {
    if (k_strcmp(name, child->name) == 0)
      return child;
    child = child->next;
  }
  return NULL;
}
