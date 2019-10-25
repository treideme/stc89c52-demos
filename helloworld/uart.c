#include "uart.h"

#include <mcs51/8051.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "burst.h"
#include "stc89xx.h"

static volatile burst_buf_t s_uart_tx_buf;

void uart_putc(char c) {\
    while(!burst_empty(s_uart_tx_buf));

    burst_init(s_uart_tx_buf, &c, 1);
    SBUF = burst_peek(s_uart_tx_buf);

    // Enable UART interrupts
    ES = 1;
}

void uart_puts(char*str, const int8_t len) {
    while(!burst_empty(s_uart_tx_buf));

    burst_init(s_uart_tx_buf, str, len);
    SBUF = burst_peek(s_uart_tx_buf);

    // Enable UART interrupts
    ES = 1;
}

void uart_isr_callback(void) {
    if (RI) {
        RI = 0;             //Clear receive interrupt flag
        P0 = SBUF;          //P0 show UART data
    }
    if (TI) {
        TI = 0;              //Clear transmit interrupt flag
        burst_pop(s_uart_tx_buf);

        if(!burst_empty(s_uart_tx_buf)) {
            SBUF = burst_peek(s_uart_tx_buf);
        }
    }
}

