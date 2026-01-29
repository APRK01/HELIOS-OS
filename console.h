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

// Clear the screen
void console_clear(void);

// Backspace (delete previous character)
void console_backspace(void);

// Blinking cursor support
void console_set_cursor_visible(int visible);
void console_draw_cursor(void);

// Get current cursor X position
uint32_t console_get_cursor_x(void);

// Get current cursor Y position
uint32_t console_get_cursor_y(void);

#endif // CONSOLE_H
