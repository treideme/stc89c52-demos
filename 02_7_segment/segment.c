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
 * @file segment.c 7 Segment display interfacing.
 * @author Thomas Reidemeister
 */
#include <mcs51/8051.h>
#include <mcs51/compiler.h> // NOP
#include <stdint.h>

const uint8_t segment_map[] = {
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
  0b01111100, // b
  0b00111001, // C
  0b01011110, // d
  0b01111001, // E
  0b01110001, // F
};

const uint8_t segment_dp = 0b10000000; // Decimal point segment

#define LED_DIGIT P0

void main(void) {
  P0 = 0x00; // Initialize port

  for(;;) {
    // Display hex digits 0-F with decimal point on
    for(uint8_t i=0; i<16; i++) {
      LED_DIGIT = segment_map[i] | segment_dp; // Display digit with decimal point
      for(uint16_t j=0; j<60000; j++); // Simple delay
      LED_DIGIT = 0x00; // Turn off all segments
    }
  }
}


