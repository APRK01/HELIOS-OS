#ifndef COMPOSITOR_H
#define COMPOSITOR_H

#include <stddef.h>
#include <stdint.h>

 
typedef struct window {
  int id;
  int x, y;
  int width, height;
  uint32_t *buffer;  
  int z_index;
  int visible;
  uint8_t alpha;  
  struct window *next;
} window_t;

 
void compositor_init(void);

 
window_t *compositor_create_window(int x, int y, int w, int h, int z_index);

 
void compositor_destroy_window(window_t *win);

 
void compositor_render(int cursor_x, int cursor_y);
void compositor_raise_window(window_t *win);

 
void window_clear(window_t *win, uint32_t color);
void window_fill_rect(window_t *win, int x, int y, int w, int h,
                      uint32_t color);
void window_draw_rect(window_t *win, int x, int y, int w, int h, uint32_t color,
                      int thickness);
void window_draw_text(window_t *win, int x, int y, const char *text,
                      uint32_t color);

 
void window_move(window_t *win, int dx, int dy);
void window_resize(window_t *win, int w, int h);

 
int compositor_get_width(void);
int compositor_get_height(void);

#endif
