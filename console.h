#ifndef CONSOLE_H
#define CONSOLE_H

#include "limine.h"
#include <stddef.h>
#include <stdint.h>

// Initialize the console with the framebuffer
void console_init(struct limine_framebuffer *framebuffer);

// Print a single character
void console_putchar(char c);

// Print a string to the console
void console_print(const char *str);

void console_print_hex(uint64_t n);
void console_print_dec(uint64_t n);
void console_clear(void);
void console_backspace(void);
void console_set_cursor_visible(int visible);

// Cursor control
uint32_t console_get_cursor_x(void);
uint32_t console_get_cursor_y(void);
void console_set_cursor(uint32_t x, uint32_t y);

#endif // CONSOLE_H
