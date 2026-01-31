#ifndef UART_H
#define UART_H

#include <stdint.h>

 
#define UART0_PHYS 0x09000000

 
#define UART_DR 0x00    
#define UART_FR 0x18    
#define UART_IBRD 0x24  
#define UART_FBRD 0x28  
#define UART_LCRH 0x2C  
#define UART_CR 0x30    
#define UART_IMSC 0x38  

 
#define UART_FR_RXFE (1 << 4)  
#define UART_FR_TXFF (1 << 5)  

 
void uart_init(uint64_t vbase);

 
char uart_getc(void);

 
int uart_has_char(void);

 
void uart_putc(char c);

#endif  
