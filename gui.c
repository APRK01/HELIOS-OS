#include "gui.h"
#include "compositor.h"
#include "console.h"
#include "keyboard.h"
#include "mouse.h"
#include "string.h"

#undef COLOR_TEXT
#undef COLOR_TEXT_DIM
#undef COLOR_ACCENT
#undef COLOR_ACCENT_H

#define PANEL_HEIGHT 48
#define TITLEBAR_HEIGHT 30
#define START_BTN_SIZE 34
#define TASK_BTN_W 120
#define TASK_BTN_H 28
#define DESKTOP_ICON_W 56
#define DESKTOP_ICON_H 56
#define DESKTOP_ICON_GAP 20
#define MENU_W 240
#define MENU_H 220

#define COLOR_BG_TOP 0x0E1B2B
#define COLOR_BG_BOTTOM 0x162B45
#define COLOR_PANEL 0x141A22
#define COLOR_PANEL_LINE 0x2B3645
#define COLOR_PANEL_TEXT 0xE5EEF9
#define COLOR_ACCENT 0x2D9CDB
#define COLOR_ACCENT_H 0x53B6ED
#define COLOR_ACCENT_DIM 0x1A6E9E
#define COLOR_WIN_BG 0xF3F6FA
#define COLOR_WIN_BORDER 0x2E3A48
#define COLOR_WIN_TITLE 0x233141
#define COLOR_WIN_TITLE_ACTIVE 0x2D3D52
#define COLOR_TEXT 0x0B0F14
#define COLOR_TEXT_DIM 0x5C6B7A
#define COLOR_MENU_BG 0x1B2430

#define PL031_BASE 0x09010000
static uint64_t rtc_hhdm = 0;

void gui_set_hhdm(uint64_t hhdm) { rtc_hhdm = hhdm; }

static uint32_t rtc_read(void) {
  if (rtc_hhdm == 0)
    return 0;
  volatile uint32_t *rtc = (volatile uint32_t *)(PL031_BASE + rtc_hhdm);
  return rtc[0];
}

typedef struct {
  window_t *win;
  window_t *shadow;
  const char *title;
  int visible;
  int id;
  int minimized;
  int maximized;
  int opened;
  int animating;
  int anim_progress;
  int anim_steps;
  int anim_from_x;
  int anim_from_y;
  int anim_from_w;
  int anim_from_h;
  int anim_to_x;
  int anim_to_y;
  int anim_to_w;
  int anim_to_h;
  int prev_x;
  int prev_y;
  int prev_w;
  int prev_h;
  int normal_x;
  int normal_y;
  int normal_w;
  int normal_h;
} app_window_t;

typedef struct {
  char *buf;
  int cap;
  int len;
  int cursor;
} text_buffer_t;

typedef struct {
  int x, y;
  const char *label;
  int app_id;
} desktop_icon_t;

enum {
  APP_TERMINAL = 1,
  APP_DONUT = 2,
  APP_EDITOR = 3,
  APP_ABOUT = 4,
};

static uint32_t lerp_color(uint32_t a, uint32_t b, int t, int tmax) {
  int ar = (a >> 16) & 0xFF;
  int ag = (a >> 8) & 0xFF;
  int ab = a & 0xFF;
  int br = (b >> 16) & 0xFF;
  int bg = (b >> 8) & 0xFF;
  int bb = b & 0xFF;
  int r = ar + ((br - ar) * t) / tmax;
  int g = ag + ((bg - ag) * t) / tmax;
  int bch = ab + ((bb - ab) * t) / tmax;
  return (r << 16) | (g << 8) | bch;
}

static void fill_gradient_v(window_t *win, uint32_t top, uint32_t bottom) {
  if (!win)
    return;
  for (int y = 0; y < win->height; y++) {
    uint32_t c = lerp_color(top, bottom, y, win->height - 1);
    window_fill_rect(win, 0, y, win->width, 1, c);
  }
}

