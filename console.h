#ifndef CONSOLE_H
#define CONSOLE_H

#include "limine.h"
#include <stddef.h>
#include <stdint.h>

 
void console_init(struct limine_framebuffer *framebuffer);

 
void console_putchar(char c);

 
void console_print(const char *str);

void console_print_hex(uint64_t n);
void console_print_dec(uint64_t n);
void console_clear(void);
void console_backspace(void);
void console_set_cursor_visible(int visible);

 
uint32_t console_get_cursor_x(void);
uint32_t console_get_cursor_y(void);
void console_set_cursor(uint32_t x, uint32_t y);
uint32_t console_get_width(void);
uint32_t console_get_height(void);

#endif  
