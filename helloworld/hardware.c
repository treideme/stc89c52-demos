#include <stdint.h>
#include <mcs51/8051.h>
#include "hardware.h"


void hardware_init() {
    EA = 0; // Disable all interrupts

    // Power control
    PCON = INIT_PCON;

    // Timer control
    TCON = INIT_TCON;
    TMOD = INIT_TMOD;

    // Initialise timer registers for baud rate generation
    // T2_overflow = 0xFFFC
    // baud rate
    //        Fclk = 12000000
    // of = -------------------
    //       32* [0xFFFF - T2]
    // 125000.0 ~ 115200
    RCAP2L = 0xFC;     //Initial timer value
    RCAP2H = 0xFF;     //Initial timer value
    //
    T2CON = INIT_T2CON;


    // Serial Control
    SCON = INIT_SCON;

    // Enable on-chip auxiliary RAM
    AUXR = INIT_AUXR;

    // Configure watchdog
    WDT_CONTR = INIT_WDT_CONTR;

    // Enable all desired interrupts
    IE = INIT_IE;
}
