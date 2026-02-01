#include "compositor.h"
#include "console.h"
#include "heap.h"
#include "string.h"

 
static window_t *window_list = NULL;

 
static uint32_t *screen_backbuffer = NULL;
static int screen_w = 0;
static int screen_h = 0;

static int next_win_id = 1;

extern const uint8_t font_8x16[95][16];

void compositor_init(void) {
  screen_w = console_get_fb_width();
  screen_h = console_get_fb_height();

   
  screen_backbuffer = (uint32_t *)malloc(screen_w * screen_h * 4);
  if (!screen_backbuffer) {
    console_print("COMPOSITOR: Failed to allocate backbuffer!\n");
    return;
  }
  k_memset(screen_backbuffer, 0, screen_w * screen_h * 4);

  console_print("COMPOSITOR: Initialized with resolution ");
  console_print_dec(screen_w);
  console_print("x");
  console_print_dec(screen_h);
  console_print("\n");
}

window_t *compositor_create_window(int x, int y, int w, int h, int z_index) {
  window_t *win = (window_t *)malloc(sizeof(window_t));
  if (!win)
    return NULL;

  win->id = next_win_id++;
  win->x = x;
  win->y = y;
  win->width = w;
  win->height = h;
  win->z_index = z_index;
  win->visible = 1;
  win->alpha = 255;

  win->buffer = (uint32_t *)malloc(w * h * 4);
  if (!win->buffer) {
    free(win);
    return NULL;
  }
  k_memset(win->buffer, 0, w * h * 4);

   
   
  win->next = window_list;
  window_list = win;

  return win;
}

void compositor_destroy_window(window_t *win) {
  if (!win)
    return;

   
  if (window_list == win) {
    window_list = win->next;
  } else {
    window_t *curr = window_list;
    while (curr && curr->next != win) {
      curr = curr->next;
    }
    if (curr) {
      curr->next = win->next;
    }
  }

  if (win->buffer)
    free(win->buffer);
  free(win);
}

 
 
static inline uint32_t blend_colors(uint32_t src, uint32_t dst, uint8_t alpha) {
  if (alpha == 255)
    return src;
  if (alpha == 0)
    return dst;

  uint32_t r_s = (src >> 16) & 0xFF;
  uint32_t g_s = (src >> 8) & 0xFF;
  uint32_t b_s = (src) & 0xFF;

  uint32_t r_d = (dst >> 16) & 0xFF;
  uint32_t g_d = (dst >> 8) & 0xFF;
  uint32_t b_d = (dst) & 0xFF;

   
   
  uint32_t r = (r_s * alpha + r_d * (255 - alpha)) >> 8;
  uint32_t g = (g_s * alpha + g_d * (255 - alpha)) >> 8;
  uint32_t b = (b_s * alpha + b_d * (255 - alpha)) >> 8;

  return (r << 16) | (g << 8) | b;
}

