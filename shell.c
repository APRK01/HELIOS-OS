#include "shell.h"
#include "console.h"
#include "donut.h"
#include "editor.h"
#include "fetch_logo.h"
#include "gui.h"
#include "heap.h"
#include "keyboard.h"
#include "mouse.h"
#include "pmm.h"
#include "process.h"
#include "string.h"
#include "uart.h"
#include "vfs.h"
#include <stddef.h>
#include <stdint.h>

fs_node_t *cwd = NULL;

#define CMD_BUFFER_SIZE 128
#define PROMPT "HELIOS:> "

#include "string.h"

static void hcf(void) {
  for (;;) {
    __asm__ volatile("wfi");
  }
}

static void cmd_version(void) { console_print("Helios OS v0.1-ARM64\n"); }

static void cmd_cls(void) { console_clear(); }

static void cmd_halt(void) {
  console_print("System halting...\n");
  hcf();
}

static void cmd_help(void) {
  console_print("Available commands:\n");
  console_print("  version    - Show OS version\n");
  console_print("  cls        - Clear screen\n");
  console_print("  help       - Show this help\n");
  console_print("  fetch      - Show system info\n");
  console_print("  halt       - Stop the CPU\n");
  console_print("  memtest    - Run memory allocation test\n");
  console_print("  ls         - List directory contents\n");
  console_print("  mkdir <d>  - Create a directory\n");
  console_print("  touch <f>  - Create an empty file\n");
  console_print("  write <f>  - Write text to a file\n");
  console_print("  cat <f>    - Show file contents\n");
  console_print("  cd <dir>   - Change directory\n");
  console_print("  echo <txt> - Print text to console\n");
  console_print("  pwd        - Print working directory\n");
  console_print("  rm <f>     - Remove a file\n");
  console_print("  rmdir <d>  - Remove empty directory\n");
  console_print("  cp <s> <d> - Copy a file\n");
  console_print("  mv <s> <d> - Move/rename a file\n");
  console_print("  edit <f>   - Text editor (F2 save, ESC quit)\n");
  console_print("  donut      - Spinner 🍩\n");
  console_print("  mouse      - Mouse demo 🖱️\n");
  console_print("  desktop    - Launch Plasma Desktop\n");
  console_print("  multitask  - Run multitasking demo\n");
}

static void cmd_fetch(void) {
  uint64_t mem = pmm_get_total_memory() / (1024 * 1024);
  int logo_height = sizeof(fetch_logo) / sizeof(char *);
  int info_count = 6;
  int total_lines = (logo_height > info_count) ? logo_height : info_count;

  int info_start_line =
      (logo_height > info_count) ? (logo_height - info_count) / 2 : 0;

  int info_col = 65;

  console_print("\n");
  for (int i = 0; i < total_lines; i++) {
    console_print("\n");
  }

  uint32_t final_y = console_get_cursor_y();

  uint32_t start_y = final_y - total_lines;

  for (int i = 0; i < total_lines; i++) {

    console_set_cursor(0, start_y + i);
    if (i < logo_height) {
      console_print(fetch_logo[i]);
    }

    console_set_cursor(info_col, start_y + i);
    int info_idx = i - info_start_line;
    if (info_idx >= 0 && info_idx < info_count) {
      if (info_idx == 0)
        console_print("\033[91mOS\033[0m      : Helios OS");
      if (info_idx == 1)
        console_print("\033[91mKernel\033[0m  : v0.1-ARM64");
      if (info_idx == 2)
        console_print("\033[94mArch\033[0m    : AArch64 (VirtIO)");
      if (info_idx == 3)
        console_print("\033[94mShell\033[0m   : Helios Shell");
      if (info_idx == 4) {
        console_print("\033[92mMemory\033[0m  : ");
        char buf[32];
        itoa(mem, buf, 10);
        console_print(buf);
        console_print(" MiB / ");
        console_print(buf);
        console_print(" MiB");
      }
      if (info_idx == 5)
        console_print("\033[93mUptime\033[0m  : 0s (Todo)");
    }
  }

  console_set_cursor(0, final_y);

  uint32_t cy = console_get_cursor_y();
  console_set_cursor(info_col, cy);
  console_print("\033[41m   \033[42m   \033[43m   \033[44m   \033[45m   "
                "\033[46m   \033[47m   \033[0m\n");
  console_print("\n");
}

