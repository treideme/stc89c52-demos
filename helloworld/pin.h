/**
 * @file pin.h Pin configuration.
 * CH559 Mini EVT.
 */
#ifndef _pin_h_
#define _pin_h_

#include <mcs51/8051.h>

/******************************************************************************
 * PDIP-40
 *
 *               +----\/----+
 *               |          |
 *      T2/P1.0 -+ 1      40+- VCC
 *    T2EX/P1.1 -+ 2      39+- P0.0
 *         P1.2 -+ 3      38+- P0.1
 *         P1.3 -+ 4      37+- P0.2
 *         P1.4 -+ 5      36+- P0.3
 *         P1.5 -+ 6      35+- P0.4
 *         P1.6 -+ 7      34+- P0.5
 *         P1.7 -+ 8      33+- P0.6
 *          RST -+ 9      32+- P0.7
 *     RxD/P3.0 -+10      31+- EA
 *     TxD/P3.1 -+11      30+- ALE
 *    INT0/P3.2 -+12      29+- PSEN
 *    INT1/P3.3 -+13      28+- P2.7
 *      T0/P3.4 -+14      27+- P2.6
 *      T1/P3.5 -+15      26+- P2.5
 *      WR/P3.6 -+16      25+- P2.4
 *      RD/P3.7 -+17      24+- P2.3
 *        XTAL2 -+18      23+- P2.2
 *        XTAL1 -+19      22+- P2.1
 *          GND -+20      21+- P2.0
 *               |          |
 *               +----------+
 *
 * | PIN | PORT  |  DIR | PU/PD | Function | Description
 * |-----|-------|------|-------|----------|------------
 * |  1  | P1.0  |  OUT |  N/A  | GPIO     |
 * |  2  | P1.1  |  OUT |  N/A  | GPIO     |
 * |  3  | P1.2  |  IN  |  PU   | GPIO     |
 * |  4  | P1.3  |  IN  |  PU   | GPIO     |
 * |  5  | P1.4  |  IN  |  PU   | GPIO     |
 * |  6  | P1.5  |  IN  |  PU   | GPIO     |
 * |  7  | P1.6  |  IN  |  PU   | GPIO     |
 * |  8  | P1.7  |  IN  |  PU   | GPIO     |
 * |  9  |  RST  |
 * | 10  | P3.0  |  IN  |  PU   | GPIO     | RxD to CH341
 * | 11  | P3.1  |  IN  |  PU   | GPIO     | TxD to CH341
 * | 12  | P3.2  |  IN  |  PU   | GPIO     |
 * | 13  | P3.3  |  IN  |  PU   | GPIO     |
 * | 14  | P3.4  |  IN  |  PU   | GPIO     |
 * | 15  | P3.5  |  IN  |  PU   | GPIO     |
 * | 16  | P3.6  |  IN  |  PU   | GPIO     |
 * | 17  | P3.7  |  IN  |  PU   | GPIO     |
 * | 18  | XTAL1 |
 * | 19  | XTAL2 |
 * | 20  | GND   |
 * | 21  | P2.0  |  IN  |  PU   | GPIO     |
 * | 22  | P2.1  |  IN  |  PU   | GPIO     | 74LS138-A (7-Segment select)
 * | 23  | P2.2  |  IN  |  PU   | GPIO     | 74LS138-B (7-Segment select)
 * | 24  | P2.3  |  IN  |  PU   | GPIO     | 74LS138-C (7-Segment select)
 * | 25  | P2.4  |  IN  |  PU   | GPIO     |
 * | 26  | P2.5  |  IN  |  PU   | GPIO     |
 * | 27  | P2.6  |  IN  |  PU   | GPIO     |
 * | 28  | P2.7  |  IN  |  PU   | GPIO     |
 * | 29  | PSEN  |
 * | 30  | ALE   |
 * | 31  | EA    |
 * | 32  | P0.7  |  IN  |  PU   | GPIO     |
 * | 33  | P0.6  |  IN  |  PU   | GPIO     |
 * | 34  | P0.5  |  IN  |  PU   | GPIO     |
 * | 35  | P0.4  |  IN  |  PU   | GPIO     |
 * | 36  | P0.3  |  IN  |  PU   | GPIO     |
 * | 37  | P0.2  |  IN  |  PU   | GPIO     |
 * | 38  | P0.1  |  IN  |  PU   | GPIO     |
 * | 39  | P0.0  |  IN  |  PU   | GPIO     |
 * | 40  | VCC
 */

/*******************************************************************************
**  Port configuration
*/

#define INIT_PORT_CFG \
  B8(00101101) /**< PORT_CFG  Port Configuration Register. @verbatim
     00001111 = reset
     ||||||||
     |||||||+-- (RW)  bP0_OD : Port 0 Open Drain Enabled (not Push-Pull)
     ||||||+--- (RW)  bP1_OD : Port 1 Open Drain Enabled (Push-Pull not OD)
     |||||+---- (RW)  bP2_OD : Port 2 Open Drain Enabled (not Push-Pull)
     ||||+----- (RW)  bP3_OD : Port 3 Open Drain Enabled (not Push-Pull)
     |||+------ (RW)  bP0_DRV: Port 0 Drive capability (5mA) not 20mA
     ||+------- (RW)  bP1_DRV: Port 0 Drive capability (20mA) not 5mA
     |+-------- (RW)  bP2_DRV: Port 0 Drive capability (5mA) not 20mA
     +--------- (RW)  bP3_DRV: Port 0 Drive capability (5mA) not 20mA
  @endverbatim
*/

