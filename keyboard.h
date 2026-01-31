#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

 
 
 
 
int keyboard_init(uint64_t hhdm_offset, uint64_t kernel_vbase,
                  uint64_t kernel_pbase);

 
int keyboard_has_char(void);

 
 
int keyboard_getc(void);

#define KEY_UP 0x101
#define KEY_DOWN 0x102
#define KEY_LEFT 0x103
#define KEY_RIGHT 0x104
#define KEY_HOME 0x105
#define KEY_END 0x106
#define KEY_PGUP 0x107
#define KEY_PGDN 0x108
#define KEY_ESC 0x109
#define KEY_F1 0x10A
#define KEY_F2 0x10B
#define KEY_DEL 0x10C

#endif  
