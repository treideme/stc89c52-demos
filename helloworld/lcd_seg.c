#include "lcd_seg.h"
#include "74ls138.h"

/**
 * Current segment to be updated.
 */
static int8_t s_current_segment = 0;
static int8_t s_dot_digit = -1;

/**
 * +--A--+
 * |     |
 * F     B
 * |     |
 * +--G--+
 * |     |
 * E     C
 * |     |
 * +--D--+ (d)
 */
static const uint8_t s_value_to_hex_segment[] = {
        //dGFEDCBA
        0b00111111, // 0
        0b00000110, // 1
        0b01011011, // 2
        0b01001111, // 3
        0b01100110, // 4
        0b01101101, // 5
        0b01111101, // 6
        0b00000111, // 7
        0b01111111, // 8
        0b01101111, // 9
        0b01110111, // A
        0b11111111, // B (. for distinguishing 8)
        0b00111001, // C
        0b10111111, // D (. for distinguishing 0)
        0b01111001, // E
        0b01110001, // F
};

/**
 * Value to write to the segment.
 */
uint32_t g_lcd_seg_value;

/**
 * Latch to disable LCD.
 */
uint8_t g_lcd_enable = 1;

static uint8_t s_digit(uint32_t *value, uint32_t numerator) {
    uint8_t digit = 0;
    while(*value >= numerator) {
        digit++;
        *value -= numerator;
    }
    return digit;
}

int8_t lcd_set_decimal(uint32_t value, int8_t dot_digit) {
    if(value >= 100000000) {
        return -1;
    }
    volatile uint32_t val = 0;


    int32_t digit = s_digit(&value, 10000000);
    val += digit<<28;
    digit = s_digit(&value, 1000000);
    val += digit<<24;
    digit = s_digit(&value, 100000);
    val += digit<<20;
    digit = s_digit(&value, 10000);
    val += digit<<16;
    digit = s_digit(&value, 1000);
    val += digit<<12;
    digit = s_digit(&value, 100);
    val += digit<<8;
    digit = s_digit(&value, 10);
    val += digit<<4;
    val += value;

    // critical section
    TIMER_INTERRUPT_ENABLE = 0;
    s_dot_digit = dot_digit;
    g_lcd_seg_value = val;
    TIMER_INTERRUPT_ENABLE = 1;

    return 0;
}

void lcd_seg_update_handler(void) {
    // Disable segments
    LATCH_PIN  = 0;

    if(g_lcd_enable) {
        // Select segment to update
        SN74LS138_SET(s_current_segment);

        uint8_t current_val = (g_lcd_seg_value>>(4*s_current_segment))&0xF;
        current_val = s_value_to_hex_segment[current_val];
        if(s_current_segment == s_dot_digit)
            DIGIT_PORT = current_val | 0b10000000;
        else
            DIGIT_PORT = current_val;
        LATCH_PIN  = 1;

        s_current_segment++;
        if(s_current_segment == 8)
            s_current_segment = 0;
    }
}
