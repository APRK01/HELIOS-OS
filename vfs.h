#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdint.h>

#define FS_FILE 0x01
#define FS_DIRECTORY 0x02
#define FS_CHARDEVICE 0x03
#define FS_BLOCKDEVICE 0x04
#define FS_PIPE 0x05
#define FS_SYMLINK 0x06
#define FS_MOUNTPOINT 0x08

struct fs_node;

// Callback definitions
typedef uint64_t (*read_type_t)(struct fs_node *, uint64_t, uint64_t,
                                uint8_t *);
typedef uint64_t (*write_type_t)(struct fs_node *, uint64_t, uint64_t,
                                 uint8_t *);
typedef void (*open_type_t)(struct fs_node *);
typedef void (*close_type_t)(struct fs_node *);
typedef struct dirent *(*readdir_type_t)(struct fs_node *, uint32_t);
typedef struct fs_node *(*finddir_type_t)(struct fs_node *, char *name);
typedef struct fs_node *(*create_type_t)(struct fs_node *, char *name);
typedef struct fs_node *(*mkdir_type_t)(struct fs_node *, char *name);

typedef struct fs_node {
  char name[128];
  uint32_t flags;
  uint32_t inode;
  uint32_t length;
  read_type_t read;
  write_type_t write;
  open_type_t open;
  close_type_t close;
  readdir_type_t readdir;
  finddir_type_t finddir;
  create_type_t create;
  mkdir_type_t mkdir;
  struct fs_node *ptr;  // Used by mountpoints or internal data
  struct fs_node *next; // For linked lists (directory children)
} fs_node_t;

struct dirent {
  char name[128];
  uint32_t inode;
};

// Global root node
extern fs_node_t *fs_root;

// Standard VFS wrappers
uint64_t vfs_read(fs_node_t *node, uint64_t offset, uint64_t size,
                  uint8_t *buffer);
uint64_t vfs_write(fs_node_t *node, uint64_t offset, uint64_t size,
                   uint8_t *buffer);
void vfs_open(fs_node_t *node, uint8_t read, uint8_t write);
void vfs_close(fs_node_t *node);
struct dirent *vfs_readdir(fs_node_t *node, uint32_t index);
fs_node_t *vfs_finddir(fs_node_t *node, char *name);
fs_node_t *vfs_create(fs_node_t *node, char *name);
fs_node_t *vfs_mkdir(fs_node_t *node, char *name);

#endif // VFS_H
