/* Lean ENC28J60 driver for the network stack (net.c).
 *
 * Descended from the AVRlib ENC28J60 driver by way of this project's earlier
 * enc28j60.c: spi_byte() and the init sequence came across verbatim, so the
 * SPI bus behaves exactly as it did on the bench. Differences, all deliberate:
 *   - no register dump, so no xprintf unless NET_CONSOLE asks for one;
 *   - ETXST is programmed (the older driver left it at 0, so the chip would
 *     transmit from the RX buffer instead of the frame just written);
 *   - ERXRDPT is written odd (silicon errata: an even value can corrupt the
 *     RX ring), and the TX logic is reset before each send (errata);
 *   - streaming API: frames are read/written in pieces straight from/to the
 *     ENC's 8 KB buffer, because a DHCP message (~300 B) is larger than all
 *     256 B of XRAM on the STC89C52RC.
 */
#ifndef ENC_H
#define ENC_H

#include <stdint.h>
#include "netcfg.h"

/* The ENC's 8 KB is the real frame store; MCU RAM never holds a frame.
 * Every ERXND/ERXRDPT seed below is odd (silicon errata). A TX region needs
 * 1 control byte + the frame + a 7-byte status vector the chip writes after
 * ETXND: 1 + 1514 + 7 = 1522 B for MTU 1500.
 */
#define ENC_RXSTART 0x0000
#if NET_RUNG >= 8
#define ENC_RXSTOP  0x13FF          /* 5 KB RX ring: three full 1518 B frames */
#define ENC_TCPSTART 0x1400         /* TCP retransmit slot, 1536 B: the last
          * unacknowledged segment stays here and
          * is re-sent by re-arming TXRTS, so a
          * retransmit costs no MCU RAM at all
          */
#define ENC_TXSTART 0x1A00          /* general TX, 1536 B */
#elif NET_MTU > 576
#define ENC_RXSTOP  0x19FF          /* 6.5 KB RX ring: four full 1518 B frames */
#define ENC_TXSTART 0x1A00
#else
#define ENC_RXSTOP  0x0BFF          /* 3 KB RX ring: five full 576 B datagrams */
#define ENC_TXSTART 0x0C00          /* TX buffer: 0x0C00 .. 0x1FFF */
#endif

void enc_init(const uint8_t *mac);

/* Receive: returns the frame length without FCS (0 = nothing pending). The
 * frame is then read sequentially with enc_rx_read(); enc_rx_done() frees it
 * whether or not every byte was read.
 */
uint16_t enc_rx_begin(void);

/* Diagnostics for bring-up on real hardware: EIR, ESTAT and EPKTCNT. The
 * RX ring is small (3 KB below rung 5) and bit-banged SPI reads ~1.8 kB/s,
 * so a burst of broadcast chatter can overrun it; EIR.RXERIF (0x01) and
 * ESTAT.BUFER (0x02) say whether that is what happened.
 */
uint8_t enc_read_revid(void);
/* Double-reads that disagreed: a direct measure of SPI link quality. */
uint8_t enc_spi_errors(void);
/* Times the RXEN watchdog had to switch receive back on. */
uint8_t enc_rxen_kicks(void);
void enc_status(uint8_t *eir, uint8_t *estat, uint8_t *pktcnt, uint8_t *econ1);

/* Bench instrumentation: how many times the receive ring had to be rebuilt,
 * and the header that last failed validation. A climbing count with the board
 * still answering confirms the chain-corruption diagnosis.
 */
uint8_t enc_recover_count(void);
void enc_last_bad_header(uint16_t *next, uint16_t *count);
void enc_rx_read(uint8_t __xdata *dst, uint8_t n);
void enc_rx_done(void);

/* Transmit: enc_tx_begin(), then any mix of enc_tx_write()/enc_tx_fill(),
 * then enc_tx_send(total frame length). The _at variants take the start of a
 * TX region (ENC_TXSTART, or ENC_TCPSTART for the retransmit slot).
 */
void enc_tx_begin_at(uint16_t base);
void enc_tx_write(const uint8_t __xdata *src, uint8_t n);
void enc_tx_fill(uint8_t value, uint16_t n);
void enc_tx_send_at(uint16_t base, uint16_t len);
#define enc_tx_begin() enc_tx_begin_at(ENC_TXSTART)
#define enc_tx_send(len) enc_tx_send_at(ENC_TXSTART, (len))

/* Overwrite n bytes at frame offset `off` of the frame being built at `base`
 * (e.g. a checksum only known after the payload streamed through), then
 * continue appending where the frame ended.
 */
void enc_tx_patch(uint16_t base, uint16_t off, const uint8_t __xdata *src, uint8_t n);

/* Send again whatever frame is still in the region at `base`. */
void enc_tx_resend(uint16_t base, uint16_t len);

#endif
