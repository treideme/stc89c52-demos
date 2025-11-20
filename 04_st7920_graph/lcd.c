/**
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @file lcd.c 7 Segment display interfacing, dynamic switching.
 * @author Thomas Reidemeister
 */
#include <mcs51/8051.h>
#include <mcs51/compiler.h> // NOP
#include <stdint.h>

#include "cindy.h"

#define ST7920_SCLK P2_7
#define ST7920_CS P2_6
#define ST7920_SID P2_5
#define ST7920_RST P3_4

#define ST7920_ADDR            0x80 // Set DDRAM/GDRAM address command
#define ST7920_EXTENDED_MODE   0x34 // Extended instruction set (GRAM vs DRAM)
#define ST7920_GRAPHICS_MODE   0x36 // Graphics mode (actually enable GRAM for display)

void delay(uint16_t t) {
  while (t--) // Simple delay loop (more than 1us at 12MHz)
    ;
}

void st7920_byte(uint8_t d) {
  for(uint8_t i = 0; i < 8; i++) { // MSB first
    ST7920_SCLK = 0; // Toggle bits on rising edge
    ST7920_SID = d & 0x80;
    d <<= 1;
    ST7920_SCLK = 1; // Reset state
  }
  ST7920_SCLK = 0; // Reset state
}

void st7920_command(uint8_t cmd) {
  ST7920_CS = 1;
  st7920_byte(0b11111000);
  //                 |+- RS set to 0 for command
  //                 +-- RW set to 0 for write
  st7920_byte(0xF0 & cmd);        // high nibble
  st7920_byte(0xF0 & (cmd << 4)); // low nibble
  ST7920_CS = 0;
}

void st7920_data(uint8_t data) {
  ST7920_CS = 1;
  st7920_byte(0b11111010);
  //                 |+- RS set to 1 for data
  //                 +-- RW set to 0 for write
  st7920_byte(0xF0 & data);        // high nibble
  st7920_byte(0xF0 & (data << 4)); // low nibble
  ST7920_CS = 0;
}

/**
 * Set graphics cursor position
 * @param x Word of bit mask in X direction (0-8) (i.e. bit 0..128)
 * @param y Row in Y direction (0-63)
 *
 * Note the 12864B-V2.3 seems to be mapped such that 256x32 pixels are 128x64 with the overflow going to the next row.
 *
 * +--------------------+--------------------+
 * | Row 1: 0...7       | Row 32: 8...15     |
 * | Row 2: 0...7       | Row 33: 8...15     |
 * |....                | ...                |
 * +--------------------+--------------------+
 */
void st7920_pos(uint8_t x, uint8_t y) {
  if(y >= 32) { // Wrap around for 128x64 mode
    x += 8;
    y -= 32;
  }
  st7920_command(ST7920_ADDR | (y & 0x3F)); // Set GDRAM Y address
  st7920_command(ST7920_ADDR | (x & 0x0F)); // Set GDRAM X address
}

void st7920_text(const char* str) {
  while (*str) {
    st7920_data((uint8_t)(*str));
    str++;
  }
}

void clear_graphics(void) {
  for(uint8_t row = 0; row < 64; row++) {
    st7920_pos(0,row);
    for(uint8_t col = 0; col < 8; col++) {
      st7920_data(0x00);
      st7920_data(0x00);
    }
  }
}

void st7920_init() { // Figure 8-bit interface from ST7920 datasheet
  ST7920_SCLK = 0; // Reset state
  ST7920_RST = 0; // Force reset
  ST7920_CS = 0;  // Defined state
  delay(40000);
  ST7920_RST = 1;
  delay(40000); // Wait for more than 40ms after Vcc rises to 4.5V
  st7920_command(ST7920_EXTENDED_MODE); // Extended mode to make GDRAM accessible
  clear_graphics();                     // Clear graphics RAM
  st7920_command(ST7920_GRAPHICS_MODE); // Enable GRAM mapping
}

void main(void) {
  st7920_init();

  // Cindy Crawford
  for(uint8_t row = 0; row < 64; row++) {
    st7920_pos(0, row);
    for(uint8_t col = 0; col < 8; col++) {
      st7920_data(cindy_crawford_helmut_newton_bitmask[row * 16 + col * 2]);
      st7920_data(cindy_crawford_helmut_newton_bitmask[row * 16 + col * 2 + 1]);
    }
  }


  for(;;) {
//    // Running line demo
//    for(uint8_t row = 8; row < 64; row+=8) {
//      for(uint8_t col = 0; col < 8; col++) {
//        st7920_pos(col, row);
//        st7920_data(0xFF); // Draw dotted line
//        st7920_data(0xFF); // Draw dotted line
//        delay(50000);
//        // Erase line again
//        st7920_pos(col, row);
//        st7920_data(0x0);
//        st7920_data(0x0);
//      }
//    }
  }
}