static void draw_clock_to_window(window_t *win, int x, int y) {
  uint32_t epoch = rtc_read();
  int total_sec = epoch % 86400;
  int hr = (total_sec / 3600 + 5) % 24;
  int min = (total_sec / 60) % 60;

  char buf[8];
  buf[0] = '0' + (hr / 10);
  buf[1] = '0' + (hr % 10);
  buf[2] = ':';
  buf[3] = '0' + (min / 10);
  buf[4] = '0' + (min % 10);
  buf[5] = 0;

  window_draw_text(win, x, y, buf, 0xFFFFFF);
}

static void draw_window_chrome(app_window_t *app, int active,
                               int close_hovered) {
  window_t *win = app->win;
  if (!win)
    return;
  window_fill_rect(win, 0, 0, win->width, win->height, COLOR_WIN_BG);
  window_fill_rect(win, 0, 0, win->width, TITLEBAR_HEIGHT,
                   active ? COLOR_WIN_TITLE_ACTIVE : COLOR_WIN_TITLE);
  window_draw_rect(win, 0, 0, win->width, win->height, COLOR_WIN_BORDER, 1);
  window_draw_text(win, 12, 8, app->title, 0xE6EEF9);

  int close_x = win->width - 26;
  int close_y = 6;
  int max_x = win->width - 48;
  int min_x = win->width - 70;
  window_fill_rect(win, min_x, close_y, 18, 18, 0x4A5B6E);
  window_draw_text(win, min_x + 5, close_y + 2, "_", 0xFFFFFF);
  window_fill_rect(win, max_x, close_y, 18, 18, 0x4A5B6E);
  window_draw_text(win, max_x + 3, close_y + 2, "[]", 0xFFFFFF);
  uint32_t close_color = close_hovered ? 0xE55454 : 0xC44A4A;
  window_fill_rect(win, close_x, close_y, 18, 18, close_color);
  window_draw_text(win, close_x + 5, close_y + 2, "x", 0xFFFFFF);
}

static void textbuf_init(text_buffer_t *tb, char *storage, int cap) {
  tb->buf = storage;
  tb->cap = cap;
  tb->len = 0;
  tb->cursor = 0;
  if (cap > 0)
    tb->buf[0] = 0;
}

static void textbuf_insert(text_buffer_t *tb, char c) {
  if (tb->len + 1 >= tb->cap)
    return;
  if (tb->cursor < 0 || tb->cursor > tb->len)
    tb->cursor = tb->len;
  for (int i = tb->len; i > tb->cursor; i--) {
    tb->buf[i] = tb->buf[i - 1];
  }
  tb->buf[tb->cursor] = c;
  tb->len++;
  tb->cursor++;
  tb->buf[tb->len] = 0;
}

static void textbuf_backspace(text_buffer_t *tb) {
  if (tb->len <= 0 || tb->cursor <= 0)
    return;
  for (int i = tb->cursor - 1; i < tb->len - 1; i++) {
    tb->buf[i] = tb->buf[i + 1];
  }
  tb->len--;
  tb->cursor--;
  tb->buf[tb->len] = 0;
}

static void textbuf_newline(text_buffer_t *tb) { textbuf_insert(tb, '\n'); }

static void draw_text_area(window_t *win, text_buffer_t *tb, int x, int y,
                           int w, int h, int show_cursor) {
  int cols = w / 8;
  int rows = h / 16;
  if (cols <= 0 || rows <= 0)
    return;

  char line[256];
  int line_len = 0;
  int row = 0;
  int cursor_row = 0;
  int cursor_col = 0;

  for (int i = 0; i <= tb->len; i++) {
    char c = (i < tb->len) ? tb->buf[i] : '\n';
    if (i == tb->cursor) {
      cursor_row = row;
      cursor_col = line_len;
    }
    if (c == '\n' || line_len >= cols) {
      line[line_len] = 0;
      if (row < rows) {
        window_draw_text(win, x, y + row * 16, line, COLOR_TEXT);
      }
      row++;
      line_len = 0;
      if (c != '\n') {
        line[line_len++] = c;
      }
      if (row >= rows)
        break;
      continue;
    }
    line[line_len++] = c;
  }

  if (show_cursor && cursor_row < rows) {
    int cx = x + cursor_col * 8;
    int cy = y + cursor_row * 16;
    window_fill_rect(win, cx, cy + 14, 8, 2, COLOR_ACCENT);
  }
}

