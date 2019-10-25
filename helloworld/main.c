/**
 * @file main.c Main entry point for SDCC.
 * @author Thomas Reidemeister <treideme@gmail.com>
 *
 * This file contains the main entry point for SDCC it has to contain any
 * interrupt service routines and the main function.
 */
#include <mcs51/8051.h>
#include "hardware.h"
#include "uart.h"

/******************************************************************************\
* Private type definitions
\******************************************************************************/

/******************************************************************************\
* Private macros
\******************************************************************************/

/******************************************************************************\
 * Private prototypes
\******************************************************************************/

/**
 * Delay routine.
 */
static void delay(unsigned int t);

/******************************************************************************\
 * External declarations
\******************************************************************************/

/******************************************************************************\
 * Private memory declarations
\******************************************************************************/

/******************************************************************************\
 * Public memory declarations
\******************************************************************************/

/******************************************************************************\
 * All Interrupt handlers.
\******************************************************************************/

/**
 * INT0 Interrupt handler.
 */
void INT0_interrupt() __interrupt(IE0_VECTOR) { }

/**
 * Timer0 Interrupt handler.
 */
void Timer0_interrupt() __interrupt(TF0_VECTOR) { }

/**
 * INT1 Interrupt handler.
 */
void INT1_interrupt() __interrupt(IE1_VECTOR) { }

/**
 * Timer1 Interrupt handler.
 */
void Timer1_interrupt() __interrupt(TF1_VECTOR) { }

/**
 * UART0 Interrupt handler.
 */
void UART0_interrupt() __interrupt(SI0_VECTOR) {
    uart_isr_callback();
}

/**
 * Timer2 Interrupt handler.
 */
void Timer2_interrupt() __interrupt(TF2_VECTOR) { }

/**
 * INT2 Interrupt handler.
 */
void INT2_interrupt() __interrupt(IE2_VECTOR) { }

/**
 * INT3 Interrupt handler.
 */
void INT3_interrupt() __interrupt(IE3_VECTOR) { }

/******************************************************************************\
 * Public functions
\******************************************************************************/

/**
 * Main program entry point.
 */
void main() {
    hardware_init();
    ES = 1;                 //Enable UART interrupt
    EA = 1;                 //Open master interrupt switch

    while (1) {
        char c;
        P2_0 = !P2_0;
        P2_1 = 1;
        uint8_t sz = uart_pollc(&c);
        if(sz) {
            uart_putsz("Hello World (got)\r\n");
        } else {
            uart_putsz("Hello World\r\n");
        }
        P2_1 = 0;
        delay(30000);
    }
}

/******************************************************************************\
* Private functions
\******************************************************************************/

static void delay(unsigned int t) {
    while (t--)
        ;
}

