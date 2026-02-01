#ifndef GUI_H
#define GUI_H

#include <stdint.h>

#define COLOR_BG 0x0A0A0A
#define COLOR_SURFACE 0x1A1A1A
#define COLOR_BORDER 0x2A2A2A
#define COLOR_TEXT 0xE0E0E0
#define COLOR_TEXT_DIM 0x808080
#define COLOR_ACCENT 0xD93025
#define COLOR_ACCENT_H 0xFF4444
#define COLOR_WHITE 0xFFFFFF
#define COLOR_TITLEBAR 0x1A1A1A

void gui_fill_rect(int x, int y, int w, int h, uint32_t color);
void gui_draw_rect(int x, int y, int w, int h, uint32_t color, int thickness);
void gui_draw_text(int x, int y, const char *text, uint32_t color);
void gui_draw_gradient_h(int x, int y, int w, int h, uint32_t c1, uint32_t c2);
void gui_draw_gradient_v(int x, int y, int w, int h, uint32_t c1, uint32_t c2);

typedef struct {
  int x, y, w, h;
  const char *label;
  uint32_t bg_color;
  uint32_t hover_color;
  uint32_t text_color;
  int hovered;
  int pressed;
} gui_button_t;

int gui_button_update(gui_button_t *btn, int mx, int my, int mb);
void gui_button_draw(gui_button_t *btn);

typedef struct {
  int x, y, w, h;
  const char *title;
  int visible;
  int dragging;
  int drag_off_x, drag_off_y;
  int close_hovered;
} gui_window_t;

void gui_window_draw(gui_window_t *win);
int gui_window_update(gui_window_t *win, int mx, int my, int mb, int *closed);

typedef struct {
  int x, y;
  const char *label;
  int id;
  int hovered;
} gui_icon_t;

void gui_icon_draw(gui_icon_t *icon);
int gui_icon_update(gui_icon_t *icon, int mx, int my, int mb);

int gui_get_screen_width(void);
int gui_get_screen_height(void);

void gui_set_hhdm(uint64_t hhdm);
void gui_desktop_run(void);

#endif