static void draw_app_content(app_window_t *app) {
  window_t *win = app->win;
  if (!win)
    return;
  int content_y = TITLEBAR_HEIGHT + 12;
  switch (app->id) {
  case APP_TERMINAL:
    window_draw_text(win, 18, content_y, "Terminal", COLOR_TEXT);
    window_draw_text(win, 18, content_y + 20,
                     "Embedded terminal (basic).", COLOR_TEXT_DIM);
    window_fill_rect(win, 18, content_y + 54, 120, 26, COLOR_ACCENT);
    window_draw_text(win, 30, content_y + 60, "Open", 0xFFFFFF);
    window_draw_rect(win, 18, content_y + 92, win->width - 36,
                     win->height - content_y - 110, COLOR_PANEL_LINE, 1);
    break;
  case APP_DONUT:
    window_draw_text(win, 18, content_y, "Donut Demo", COLOR_TEXT);
    window_draw_text(win, 18, content_y + 20,
                     "Launch the 3D donut demo.", COLOR_TEXT_DIM);
    window_fill_rect(win, 18, content_y + 54, 120, 26, COLOR_ACCENT);
    window_draw_text(win, 30, content_y + 60, "Open", 0xFFFFFF);
    break;
  case APP_EDITOR:
    window_draw_text(win, 18, content_y, "Text Editor", COLOR_TEXT);
    window_draw_text(win, 18, content_y + 20,
                     "Embedded editor (basic).", COLOR_TEXT_DIM);
    window_fill_rect(win, 18, content_y + 54, 120, 26, COLOR_ACCENT);
    window_draw_text(win, 30, content_y + 60, "Open", 0xFFFFFF);
    window_draw_rect(win, 18, content_y + 92, win->width - 36,
                     win->height - content_y - 110, COLOR_PANEL_LINE, 1);
    break;
  case APP_ABOUT:
  default:
    window_draw_text(win, 18, content_y, "HELIOS OS", COLOR_TEXT);
    window_draw_text(win, 18, content_y + 20,
                     "Modern compositor and desktop shell.", COLOR_TEXT_DIM);
    window_draw_text(win, 18, content_y + 40,
                     "Build: v0.4 Fresh", COLOR_TEXT_DIM);
    break;
  }
}

static int window_hit_test(app_window_t *app, int mx, int my) {
  window_t *win = app->win;
  if (!win || !app->visible)
    return 0;
  return mx >= win->x && mx < win->x + win->width && my >= win->y &&
         my < win->y + win->height;
}

static int titlebar_hit_test(app_window_t *app, int mx, int my) {
  window_t *win = app->win;
  if (!win || !app->visible)
    return 0;
  return mx >= win->x && mx < win->x + win->width && my >= win->y &&
         my < win->y + TITLEBAR_HEIGHT;
}

static int close_hit_test(app_window_t *app, int mx, int my) {
  window_t *win = app->win;
  if (!win || !app->visible)
    return 0;
  int close_x = win->x + win->width - 26;
  int close_y = win->y + 6;
  return mx >= close_x && mx < close_x + 18 && my >= close_y &&
         my < close_y + 18;
}

static int max_hit_test(app_window_t *app, int mx, int my) {
  window_t *win = app->win;
  if (!win || !app->visible)
    return 0;
  int max_x = win->x + win->width - 48;
  int max_y = win->y + 6;
  return mx >= max_x && mx < max_x + 18 && my >= max_y && my < max_y + 18;
}

static int min_hit_test(app_window_t *app, int mx, int my) {
  window_t *win = app->win;
  if (!win || !app->visible)
    return 0;
  int min_x = win->x + win->width - 70;
  int min_y = win->y + 6;
  return mx >= min_x && mx < min_x + 18 && my >= min_y && my < min_y + 18;
}

static int open_btn_hit_test(app_window_t *app, int mx, int my) {
  window_t *win = app->win;
  if (!win || !app->visible)
    return 0;
  int bx = win->x + 18;
  int by = win->y + TITLEBAR_HEIGHT + 12 + 54;
  return mx >= bx && mx < bx + 120 && my >= by && my < by + 26;
}

