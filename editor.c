#include "editor.h"
#include "console.h"
#include "keyboard.h"
#include "string.h"
#include "uart.h"
#include "vfs.h"
#include <stddef.h>

 
static editor_state_t editor;

 
#define EDITOR_ROWS 20
#define EDITOR_COLS 80

 
static void editor_init(const char *filename);
static void editor_load_file(void);
static void editor_save_file(void);
static void editor_render(void);
static void editor_render_status(void);
static int editor_handle_key(int c);

 
static void editor_init(const char *filename) {
   
  for (size_t i = 0; i < EDITOR_MAX_LINES; i++) {
    editor.lines[i][0] = '\0';
  }
  editor.num_lines = 1;  
  editor.cursor_line = 0;
  editor.cursor_col = 0;
  editor.scroll_offset = 0;
  editor.modified = 0;

   
  size_t i = 0;
  while (filename[i] && i < 127) {
    editor.filename[i] = filename[i];
    i++;
  }
  editor.filename[i] = '\0';
  editor.file = NULL;
}

 
static void editor_load_file(void) {
  extern fs_node_t *cwd;
  if (cwd == NULL) {
    extern fs_node_t *fs_root;
    cwd = fs_root;
  }

  fs_node_t *f = vfs_finddir(cwd, editor.filename);
  if (f && !(f->flags & FS_DIRECTORY)) {
    editor.file = f;

     
    if (f->length > 0) {
      uint8_t buffer[4096];
      size_t read_len =
          f->length < sizeof(buffer) - 1 ? f->length : sizeof(buffer) - 1;
      vfs_read(f, 0, read_len, buffer);
      buffer[read_len] = '\0';

       
      editor.num_lines = 0;
      size_t col = 0;
      for (size_t i = 0; i < read_len && editor.num_lines < EDITOR_MAX_LINES;
           i++) {
        if (buffer[i] == '\n') {
          editor.lines[editor.num_lines][col] = '\0';
          editor.num_lines++;
          col = 0;
        } else if (col < EDITOR_MAX_LINE_LEN - 1) {
          editor.lines[editor.num_lines][col] = buffer[i];
          col++;
        }
      }
       
      if (col > 0 || editor.num_lines == 0) {
        editor.lines[editor.num_lines][col] = '\0';
        editor.num_lines++;
      }
    }
  } else {
     
    editor.file = NULL;
  }

  if (editor.num_lines == 0) {
    editor.num_lines = 1;
  }
}

 
static void editor_save_file(void) {
  extern fs_node_t *cwd;
  if (cwd == NULL) {
    extern fs_node_t *fs_root;
    cwd = fs_root;
  }

   
  if (editor.file == NULL) {
    editor.file = vfs_create(cwd, editor.filename);
    if (editor.file == NULL) {
       
      return;
    }
  }

   
  uint8_t buffer[4096];
  size_t pos = 0;

  for (size_t line = 0; line < editor.num_lines && pos < sizeof(buffer) - 1;
       line++) {
    size_t col = 0;
    while (editor.lines[line][col] && pos < sizeof(buffer) - 1) {
      buffer[pos++] = editor.lines[line][col++];
    }
    if (pos < sizeof(buffer) - 1 && line < editor.num_lines - 1) {
      buffer[pos++] = '\n';
    }
  }

   
  vfs_write(editor.file, 0, pos, buffer);
  editor.file->length = pos;
  editor.modified = 0;
}

 
static void editor_render(void) {
   
  console_set_cursor(0, 0);

  for (size_t row = 0; row < EDITOR_ROWS; row++) {
    size_t line_num = editor.scroll_offset + row;

     
    for (int i = 0; i < EDITOR_COLS; i++) {
      console_putchar(' ');
    }
    console_set_cursor(0, row);

    if (line_num < editor.num_lines) {
       
      char num[8];
      int n = line_num + 1;
      int idx = 0;
      if (n >= 100)
        num[idx++] = '0' + (n / 100) % 10;
      if (n >= 10)
        num[idx++] = '0' + (n / 10) % 10;
      num[idx++] = '0' + n % 10;
      num[idx] = '\0';
      console_print(num);
      console_print("| ");

       
      size_t max_chars = EDITOR_COLS - 5;
      for (size_t i = 0; i < max_chars && editor.lines[line_num][i]; i++) {
        console_putchar(editor.lines[line_num][i]);
      }
    } else {
      console_print("~");
    }
  }

   
  editor_render_status();

   
  size_t screen_line = editor.cursor_line - editor.scroll_offset;
  console_set_cursor(5 + editor.cursor_col, screen_line);
  console_set_cursor_visible(1);
}

 
static void editor_render_status(void) {
  console_set_cursor(0, EDITOR_ROWS);

   
  for (int i = 0; i < EDITOR_COLS; i++)
    console_putchar('-');

  console_set_cursor(0, EDITOR_ROWS);
  console_print("-- ");
  console_print(editor.filename);
  if (editor.modified)
    console_print(" [+]");
  console_print(" -- Ln ");

   
  char num[8];
  int n = editor.cursor_line + 1;
  int idx = 0;
  if (n >= 100)
    num[idx++] = '0' + (n / 100) % 10;
  if (n >= 10)
    num[idx++] = '0' + (n / 10) % 10;
  num[idx++] = '0' + n % 10;
  num[idx] = '\0';
  console_print(num);
  console_print(" Col ");

  n = editor.cursor_col + 1;
  idx = 0;
  if (n >= 100)
    num[idx++] = '0' + (n / 100) % 10;
  if (n >= 10)
    num[idx++] = '0' + (n / 10) % 10;
  num[idx++] = '0' + n % 10;
  num[idx] = '\0';
  console_print(num);

  console_print(" | F2:Save ESC:Quit");
}

 
static int editor_handle_key(int c) {
  char *line = editor.lines[editor.cursor_line];
  size_t line_len = 0;
  while (line[line_len])
    line_len++;

   
  if (c == KEY_ESC) {
    return 1;
  }

   
  if (c == KEY_F2) {
    editor_save_file();
    return 0;
  }

   
  if (c == KEY_UP) {
    if (editor.cursor_line > 0) {
      editor.cursor_line--;
       
      size_t new_len = 0;
      while (editor.lines[editor.cursor_line][new_len])
        new_len++;
      if (editor.cursor_col > new_len)
        editor.cursor_col = new_len;
       
      if (editor.cursor_line < editor.scroll_offset) {
        editor.scroll_offset = editor.cursor_line;
      }
    }
    return 0;
  }

  if (c == KEY_DOWN) {
    if (editor.cursor_line < editor.num_lines - 1) {
      editor.cursor_line++;
      size_t new_len = 0;
      while (editor.lines[editor.cursor_line][new_len])
        new_len++;
      if (editor.cursor_col > new_len)
        editor.cursor_col = new_len;
      if (editor.cursor_line >= editor.scroll_offset + EDITOR_ROWS) {
        editor.scroll_offset = editor.cursor_line - EDITOR_ROWS + 1;
      }
    }
    return 0;
  }

  if (c == KEY_LEFT) {
    if (editor.cursor_col > 0) {
      editor.cursor_col--;
    } else if (editor.cursor_line > 0) {
       
      editor.cursor_line--;
      size_t prev_len = 0;
      while (editor.lines[editor.cursor_line][prev_len])
        prev_len++;
      editor.cursor_col = prev_len;
    }
    return 0;
  }

  if (c == KEY_RIGHT) {
    if (editor.cursor_col < line_len) {
      editor.cursor_col++;
    } else if (editor.cursor_line < editor.num_lines - 1) {
       
      editor.cursor_line++;
      editor.cursor_col = 0;
    }
    return 0;
  }

  if (c == KEY_HOME) {
    editor.cursor_col = 0;
    return 0;
  }

  if (c == KEY_END) {
    editor.cursor_col = line_len;
    return 0;
  }

   
  if (c == '\r' || c == '\n') {
    if (editor.num_lines < EDITOR_MAX_LINES - 1) {
       
      for (size_t i = editor.num_lines; i > editor.cursor_line + 1; i--) {
        for (size_t j = 0; j < EDITOR_MAX_LINE_LEN; j++) {
          editor.lines[i][j] = editor.lines[i - 1][j];
        }
      }
      editor.num_lines++;

       
      char *new_line = editor.lines[editor.cursor_line + 1];
      size_t j = 0;
      for (size_t i = editor.cursor_col; line[i]; i++) {
        new_line[j++] = line[i];
      }
      new_line[j] = '\0';
      line[editor.cursor_col] = '\0';

      editor.cursor_line++;
      editor.cursor_col = 0;
      editor.modified = 1;
    }
    return 0;
  }

   
  if (c == 0x7F || c == 0x08) {
    if (editor.cursor_col > 0) {
       
      for (size_t i = editor.cursor_col - 1; line[i]; i++) {
        line[i] = line[i + 1];
      }
      editor.cursor_col--;
      editor.modified = 1;
    } else if (editor.cursor_line > 0) {
       
      char *prev_line = editor.lines[editor.cursor_line - 1];
      size_t prev_len = 0;
      while (prev_line[prev_len])
        prev_len++;

       
      for (size_t i = 0; line[i] && prev_len + i < EDITOR_MAX_LINE_LEN - 1;
           i++) {
        prev_line[prev_len + i] = line[i];
        prev_line[prev_len + i + 1] = '\0';
      }

       
      for (size_t i = editor.cursor_line; i < editor.num_lines - 1; i++) {
        for (size_t j = 0; j < EDITOR_MAX_LINE_LEN; j++) {
          editor.lines[i][j] = editor.lines[i + 1][j];
        }
      }
      editor.num_lines--;

      editor.cursor_line--;
      editor.cursor_col = prev_len;
      editor.modified = 1;
    }
    return 0;
  }

   
  if (c >= 32 && c < 127) {
    if (line_len < EDITOR_MAX_LINE_LEN - 1) {
       
      for (size_t i = line_len + 1; i > editor.cursor_col; i--) {
        line[i] = line[i - 1];
      }
      line[editor.cursor_col] = (char)c;
      editor.cursor_col++;
      editor.modified = 1;
    }
    return 0;
  }

  return 0;
}

 
void editor_open(const char *filename) {
  if (!filename || !*filename) {
    console_print("Usage: edit <filename>\n");
    return;
  }

   
  editor_init(filename);

   
  editor_load_file();

   
  console_clear();

   
  while (1) {
    editor_render();

     
    int c = 0;
    while (c == 0) {
      if (keyboard_has_char()) {
        c = keyboard_getc();
      } else if (uart_has_char()) {
        c = uart_getc();
      }
      __asm__ volatile("yield");
    }

     
    if (editor_handle_key(c)) {
      break;  
    }
  }

   
  console_clear();
  console_print("Exited editor.\n");
}
