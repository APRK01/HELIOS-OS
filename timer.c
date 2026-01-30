#include "timer.h"
#include "console.h"
#include "gic.h"

static uint64_t timer_interval = 0;

// Read counter frequency
static inline uint64_t read_cntfrq(void) {
  uint64_t val;
  __asm__ volatile("mrs %0, cntfrq_el0" : "=r"(val));
  return val;
}

// Write timer value (countdown)
static inline void write_cntv_tval(uint64_t val) {
  __asm__ volatile("msr cntv_tval_el0, %0" ::"r"(val));
}

// Write timer control
static inline void write_cntv_ctl(uint64_t val) {
  __asm__ volatile("msr cntv_ctl_el0, %0" ::"r"(val));
}

void timer_init(uint64_t interval_ms) {
  // Get frequency
  uint64_t freq = read_cntfrq();
  console_print("TIMER: Frequency = ");
  console_print_dec(freq / 1000000);
  console_print(" MHz\n");

  // Calculate ticks for interval
  timer_interval = (freq * interval_ms) / 1000;

  // Set timer value
  write_cntv_tval(timer_interval);

  // Enable timer (bit 0), unmask (bit 1 = 0)
  write_cntv_ctl(1);

  // Enable IRQ in GIC
  gic_enable_irq(TIMER_IRQ);

  console_print("TIMER: Initialized (");
  console_print_dec(interval_ms);
  console_print("ms interval).\n");
}

// Called by IRQ handler to reset timer
void timer_reload(void) { write_cntv_tval(timer_interval); }
