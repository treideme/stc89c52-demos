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
 * @file i2c.c Example for bitbang I2C communication with AT24C02 EEPROM.
 * @author Thomas Reidemeister
 */
#include <mcs51/8051.h>
#include <mcs51/compiler.h> // NOP
#include <stdint.h>

#define IRDA_RX P3_2

// Use timer and ext tool https://reidemeister.com/tools
void timer0_init(void) {
  TMOD &= 0xF0;	/* Clear Timer 0 mode bits */
  TMOD |= 0x1;	/* Set Timer 0 mode to 16-bit */
  TH0 = 0xfc;	/* Set Timer 0 high byte for 16-bit mode */
  TL0 = 0x18;	/* Set Timer 0 low byte for 16-bit mode */
  TF0 = 0;	/* Clear Timer 0 overflow flag */
  TR0 = 1;	/* Start Timer 0 */

  // Also enable interrupt
  ET0 = 1;
}

void ext_init(void) {
  IT0 = 1;	/* INT0 (P3.2) Falling Edge */
  EX0 = 1;	/* Enable INT0 (P3.2) */
}

uint8_t ms_counter = 0;
int8_t pulse_count = 0;
uint32_t pattern = 0;
uint32_t last_pattern = 0xFFFFFFFF;

void tf0_isr(void) __interrupt(TF0_VECTOR) {
  if(ms_counter<50) {
    ms_counter++;
  }

  // Reload Timer 0 for next interrupt
  TH0 = 0xfc;	/* Set Timer 0 high byte for 16-bit mode */
  TL0 = 0x18;	/* Set Timer 0 low byte for 16-bit mode */
}

void int0_isr(void) __interrupt(IE0_VECTOR) {
  uint8_t cur_timer = ms_counter;

  // Reset for next pulse (including resetting timer)
  ms_counter = 0;
  TH0 = 0xfc;	/* Set Timer 0 high byte for 16-bit mode */
  TL0 = 0x18;	/* Set Timer 0 low byte for 16-bit mode */

  pulse_count++;

  if(cur_timer == 50) {
    // Timeout occurred, assume sync pulse
    pulse_count = -2; // Ignore sync edges
    pattern = 0;      // Reset pattern
  } else if(pulse_count >= 0 && pulse_count < 31) {
    // Record bits after sync
    if(cur_timer >= 2) { // Threshold between 0 and 1
      pattern |= (uint32_t)0x00000001 << (31 - pulse_count); // MSB first
      last_pattern = pulse_count;
    }
  }
  if(pulse_count >= 32) {
    // Ignore extra pulses
    last_pattern = pattern;
  }
}

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

#define LED_DIGIT P0

void delay(uint16_t t) {
  while (t--)
    ;
}

void main(void) {
  timer0_init();
  ext_init();
  EA = 1; // Enable global interrupts

  P0 = 0x00; // Initialize port
  P2 = 0x00;

  for(;;) {
    // Display hex digits 0-7 with decimal point on, incrementing on each digitit
    for(uint8_t i=0; i<8; i++) {
      P2 = i<<2; // activate digit i (P2_2..P2_4)

      // Figure out the corresponding nibble in the 32-bit code
      uint8_t nibble = (last_pattern >> i*4) & 0x0F;
      LED_DIGIT = segment_map[nibble];
      delay(200); // Short delay for multiplexing
      LED_DIGIT = 0x00; // Turn off all segments
    }
  }
}


