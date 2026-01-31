#include "gic.h"
#include "console.h"

 
static inline void mmio_write(uint64_t addr, uint32_t val) {
  *(volatile uint32_t *)addr = val;
}

static inline uint32_t mmio_read(uint64_t addr) {
  return *(volatile uint32_t *)addr;
}

void gic_init(void) {
   
  mmio_write(GICD_CTLR, 0);

   
  mmio_write(GICD_CTLR, 1);

   
  mmio_write(GICC_PMR, 0xFF);

   
  mmio_write(GICC_CTLR, 1);

  console_print("GIC: Initialized.\n");
}

void gic_enable_irq(uint32_t irq) {
   
  uint32_t reg = irq / 32;
  uint32_t bit = irq % 32;

   
  uint32_t prio_reg = irq / 4;
  uint32_t prio_shift = (irq % 4) * 8;
  uint64_t prio_addr = GICD_IPRIORITYR + prio_reg * 4;
  uint32_t prio_val = mmio_read(prio_addr);
  prio_val &= ~(0xFF << prio_shift);
  prio_val |= (0x80 << prio_shift);  
  mmio_write(prio_addr, prio_val);

   
  if (irq >= 32) {
    uint32_t target_reg = irq / 4;
    uint32_t target_shift = (irq % 4) * 8;
    uint64_t target_addr = GICD_ITARGETSR + target_reg * 4;
    uint32_t target_val = mmio_read(target_addr);
    target_val |= (1 << target_shift);  
    mmio_write(target_addr, target_val);
  }

   
  mmio_write(GICD_ISENABLER + reg * 4, 1 << bit);
}

uint32_t gic_acknowledge(void) {
  return mmio_read(GICC_IAR) & 0x3FF;  
}

void gic_end_of_interrupt(uint32_t irq) { mmio_write(GICC_EOIR, irq); }
