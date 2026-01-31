#ifndef EDITOR_H
#define EDITOR_H

#include "vfs.h"
#include <stddef.h>
#include <stdint.h>

 
#define EDITOR_MAX_LINES 1000
#define EDITOR_MAX_LINE_LEN 256

 
typedef struct {
  char lines[EDITOR_MAX_LINES][EDITOR_MAX_LINE_LEN];
  size_t num_lines;
  size_t cursor_line;
  size_t cursor_col;
  size_t scroll_offset;  
  int modified;
  char filename[128];
  fs_node_t *file;
} editor_state_t;

 
void editor_open(const char *filename);

#endif  
