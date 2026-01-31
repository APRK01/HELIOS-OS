#include "timer.h"
#include "console.h"
#include "gic.h"

static uint64_t timer_interval = 0;

 
static inline uint64_t read_cntfrq(void) {
  uint64_t val;
  __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(val));
  return val;
}

 
static inline void write_cntv_tval(uint64_t val) {
  __asm__ volatile("msr cntv_tval_el0, %0" ::"r"(val));
}

 
static inline void write_cntv_ctl(uint64_t val) {
  __asm__ volatile("msr cntv_ctl_el0, %0" ::"r"(val));
}

void timer_init(uint64_t interval_ms) {
   
  uint64_t freq = read_cntfrq();
  console_print("TIMER: Frequency = ");
  console_print_dec(freq / 1000000);
  console_print(" MHz\n");

   
  timer_interval = (freq * interval_ms) / 1000;

   
  write_cntv_tval(timer_interval);

   
  write_cntv_ctl(1);

   
  gic_enable_irq(TIMER_IRQ);

  console_print("TIMER: Initialized (");
  console_print_dec(interval_ms);
  console_print("ms interval).\n");
}

 
void timer_reload(void) { write_cntv_tval(timer_interval); }
