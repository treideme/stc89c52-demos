#include "button_array.h"

uint16_t g_buttons_state;

/**
 * Explicitly poll every switch across the port, faster and more compact than
 * loop construct.
 */
void buttons_poll() {
    g_buttons_state = 0;
    uint8_t i = 0;
    P1_3 = 0;
    P1_2 = 1;
    P1_1 = 1;
    P1_0 = 1;
    if(!P1_7)
        g_buttons_state |= 0x1; //SW1
    if(!P1_6)
        g_buttons_state |= 0x10; //SW5
    if(!P1_5)
        g_buttons_state |= 0x100; //SW9
    if(!P1_4)
        g_buttons_state |= 0x1000; //SW13
    P1_3 = 1;
    P1_2 = 0;
    P1_1 = 1;
    P1_0 = 1;
    if(!P1_7)
        g_buttons_state |= 0x2; //SW2
    if(!P1_6)
        g_buttons_state |= 0x20; //SW6
    if(!P1_5)
        g_buttons_state |= 0x200; //SW10
    if(!P1_4)
        g_buttons_state |= 0x2000; //SW14
    P1_3 = 1;
    P1_2 = 1;
    P1_1 = 0;
    P1_0 = 1;
    if(!P1_7)
        g_buttons_state |= 0x4; //SW3
    if(!P1_6)
        g_buttons_state |= 0x40; //SW7
    if(!P1_5)
        g_buttons_state |= 0x400; //SW11
    if(!P1_4)
        g_buttons_state |= 0x4000; //SW15
    P1_3 = 1;
    P1_2 = 1;
    P1_1 = 1;
    P1_0 = 0;
    if(!P1_7)
        g_buttons_state |= 0x8; //SW4
    if(!P1_6)
        g_buttons_state |= 0x80; //SW8
    if(!P1_5)
        g_buttons_state |= 0x800; //SW12
    if(!P1_4)
        g_buttons_state |= 0x8000; //SW16
}
