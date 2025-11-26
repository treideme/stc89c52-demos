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
#include <mcs51/8052.h>
#include <mcs51/compiler.h> // NOP
#include <stdint.h>
#include "xprintf.h"

void delay(uint16_t us) {
  while(us) {
    us--;
  }
}

void uart_init(void) {
  PCON &= 0x7F;	/* SMOD - Baud rate not doubled */
  SCON = 0x50;	/* 8 bits and variable baudrate */
  T2CON &= ~0x03;	/* Set Timer2 as timer */
  T2CON |= 0x30;	/* Set Timer 2 as UART TCLK and RCLK */
  RCAP2L = 0xd9;	/* Initial timer low value */
  RCAP2H = 0xff;	/* Initial timer high value */
//  RCAP2H = 0;	/* Disable Timer 2 interrupt */
  TR2 = 1;	/* Timer2 start run */
}

void main(void) {
  uart_init();
  for(;;) {

    PRINTF("%s\n", "Hello World!");
    delay(0xFFFF);
  }
}

int putchar (int ch) {
  if(ch == '\n') putchar('\r');
  for(SBUF = ch; !TI; );
  TI = 0;
  return ch;
}