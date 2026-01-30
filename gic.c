#include "gic.h"
#include "console.h"

// MMIO read/write helpers
static inline void mmio_write(uint64_t addr, uint32_t val) {
  *(volatile uint32_t *)addr = val;
}

static inline uint32_t mmio_read(uint64_t addr) {
  return *(volatile uint32_t *)addr;
}

void gic_init(void) {
  // 1. Disable Distributor
  mmio_write(GICD_CTLR, 0);

  // 2. Enable forwarding of Group 1 interrupts
  mmio_write(GICD_CTLR, 1);

  // 3. CPU Interface: Set priority mask to allow all
  mmio_write(GICC_PMR, 0xFF);

  // 4. Enable CPU Interface
  mmio_write(GICC_CTLR, 1);

  console_print("GIC: Initialized.\n");
}

void gic_enable_irq(uint32_t irq) {
  // Each ISENABLER register handles 32 IRQs
  uint32_t reg = irq / 32;
  uint32_t bit = irq % 32;

  // Set priority (lower = higher priority, 0 is highest)
  uint32_t prio_reg = irq / 4;
  uint32_t prio_shift = (irq % 4) * 8;
  uint64_t prio_addr = GICD_IPRIORITYR + prio_reg * 4;
  uint32_t prio_val = mmio_read(prio_addr);
  prio_val &= ~(0xFF << prio_shift);
  prio_val |= (0x80 << prio_shift); // Priority 0x80 (middle)
  mmio_write(prio_addr, prio_val);

  // For SPIs (32+), set target to CPU 0
  if (irq >= 32) {
    uint32_t target_reg = irq / 4;
    uint32_t target_shift = (irq % 4) * 8;
    uint64_t target_addr = GICD_ITARGETSR + target_reg * 4;
    uint32_t target_val = mmio_read(target_addr);
    target_val |= (1 << target_shift); // CPU 0
    mmio_write(target_addr, target_val);
  }

  // Enable the IRQ
  mmio_write(GICD_ISENABLER + reg * 4, 1 << bit);
}

uint32_t gic_acknowledge(void) {
  return mmio_read(GICC_IAR) & 0x3FF; // IRQ ID is in bits 0-9
}

void gic_end_of_interrupt(uint32_t irq) { mmio_write(GICC_EOIR, irq); }
