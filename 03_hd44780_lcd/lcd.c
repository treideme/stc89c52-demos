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
 * @file segment.c 7 Segment display interfacing, dynamic switching.
 * @author Thomas Reidemeister
 */
#include <mcs51/8051.h>
#include <mcs51/compiler.h> // NOP
#include <stdint.h>

#define HD44780_E  P2_7
#define HD44780_RS P2_6
#define HD44780_RW P2_5
#define HD44780_DATA P0

#define HD44780_FUNC_SET        0x30
#define HD44780_DISP_CLEAR      0x01
#define HD44780_DISP_OFF        0x08
#define HD44780_DISP_ON         0x0C
#define HD44780_CURSOR_ON       0x0E
#define HD44780_CURSOR_OFF      0x0C
#define HD44780_CURSOR_BLINK    0x0F
#define HD44780_RETURN_HOME     0x02
#define HD44780_ENTRY_MODE      0x06
#define HD44780_2_ROWS          0x08
#define HD44780_POSITION        0x80
#define HD44780_ROW2_START      0x40

void delay(uint16_t t) {
  while (t--) // Simple delay loop (more than 1us at 12MHz)
    ;
}

void hd44780_byte(uint8_t d) {
  HD44780_E = 1;
  HD44780_DATA = d;
  delay(10); // Enable pulse width
  HD44780_E = 0;
  delay(10); // Data hold time
}

void hd44780_command(uint8_t cmd) {
  HD44780_RS = 0; // Command mode
  HD44780_RW = 0; // Write mode
  hd44780_byte(cmd);
  delay(100); // Wait for command to process
}

void hd44780_data(uint8_t data) {
  HD44780_RS = 1; // Data mode
  HD44780_RW = 0; // Write mode
  hd44780_byte(data);
  delay(100); // Wait for data to process
}

void hd44780_text(const char* str) {
  while (*str) {
    hd44780_data((uint8_t)(*str));
    str++;
  }
}

void hd44780_init() { // Figure 23 from HD44780 datasheet
  delay(15000); // Wait for more than 15ms after Vcc rises to 4.5V

  hd44780_command(HD44780_FUNC_SET);
  delay(5000); // Wait for more than 4.1ms
  hd44780_command(HD44780_FUNC_SET);
  delay(1000); // Wait for more than 1ms
  hd44780_command(HD44780_FUNC_SET);
  delay(100); // Wait for more than 100us

  hd44780_command(HD44780_FUNC_SET | HD44780_2_ROWS);
  hd44780_command(HD44780_DISP_OFF);
  hd44780_command(HD44780_DISP_CLEAR);
  hd44780_command(HD44780_ENTRY_MODE);
}

void main(void) {
  hd44780_init();
  hd44780_command(HD44780_DISP_ON);
  hd44780_text("Hello, World!");
  hd44780_command((HD44780_POSITION | HD44780_ROW2_START) | 1);
  hd44780_text("From 8051!");
  for(;;) {
  }
}



