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
 * @file enc28j60_cfg.h Interface definition for driver code.
 * @author Thomas Reidemeister
 */
#ifndef ENC28J60_CFG_H
#define ENC28J60_CFG_H

/* Which SPI transport the driver uses.
 *
 *   0 = bit-banged on four P0 pins. Portable, needs no extra parts, and
 *       measured at 26 machine cycles per bit (emu8051, SDCC
 *       --opt-code-size) -- 35 kHz at 11.0592 MHz in 12T, 71 kHz in 6T.
 *   1 = the UART's synchronous Mode 0 used as a hardware shift register.
 *       One machine cycle per bit: 921.6 kHz in 12T, 1.8432 MHz in 6T.
 *       26x faster, and it frees the CPU during a byte, which is what lets
 *       a software debug console survive alongside it.
 *
 * Mode 0 costs three things. It takes P3.0/P3.1, so the CH340 console is
 * gone and the debug UART moves (see softuart.h). It is LSB-first, so every
 * byte is reversed through a 256-byte table. And it needs two passive-ish
 * parts in the adapter harness -- NOT on the dev board, nothing is cut:
 *
 *   - one inverting gate between P3.1 and the ENC's SCK. Datasheet 4.1:
 *     the part "supports SPI mode 0,0 only" and "selectable clock polarity
 *     is not supported", while Mode 0's shift clock idles HIGH.
 *   - one tri-state gate on the ENC's SO, active-low enable on P1.6, so SO
 *     can be taken off the shared data pin during a write burst. Both come
 *     out of a single 74HC240.
 */
#ifndef ENC28J60_SPI_MODE0
#define ENC28J60_SPI_MODE0 0
#endif

#if ENC28J60_SPI_MODE0

/* P3.1 drives the inverter, so the level written here is the OPPOSITE of
 * what reaches the ENC. SCK idle low at the part means P3.1 high. */
#define ENC28J60_SPI_SCK    P3_1
#define ENC28J60_SPI_MOSI   P3_0
#define ENC28J60_SPI_MISO   P3_0  /* same wire; SO joins it behind the tri-state */
#define ENC28J60_CONTROL_CS P1_4  /* keypad row 4 and nothing else */
#define ENC28J60_SO_OE      P1_6  /* active low; keypad row 2 and nothing else */
#define ENC28J60_SCK_IDLE   1     /* inverted */

#else

#define ENC28J60_CONTROL_CS P0_3
#define ENC28J60_SPI_SCK P0_2
#define ENC28J60_SPI_MISO P0_1
#define ENC28J60_SPI_MOSI P0_0
#define ENC28J60_SCK_IDLE   0

#endif

#define ENC28J60_MAC0 0x01
#define ENC28J60_MAC1 0x02
#define ENC28J60_MAC2 0x03
#define ENC28J60_MAC3 0x04
#define ENC28J60_MAC4 0x05
#define ENC28J60_MAC5 0x06

#define MIN(a,b) ((a)<(b)?(a):(b))


#endif // ENC28J60_CFG_H