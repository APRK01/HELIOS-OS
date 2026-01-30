#include "console.h"
#include "gic.h"
#include "process.h"

// Defined in timer.c
extern void timer_reload(void);

// Called from vectors.S on IRQ
void irq_handler(void) {
  uint32_t irq = gic_acknowledge();

  // Minimal handler - just ACK timer to prevent infinite IRQs
  // Don't call timer_reload so timer only fires once

  // Signal end of interrupt
  gic_end_of_interrupt(irq);
}
