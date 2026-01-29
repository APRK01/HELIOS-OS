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

// Get an ASCII character (blocking if no key is available)
char keyboard_getc(void);

#endif // KEYBOARD_H