#include "heap.h"

static void cmd_memtest(void) {
  console_print("Testing malloc(100)...\n");
  char *ptr = (char *)malloc(100);
  if (ptr) {
    console_print("Allocated at: ");
    console_print_hex((uint64_t)ptr);
    console_print("\n");

    console_print("Writing data...\n");
    ptr[0] = 'H';
    ptr[1] = 'i';
    ptr[99] = '!';
    console_print("Data valid.\n");

    console_print("Freeing...\n");
    free(ptr);
    console_print("Freed.\n");
  } else {
    console_print("Malloc failed!\n");
  }
}

static void cmd_ls(void) {
  if (cwd == NULL)
    cwd = fs_root;
  if (cwd == NULL) {
    console_print("Error: Filesystem not mounted.\n");
    return;
  }

  struct dirent *entry;
  uint32_t i = 0;
  while ((entry = vfs_readdir(cwd, i)) != NULL) {
    console_print(entry->name);

    fs_node_t *node = vfs_finddir(cwd, entry->name);
    if (node && (node->flags & 0x7) == FS_DIRECTORY) {
      console_print("/");
    }
    console_print("  ");
    i++;
  }
  console_print("\n");
}

static void cmd_touch(char *args) {
  if (cwd == NULL)
    cwd = fs_root;
  if (!args || !*args) {
    console_print("Usage: touch <filename>\n");
    return;
  }
  fs_node_t *f = vfs_create(cwd, args);
  if (f)
    console_print("File created.\n");
  else
    console_print("Failed to create file.\n");
}

static void cmd_mkdir(char *args) {
  if (cwd == NULL)
    cwd = fs_root;
  if (!args || !*args) {
    console_print("Usage: mkdir <dirname>\n");
    return;
  }
  fs_node_t *d = vfs_mkdir(cwd, args);
  if (d)
    console_print("Directory created.\n");
  else
    console_print("Failed to create directory.\n");
}

static void cmd_cat(char *args) {
  if (cwd == NULL)
    cwd = fs_root;
  if (!args || !*args) {
    console_print("Usage: cat <filename>\n");
    return;
  }
  fs_node_t *f = vfs_finddir(cwd, args);
  if (!f) {
    console_print("File not found.\n");
    return;
  }
  if ((f->flags & 0x7) == FS_DIRECTORY) {
    console_print("Error: Is a directory\n");
    return;
  }

  char buffer[256];
  uint64_t offset = 0;
  uint64_t read_bytes = 0;
  while ((read_bytes = vfs_read(f, offset, 255, (uint8_t *)buffer)) > 0) {
    buffer[read_bytes] = 0;
    console_print(buffer);
    offset += read_bytes;
  }
  console_print("\n");
}

static void cmd_write(char *args) {
  if (cwd == NULL)
    cwd = fs_root;
  if (!args || !*args) {
    console_print("Usage: write <filename> <text>\n");
    return;
  }

  char *filename = args;
  char *content = NULL;
  for (int i = 0; args[i]; i++) {
    if (args[i] == ' ') {
      args[i] = 0;
      content = &args[i + 1];
      while (*content == ' ')
        content++;
      break;
    }
  }

  if (!content) {
    console_print("Usage: write <filename> <text>\n");
    return;
  }

  fs_node_t *f = vfs_finddir(cwd, filename);
  if (!f) {
    f = vfs_create(cwd, filename);
    if (!f) {
      console_print("Failed to create file.\n");
      return;
    }
  }

  uint64_t len = 0;
  while (content[len])
    len++;

  vfs_write(f, 0, len, (uint8_t *)content);
  console_print("Wrote to file.\n");
}

