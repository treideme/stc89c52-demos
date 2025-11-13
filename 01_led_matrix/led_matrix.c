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
 * @file led_74H595.c Hello World project for the STC89C52 chip.
 * @author Thomas Reidemeister
 */
#include <mcs51/8051.h>
#include <mcs51/compiler.h> // NOP
#include <stdint.h>

#define SRCLK P3_6
#define RCLK  P3_5
#define SER   P3_4

// draw a zero
uint8_t matrix_rows[] = {
  0b00000000, // ........
  0b00011100, // ...###..
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00011100, // ...###..
};

void HC575_write(uint8_t value) {
  SRCLK=0;
  RCLK=0;
  for(uint8_t i=0; i<8; i++) {
    SER = value >> 7;
    value <<= 1;
    SRCLK = 1;
    NOP();
    NOP();
    SRCLK = 0;
  }
  RCLK = 1;
  NOP();
  NOP();
  RCLK = 0;
}

void main(void) {
  for(;;) {
    P0 = 0xFF;
    // Shift a single '1' through the 74HC595 to light up LEDs one by one
    for(uint8_t i=0; i<8; i++) {
      P0 = ~matrix_rows[i];
      uint8_t scan_line = 7-i; // Scan from top to bottom
      HC575_write((1 << scan_line)); // invert since active low
      HC575_write(0); // invert since active low, to avoid ghosting
      for(uint16_t j=0; j<30000; j++); // Simple delay (reduce for perceptible zero)
    }
  }
}