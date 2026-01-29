#include "uart.h"

static uint64_t uart_base = UART0_PHYS; // Default to physical, update in init

// Memory-mapped I/O helpers with barriers
static inline void mmio_write(uint64_t reg, uint32_t val) {
  *(volatile uint32_t *)reg = val;
  __asm__ volatile("dmb sy" ::: "memory");
}

static inline uint32_t mmio_read(uint64_t reg) {
  uint32_t val = *(volatile uint32_t *)reg;
  __asm__ volatile("dmb sy" ::: "memory");
  return val;
}

void uart_init(uint64_t vbase) {
  uart_base = vbase;

  // Disable UART while configuring
  mmio_write(uart_base + UART_CR, 0);

  // Clear pending interrupts
  mmio_write(uart_base + UART_IMSC, 0);

  // Set baud rate (115200 with 24MHz clock)
  mmio_write(uart_base + UART_IBRD, 13);
  mmio_write(uart_base + UART_FBRD, 1);

  // 8 bits, no parity, 1 stop bit, enable FIFOs
  mmio_write(uart_base + UART_LCRH, (3 << 5) | (1 << 4));

  // Enable UART, TX, and RX
  mmio_write(uart_base + UART_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

int uart_has_char(void) {
  return !(mmio_read(uart_base + UART_FR) & UART_FR_RXFE);
}

char uart_getc(void) {
  while (!uart_has_char()) {
    __asm__ volatile("yield");
  }
  return (char)(mmio_read(uart_base + UART_DR) & 0xFF);
}

void uart_putc(char c) {
  while (mmio_read(uart_base + UART_FR) & UART_FR_TXFF) {
    __asm__ volatile("yield");
  }
  mmio_write(uart_base + UART_DR, (uint32_t)c);
}
