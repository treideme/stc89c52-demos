#ifndef _lcd_seg_h_
#define _lcd_seg_h_

#include <stdint.h>
#include <mcs51/8051.h>

#define DIGIT_PORT             P0
#define LATCH_PIN              P1_0
#define TIMER_INTERRUPT_ENABLE ET1

extern uint32_t g_lcd_seg_value;

/**
 * This function cycles the LCD value on the external interface.
 * Call this from a fast timer interrupt routine.
 */
void lcd_seg_update_handler(void);

/**
 * Set the 8-digits to a decimal value.
 * @param value     Value to display.
 * @param dot_digit Which digit has a decimal dot (-1 for none).
 * @return -1 if value does not fit 7-segment array.
 */
int8_t lcd_set_decimal(uint32_t value, int8_t dot_digit);

#endif /// _lcd_seg_h_
