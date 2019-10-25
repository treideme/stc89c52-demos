#ifndef _uart_h_
#define _uart_h_

#include <stddef.h>
#include <stdint.h>

/**
 * Output a string constant to UART.
 * @param str A statically defined string.
 */
#define uart_putsz(str) uart_puts(str, sizeof(str))

/**
 * Output a single char to UART.
 * @param c
 */
void uart_putc(char c);

/**
 * Output a sequence of data to UART.
 * @param str String to output.
 * @param len Size of string to output.
 */
void uart_puts(char*str, const int8_t len);

/**
 * Poll and return a char if one was in the input buffer.
 * @param c Pointer to char to fill.
 * @return 1 if char was received, 0 if input buffer was empty.
 */
uint8_t uart_pollc(char*c);

/**
 * Interrupt service routine callback from UART.
 */
void uart_isr_callback(void);

#endif /* _uart_h_ */
