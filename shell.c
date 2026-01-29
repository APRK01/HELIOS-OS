#include "shell.h"
#include "console.h"
#include "uart.h"
#include <stddef.h>
#include <stdint.h>

#define CMD_BUFFER_SIZE 128
#define PROMPT "HELIOS:> "

// Custom string comparison function
static int k_strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(unsigned char *)s1 - *(unsigned char *)s2;
}

// Halt the CPU
static void hcf(void) {
  for (;;) {
    __asm__ volatile("wfi");
  }
}

// Command handlers
static void cmd_version(void) { console_print("Helios OS v0.1-ARM64\n"); }

static void cmd_cls(void) { console_clear(); }

static void cmd_halt(void) {
  console_print("System halting...\n");
  hcf();
}

static void cmd_help(void) {
  console_print("Available commands:\n");
  console_print("  version  - Show OS version\n");
  console_print("  cls      - Clear screen\n");
  console_print("  help     - Show this help\n");
  console_print("  halt     - Stop the CPU\n");
}

static void cmd_unknown(const char *cmd) {
  console_print("Error: Unknown command '");
  console_print(cmd);
  console_print("'\n");
}

// Process a command
static void process_command(const char *cmd) {
  // Skip empty commands
  if (cmd[0] == '\0') {
    return;
  }

  if (k_strcmp(cmd, "version") == 0) {
    cmd_version();
  } else if (k_strcmp(cmd, "cls") == 0) {
    cmd_cls();
  } else if (k_strcmp(cmd, "halt") == 0) {
    cmd_halt();
  } else if (k_strcmp(cmd, "help") == 0) {
    cmd_help();
  } else {
    cmd_unknown(cmd);
  }
}

#include "keyboard.h"

void shell_run(void) {
  char cmd_buffer[CMD_BUFFER_SIZE];
  size_t cmd_pos = 0;

  // Print welcome message
  console_print("Welcome to Helios OS Shell\n");
  console_print("Type 'help' for available commands\n\n");

  // Main shell loop
  while (1) {
    // Print prompt
    console_print(PROMPT);
    cmd_pos = 0;

    // Read command
    uint64_t blink_counter = 0;
    int cursor_visible = 0;

    while (1) {
      char c = 0;

      // Poll both sources with cursor blink
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

      // Hide cursor to print the actual character
      console_set_cursor_visible(0);

      if (c == '\r' || c == '\n') {
        // End of command
        console_putchar('\n');
        cmd_buffer[cmd_pos] = '\0';
        break;
      } else if (c == 0x7F || c == 0x08) {
        // Backspace (DEL or BS)
        if (cmd_pos > 0) {
          cmd_pos--;
          console_backspace();
        }
      } else if (c >= 32 && c < 127) {
        // Printable character
        if (cmd_pos < CMD_BUFFER_SIZE - 1) {
          cmd_buffer[cmd_pos++] = c;
          console_putchar(c); // Echo to screen
        }
      }
    }

    // Process the command
    process_command(cmd_buffer);
  }
}