static void cmd_cd(char *args) {
  if (cwd == NULL)
    cwd = fs_root;

  if (!args || !*args || (args[0] == '/' && args[1] == '\0')) {
    cwd = fs_root;
    return;
  }

  fs_node_t *folder = vfs_finddir(cwd, args);
  if (!folder) {
    console_print("Directory not found.\n");
    return;
  }

  cwd = folder;
}

static void task_a(void) {
  for (int i = 0; i < 5; i++) {
    console_print(" [Task A] Running...\n");
    yield();
  }
  console_print(" [Task A] Finished.\n");

  while (1)
    yield();
}

static void task_b(void) {
  for (int i = 0; i < 5; i++) {
    console_print(" [Task B] Running...\n");
    yield();
  }
  console_print(" [Task B] Finished.\n");
  while (1)
    yield();
}

static void cmd_multitask(void) {
  console_print("Starting Cooperative Multitasking Test...\n");
  process_create(task_a, "Task A");
  process_create(task_b, "Task B");

  console_print("Shell yielding to tasks...\n");
  for (int i = 0; i < 15; i++) {
    yield();
  }
  console_print("Test Complete.\n");
}

static void cmd_unknown(const char *cmd) {
  console_print("Error: Unknown command '");
  console_print(cmd);
  console_print("'\n");
}

static void cmd_echo(char *args) {
  if (args && *args) {
    console_print(args);
  }
  console_print("\n");
}

static void cmd_pwd(void) {
  if (cwd == NULL || cwd == fs_root) {
    console_print("/\n");
  } else {

    console_print("/");
    console_print(cwd->name);
    console_print("\n");
  }
}

static void cmd_rm(char *args) {
  if (!args || !*args) {
    console_print("Usage: rm <filename>\n");
    return;
  }

  if (cwd == NULL)
    cwd = fs_root;

  fs_node_t *file = vfs_finddir(cwd, args);
  if (!file) {
    console_print("Error: File not found\n");
    return;
  }

  if (file->flags & FS_DIRECTORY) {
    console_print("Error: Is a directory. Use rmdir.\n");
    return;
  }

  fs_node_t *parent = cwd;
  fs_node_t *prev = NULL;
  fs_node_t *curr = (fs_node_t *)parent->ptr;

  while (curr) {
    if (curr == file) {
      if (prev) {
        prev->next = curr->next;
      } else {
        parent->ptr = curr->next;
      }

      console_print("Removed: ");
      console_print(args);
      console_print("\n");
      return;
    }
    prev = curr;
    curr = curr->next;
  }
  console_print("Error: Could not remove file\n");
}

static void cmd_rmdir(char *args) {
  if (!args || !*args) {
    console_print("Usage: rmdir <directory>\n");
    return;
  }

  if (cwd == NULL)
    cwd = fs_root;

  fs_node_t *dir = vfs_finddir(cwd, args);
  if (!dir) {
    console_print("Error: Directory not found\n");
    return;
  }

  if (!(dir->flags & FS_DIRECTORY)) {
    console_print("Error: Not a directory. Use rm.\n");
    return;
  }

  if (dir->ptr != NULL) {
    console_print("Error: Directory not empty\n");
    return;
  }

  fs_node_t *parent = cwd;
  fs_node_t *prev = NULL;
  fs_node_t *curr = (fs_node_t *)parent->ptr;

  while (curr) {
    if (curr == dir) {
      if (prev) {
        prev->next = curr->next;
      } else {
        parent->ptr = curr->next;
      }
      console_print("Removed directory: ");
      console_print(args);
      console_print("\n");
      return;
    }
    prev = curr;
    curr = curr->next;
  }
  console_print("Error: Could not remove directory\n");
}

