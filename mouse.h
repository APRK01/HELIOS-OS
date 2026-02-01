#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

int mouse_init(uint64_t hhdm_offset, uint64_t kernel_vbase,
               uint64_t kernel_pbase);
void mouse_poll(void);
int mouse_get_x(void);
int mouse_get_y(void);
int mouse_get_buttons(void);

#define MOUSE_BTN_LEFT (1 << 0)
#define MOUSE_BTN_RIGHT (1 << 1)
#define MOUSE_BTN_MIDDLE (1 << 2)

#endif