void compositor_render(int cursor_x, int cursor_y) {
  if (!screen_backbuffer)
    return;

   
   
   
   
  for (int i = 0; i < screen_w * screen_h; i++) {
    screen_backbuffer[i] = 0x222222;  
  }

   
   
   
   
   
   
   

#define MAX_WINDOWS 32
  window_t *wins[MAX_WINDOWS];
  int count = 0;
  window_t *curr = window_list;
  while (curr && count < MAX_WINDOWS) {
    wins[count++] = curr;
    curr = curr->next;
  }

   
  for (int i = 0; i < count - 1; i++) {
    for (int j = 0; j < count - i - 1; j++) {
      if (wins[j]->z_index > wins[j + 1]->z_index) {
        window_t *temp = wins[j];
        wins[j] = wins[j + 1];
        wins[j + 1] = temp;
      }
    }
  }

   
  for (int i = 0; i < count; i++) {
    window_t *w = wins[i];
    if (!w->visible)
      continue;

     
    int dest_x_start = w->x;
    int dest_y_start = w->y;
    int dest_x_end = w->x + w->width;
    int dest_y_end = w->y + w->height;

    int src_x = 0;
    int src_y = 0;

     
    if (dest_x_start < 0) {
      src_x -= dest_x_start;
      dest_x_start = 0;
    }
    if (dest_y_start < 0) {
      src_y -= dest_y_start;
      dest_y_start = 0;
    }
    if (dest_x_end > screen_w)
      dest_x_end = screen_w;
    if (dest_y_end > screen_h)
      dest_y_end = screen_h;

    for (int y = dest_y_start; y < dest_y_end; y++) {
      int row_idx = y * screen_w + dest_x_start;
      int win_row_idx = (src_y + (y - dest_y_start)) * w->width + src_x;
      for (int x = dest_x_start; x < dest_x_end; x++) {
        uint32_t src_pixel = w->buffer[win_row_idx++];
         
         
         
         
         

        screen_backbuffer[row_idx] =
            blend_colors(src_pixel, screen_backbuffer[row_idx], w->alpha);
        row_idx++;
      }
    }
  }

   
  int mx = cursor_x;
  int my = cursor_y;

   
  for (int i = 0; i < 12; i++) {
    for (int j = 0; j < 12 - i; j++) {
      int px = mx + j;
      int py = my + i;
      if (px >= 0 && px < screen_w && py >= 0 && py < screen_h) {
         
        if (j == 0 || i == 0 || j == 11 - i)
          screen_backbuffer[py * screen_w + px] = 0x000000;
        else
          screen_backbuffer[py * screen_w + px] = 0xFFFFFF;
      }
    }
  }

   
  extern void put_pixel(uint32_t x, uint32_t y,
                        uint32_t color);  

  for (int y = 0; y < screen_h; y++) {
    for (int x = 0; x < screen_w; x++) {
      put_pixel(x, y, screen_backbuffer[y * screen_w + x]);
    }
  }
}

 

void window_clear(window_t *win, uint32_t color) {
  if (!win || !win->buffer)
    return;
  for (int i = 0; i < win->width * win->height; i++) {
    win->buffer[i] = color;
  }
}

void window_fill_rect(window_t *win, int x, int y, int w, int h,
                      uint32_t color) {
  if (!win)
    return;
  for (int py = y; py < y + h; py++) {
    if (py < 0 || py >= win->height)
      continue;
    for (int px = x; px < x + w; px++) {
      if (px < 0 || px >= win->width)
        continue;
      win->buffer[py * win->width + px] = color;
    }
  }
}

void window_draw_rect(window_t *win, int x, int y, int w, int h, uint32_t color,
                      int thickness) {
  if (!win)
    return;
   
  window_fill_rect(win, x, y, w, thickness, color);
  window_fill_rect(win, x, y + h - thickness, w, thickness, color);
   
  window_fill_rect(win, x, y, thickness, h, color);
  window_fill_rect(win, x + w - thickness, y, thickness, h, color);
}

void window_draw_text(window_t *win, int x, int y, const char *text,
                      uint32_t color) {
  if (!win || !text)
    return;
  int ox = x;
  while (*text) {
    if (*text == '\n') {
      x = ox;
      y += 16;
      text++;
      continue;
    }
    if (*text >= 32 && *text < 127) {
      const uint8_t *glyph = font_8x16[*text - 32];
      for (int row = 0; row < 16; row++) {
        if (y + row < 0 || y + row >= win->height)
          continue;
        uint8_t bits = glyph[row];
        for (int col = 0; col < 8; col++) {
          if (x + col < 0 || x + col >= win->width)
            continue;
          if (bits & (0x80 >> col))
            win->buffer[(y + row) * win->width + (x + col)] = color;
        }
      }
    }
    x += 8;
    text++;
  }
}

void window_move(window_t *win, int dx, int dy) {
  if (win) {
    win->x += dx;
    win->y += dy;
  }
}

int compositor_get_width(void) { return screen_w; }
int compositor_get_height(void) { return screen_h; }
