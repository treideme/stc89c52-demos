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

#define ST7920_SCLK P2_7
#define ST7920_CS P2_6
#define ST7920_SID P2_5
#define ST7920_RST P3_4

#define ST7920_FUNC_SET        0x30
#define ST7920_DISP_CLEAR      0x01
#define ST7920_DISP_OFF        0x08
#define ST7920_DISP_ON         0x0C
#define ST7920_CURSOR_ON       0x0E
#define ST7920_CURSOR_OFF      0x0C
#define ST7920_CURSOR_BLINK    0x0F
#define ST7920_RETURN_HOME     0x02
#define ST7920_ENTRY_MODE      0x06
#define ST7920_2_ROWS          0x08
#define ST7920_POSITION        0x80
#define ST7920_ROW2_START      0x40
#define ST7920_CGRAM_ADDR      0x40
#define ST7920_DRAM_ADDR       0x80

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
  delay(10);
  st7920_byte(0b11111000);
  //                 |+- RS set to 0 for command
  //                 +-- RW set to 0 for write
  st7920_byte(0xF0 & (cmd << 4)); // high nibble
  st7920_byte(0x0F & cmd);        // low nibble
  ST7920_CS = 0;
  ST7920_SID = 1;
}

void st7920_data(uint8_t data) {
  ST7920_CS = 1;
  st7920_byte(0b11111010);
  //                 |+- RS set to 1 for data
  //                 +-- RW set to 0 for write
  st7920_byte(0xF0 & (data << 4)); // high nibble
  st7920_byte(0x0F & data);        // low nibble
  ST7920_CS = 0;
  ST7920_SID = 1;
}

void st7920_text(const char* str) {
  while (*str) {
    st7920_data((uint8_t)(*str));
    str++;
  }
}

void st7920_init() { // Figure 8-bit interface from ST7920 datasheet
  ST7920_SCLK = 0; // Reset state
  ST7920_RST = 0; // Force reset
  ST7920_CS = 0;  // Defined state
  ST7920_SID = 1; // Per manual
  delay(40000);
  ST7920_RST = 1;
  delay(40000); // Wait for more than 40ms after Vcc rises to 4.5V
//
//  st7920_command(ST7920_FUNC_SET);
//  delay(100); // Wait for more than 100us
//  st7920_command(ST7920_FUNC_SET);
//  delay(40); // Wait for more than 40us
//
//  st7920_command(ST7920_DISP_OFF);
//  delay(100); // Wait for more than 100us
//
//  st7920_command(ST7920_DISP_CLEAR);
//  delay(10000); // Wait for more than 10ms
//
//  st7920_command(ST7920_ENTRY_MODE);
//  delay(100); // Wait for more than 100us
}

void st7920_custom_char(uint8_t location, const uint8_t* charmap) {
  location &= 0x7; // We only have 8 locations 0-7
  st7920_command(ST7920_CGRAM_ADDR | (location << 3)); // each location takes 8 bytes
  for (uint8_t i = 0; i < 8; i++) {
    st7920_data(charmap[i]);
  }
  // Return to DDRAM
  st7920_command(ST7920_DRAM_ADDR);
}

void main(void) {
  st7920_init();
//  st7920_command(ST7920_DISP_ON);
  st7920_command(0x80);
  st7920_text("Hello, World!");
  for(;;) {
  }
}