static void draw_taskbar(window_t *panel, int sw, int start_hovered) {
  window_fill_rect(panel, 0, 0, sw, PANEL_HEIGHT, COLOR_PANEL);
  window_fill_rect(panel, 0, 0, sw, 1, COLOR_PANEL_LINE);
  window_fill_rect(panel, 10, 7, START_BTN_SIZE, START_BTN_SIZE,
                   start_hovered ? COLOR_ACCENT_H : COLOR_ACCENT);
  window_draw_text(panel, 19, 16, "H", 0xFFFFFF);
  draw_clock_to_window(panel, sw - 60, 16);
}

static void draw_start_menu(window_t *menu) {
  if (!menu)
    return;
  window_fill_rect(menu, 0, 0, menu->width, menu->height, COLOR_MENU_BG);
  window_draw_rect(menu, 0, 0, menu->width, menu->height, COLOR_PANEL_LINE, 1);
  window_draw_text(menu, 16, 16, "Launch", 0xDCE8F5);
  window_draw_text(menu, 20, 54, "Terminal", 0xFFFFFF);
  window_draw_text(menu, 20, 86, "Donut Demo", 0xFFFFFF);
  window_draw_text(menu, 20, 118, "Editor", 0xFFFFFF);
  window_draw_text(menu, 20, 150, "About", 0xFFFFFF);
}

