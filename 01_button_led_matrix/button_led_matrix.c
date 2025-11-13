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

// character rom for 8x8 matrix display
const uint8_t matrix_chars[] = {
  // 0
  0b00000000, // ........
  0b00011100, // ...###..
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00011100, // ...###..

  // 1
  0b00000000, // ........
  0b00001000, // ....#...
  0b00011000, // ...##...
  0b00001000, // ....#...
  0b00001000, // ....#...
  0b00001000, // ....#...
  0b00001000, // ....#...
  0b00011110, // ...####.

  // 2
  0b00000000, // ........
  0b00011100, // ...###..
  0b00100010, // ..#...#.
  0b00000100, // .....#..
  0b00001000, // ....#...
  0b00010000, // ...#....
  0b00111110, // ..#####.
  0b00000000, // ........

  // 3
  0b00000000, // ........
  0b00011100, // ...###..
  0b00100010, // ..#...#.
  0b00000100, // .....#..
  0b00001100, // ....##..
  0b00000010, // ......#.
  0b00100010, // ..#...#.
  0b00011100, // ...###..

  // 4
  0b00000000, // ........
  0b00000000, // ........
  0b00101000, // ..#.#...
  0b00101000, // ..#.#...
  0b00111110, // ..#####.
  0b00001000, // ....#...
  0b00001000, // ....#...
  0b00001000, // ....#...

  // 5
  0b00000000, // ........
  0b00111110, // ..#####.
  0b00100000, // ..#.....
  0b00111000, // ..###...
  0b00000100, // .....#..
  0b00000010, // ......#.
  0b00100010, // ..#...#.
  0b00011100, // ...###..

  // 6
  0b00000000, // ........
  0b00001100, // ....##..
  0b00010000, // ...#....
  0b00111100, // ..####..
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00011100, // ...###..

  // 7
  0b00000000, // ........
  0b00111110, // ..#####.
  0b00000010, // ......#.
  0b00000100, // .....#..
  0b00001000, // ....#...
  0b00010000, // ...#....
  0b00100000, // ..#.....
  0b00100000, // ..#.....

  // 8
  0b00000000, // ........
  0b00011100, // ...###..
  0b00100010, // ..#...#.
  0b00011100, // ...###..
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00011100, // ...###..

  // 9
  0b00000000, // ........
  0b00011100, // ...###..
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00011110, // ...####.
  0b00000010, // ......#.
  0b00000100, // .....#..
  0b00011000, // ...##...

  // A
  0b00000000, // ........
  0b00001100, // ....##..
  0b00010010, // ...#..#.
  0b00100001, // ..#....#
  0b00111111, // ..######
  0b00100001, // ..#....#
  0b00100001, // ..#....#
  0b00000000, // ........

  // B
  0b00000000, // ........
  0b00111100, // ..####..
  0b00100010, // ..#...#.
  0b00111100, // ..####..
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00111100, // ..####..
  0b00000000, // ........

  // C
  0b00000000, // ........
  0b00011110, // ...####.
  0b00100000, // ..#.....
  0b00100000, // ..#.....
  0b00100000, // ..#.....
  0b00100000, // ..#.....
  0b00011110, // ...####.
  0b00000000, // ........

  // D
  0b00000000, // ........
  0b00111100, // ..####..
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00100010, // ..#...#.
  0b00111100, // ..####..
  0b00000000, // ........

  // E
  0b00000000, // ........
  0b00111110, // ..#####.
  0b00100000, // ..#.....
  0b00111000, // ..###...
  0b00100000, // ..#.....
  0b00100000, // ..#.....
  0b00111110, // ..#####.
  0b00000000, // ........

  // F
  0b00000000, // ........
  0b00111110, // ..#####.
  0b00100000, // ..#.....
  0b00111000, // ..###...
  0b00100000, // ..#.....
  0b00100000, // ..#.....
  0b00100000, // ..#.....
  0b00000000, // ........
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

#define GPIO_KEYPAD P1

int8_t key_value;

void scan_keypad(void) {
  GPIO_KEYPAD = 0x0F; // Enable P10..P13 (all rows) to see if any key is pressed
  NOP(); NOP();
  key_value = -1; // Assume no key pressed
  if(GPIO_KEYPAD != 0x0F) { // A key is pressed, shorted to ground
    switch (GPIO_KEYPAD) {
      case 0x07: // Column 0
        key_value = 0;
        break;
      case 0x0B: // Column 1
        key_value = 1;
        break;
      case 0x0D: // Column 2
        key_value = 2;
        break;
      case 0x0E: // Column 3
        key_value = 3;
        break;
    }
    NOP();
    NOP();
    // Enable all columns and find the row
    GPIO_KEYPAD = 0xF0;
    NOP();
    NOP();
    if (key_value != -1) {
      switch (GPIO_KEYPAD) {
        case 0x70: // Row 0
          key_value += 0;
          break;
        case 0xB0: // Row 1
          key_value += 4;
          break;
        case 0xD0: // Row 2
          key_value += 8;
          break;
        case 0xE0: // Row 3
          key_value += 12;
          break;
      }
    }
  }
  GPIO_KEYPAD = 0x00; // Disable all rows and columns
  NOP(); NOP();
}

void display_digit(int8_t digit) {
  if(digit > 0x0F || digit < 0) return; // Invalid digit
  P0 = 0xFF;
  // Shift a single '1' through the 74HC595 to light up LEDs one by one
  for(uint8_t i=0; i<8; i++) {
    P0 = ~matrix_chars[i+(digit*8)];
    uint8_t scan_line = 7-i; // Scan from top to bottom
    HC575_write((1 << scan_line)); // invert since active low
    HC575_write(0); // invert since active low, to avoid ghosting
//    for(uint16_t j=0; j<30000; j++); // Simple delay (reduce for perceptible zero)
  }
}

void main(void) {
  for(;;) {
    scan_keypad();
    if(key_value >= 0) {
      display_digit(key_value);
    } else {
      HC575_write(0); // Turn off all rows
    }
  }
}