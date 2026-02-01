#include "gui.h"
#include "compositor.h"
#include "console.h"
#include "heap.h"
#include "keyboard.h"
#include "mouse.h"
#include "string.h"

 
#define PANEL_HEIGHT 44
#define WIN_TITLE_H 30
#define WIN_BORDER 0x3C7FB8
#define WIN_GLASS 0x88BBDD
#define PANEL_COLOR 0xCC222222  
#define START_BTN_COLOR 0x3399DD

 
#define PL031_BASE 0x09010000
static uint64_t rtc_hhdm = 0;

void gui_set_hhdm(uint64_t hhdm) { rtc_hhdm = hhdm; }

static uint32_t rtc_read(void) {
  if (rtc_hhdm == 0)
    return 0;
  volatile uint32_t *rtc = (volatile uint32_t *)(PL031_BASE + rtc_hhdm);
  return rtc[0];
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

 
void gui_desktop_run(void) {
   
  compositor_init();
  int sw = compositor_get_width();
  int sh = compositor_get_height();

   

   
   
   
  window_t *desktop = compositor_create_window(0, 0, sw, sh, 0);
   
   
  window_fill_rect(desktop, 0, 0, sw, sh, 0x1E4A7A);  

   
  window_t *panel =
      compositor_create_window(0, sh - PANEL_HEIGHT, sw, PANEL_HEIGHT, 100);
  panel->alpha = 200;  
  window_fill_rect(panel, 0, 0, sw, PANEL_HEIGHT, 0x000000);
   
  window_fill_rect(panel, 4, 4, 36, 36, START_BTN_COLOR);
  window_draw_text(panel, 12, 14, "K", 0xFFFFFF);

   
  window_t *about_win =
      compositor_create_window(sw / 2 - 150, sh / 2 - 100, 300, 200, 10);
  about_win->alpha = 240;
   
  window_fill_rect(about_win, 0, 0, 300, 30, 0x4A90C2);    
  window_fill_rect(about_win, 0, 30, 300, 170, 0xEEEEEE);  
  window_draw_text(about_win, 10, 8, "About HELIOS", 0xFFFFFF);
  window_draw_text(about_win, 20, 50, "HELIOS OS", 0x000000);
  window_draw_text(about_win, 20, 70, "Wayland-like Compositor", 0x444444);
  window_draw_text(about_win, 20, 90, "v0.3 Plasma/Glass", 0x444444);

   
  window_draw_text(desktop, 30, 30, "> Terminal", 0xFFFFFF);
  window_draw_text(desktop, 30, 80, "O Donut", 0xFFFFFF);
  window_draw_text(desktop, 30, 130, "[] Editor", 0xFFFFFF);

   
  int running = 1;
  int prev_sec = -1;

   
  int dragging_win = 0;
  int drag_off_x = 0;
  int drag_off_y = 0;

  while (running) {
    mouse_poll();
    int mx = mouse_get_x();
    int my = mouse_get_y();
    int mb = mouse_get_buttons();

     
    int cur_sec = rtc_read() % 60;
    if (cur_sec != prev_sec) {
      window_fill_rect(panel, sw - 60, 14, 50, 16,
                       0x000000);  
      draw_clock_to_window(panel, sw - 56, 14);
      prev_sec = cur_sec;
    }

     
     
    if (about_win->visible) {
      if (dragging_win) {
        if (mb & MOUSE_BTN_LEFT) {
          about_win->x = mx - drag_off_x;
          about_win->y = my - drag_off_y;
        } else {
          dragging_win = 0;
        }
      } else {
         
        if ((mb & MOUSE_BTN_LEFT) && mx >= about_win->x &&
            mx < about_win->x + about_win->width && my >= about_win->y &&
            my < about_win->y + 30) {
          dragging_win = 1;
          drag_off_x = mx - about_win->x;
          drag_off_y = my - about_win->y;
        }

         
        if ((mb & MOUSE_BTN_LEFT) &&
            mx >= about_win->x + about_win->width - 30 &&
            mx < about_win->x + about_win->width && my >= about_win->y &&
            my < about_win->y + 30) {
          about_win->visible = 0;
        }
      }
    }

     
    static int was_click = 0;
    if ((mb & MOUSE_BTN_LEFT) && !was_click) {
       
      if (mx < 45 && my > sh - PANEL_HEIGHT) {
         
        about_win->visible = !about_win->visible;
         
        if (about_win->visible) {
           
           
        }
      }

       
      if (mx > 20 && mx < 150) {
        if (my > 20 && my < 50) {  
           
          console_clear();
          return;  
        }
        if (my > 70 && my < 100) {  
          extern void donut_start(void);
          console_clear();
          donut_start();
           
           
           
           
        }
        if (my > 120 && my < 150) {  
          extern void editor_open(const char *);
          console_clear();
          editor_open("new.txt");
        }
      }
    }
    was_click = (mb & MOUSE_BTN_LEFT);

     
    compositor_render(mx, my);

     
    if (keyboard_has_char()) {
      if (keyboard_getc() == 27)
        running = 0;
    }
  }

   
  compositor_destroy_window(desktop);
  compositor_destroy_window(panel);
  compositor_destroy_window(about_win);
   
  console_clear();
}