static void cmd_cp(char *args) {
  if (!args || !*args) {
    console_print("Usage: cp <source> <destination>\n");
    return;
  }

  char *src = args;
  char *dst = NULL;
  for (int i = 0; args[i]; i++) {
    if (args[i] == ' ') {
      args[i] = 0;
      dst = &args[i + 1];
      while (*dst == ' ')
        dst++;
      break;
    }
  }

  if (!dst || !*dst) {
    console_print("Usage: cp <source> <destination>\n");
    return;
  }

  if (cwd == NULL)
    cwd = fs_root;

  fs_node_t *srcFile = vfs_finddir(cwd, src);
  if (!srcFile) {
    console_print("Error: Source file not found\n");
    return;
  }

  if (srcFile->flags & FS_DIRECTORY) {
    console_print("Error: Cannot copy directories\n");
    return;
  }

  fs_node_t *dstFile = vfs_create(cwd, dst);
  if (!dstFile) {
    console_print("Error: Could not create destination file\n");
    return;
  }

  if (srcFile->length > 0) {
    uint8_t buffer[1024];
    uint64_t offset = 0;
    while (offset < srcFile->length) {
      uint64_t toRead = srcFile->length - offset;
      if (toRead > sizeof(buffer))
        toRead = sizeof(buffer);
      uint64_t read = vfs_read(srcFile, offset, toRead, buffer);
      if (read == 0)
        break;
      vfs_write(dstFile, offset, read, buffer);
      offset += read;
    }
  }

  console_print("Copied ");
  console_print(src);
  console_print(" to ");
  console_print(dst);
  console_print("\n");
}

static void cmd_mv(char *args) {
  if (!args || !*args) {
    console_print("Usage: mv <source> <destination>\n");
    return;
  }

  char *src = args;
  char *dst = NULL;
  for (int i = 0; args[i]; i++) {
    if (args[i] == ' ') {
      args[i] = 0;
      dst = &args[i + 1];
      while (*dst == ' ')
        dst++;
      break;
    }
  }

  if (!dst || !*dst) {
    console_print("Usage: mv <source> <destination>\n");
    return;
  }

  if (cwd == NULL)
    cwd = fs_root;

  fs_node_t *srcFile = vfs_finddir(cwd, src);
  if (!srcFile) {
    console_print("Error: Source file not found\n");
    return;
  }

  int i = 0;
  while (dst[i] && i < 127) {
    srcFile->name[i] = dst[i];
    i++;
  }
  srcFile->name[i] = 0;

  console_print("Renamed ");
  console_print(src);
  console_print(" to ");
  console_print(dst);
  console_print("\n");
}

static void process_command(char *cmd) {

  if (cmd[0] == '\0')
    return;

  char *args = NULL;

  for (int i = 0; cmd[i]; i++) {
    if (cmd[i] == ' ') {
      cmd[i] = 0;
      args = &cmd[i + 1];

      while (*args == ' ')
        args++;
      break;
    }
  }

  if (k_strcmp(cmd, "version") == 0) {
    cmd_version();
  } else if (k_strcmp(cmd, "cls") == 0) {
    cmd_cls();
  } else if (k_strcmp(cmd, "halt") == 0) {
    cmd_halt();
  } else if (k_strcmp(cmd, "help") == 0) {
    cmd_help();
  } else if (k_strcmp(cmd, "fetch") == 0) {
    cmd_fetch();
  } else if (k_strcmp(cmd, "memtest") == 0) {
    cmd_memtest();
  } else if (k_strcmp(cmd, "ls") == 0) {
    cmd_ls();
  } else if (k_strcmp(cmd, "touch") == 0) {
    cmd_touch(args);
  } else if (k_strcmp(cmd, "mkdir") == 0) {
    cmd_mkdir(args);
  } else if (k_strcmp(cmd, "cat") == 0) {
    cmd_cat(args);
  } else if (k_strcmp(cmd, "write") == 0) {
    cmd_write(args);
  } else if (k_strcmp(cmd, "cd") == 0) {
    cmd_cd(args);
  } else if (k_strcmp(cmd, "multitask") == 0) {
    cmd_multitask();
  } else if (k_strcmp(cmd, "echo") == 0) {
    cmd_echo(args);
  } else if (k_strcmp(cmd, "pwd") == 0) {
    cmd_pwd();
  } else if (k_strcmp(cmd, "rm") == 0) {
    cmd_rm(args);
  } else if (k_strcmp(cmd, "rmdir") == 0) {
    cmd_rmdir(args);
  } else if (k_strcmp(cmd, "cp") == 0) {
    cmd_cp(args);
  } else if (k_strcmp(cmd, "mv") == 0) {
    cmd_mv(args);
  } else if (k_strcmp(cmd, "edit") == 0) {
    editor_open(args);
  } else if (k_strcmp(cmd, "donut") == 0) {
    donut_start();
  } else if (k_strcmp(cmd, "mouse") == 0) {
    console_clear();
    console_print("Mouse Demo - Move mouse around. Press Q to quit.\n");
    while (1) {
      mouse_poll();
      int mx = mouse_get_x();
      int my = mouse_get_y();
      console_draw_cursor(mx, my);
      if (keyboard_has_char()) {
        int c = keyboard_getc();
        if (c == 'q' || c == 'Q')
          break;
      }
      for (volatile int i = 0; i < 10000; i++)
        __asm__ volatile("" ::: "memory");
    }
    console_clear();
  } else if (k_strcmp(cmd, "desktop") == 0) {
    gui_desktop_run();
    console_clear();
  } else {

    console_print("Error: Unknown command '");
    console_print(cmd);
    console_print("'\n");
  }
}

