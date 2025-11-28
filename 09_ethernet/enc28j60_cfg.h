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

#define ENC28J60_CONTROL_CS P0_3
#define ENC28J60_SPI_SCK P0_2
#define ENC28J60_SPI_MISO P0_1
#define ENC28J60_SPI_MOSI P0_0

#define ENC28J60_MAC0 0x01
#define ENC28J60_MAC1 0x02
#define ENC28J60_MAC2 0x03
#define ENC28J60_MAC3 0x04
#define ENC28J60_MAC4 0x05
#define ENC28J60_MAC5 0x06

#define MIN(a,b) ((a)<(b)?(a):(b))


#endif // ENC28J60_CFG_H