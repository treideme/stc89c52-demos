#ifndef _uart_h_
#define _uart_h_

#include <stddef.h>
#include <stdint.h>

int8_t uart_putc(const char c);
int8_t uart_puts(const char*str);
int8_t uart_putsn(const char*str, const size_t len);
void uart_isr_callback(void);

#endif /* _uart_h_ */
