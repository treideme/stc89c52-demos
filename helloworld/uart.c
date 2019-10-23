#include "uart.h"

#include <mcs51/8051.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "circular.h"
#include "stc89xx.h"

static volatile circular_buf_t s_uart_tx_buf;

int8_t uart_putc(const char c) {\
    ES = 0; // Disable UART interrupt

    // Seed transmission
    if(circular_buffer_empty(&s_uart_tx_buf)) // check if we need to seed UART transmission
        SBUF = c; //Send data to UART buffer

    int8_t res = circular_put(&s_uart_tx_buf, c);       // push char to queue

    // Enable UART interrupts
    ES = 1;

    return res;
}

int8_t uart_puts(const char*str) {
    while(*str) {
        int8_t res = uart_putc(*str++);
        if(res != 0)
            return res;
    }
    return 0;
}

void uart_isr_callback(void) {
    if (RI) {
        RI = 0;             //Clear receive interrupt flag
        P0 = SBUF;          //P0 show UART data
    }
    if (TI) {
        TI = 0;              //Clear transmit interrupt flag
        circular_pop(&s_uart_tx_buf);

        if(!circular_buffer_empty(&s_uart_tx_buf)) {
            char d = circular_peek(&s_uart_tx_buf);
            SBUF = d;
        }
    }
}

