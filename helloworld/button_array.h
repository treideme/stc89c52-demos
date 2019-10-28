#ifndef _button_array_h_
#define _button_array_h_

#include <stdint.h>
#include <mcs51/compiler.h>
#include <mcs51/8051.h>

/**
 * Each bit represents SW1..SW16
 */
extern uint16_t g_buttons_state;

void buttons_poll();

#endif // _button_array_h_
