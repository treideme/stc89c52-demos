#include "uart.h"

#include <mcs51/8051.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "burst.h"
#include "stc89xx.h"

static volatile burst_buf_t s_uart_tx_buf;
static volatile uint8_t s_uart_rx_byte;
static volatile uint8_t s_uart_rx_full;

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

uint8_t uart_pollc(char*c) {
    uint8_t sz = 0;
    if(s_uart_rx_full) {
        *c = s_uart_rx_byte;
        s_uart_rx_full = 0;
        sz = 1;
    }

    return sz;
}

void uart_isr_callback(void) {
    if (RI) {
        RI = 0;             //Clear receive interrupt flag

        if(!s_uart_rx_full) {
            s_uart_rx_byte = SBUF;
            s_uart_rx_full = 1;
        }
    }
    if (TI) {
        TI = 0;              //Clear transmit interrupt flag
        burst_pop(s_uart_tx_buf);

        if(!burst_empty(s_uart_tx_buf)) {
            SBUF = burst_peek(s_uart_tx_buf);
        }
    }
}