#include "keyboard.h"

#define HISTORY_MAX 10
static char history[HISTORY_MAX][CMD_BUFFER_SIZE];
static int history_count = 0;
static int history_pos = 0;

static void history_add(const char *cmd) {
  if (cmd[0] == '\0')
    return;

  if (history_count > 0 && k_strcmp(history[history_count - 1], cmd) == 0) {
    return;
  }

  if (history_count < HISTORY_MAX) {

    int i = 0;
    while (cmd[i] && i < CMD_BUFFER_SIZE - 1) {
      history[history_count][i] = cmd[i];
      i++;
    }
    history[history_count][i] = 0;
    history_count++;
  } else {

    for (int i = 0; i < HISTORY_MAX - 1; i++) {

      char *dst = history[i];
      char *src = history[i + 1];
      while (*src)
        *dst++ = *src++;
      *dst = 0;
    }

    int i = 0;
    while (cmd[i] && i < CMD_BUFFER_SIZE - 1) {
      history[HISTORY_MAX - 1][i] = cmd[i];
      i++;
    }
    history[HISTORY_MAX - 1][i] = 0;
  }
}

void shell_run(void) {
  char cmd_buffer[CMD_BUFFER_SIZE];
  size_t len = 0;
  size_t cursor_pos = 0;

  console_print("Welcome to Helios OS Shell\n");
  console_print("Type 'help' for commands.\n\n");

  while (1) {
    console_print("user@helios:");
    if (cwd && cwd->name[0]) {
      console_print(cwd->name);
    } else {
      console_print("~");
    }
    console_print("$ ");

    uint32_t prompt_x = console_get_cursor_x();
    uint32_t prompt_y = console_get_cursor_y();

    len = 0;
    cursor_pos = 0;
    cmd_buffer[0] = 0;
    history_pos = history_count;

    uint64_t blink_counter = 0;
    int cursor_visible = 0;

    while (1) {
      int c = 0;

      for (int i = 0; i < 50000; i++) {
        if (keyboard_has_char()) {
          c = keyboard_getc();
          break;
        } else if (uart_has_char()) {
          c = uart_getc();
          break;
        }
        __asm__ volatile("yield");
      }

      blink_counter++;
      if (blink_counter > 20) {
        cursor_visible = !cursor_visible;
        console_set_cursor_visible(cursor_visible);
        blink_counter = 0;
      }

      if (c == 0)
        continue;

      console_set_cursor_visible(0);

      if (c == KEY_UP) {
        if (history_pos > 0) {
          history_pos--;

          console_set_cursor(prompt_x, prompt_y);
          for (size_t i = 0; i < len; i++)
            console_putchar(' ');
          console_set_cursor(prompt_x, prompt_y);

          char *src = history[history_pos];
          len = 0;
          while (src[len]) {
            cmd_buffer[len] = src[len];
            console_putchar(src[len]);
            len++;
          }
          cmd_buffer[len] = 0;
          cursor_pos = len;
        }
      } else if (c == KEY_DOWN) {
        if (history_pos < history_count) {
          history_pos++;

          console_set_cursor(prompt_x, prompt_y);
          for (size_t i = 0; i < len; i++)
            console_putchar(' ');
          console_set_cursor(prompt_x, prompt_y);

          if (history_pos < history_count) {

            char *src = history[history_pos];
            len = 0;
            while (src[len]) {
              cmd_buffer[len] = src[len];
              console_putchar(src[len]);
              len++;
            }
            cmd_buffer[len] = 0;
            cursor_pos = len;
          } else {

            cmd_buffer[0] = 0;
            len = 0;
            cursor_pos = 0;
          }
        }
      } else if (c == KEY_LEFT) {

        if (cursor_pos > 0) {

          console_set_cursor(prompt_x + cursor_pos, prompt_y);
          if (cursor_pos < len) {
            console_putchar(cmd_buffer[cursor_pos]);
          } else {
            console_putchar(' ');
          }
          cursor_pos--;
          console_set_cursor(prompt_x + cursor_pos, prompt_y);
        }
      } else if (c == KEY_RIGHT) {

        if (cursor_pos < len) {

          console_set_cursor(prompt_x + cursor_pos, prompt_y);
          console_putchar(cmd_buffer[cursor_pos]);
          cursor_pos++;
          console_set_cursor(prompt_x + cursor_pos, prompt_y);
        }
      } else if (c == KEY_HOME) {

        console_set_cursor(prompt_x + cursor_pos, prompt_y);
        if (cursor_pos < len)
          console_putchar(cmd_buffer[cursor_pos]);
        cursor_pos = 0;
        console_set_cursor(prompt_x, prompt_y);
      } else if (c == KEY_END) {

        console_set_cursor(prompt_x + cursor_pos, prompt_y);
        if (cursor_pos < len)
          console_putchar(cmd_buffer[cursor_pos]);
        cursor_pos = len;
        console_set_cursor(prompt_x + len, prompt_y);
      } else if (c == '\r' || c == '\n') {
        console_putchar('\n');
        cmd_buffer[len] = '\0';
        history_add(cmd_buffer);
        break;
      } else if (c == 0x7F || c == 0x08) {

        if (cursor_pos > 0) {

          for (size_t i = cursor_pos - 1; i < len - 1; i++) {
            cmd_buffer[i] = cmd_buffer[i + 1];
          }
          len--;
          cursor_pos--;
          cmd_buffer[len] = 0;

          console_set_cursor(prompt_x + cursor_pos, prompt_y);
          for (size_t i = cursor_pos; i < len; i++) {
            console_putchar(cmd_buffer[i]);
          }
          console_putchar(' ');

          console_set_cursor(prompt_x + cursor_pos, prompt_y);
        }
      } else if (c >= 32 && c < 127) {

        if (len < CMD_BUFFER_SIZE - 1) {

          for (size_t i = len; i > cursor_pos; i--) {
            cmd_buffer[i] = cmd_buffer[i - 1];
          }
          cmd_buffer[cursor_pos] = (char)c;
          len++;
          cursor_pos++;
          cmd_buffer[len] = 0;

          console_set_cursor(prompt_x + cursor_pos - 1, prompt_y);
          for (size_t i = cursor_pos - 1; i < len; i++) {
            console_putchar(cmd_buffer[i]);
          }

          console_set_cursor(prompt_x + cursor_pos, prompt_y);
        }
      }

      if (c != 0) {
        console_set_cursor(prompt_x + cursor_pos, prompt_y);
        console_set_cursor_visible(1);
      }
    }

    process_command(cmd_buffer);
  }
}
