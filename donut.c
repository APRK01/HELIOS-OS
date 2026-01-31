#include "donut.h"
#include "console.h"
#include "keyboard.h"
#include "string.h"

 
#define SHIFT 10
#define ONE (1 << SHIFT)
#define PI_INT 3217  

 
 
 

 
 
 
 
 

 
 
 

static int sin_val[64] = {
    0,    100,  200,  299,  397,  495,  591,  686,  780,  871,  960,
    1047, 1132, 1214, 1293, 1369, 1441, 1510, 1575, 1636, 1693, 1745,
    1793, 1837, 1876, 1911, 1941, 1967, 1988, 2005, 2017, 2024, 2027,
    2024, 2017, 2005, 1988, 1967, 1941, 1911, 1876, 1837, 1793, 1745,
    1693, 1636, 1575, 1510, 1441, 1369, 1293, 1214, 1132, 1047, 960,
    871,  780,  686,  591,  495,  397,  299,  200,  100};
 
 
 
 

 
 
 

static int k_sin(int deg) {
   
  while (deg < 0)
    deg += 360;
  deg %= 360;

   
   
  static const int s[] = {
      0,    18,   36,   54,   71,   89,   107,  125,  143,  160,  178,  195,
      213,  230,  248,  265,  282,  299,  316,  333,  350,  367,  384,  400,
      416,  433,  449,  465,  481,  496,  512,  527,  543,  558,  573,  587,
      602,  616,  630,  644,  658,  672,  685,  698,  711,  724,  737,  749,
      761,  773,  784,  796,  807,  818,  828,  839,  849,  859,  868,  878,
      887,  896,  904,  912,  920,  928,  935,  943,  949,  956,  962,  968,
      974,  979,  984,  989,  994,  998,  1002, 1005, 1008, 1011, 1014, 1016,
      1018, 1020, 1021, 1022, 1023, 1023, 1024};  

  if (deg <= 90)
    return s[deg];
  if (deg <= 180)
    return s[180 - deg];
  if (deg <= 270)
    return -s[deg - 180];
  return -s[360 - deg];
}

static int k_cos(int deg) { return k_sin(deg + 90); }

 
char b[1760];
int z[1760];

static void delay_short(void) {
  for (volatile int i = 0; i < 50000; i++) {
    __asm__ volatile("" ::: "memory");
  }
}

void donut_start(void) {
  console_clear();
  console_set_cursor_visible(0);

  int A = 0, B = 0;

  while (1) {
    if (keyboard_has_char()) {
      if (keyboard_getc() == 'q')
        break;
    }

    k_memset(b, 32, 1760);
    k_memset(z, 0, 1760 * sizeof(int));

     
     

    int cosA = k_cos(A), sinA = k_sin(A);
    int cosB = k_cos(B), sinB = k_sin(B);

    for (int j = 0; j < 360; j += 7) {  
      int ct = k_cos(j);
      int st = k_sin(j);

      for (int i = 0; i < 360; i += 2) {  
        int sp = k_sin(i);
        int cp = k_cos(i);

        int h = ct + 2048;  
                            
         
         
         

        int D = 1024 * 1024 /
                (sp * h / 1024 * sinA / 1024 + st * cosA / 1024 + 5120);
         
         
         
         
         
         
         
         
         

        int t1 = (sp * h) >> 10;
        int t2 = (t1 * sinA) >> 10;
        int t3 = (st * cosA) >> 10;
        int denom = t2 + t3 + 5120;  

         
         

         
         
         

         
        int t4 = (t1 * cosA) >> 10;
        int t5 = (st * sinA) >> 10;
        int t = t4 - t5;

         
         

        int cp_h = (cp * h) >> 10;
        int term_x = (cp_h * cosB - t * sinB) >> 10;
        int term_y = (cp_h * sinB + t * cosB) >> 10;

         
         
         
         

        int x = 40 + (30 * term_x * 1024 / denom) / 1024;  
         
         
         
        x = 40 + (30 * term_x) / denom;
        int y = 12 + (15 * term_y) / denom;

        int o = x + 80 * y;

         
         

        int N = 8 * ((st * sinA - sp * ct / 1024 * cosA / 1024) * cosB / 1024 -
                     sp * ct / 1024 * sinA / 1024 - st * cosA / 1024 -
                     cp * ct / 1024 * sinB / 1024);

         
        int sp_ct = (sp * ct) >> 10;
        int st_sinA = (st * sinA) >> 10;
        int sp_ct_cosA = (sp_ct * cosA) >> 10;
        int part1 = (st_sinA - sp_ct_cosA);
        int part1_cosB = (part1 * cosB) >> 10;

        int sp_ct_sinA = (sp_ct * sinA) >> 10;
        int st_cosA = (st * cosA) >> 10;
        int cp_ct = (cp * ct) >> 10;
        int cp_ct_sinB = (cp_ct * sinB) >> 10;

        int L = part1_cosB - sp_ct_sinA - st_cosA - cp_ct_sinB;
         
         
         

        if (y > 0 && y < 22 && x > 0 && x < 80) {  
          if (denom > z[o]) {                      
             
             
             
             

            if (L > 0) {
              int idx = (L * 10) >> 10;  
              if (idx > 11)
                idx = 11;
              if (idx < 0)
                idx = 0;
              b[o] = ".,-~:;=!*#$@"[idx];
            } else {
              b[o] = '.';
            }
          }
        }
      }
    }

     
    console_set_cursor(0, 0);
     
     
     

    for (int k = 0; k < 1760; k++) {
      if (k % 80 == 79) {
        console_putchar('\n');
      } else {
        console_putchar(b[k] ? b[k] : ' ');
      }
    }

    A += 4;
    B += 2;
    A %= 360;
    B %= 360;

     
    for (volatile int i = 0; i < 5000000; i++) {
      __asm__ volatile("" ::: "memory");
    }
  }

  console_clear();
  console_set_cursor_visible(1);
}
