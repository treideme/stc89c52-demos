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

    // Initialise Timer2 registers for baud rate generation
    // T2_overflow = 0xFFFC
    // baud rate
    //        Fclk = 12000000 * 2 (12T mode)
    // of = -------------------
    //       16* [0xFFFF + 1 - RCAP]
    // 0xFFF3 ~ 57692 ~ 57600 bps
    RCAP2L = 0xF3;     //Initial timer value
    RCAP2H = 0xFF;     //Initial timer value
    //
    T2_MOD = INIT_T2_MOD;
    T2_CON = INIT_T2_CON;

    // Serial Control
    SCON = INIT_SCON;

    // Enable on-chip auxiliary RAM
    AUXR = INIT_AUXR;

    // Configure watchdog
    WDT_CONTR = INIT_WDT_CONTR;

    // Enable all desired interrupts
    IE = INIT_IE;
}