#define INIT_PIN_FUNC \
  B8(00010000) /**< PIN_FUNC  Pin Function Select Register. @verbatim
     00000000 = reset
     ||||||||
     |||||||+-- (RW)  bXBUS_AL_OE  : xBUS Low Address Enable (P4.0-P4.5,P3.5,P2.7)
     ||||||+--- (RW)  bXBUS_AH_OE  : xBus High Address Enable (P2)
     |||||+---- (RW)  bXBUS_CS_OE  : xBus Chip Select Enable (P3.4)
     ||||+----- (RW)  bXBUS_EN     : xBUS Enable, P0 data, P3.6/P3.7 as read/write strobe
     |||+------ (RW)  bUART0_PIN_X : UART0 mapping (0, RXD0/TXD0=P3.0/P3.1; 1, P0.2/P0.3)
     ||+------- (RW)  bT2EX_PIN_X  : T2EX/CAP2 mapping (0, P1.1; 1, P2.5)
     |+-------- (RW)  bTMR3_PIN_X  : PWM3/CAP3 mapping (0, P1.2; 1, P4.2)
     +--------- (RW)  bPWM1_PIN_X  : PWM1/PWM2 mapping (0, P2.4/P2.5; 1, P4.3/P4.5)
  @endverbatim
*/

#define INIT_P0_DIR \
  B8(00001000) /**< P0_DIR  P0 port direction control register. @verbatim
     00000000 = reset
     ||||||||
     |||||||+-- (RW)  P0_0 : Port 0 Direction register (Input)
     ||||||+--- (RW)  P0_1 : Port 1 Direction register (Input)
     |||||+---- (RW)  P0_2 : Port 2 Direction register (Input)
     ||||+----- (RW)  P0_3 : Port 3 Direction register (Output)
     |||+------ (RW)  P0_4 : Port 4 Direction register (Input)
     ||+------- (RW)  P0_5 : Port 5 Direction register (Input)
     |+-------- (RW)  P0_6 : Port 6 Direction register (Input)
     +--------- (RW)  P0_7 : Port 7 Direction register (Input)
  @endverbatim
*/

#define INIT_P0_PU \
  B8(11111111) /**< P0_PU  P0 port pull-up register. @verbatim
     11111111 = reset
     ||||||||
     |||||||+-- (RW)  P0_0 : Port 0 Pull-up (Enabled)
     ||||||+--- (RW)  P0_1 : Port 1 Pull-up (Enabled)
     |||||+---- (RW)  P0_2 : Port 2 Pull-up (Enabled)
     ||||+----- (RW)  P0_3 : Port 3 Pull-up (Enabled)
     |||+------ (RW)  P0_4 : Port 2 Pull-up (Enabled)
     ||+------- (RW)  P0_5 : Port 2 Pull-up (Enabled)
     |+-------- (RW)  P0_6 : Port 2 Pull-up (Enabled)
     +--------- (RW)  P0_7 : Port 2 Pull-up (Enabled)
  @endverbatim
*/

#define INIT_P1_DIR \
  B8(11110000) /**< P1_DIR  P1 port direction control register. @verbatim
     00000000 = reset
     ||||||||
     |||||||+-- (RW)  P1_0 : Port 0 Direction register (Input)
     ||||||+--- (RW)  P1_1 : Port 1 Direction register (Input)
     |||||+---- (RW)  P1_2 : Port 2 Direction register (Input)
     ||||+----- (RW)  P1_3 : Port 3 Direction register (Input)
     |||+------ (RW)  P1_4 : Port 4 Direction register (Output)
     ||+------- (RW)  P1_5 : Port 5 Direction register (Output)
     |+-------- (RW)  P1_6 : Port 6 Direction register (Output)
     +--------- (RW)  P1_7 : Port 7 Direction register (Output)
  @endverbatim
*/

#define INIT_P1_PU \
  B8(00001111) /**< P1_PU  P1 port pull-up register. @verbatim
     11111111 = reset
     ||||||||
     |||||||+-- (RW)  P1_0 : Port 0 Pull-up (Enabled)
     ||||||+--- (RW)  P1_1 : Port 1 Pull-up (Enabled)
     |||||+---- (RW)  P1_2 : Port 2 Pull-up (Enabled)
     ||||+----- (RW)  P1_3 : Port 3 Pull-up (Enabled)
     |||+------ (RW)  P1_4 : Port 2 Pull-up (Disabled)
     ||+------- (RW)  P1_5 : Port 2 Pull-up (Disabled)
     |+-------- (RW)  P1_6 : Port 2 Pull-up (Disabled)
     +--------- (RW)  P1_7 : Port 2 Pull-up (Disabled)
  @endverbatim
*/

