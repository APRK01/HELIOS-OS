#ifndef UART_H
#define UART_H

#include <stdint.h>

// PL011 UART base address for QEMU virt machine (Physical)
#define UART0_PHYS 0x09000000

// PL011 register offsets
#define UART_DR 0x00   // Data Register
#define UART_FR 0x18   // Flag Register
#define UART_IBRD 0x24 // Integer Baud Rate Divisor
#define UART_FBRD 0x28 // Fractional Baud Rate Divisor
#define UART_LCRH 0x2C // Line Control Register
#define UART_CR 0x30   // Control Register
#define UART_IMSC 0x38 // Interrupt Mask Set/Clear

// Flag register bits
#define UART_FR_RXFE (1 << 4) // Receive FIFO empty
#define UART_FR_TXFF (1 << 5) // Transmit FIFO full

// Initialize the UART with the mapped base address
void uart_init(uint64_t vbase);

// Get a character from UART (blocking poll)
char uart_getc(void);

// Check if a character is available (non-blocking)
int uart_has_char(void);

// Send a character via UART
void uart_putc(char c);

#endif // UART_H
