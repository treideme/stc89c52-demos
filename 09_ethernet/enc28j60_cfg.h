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

/* SPI pins, overridable from the build so the old P0 wiring can still be
 * selected with -D for comparison.
 *
 * P0 does not work on the HC6800-ES. It is open-drain into a bus shared with
 * the always-enabled 74HC245, the LED-matrix rows and the LCD data lines, and
 * every register read came back doubled (EREVID 0x06 -> 0x0C). P1.4/P1.6/P1.7
 * carry only open keypad contacts and P3.3 an open key plus an unpopulated
 * header; those read exact across three cold boots.
 */
#ifndef ENC28J60_CONTROL_CS
#define ENC28J60_CONTROL_CS P3_3
#endif
#ifndef ENC28J60_SPI_SCK
#define ENC28J60_SPI_SCK P1_7
#endif
#ifndef ENC28J60_SPI_MISO
#define ENC28J60_SPI_MISO P1_4
#endif
#ifndef ENC28J60_SPI_MOSI
#define ENC28J60_SPI_MOSI P1_6
#endif

/* 0x02 = locally administered unicast. The original 0x01 has the
 * multicast bit set, which is invalid as a source address.
 */
#define ENC28J60_MAC0 0x02
#define ENC28J60_MAC1 0x02
#define ENC28J60_MAC2 0x03
#define ENC28J60_MAC3 0x04
#define ENC28J60_MAC4 0x05
#define ENC28J60_MAC5 0x06

#define MIN(a,b) ((a)<(b)?(a):(b))


#endif /* ENC28J60_CFG_H */