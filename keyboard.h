#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// Initialize the VirtIO keyboard
// hhdm_offset: virtual offset of physical memory
// kernel_vbase: virtual base address of the kernel
// kernel_pbase: physical base address of the kernel
int keyboard_init(uint64_t hhdm_offset, uint64_t kernel_vbase,
                  uint64_t kernel_pbase);

// Check if a key is available (non-blocking)
int keyboard_has_char(void);

// Get a key code (blocking)
// Returns 0-255 for ASCII, or special codes below
int keyboard_getc(void);

#define KEY_UP 0x101
#define KEY_DOWN 0x102
#define KEY_LEFT 0x103
#define KEY_RIGHT 0x104
#define KEY_HOME 0x105
#define KEY_END 0x106
#define KEY_PGUP 0x107
#define KEY_PGDN 0x108

#endif // KEYBOARD_H
