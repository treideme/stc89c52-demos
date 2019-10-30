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
#include "lcd_seg.h"
#include "button_array.h"
#include "shift.h"
#include "ds1302.h"

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
void Timer1_interrupt() __interrupt(TF1_VECTOR) {
    TR1 = 0;    // Reload timer to 1kHz
    TH1 = 0xF8;
    TL1 = 0x38;
    TR1 = 1;

    buttons_poll();
    lcd_seg_update_handler();
}

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
    uint8_t data = 0x01;

    DS1302_write(0x8E, 0x00);
    DS1302_write(0x80, 0x00);
    DS1302_write(0x8E, 0x80);

    while (1) {
        // Poll DS1302 for time
        uint8_t seconds = DS1302_read(0x81);

        char c;
        lcd_set_decimal(seconds,-1);
        uint8_t sz = uart_pollc(&c);
        if(sz) {
            uart_putsz("Hello World (got)\r\n");
        } else {
            uart_putsz("Hello World\r\n");
        }
//        // LED8..15 (Enable JP595 for this to work)
//        P3_5 = 0;
//        SHIFT_OUT(P3_4, P3_6, ~g_buttons_state);
//        P3_5 = 1;
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

