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
 * @file softuart.h A transmit-only debug console on a spare pin.
 * @author Thomas Reidemeister
 *
 * Driving the ENC28J60 from UART Mode 0 costs P3.0 and P3.1, and on the
 * HC6800-ES those two pins are the CH340 console AND the ISP flash path.
 * So the console has to move, and there is no second UART on an STC89C52.
 *
 * Clip a USB-serial adapter to P1.7 and ground. P1.7 is keypad row 1 and
 * nothing else on this board, so it is free as long as no key is held.
 *
 * P1.7 idles at 5 V. An FT232RL's RXD is 5 V tolerant; a 3.3 V-only
 * adapter needs a divider. Adapter RXD to P1.7, adapter GND to board GND,
 * and leave the adapter's TXD unconnected -- this is output only.
 *
 * Timer 1 rather than a counted delay loop, because the reload value is
 * exact arithmetic on the crystal instead of a guess about how many cycles
 * SDCC emitted. At 11.0592 MHz the numbers come out whole at every
 * standard rate, which is the entire reason that crystal exists:
 *
 *   6T  (1.8432 MHz cycles): 9600 -> 192 cycles/bit, 38400 -> 48
 *   12T (0.9216 MHz cycles): 9600 ->  96 cycles/bit, 38400 -> 12
 *
 * Transmission blocks, and that is deliberate. It cannot be preempted into
 * corruption, and it cannot corrupt the SPI either -- Mode 0 shifts in
 * hardware, so the two share the CPU without either masking interrupts.
 * That coexistence is not a nicety; a bit-banged SPI and a bit-banged
 * console on one 8051 cannot both be correct.
 */
#ifndef SOFTUART_H
#define SOFTUART_H

#include <mcs51/8051.h>
#include <stdint.h>

#ifndef SOFTUART_TX
#define SOFTUART_TX P1_7
#endif

/* Machine cycles per bit. Override from the build if the crystal or the
 * 6T/12T fuse differs -- getting this wrong is the one failure that looks
 * like noise on the terminal rather than like a bug. */
#ifndef SOFTUART_TICKS_PER_BIT
#define SOFTUART_TICKS_PER_BIT 192
#endif

void softuart_init(void);
void softuart_putc(char c);
void softuart_puts(const char *s);
void softuart_hex8(uint8_t v);
void softuart_hex16(uint16_t v);

#endif /* SOFTUART_H */
