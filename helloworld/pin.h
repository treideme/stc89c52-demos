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
 * | PIN | PORT  |PU/PD | Function | Description
 * |-----|-------|------|----------|------------
 * |  1  | P1.0  | PU   | GPIO     | 74HCT573-EN -> 7 Segment enable
 * |  2  | P1.1  | PU   | GPIO     |
 * |  3  | P1.2  | PU   | GPIO     |
 * |  4  | P1.3  | PU   | GPIO     |
 * |  5  | P1.4  | PU   | GPIO     |
 * |  6  | P1.5  | PU   | GPIO     |
 * |  7  | P1.6  | PU   | GPIO     |
 * |  8  | P1.7  | PU   | GPIO     |
 * |  9  |  RST  |
 * | 10  | P3.0  | PU   | UART     | RxD to CH341
 * | 11  | P3.1  | PU   | UART     | TxD to CH341
 * | 12  | P3.2  | PU   | GPIO     |
 * | 13  | P3.3  | PU   | GPIO     |
 * | 14  | P3.4  | PU   | GPIO     |
 * | 15  | P3.5  | PU   | GPIO     |
 * | 16  | P3.6  | PU   | GPIO     |
 * | 17  | P3.7  | PU   | GPIO     |
 * | 18  | XTAL1 |
 * | 19  | XTAL2 |
 * | 20  | GND   |
 * | 21  | P2.0  | PU   | GPIO     |
 * | 22  | P2.1  | PU   | GPIO     | 74LS138-A (7-Segment select)
 * | 23  | P2.2  | PU   | GPIO     | 74LS138-B (7-Segment select)
 * | 24  | P2.3  | PU   | GPIO     | 74LS138-C (7-Segment select)
 * | 25  | P2.4  | PU   | GPIO     |
 * | 26  | P2.5  | PU   | GPIO     |
 * | 27  | P2.6  | PU   | GPIO     |
 * | 28  | P2.7  | PU   | GPIO     |
 * | 29  | PSEN  |
 * | 30  | ALE   |
 * | 31  | EA    |
 * | 32  | P0.7  | PU   | GPIO     | 74HCT573-7 -> 7-Segment element d
 * | 33  | P0.6  | PU   | GPIO     | 74HCT573-6 -> 7-Segment element G
 * | 34  | P0.5  | PU   | GPIO     | 74HCT573-5 -> 7-Segment element F
 * | 35  | P0.4  | PU   | GPIO     | 74HCT573-4 -> 7-Segment element E
 * | 36  | P0.3  | PU   | GPIO     | 74HCT573-3 -> 7-Segment element D
 * | 37  | P0.2  | PU   | GPIO     | 74HCT573-2 -> 7-Segment element C
 * | 38  | P0.1  | PU   | GPIO     | 74HCT573-1 -> 7-Segment element B
 * | 39  | P0.0  | PU   | GPIO     | 74HCT573-0 -> 7-Segment element A
 * | 40  | VCC
 */

#endif /* _pin_h_ */