void gui_desktop_run(void) {
  compositor_init();
  int sw = compositor_get_width();
  int sh = compositor_get_height();

  static char terminal_storage[4096];
  static char editor_storage[8192];
  text_buffer_t terminal_buf;
  text_buffer_t editor_buf;
  textbuf_init(&terminal_buf, terminal_storage, sizeof(terminal_storage));
  textbuf_init(&editor_buf, editor_storage, sizeof(editor_storage));

  window_t *desktop = compositor_create_window(0, 0, sw, sh, 0);
  window_t *panel =
      compositor_create_window(0, sh - PANEL_HEIGHT, sw, PANEL_HEIGHT, 1000);
  panel->alpha = 240;
  window_t *menu =
      compositor_create_window(12, sh - PANEL_HEIGHT - MENU_H - 8, MENU_W,
                               MENU_H, 900);
  menu->alpha = 245;
  menu->visible = 0;

  app_window_t apps[4];
  k_memset(apps, 0, sizeof(apps));
  apps[0].id = APP_TERMINAL;
  apps[0].title = "Terminal";
  apps[0].win = compositor_create_window(140, 140, 420, 280, 10);
  apps[0].shadow = compositor_create_window(134, 136, 432, 292, 9);
  apps[0].shadow->alpha = 120;
  window_fill_rect(apps[0].shadow, 0, 0, apps[0].shadow->width,
                   apps[0].shadow->height, 0x000000);
  apps[0].visible = 0;
  apps[0].normal_x = apps[0].win->x;
  apps[0].normal_y = apps[0].win->y;
  apps[0].normal_w = apps[0].win->width;
  apps[0].normal_h = apps[0].win->height;

  apps[1].id = APP_DONUT;
  apps[1].title = "Donut Demo";
  apps[1].win = compositor_create_window(180, 180, 420, 260, 11);
  apps[1].shadow = compositor_create_window(174, 176, 432, 272, 10);
  apps[1].shadow->alpha = 120;
  window_fill_rect(apps[1].shadow, 0, 0, apps[1].shadow->width,
                   apps[1].shadow->height, 0x000000);
  apps[1].visible = 0;
  apps[1].normal_x = apps[1].win->x;
  apps[1].normal_y = apps[1].win->y;
  apps[1].normal_w = apps[1].win->width;
  apps[1].normal_h = apps[1].win->height;

  apps[2].id = APP_EDITOR;
  apps[2].title = "Editor";
  apps[2].win = compositor_create_window(220, 160, 460, 300, 12);
  apps[2].shadow = compositor_create_window(214, 156, 472, 312, 11);
  apps[2].shadow->alpha = 120;
  window_fill_rect(apps[2].shadow, 0, 0, apps[2].shadow->width,
                   apps[2].shadow->height, 0x000000);
  apps[2].visible = 0;
  apps[2].normal_x = apps[2].win->x;
  apps[2].normal_y = apps[2].win->y;
  apps[2].normal_w = apps[2].win->width;
  apps[2].normal_h = apps[2].win->height;

  apps[3].id = APP_ABOUT;
  apps[3].title = "About";
  apps[3].win =
      compositor_create_window(sw / 2 - 170, sh / 2 - 140, 340, 220, 13);
  apps[3].shadow =
      compositor_create_window(sw / 2 - 176, sh / 2 - 144, 352, 232, 12);
  apps[3].shadow->alpha = 120;
  window_fill_rect(apps[3].shadow, 0, 0, apps[3].shadow->width,
                   apps[3].shadow->height, 0x000000);
  apps[3].visible = 1;
  apps[3].normal_x = apps[3].win->x;
  apps[3].normal_y = apps[3].win->y;
  apps[3].normal_w = apps[3].win->width;
  apps[3].normal_h = apps[3].win->height;

  desktop_icon_t icons[] = {
      {30, 40, "Terminal", APP_TERMINAL},
      {30, 40 + DESKTOP_ICON_H + DESKTOP_ICON_GAP, "Donut", APP_DONUT},
      {30, 40 + (DESKTOP_ICON_H + DESKTOP_ICON_GAP) * 2, "Editor", APP_EDITOR},
      {30, 40 + (DESKTOP_ICON_H + DESKTOP_ICON_GAP) * 3, "About", APP_ABOUT},
  };

  int running = 1;
  int prev_sec = -1;
  int was_click = 0;
  app_window_t *active_app = NULL;
  app_window_t *drag_app = NULL;
  int drag_off_x = 0;
  int drag_off_y = 0;
  int launch_app_id = 0;
  int task_btn_x[4];
  int task_btn_y = sh - PANEL_HEIGHT + 10;

  while (running) {
    mouse_poll();
    int mx = mouse_get_x();
    int my = mouse_get_y();
    int mb = mouse_get_buttons();

    int layout_x = 10 + START_BTN_SIZE + 16;
    for (int i = 0; i < 4; i++) {
      task_btn_x[i] = layout_x;
      layout_x += TASK_BTN_W + 10;
    }

    int start_hovered =
        mx >= 10 && mx < 10 + START_BTN_SIZE && my >= sh - PANEL_HEIGHT + 7 &&
        my < sh - PANEL_HEIGHT + 7 + START_BTN_SIZE;

    int cur_sec = rtc_read() % 60;
    if (cur_sec != prev_sec) {
      prev_sec = cur_sec;
    }

    if ((mb & MOUSE_BTN_LEFT) && !was_click) {
      int clicked_taskbar = my >= sh - PANEL_HEIGHT;

      if (start_hovered && clicked_taskbar) {
        menu->visible = !menu->visible;
      } else if (menu->visible &&
                 mx >= menu->x && mx < menu->x + menu->width &&
                 my >= menu->y && my < menu->y + menu->height) {
        int rel_y = my - menu->y;
        if (rel_y > 44 && rel_y < 72) {
          apps[0].visible = 1;
          apps[0].minimized = 0;
          apps[0].opened = 1;
          active_app = &apps[0];
          compositor_raise_window(apps[0].shadow);
          compositor_raise_window(apps[0].win);
        } else if (rel_y > 76 && rel_y < 104) {
          apps[1].visible = 1;
          apps[1].minimized = 0;
          apps[1].opened = 1;
          active_app = &apps[1];
          compositor_raise_window(apps[1].shadow);
          compositor_raise_window(apps[1].win);
        } else if (rel_y > 108 && rel_y < 136) {
          apps[2].visible = 1;
          apps[2].minimized = 0;
          apps[2].opened = 1;
          active_app = &apps[2];
          compositor_raise_window(apps[2].shadow);
          compositor_raise_window(apps[2].win);
        } else if (rel_y > 140 && rel_y < 168) {
          apps[3].visible = 1;
          apps[3].minimized = 0;
          apps[3].opened = 1;
          active_app = &apps[3];
          compositor_raise_window(apps[3].shadow);
          compositor_raise_window(apps[3].win);
        }
        menu->visible = 0;
      } else if (clicked_taskbar) {
        int bx = 10 + START_BTN_SIZE + 16;
        for (int i = 0; i < 4; i++) {
          if (!apps[i].win)
            continue;
          int by = sh - PANEL_HEIGHT + 10;
          if (mx >= bx && mx < bx + TASK_BTN_W && my >= by &&
              my < by + TASK_BTN_H) {
            if (apps[i].visible && active_app == &apps[i]) {
              apps[i].minimized = 1;
              apps[i].visible = 1;
              apps[i].animating = 1;
              apps[i].anim_progress = 0;
              apps[i].anim_steps = 8;
              apps[i].anim_from_x = apps[i].win->x;
              apps[i].anim_from_y = apps[i].win->y;
              apps[i].anim_from_w = apps[i].win->width;
              apps[i].anim_from_h = apps[i].win->height;
              apps[i].anim_to_x = bx;
              apps[i].anim_to_y = by;
              apps[i].anim_to_w = TASK_BTN_W;
              apps[i].anim_to_h = TASK_BTN_H;
            } else {
              apps[i].visible = 1;
              if (apps[i].minimized) {
                apps[i].minimized = 0;
                apps[i].animating = 1;
                apps[i].anim_progress = 0;
                apps[i].anim_steps = 8;
                apps[i].anim_from_x = bx;
                apps[i].anim_from_y = by;
                apps[i].anim_from_w = TASK_BTN_W;
                apps[i].anim_from_h = TASK_BTN_H;
                apps[i].anim_to_x = apps[i].normal_x;
                apps[i].anim_to_y = apps[i].normal_y;
                apps[i].anim_to_w = apps[i].normal_w;
                apps[i].anim_to_h = apps[i].normal_h;
              } else {
                apps[i].minimized = 0;
              }
              apps[i].opened = 1;
              active_app = &apps[i];
              compositor_raise_window(apps[i].shadow);
              compositor_raise_window(apps[i].win);
            }
            break;
          }
          bx += TASK_BTN_W + 10;
        }
      } else {
        int icon_count = (int)(sizeof(icons) / sizeof(icons[0]));
        for (int i = 0; i < icon_count; i++) {
          int ix = icons[i].x;
          int iy = icons[i].y;
          if (mx >= ix && mx < ix + DESKTOP_ICON_W && my >= iy &&
              my < iy + DESKTOP_ICON_H) {
            int app_id = icons[i].app_id;
            for (int j = 0; j < 4; j++) {
              if (apps[j].id == app_id) {
                apps[j].visible = 1;
                apps[j].minimized = 0;
                apps[j].opened = 1;
                apps[j].animating = 1;
                apps[j].anim_progress = 0;
                apps[j].anim_steps = 8;
                apps[j].anim_from_x = task_btn_x[j];
                apps[j].anim_from_y = task_btn_y;
                apps[j].anim_from_w = TASK_BTN_W;
                apps[j].anim_from_h = TASK_BTN_H;
                apps[j].anim_to_x = apps[j].normal_x;
                apps[j].anim_to_y = apps[j].normal_y;
                apps[j].anim_to_w = apps[j].normal_w;
                apps[j].anim_to_h = apps[j].normal_h;
                active_app = &apps[j];
                compositor_raise_window(apps[j].shadow);
                compositor_raise_window(apps[j].win);
                break;
              }
            }
            break;
          }
        }

        app_window_t *top = NULL;
        for (int i = 0; i < 4; i++) {
          if (!window_hit_test(&apps[i], mx, my))
            continue;
          if (!top || apps[i].win->z_index > top->win->z_index)
            top = &apps[i];
        }

        if (top) {
          active_app = top;
          compositor_raise_window(top->shadow);
          compositor_raise_window(top->win);
          if (close_hit_test(top, mx, my)) {
            top->visible = 0;
            top->minimized = 0;
            top->animating = 0;
          } else if (min_hit_test(top, mx, my)) {
            top->minimized = 1;
            top->visible = 1;
            top->animating = 1;
            top->anim_progress = 0;
            top->anim_steps = 8;
            top->anim_from_x = top->win->x;
            top->anim_from_y = top->win->y;
            top->anim_from_w = top->win->width;
            top->anim_from_h = top->win->height;
            top->anim_to_x = task_btn_x[top - apps];
            top->anim_to_y = task_btn_y;
            top->anim_to_w = TASK_BTN_W;
            top->anim_to_h = TASK_BTN_H;
          } else if (max_hit_test(top, mx, my)) {
            if (!top->maximized) {
              top->prev_x = top->win->x;
              top->prev_y = top->win->y;
              top->prev_w = top->win->width;
              top->prev_h = top->win->height;
              top->win->x = 12;
              top->win->y = 12;
              window_resize(top->win, sw - 24, sh - PANEL_HEIGHT - 24);
              top->maximized = 1;
              top->normal_x = top->win->x;
              top->normal_y = top->win->y;
              top->normal_w = top->win->width;
              top->normal_h = top->win->height;
            } else {
              top->win->x = top->prev_x;
              top->win->y = top->prev_y;
              window_resize(top->win, top->prev_w, top->prev_h);
              top->maximized = 0;
              top->normal_x = top->win->x;
              top->normal_y = top->win->y;
              top->normal_w = top->win->width;
              top->normal_h = top->win->height;
            }
          } else if (open_btn_hit_test(top, mx, my)) {
            launch_app_id = top->id;
          } else if (titlebar_hit_test(top, mx, my)) {
            drag_app = top;
            drag_off_x = mx - top->win->x;
            drag_off_y = my - top->win->y;
          }
        } else {
          active_app = NULL;
        }
      }
    }

    if (!(mb & MOUSE_BTN_LEFT)) {
      drag_app = NULL;
    }

    if (drag_app && (mb & MOUSE_BTN_LEFT)) {
      drag_app->win->x = mx - drag_off_x;
      drag_app->win->y = my - drag_off_y;
    }

    for (int i = 0; i < 4; i++) {
      if (!apps[i].win)
        continue;
      if (!apps[i].maximized && !apps[i].minimized && !apps[i].animating &&
          apps[i].visible) {
        apps[i].normal_x = apps[i].win->x;
        apps[i].normal_y = apps[i].win->y;
        apps[i].normal_w = apps[i].win->width;
        apps[i].normal_h = apps[i].win->height;
      }
      if (apps[i].animating) {
        int t = apps[i].anim_progress;
        int steps = apps[i].anim_steps;
        if (steps <= 0)
          steps = 1;
        int nx = apps[i].anim_from_x +
                 (apps[i].anim_to_x - apps[i].anim_from_x) * t / steps;
        int ny = apps[i].anim_from_y +
                 (apps[i].anim_to_y - apps[i].anim_from_y) * t / steps;
        int nw = apps[i].anim_from_w +
                 (apps[i].anim_to_w - apps[i].anim_from_w) * t / steps;
        int nh = apps[i].anim_from_h +
                 (apps[i].anim_to_h - apps[i].anim_from_h) * t / steps;
        apps[i].win->x = nx;
        apps[i].win->y = ny;
        window_resize(apps[i].win, nw, nh);
        apps[i].anim_progress++;
        if (apps[i].anim_progress > steps) {
          apps[i].animating = 0;
          apps[i].win->x = apps[i].anim_to_x;
          apps[i].win->y = apps[i].anim_to_y;
          window_resize(apps[i].win, apps[i].anim_to_w, apps[i].anim_to_h);
          if (apps[i].minimized) {
            apps[i].visible = 0;
          }
        }
      }
    }

    fill_gradient_v(desktop, COLOR_BG_TOP, COLOR_BG_BOTTOM);
    int icon_count = (int)(sizeof(icons) / sizeof(icons[0]));
    for (int i = 0; i < icon_count; i++) {
      int ix = icons[i].x;
      int iy = icons[i].y;
      int hover = mx >= ix && mx < ix + DESKTOP_ICON_W && my >= iy &&
                  my < iy + DESKTOP_ICON_H;
      window_fill_rect(desktop, ix, iy, DESKTOP_ICON_W, DESKTOP_ICON_H,
                       hover ? COLOR_ACCENT_DIM : 0x223247);
      window_draw_text(desktop, ix + 8, iy + 20, "[]", 0xFFFFFF);
      window_draw_text(desktop, ix - 2, iy + 62, icons[i].label, 0xFFFFFF);
    }

    for (int i = 0; i < 4; i++) {
      if (!apps[i].win)
        continue;
      apps[i].win->visible = apps[i].visible;
      if (apps[i].shadow) {
        apps[i].shadow->visible = apps[i].visible;
        apps[i].shadow->x = apps[i].win->x - 6;
        apps[i].shadow->y = apps[i].win->y - 4;
        window_resize(apps[i].shadow, apps[i].win->width + 12,
                      apps[i].win->height + 12);
        window_fill_rect(apps[i].shadow, 0, 0, apps[i].shadow->width,
                         apps[i].shadow->height, 0x000000);
      }
      if (!apps[i].visible)
        continue;
      int close_hover =
          close_hit_test(&apps[i], mx, my);
      int active = (active_app == &apps[i]);
      draw_window_chrome(&apps[i], active, close_hover);
      draw_app_content(&apps[i]);

      if (apps[i].id == APP_TERMINAL) {
        int area_x = 24;
        int area_y = TITLEBAR_HEIGHT + 108;
        int area_w = apps[i].win->width - 48;
        int area_h = apps[i].win->height - area_y - 16;
        draw_text_area(apps[i].win, &terminal_buf, area_x, area_y, area_w,
                       area_h, active);
      } else if (apps[i].id == APP_EDITOR) {
        int area_x = 24;
        int area_y = TITLEBAR_HEIGHT + 108;
        int area_w = apps[i].win->width - 48;
        int area_h = apps[i].win->height - area_y - 16;
        draw_text_area(apps[i].win, &editor_buf, area_x, area_y, area_w,
                       area_h, active);
      }
    }

    draw_taskbar(panel, sw, start_hovered);
    int tbx = 10 + START_BTN_SIZE + 16;
    for (int i = 0; i < 4; i++) {
      if (!apps[i].win)
        continue;
      int by = 10;
      uint32_t btn_color = (active_app == &apps[i] && apps[i].visible)
                               ? COLOR_ACCENT
                               : COLOR_PANEL_LINE;
      window_fill_rect(panel, tbx, by, TASK_BTN_W, TASK_BTN_H, btn_color);
      window_draw_text(panel, tbx + 8, by + 6, apps[i].title,
                       COLOR_PANEL_TEXT);
      if (!apps[i].visible && (apps[i].minimized || apps[i].opened)) {
        window_fill_rect(panel, tbx + 8, by + TASK_BTN_H - 4, 24, 2,
                         COLOR_ACCENT_H);
      }
      tbx += TASK_BTN_W + 10;
    }

    menu->visible = menu->visible ? 1 : 0;
    if (menu->visible) {
      draw_start_menu(menu);
    }

    compositor_render(mx, my);

    if (launch_app_id != 0) {
      console_clear();
      if (launch_app_id == APP_TERMINAL) {
        return;
      } else if (launch_app_id == APP_DONUT) {
        extern void donut_start(void);
        donut_start();
      } else if (launch_app_id == APP_EDITOR) {
        extern void editor_open(const char *);
        editor_open("new.txt");
      }
      launch_app_id = 0;
      console_clear();
      return;
    }

    if (keyboard_has_char()) {
      int c = keyboard_getc();
      if (c == 27) {
        running = 0;
      } else if (active_app && active_app->visible && !active_app->minimized &&
                 !active_app->animating) {
        if (active_app->id == APP_TERMINAL) {
          if (c == 8) {
            textbuf_backspace(&terminal_buf);
          } else if (c == '\n') {
            textbuf_newline(&terminal_buf);
          } else if (c >= 32 && c < 127) {
            textbuf_insert(&terminal_buf, (char)c);
          }
        } else if (active_app->id == APP_EDITOR) {
          if (c == 8) {
            textbuf_backspace(&editor_buf);
          } else if (c == '\n') {
            textbuf_newline(&editor_buf);
          } else if (c >= 32 && c < 127) {
            textbuf_insert(&editor_buf, (char)c);
          }
        }
      }
    }

    was_click = (mb & MOUSE_BTN_LEFT);
  }

  compositor_destroy_window(desktop);
  compositor_destroy_window(panel);
  compositor_destroy_window(menu);
  for (int i = 0; i < 4; i++) {
    if (apps[i].shadow)
      compositor_destroy_window(apps[i].shadow);
    compositor_destroy_window(apps[i].win);
  }
  console_clear();
}
