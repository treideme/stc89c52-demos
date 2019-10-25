#ifndef _uart_h_
#define _uart_h_

#include <stddef.h>
#include <stdint.h>

#define uart_putsz(str) uart_puts(str, sizeof(str))

void uart_putc(char c);
void uart_puts(char*str, const int8_t len);
void uart_isr_callback(void);

#endif /* _uart_h_ */
