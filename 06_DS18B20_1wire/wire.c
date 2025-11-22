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
 * @file wire.c 1 Wire example for temperature probe.
 * @author Thomas Reidemeister
 */
#include <mcs51/8051.h>
#include <mcs51/compiler.h> // NOP
#include <stdint.h>

#define DS18B20_DQ P3_7 // 1 wire data pin for DS18B20

void delay(uint16_t t) {
  while (t--) // Simple delay loop (more than 1us at 12MHz)
    ;
}

// 1-wire interface

void wire_init() { // https://www.analog.com/media/en/technical-documentation/data-sheets/ds18b20.pdf p 15
  P3_7 = 0;   // Assert reset pulse
  delay(480); // Hold for at least 480us
  P3_7 = 1;   // Release bus
  delay(5);   // Hysteresis
  while(P3_7); // Wait for presence pulse
  delay(100);
  // FIXME: Add timeout to avoid infinite loop
}

void wire_write_byte(uint8_t byte) {
  volatile uint8_t j = 0;
  for(uint8_t i = 0; i < 8; i++) {
    DS18B20_DQ = 0; // Start time slot
    j++; // small delay
    DS18B20_DQ = (byte & 0x01);  // Write bit (0 or 1 ... math also introduces needed delay)
    j = 6;
    while(j--); // 68us
    DS18B20_DQ = 1; // Finish time slot
    byte >>= 1;
  }
}

uint8_t wire_read_byte(void) {
  volatile uint8_t j = 0, byte = 0;

  for(uint8_t i = 0; i < 8; i++) {
    DS18B20_DQ = 0; // Start time slot
    j++; // small delay
    DS18B20_DQ = 1; // Release bus
    j = 1;
    while(j--); // 68us
    byte |= (DS18B20_DQ<<i);
  }
  return byte;
}

void ds18b20_start_conversion() {
  wire_init();
  wire_write_byte(0xCC); // Skip ROM (only thing connected to P3_7)
  wire_write_byte(0x44); // Convert T
}

int16_t ds18b20_read_temperature() {
  uint16_t temp = 0;
  wire_init();
  wire_write_byte(0xCC); // Skip ROM
  wire_write_byte(0xBE); // Read Scratchpad
  temp = wire_read_byte();
  temp |= wire_read_byte() << 8;
  return temp;
}

uint16_t temp_to_celsius(uint16_t raw) {
  // DS18B20 outputs temperature in 1/16 degrees C
  return (raw * 10) / 16; // Return temperature in 0.1 degrees C
}

// LCD code
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

// Use timer tool https://reidemeister.com/tools -> 10ms delay T0 16-bit
void timer0_init(void) {
  TMOD &= 0xF0;	/* Clear Timer 0 mode bits */
  TMOD |= 0x1;	/* Set Timer 0 mode to 16-bit */
  TH0 = 0xdc;	/* Set Timer 0 high byte for 16-bit mode */
  TL0 = 0x00;	/* Set Timer 0 low byte for 16-bit mode */
  TF0 = 0;	/* Clear Timer 0 overflow flag */
  TR0 = 1;	/* Start Timer 0 */
}

// Use timer to shift 7-segment display
volatile uint8_t seg_digit = 0;     // current display index
volatile uint8_t decimal_index = 2; // where is the decimal point
volatile uint8_t digits[8];         // Buffer for digits

void tf0_isr(void) __interrupt(TF0_VECTOR) {
  LED_DIGIT = 0x00; // Turn off all segments
  P2 = seg_digit<<2; // activate digit i (P2_2..P2_4)

  // Figure out what digit to display
  uint8_t val = segment_map[digits[seg_digit]];
  if(seg_digit == decimal_index) {
    val |= segment_dp;
  }
  LED_DIGIT = val;
  seg_digit++;
  if(seg_digit > 7)
    seg_digit = 0;

  // Reload Timer 0 for next interrupt
  TH0 = 0xfc;	/* Set Timer 0 high byte for 16-bit mode */
  TL0 = 0x66;	/* Set Timer 0 low byte for 16-bit mode */
  TF0 = 0;	/* Clear Timer 0 overflow flag */
}

void int_to_digits(int16_t val, uint8_t *ptr) {
  for(uint8_t i = 0; i < 8; i++) {
    ptr[i] = 0;
  }
  int i = 7; // Start filling the array from the rightmost index

  int16_t temp_N = (val < 0) ? -val : val;

  // Loop while there are digits left and we have array space
  while (temp_N > 0 && i >= 0) {
    // 1. Get the rightmost digit (modulo 10)
    ptr[7-i] = temp_N % 10;

    // 2. Remove the rightmost digit (integer division by 10)
    temp_N = temp_N / 10;

    // 3. Move to the next position in the array (left)
    i = i - 1;
  }
}


void main(void) {
  uint16_t temperature = 0;
  timer0_init();
  // init digits
  for(uint8_t i = 0; i < 8; i++) {
    digits[i] = 0;
  }

  ET0 = 1;	/* Enable Timer 0 interrupt */
  EA  = 1; /* Enable global interrupts */

  for(;;) {
    ds18b20_start_conversion();
    for (uint8_t i = 0; i < 7; i++) {
      delay(10000);
    }
    temperature = ds18b20_read_temperature();
    temperature = temp_to_celsius(temperature);

    EA  = 0; /* Disable for aliasing issues */
    // convert temperature to bcd
    int_to_digits(temperature, digits);
    EA = 1;

  }
}