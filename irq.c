#include "console.h"
#include "gic.h"
#include "process.h"

 
extern void timer_reload(void);

 
void irq_handler(void) {
  uint32_t irq = gic_acknowledge();

   
   

   
  gic_end_of_interrupt(irq);
}
