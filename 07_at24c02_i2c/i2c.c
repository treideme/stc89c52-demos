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

#define I2C_SCL P2_1
#define I2C_SDA P2_0

#define DELAY_10US() NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP(); NOP()

void delay(uint16_t t) {
  while (t--) // Simple delay loop (more than 1us at 12MHz)
    ;
}

void i2c_start(void) {
  I2C_SDA = 1;
  I2C_SCL = 1;
  DELAY_10US();
  I2C_SDA = 0;
  DELAY_10US();
  I2C_SCL = 0;
  DELAY_10US();
}

void i2c_stop(void) {
  I2C_SDA = 0;
  DELAY_10US();
  I2C_SCL = 1;
  DELAY_10US();
  I2C_SDA = 1;
  DELAY_10US();
}

uint8_t i2c_write(uint8_t byte) {
  uint8_t timer = 0;
  for(uint8_t i = 0; i < 8; i++) { // MSB first
    // send MSB first
    I2C_SDA = byte >> 7;
    byte <<= 1;
    DELAY_10US();
    I2C_SCL = 1;
    DELAY_10US();
    I2C_SCL = 0;
    DELAY_10US();
  }
  // ACK bit
  I2C_SDA = 1; // Release SDA for ACK
  DELAY_10US();
  I2C_SCL = 1;
  while(I2C_SDA) { // Wait for ACK (or timeout as NACK)
    timer++;
    if(timer > 250) {
      I2C_SCL = 0;
      DELAY_10US();
      return 0; // NACK
    }
  }
  I2C_SCL = 0;
  DELAY_10US();
  return 1; // ACK
}

uint8_t i2c_read() {
  uint8_t byte = 0;
  for(uint8_t i = 0; i < 8; i++) {
    I2C_SCL = 1;
    DELAY_10US();
    byte |= I2C_SDA;
    I2C_SCL = 0;
    DELAY_10US();
    byte <<= 1;
  }
  return byte;
}

#define AT24C02_ADDR 0xA0 // 7-bit address + Write bit

void at24c02_write_byte(uint8_t mem_addr, uint8_t data) {
  i2c_start();
  i2c_write(AT24C02_ADDR); // Device address + Write
  i2c_write(mem_addr); // Memory address
  i2c_write(data); // Data byte
  i2c_stop();
}

uint8_t at24c02_read_byte(uint8_t mem_addr) {
  uint8_t data = 0;
  i2c_start();
  i2c_write(AT24C02_ADDR); // Device address + Write
  i2c_write(mem_addr); // Memory address
  i2c_start(); // Repeated start
  i2c_write(AT24C02_ADDR | 0x01); // Device address + Read
  data = i2c_read();
  i2c_stop();
  return data;
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
volatile uint8_t digits[8];         // Buffer for digits

void tf0_isr(void) __interrupt(TF0_VECTOR) {
    LED_DIGIT = 0x00; // Turn off all segments
    P2 = seg_digit<<2; // activate digit i (P2_2..P2_4)

    // Figure out what digit to display
    uint8_t val = segment_map[digits[seg_digit]];
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

#define K3 P3_2
#define K4 P3_3

void main(void) {
  timer0_init();
  EA = 1; // Enable global interrupts
  ET0 = 1;	/* Enable Timer 0 interrupt */

  for(;;) {
    if(!K3) { // Write current_value to address 0x00 on button press
      delay(5000); // Debounce delay
      if(!K3) { // Confirm button still pressed
        EA = 0; // Disable global interrupts
        at24c02_write_byte(0x00, 0xFF);
        EA = 1; // Enable global interrupts
        while (!K3); // Wait for button release
      }
    }
    if(!K4) { // Read byte from address 0x00 on button press
      delay(5000); // Debounce delay
      if(!K4) { // Confirm button still pressed
        EA = 0; // Disable global interrupts
        uint8_t data = at24c02_read_byte(0x00);
        EA = 1; // Enable global interrupts
        int_to_digits(data, digits);
        while (!K4); // Wait for button release
      }
    }
  }
}