#define INIT_P2_DIR \
  B8(00000000) /**< P2_DIR  P2 port direction control register. @verbatim
     00000000 = reset
     ||||||||
     |||||||+-- (RW)  P2_0 : Port 0 Direction register (Input)
     ||||||+--- (RW)  P2_1 : Port 1 Direction register (Input)
     |||||+---- (RW)  P2_2 : Port 2 Direction register (Input)
     ||||+----- (RW)  P2_3 : Port 3 Direction register (Input)
     |||+------ (RW)  P2_4 : Port 4 Direction register (Input)
     ||+------- (RW)  P2_5 : Port 5 Direction register (Input)
     |+-------- (RW)  P2_6 : Port 6 Direction register (Input)
     +--------- (RW)  P2_7 : Port 7 Direction register (Input)
  @endverbatim
*/

#define INIT_P2_PU \
  B8(11111111) /**< P2_PU  P2 port pull-up register. @verbatim
     11111111 = reset
     ||||||||
     |||||||+-- (RW)  P2_0 : Port 0 Pull-up (Enabled)
     ||||||+--- (RW)  P2_1 : Port 1 Pull-up (Enabled)
     |||||+---- (RW)  P2_2 : Port 2 Pull-up (Enabled)
     ||||+----- (RW)  P2_3 : Port 3 Pull-up (Enabled)
     |||+------ (RW)  P2_4 : Port 2 Pull-up (Enabled)
     ||+------- (RW)  P2_5 : Port 2 Pull-up (Enabled)
     |+-------- (RW)  P2_6 : Port 2 Pull-up (Enabled)
     +--------- (RW)  P2_7 : Port 2 Pull-up (Enabled)
  @endverbatim
*/

#define INIT_P3_DIR \
  B8(00000000) /**< P3_DIR  P3 port direction control register. @verbatim
     00000000 = reset
     ||||||||
     |||||||+-- (RW)  P3_0 : Port 0 Direction register (Input)
     ||||||+--- (RW)  P3_1 : Port 1 Direction register (Input)
     |||||+---- (RW)  P3_2 : Port 2 Direction register (Input)
     ||||+----- (RW)  P3_3 : Port 3 Direction register (Input)
     |||+------ (RW)  P3_4 : Port 4 Direction register (Input)
     ||+------- (RW)  P3_5 : Port 5 Direction register (Input)
     |+-------- (RW)  P3_6 : Port 6 Direction register (Input)
     +--------- (RW)  P3_7 : Port 7 Direction register (Input)
  @endverbatim
*/

#define INIT_P3_PU \
  B8(11111111) /**< P3_PU  P3 port pull-up register. @verbatim
     11111111 = reset
     ||||||||
     |||||||+-- (RW)  P3_0 : Port 0 Pull-up (Enabled)
     ||||||+--- (RW)  P3_1 : Port 1 Pull-up (Enabled)
     |||||+---- (RW)  P3_2 : Port 2 Pull-up (Enabled)
     ||||+----- (RW)  P3_3 : Port 3 Pull-up (Enabled)
     |||+------ (RW)  P3_4 : Port 2 Pull-up (Enabled)
     ||+------- (RW)  P3_5 : Port 2 Pull-up (Enabled)
     |+-------- (RW)  P3_6 : Port 2 Pull-up (Enabled)
     +--------- (RW)  P3_7 : Port 2 Pull-up (Enabled)
  @endverbatim
*/

#define INIT_P4_DIR \
  B8(00000000) /**< P4_DIR  P4 port direction control register. @verbatim
     00000000 = reset
     ||||||||
     |||||||+-- (RW)  P4_0 : Port 0 Direction register (Input)
     ||||||+--- (RW)  P4_1 : Port 1 Direction register (Input)
     |||||+---- (RW)  P4_2 : Port 2 Direction register (Input)
     ||||+----- (RW)  P4_3 : Port 3 Direction register (Input)
     |||+------ (RW)  P4_4 : Port 4 Direction register (Input)
     ||+------- (RW)  P4_5 : Port 5 Direction register (Input)
     |+-------- (RW)  P4_6 : Port 6 Direction register (Input)
     +--------- (RW)  P4_7 : Port 7 Direction register (Input)
  @endverbatim
*/

#define INIT_P4_PU \
  B8(11111111) /**< P4_PU  P4 port pull-up register. @verbatim
     11111111 = reset
     ||||||||
     |||||||+-- (RW)  P4_0 : Port 0 Pull-up (Enabled)
     ||||||+--- (RW)  P4_1 : Port 1 Pull-up (Enabled)
     |||||+---- (RW)  P4_2 : Port 2 Pull-up (Enabled)
     ||||+----- (RW)  P4_3 : Port 3 Pull-up (Enabled)
     |||+------ (RW)  P4_4 : Port 2 Pull-up (Enabled)
     ||+------- (RW)  P4_5 : Port 2 Pull-up (Enabled)
     |+-------- (RW)  P4_6 : Port 2 Pull-up (Enabled)
     +--------- (RW)  P4_7 : Port 2 Pull-up (Enabled)
  @endverbatim
*/

#endif /* _pin_h_ */
