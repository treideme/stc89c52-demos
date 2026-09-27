/* Lean ENC28J60 driver -- see enc.h for what differs from 09_ethernet. */
#include <mcs51/8052.h>
#include "enc.h"
#include "netcfg.h"

/* Pins exactly as 09_ethernet/enc28j60_cfg.h (the bench wiring). */
/* SPI pins. Overridable from the build so one source serves both wirings.
 * On the HC6800-ES, P0 does NOT work: it is open-drain (highs only through
 * the board's 10k packs) into a bus shared with the always-enabled 74HC245,
 * the LED-matrix rows and the LCD data lines. Measured 2026-09-26: on P0 every
 * register read back doubled (EREVID 0x06 -> 0x0C); on P1/P3, which drive
 * their highs actively, all 16 write/read patterns came back exact across
 * three cold boots. P1.4/P1.6/P1.7 carry only open keypad contacts and P3.3
 * an open key plus an unpopulated header, so they are the free pins here.
 */
#ifndef CS
#define CS   P3_3
#endif
#ifndef SCK
#define SCK  P1_7
#endif
#ifndef MISO
#define MISO P1_4
#endif
#ifndef MOSI
#define MOSI P1_6
#endif

/* Opcodes */
#define RCR  0x00
#define WCR  0x40
#define BFS  0x80
#define BFC  0xA0
#define RBM  0x3A
#define WBM  0x7A
#define SRC  0xFF

/* Registers: address | bank<<5 | 0x80 when the read needs a dummy byte
 * (MAC/MII), the AVRlib encoding this driver inherited.
 */
#define ERDPTL   0x00
#define ERDPTH   0x01
#define EWRPTL   0x02
#define EWRPTH   0x03
#define ETXSTL   0x04
#define ETXSTH   0x05
#define ETXNDL   0x06
#define ETXNDH   0x07
#define ERXSTL   0x08
#define ERXSTH   0x09
#define ERXNDL   0x0A
#define ERXNDH   0x0B
#define ERXRDPTL 0x0C
#define ERXWRPTL 0x0E
#define ERXRDPTH 0x0D
#define EIR      0x1C
#define ECON2    0x1E
#define ECON1    0x1F
#define ESTAT    0x1D
#define ERXFCON  (0x18 | 0x20)
#define EPKTCNT  (0x19 | 0x20)
#define MACON1   (0x00 | 0x40 | 0x80)
#define MACON3   (0x02 | 0x40 | 0x80)
#define MABBIPG  (0x04 | 0x40 | 0x80)
#define MAIPGL   (0x06 | 0x40 | 0x80)
#define MAIPGH   (0x07 | 0x40 | 0x80)
#define MAMXFLL  (0x0A | 0x40 | 0x80)
#define MAMXFLH  (0x0B | 0x40 | 0x80)
#define MIREGADR (0x14 | 0x40 | 0x80)
#define MIWRL    (0x16 | 0x40 | 0x80)
#define MIWRH    (0x17 | 0x40 | 0x80)
#define MAADR1   (0x04 | 0x60 | 0x80)   /* datasheet names: MAADR1 = octet 0 */
#define MAADR2   (0x05 | 0x60 | 0x80)
#define MAADR3   (0x02 | 0x60 | 0x80)
#define MAADR4   (0x03 | 0x60 | 0x80)
#define MAADR5   (0x00 | 0x60 | 0x80)
#define MAADR6   (0x01 | 0x60 | 0x80)
#define MISTAT   (0x0A | 0x60 | 0x80)

#define EREVID   (0x12 | 0x60)   /* read-only 0x06: SPI canary */

#define PHCON2   0x10
#define PHLCON   0x14

static uint16_t next_pkt;
static uint16_t cur_len;
/* Bumped by rx_recover(). Instrumentation for the bench: if this climbs while
 * the board keeps answering, the wedge diagnosis is confirmed and the
 * recovery is doing its job. Reported via enc_recover_count().
 */
static uint8_t recover_count;
static uint8_t spi_errors;   /* disagreeing double-reads: link quality */
#if ENC_LINK_DEFENCES
static uint16_t rxen_check;
#endif
static uint8_t rxen_kicks;   /* times receive had to be switched back on */
static uint16_t last_bad_next;
static uint16_t last_bad_count;

/* ---- verbatim from the AVRlib-descended driver --------------------------- */
static uint8_t spi_byte(uint8_t d) {
  uint8_t res = 0;
  for(uint8_t i = 0; i < 8; i++) {
    MOSI = d & 0x80;
    d <<= 1;

    SCK = 1;
#ifdef ENC_SPI_SLOW
    /* Widen the clock: if reads are corrupted by edge speed or settling on
     * this wiring, more time here should reduce them. Measured with the
     * EREVID canary; enc_read_revid() exposes it for a bench build.
     */
    __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
#endif

    res <<= 1;
    res |= MISO; /* sample MISO on the rising edge (mode 0) */

    SCK = 0;
#ifdef ENC_SPI_SLOW
    __asm__("nop"); __asm__("nop"); __asm__("nop"); __asm__("nop");
#endif
  }
  return res;
}
/* -------------------------------------------------------------------------- */

#if ENC_LINK_DEFENCES
static void delay(uint16_t n);   /* defined below; used by spi_resync */
#endif

/* Re-synchronise the ENC's SPI state machine.
 * Why: on this bench the link is marginal - an EREVID canary (read-only
 * 0x06) comes back wrong 1-3 times per run under traffic, and corrupted
 * frame headers show payload bytes where a header belongs. A slave that
 * missed or gained a clock edge stays out of step for every later byte,
 * because it is mid-opcode. Raising CS ends the transaction and returns the
 * ENC to "expect an opcode", which is the one recovery available from the
 * master side. Cheap enough to use liberally: four SPI byte times.
 * Slowing the clock does NOT help (tried: 4 NOPs per edge made it worse), so
 * this is not edge speed - it is lost synchronisation.
 */
#if ENC_LINK_DEFENCES
static void spi_resync(void)
{
  CS = 1;
  SCK = 0;
  delay(4);
  CS = 0;                                /* a select with no clocks ... */
  delay(1);
  CS = 1;                                /* ... then release: state machine idle */
  delay(4);
}
#else
#define spi_resync() ((void)0)
#endif

static void op_write(uint8_t op, uint8_t addr, uint8_t data)
{
  SCK = 0;
  CS = 0;
  spi_byte(op | (addr & 0x1F));
  spi_byte(data);
  CS = 1;
}

static uint8_t op_read(uint8_t addr)
{
  uint8_t d;
  SCK = 0;
  CS = 0;
  spi_byte(RCR | (addr & 0x1F));
  if (addr & 0x80)
    spi_byte(0);                     /* MAC/MII dummy byte */
  d = spi_byte(0);
  CS = 1;
  return d;
}

static void set_bank(uint8_t addr)
{
  op_write(BFC, ECON1, 0x03);
  op_write(BFS, ECON1, (addr & 0x60) >> 5);
}

static void wr(uint8_t addr, uint8_t v)
{
  set_bank(addr);
  op_write(WCR, addr, v);
}

static uint8_t rd(uint8_t addr)
{
  set_bank(addr);
  return op_read(addr);
}

/* Read a register twice and only believe a value that repeats. A single
 * corrupted transfer is what wedges the ring, and these registers are read
 * far less often than frame bytes, so the second read is affordable. On
 * disagreement, resync and try once more; the caller still validates.
 */
#if ENC_LINK_DEFENCES
static uint8_t rd_stable(uint8_t addr)
{
  uint8_t a = rd(addr);
  uint8_t b = rd(addr);
  if (a == b)
    return a;
  spi_resync();
  a = rd(addr);
  b = rd(addr);
  spi_errors++;
  return (a == b) ? a : 0;
}
#else
#define rd_stable(addr) rd(addr)
#endif

static void wr16(uint8_t addr_lo, uint16_t v)
{
  wr(addr_lo, (uint8_t)v);
  wr(addr_lo + 1, (uint8_t)(v >> 8));
}

static void phy_write(uint8_t reg, uint16_t v)
{
  wr(MIREGADR, reg);
  wr16(MIWRL, v);
  while (rd(MISTAT) & 0x01)
    ;
}

static void delay(uint16_t n)
{
  while (n--)
    ;
}

void enc_init(const uint8_t *mac)
{
  SCK = 0;
  CS = 1;
  delay(50);
  op_write(SRC, 0, SRC);
  delay(50000);                         /* errata B4: CLKRDY unreliable, wait */

  next_pkt = ENC_RXSTART;
  wr16(ERXSTL, ENC_RXSTART);
  wr16(ERXRDPTL, ENC_RXSTOP);           /* odd, just "behind" ERXST (errata) */
  wr16(ERXNDL, ENC_RXSTOP);
  wr16(ETXSTL, ENC_TXSTART);            /* 09_ethernet never sets this;
             * enc_tx_send_at() sets it per frame
             */

  /* Accept frames to our MAC and broadcasts (ARP, DHCP replies), CRC-checked. */
  wr(ERXFCON, 0x80 | 0x20 | 0x01);      /* UCEN | CRCEN | BCEN */

  wr16(MACON1, 0x01 | 0x04 | 0x08);     /* MARXEN | RXPAUS | TXPAUS */
  op_write(BFS, MACON3, 0x20 | 0x10 | 0x02); /* PADCFG0 | TXCRCEN | FRMLNEN */
  wr16(MAIPGL, 0x0C12);
  wr(MABBIPG, 0x12);
  wr16(MAMXFLL, NET_MAX_FRAME);

  wr(MAADR1, mac[0]);
  wr(MAADR2, mac[1]);
  wr(MAADR3, mac[2]);
  wr(MAADR4, mac[3]);
  wr(MAADR5, mac[4]);
  wr(MAADR6, mac[5]);

  phy_write(PHCON2, 0x0100);            /* HDLDIS: no half-duplex loopback */
  phy_write(PHLCON, 0x0476);            /* LEDs: link / activity, as 09_ethernet */
  set_bank(ECON1);
  op_write(BFS, ECON1, 0x04);           /* RXEN */
}

uint8_t enc_recover_count(void)
{
  return recover_count;
}

uint8_t enc_spi_errors(void)
{
  return spi_errors;
}

uint8_t enc_rxen_kicks(void)
{
  return rxen_kicks;
}

void enc_last_bad_header(uint16_t *next, uint16_t *count)
{
  *next = last_bad_next;
  *count = last_bad_count;
}

/* SPI canary. EREVID is a read-only constant (0x06 on this silicon), so any
 * other value means the SPI transfer itself was corrupted - which is the
 * difference between a protocol bug and marginal wiring.
 */
uint8_t enc_read_revid(void)
{
  return rd(EREVID);
}

void enc_status(uint8_t *eir, uint8_t *estat, uint8_t *pktcnt, uint8_t *econ1)
{
  *eir = rd(EIR);
  *estat = rd(ESTAT);
  *pktcnt = rd(EPKTCNT);
  *econ1 = rd(ECON1);                 /* bit 2 = RXEN: is receive even on? */
}

/* Put the receive ring back to a known state.
 * Why this exists. If the next-packet-pointer chain is ever corrupted,
 * enc_rx_begin() reads a garbage header, next_pkt becomes garbage, and
 * enc_rx_done() writes ERXRDPT far outside the ring. The hardware then
 * computes its free space from that pointer, decides the buffer is
 * permanently full, and silently drops every frame: RXERIF and ESTAT.BUFER
 * latch, EPKTCNT stays 0, and because enc_rx_begin() returns early on
 * EPKTCNT == 0 the driver never looks again. The board keeps transmitting
 * nothing and never recovers without an MCU reset.
 * Reproduced by corrupting one packet's pointer: ERXRDPT ended at 0x7FFE,
 * EIR 0x09, ESTAT 0x41, PKTCNT 0 and 0 of 5 ARPs answered - the same
 * registers the bench board showed after a soak.
 */
static void rx_recover(void)
{
  recover_count++;
  spi_resync();          /* the ring is suspect because a transfer was */
  op_write(BFC, ECON1, 0x04);           /* receive off while we fix pointers */
  next_pkt = ENC_RXSTART;
  wr16(ERXSTL, ENC_RXSTART);
  wr16(ERXNDL, ENC_RXSTOP);
  wr16(ERXWRPTL, ENC_RXSTART);
  wr16(ERXRDPTL, ENC_RXSTOP);           /* odd, just "behind" ERXST (errata) */
  while (rd(EPKTCNT))                   /* drain the pending-packet counter */
    op_write(BFS, ECON2, 0x40);         /* PKTDEC */
  op_write(BFC, EIR, 0x01);             /* clear RXERIF ... */
  op_write(BFC, ESTAT, 0x40);           /* ... and ESTAT.BUFER, both sticky */
  op_write(BFS, ECON1, 0x04);           /* receive on */
}

uint16_t enc_rx_begin(void)
{
  uint8_t h[6];
#if ENC_LINK_DEFENCES
  /* The comparison copy lives in XRAM: rung 8 has only a few bytes of the
   * 128 B internal RAM to spare, and six more here stopped it linking.
   */
  static __xdata uint8_t h2[6];
  uint8_t attempt, retries = 0;
#endif
  uint8_t i;
  uint16_t count;

#if ENC_LINK_DEFENCES
  /* RXEN watchdog. On a marginal link even control writes get corrupted,
   * and a lost ECON1 write leaves receive switched OFF: the board then looks
   * perfectly healthy (no errors, CLKRDY set) and is simply deaf, forever.
   * Observed on the bench as ECON1 00/80 while traffic was flowing. Re-assert
   * it periodically rather than trusting the write that set it. Every 256
   * polls so the cost is negligible next to a frame read.
   */
  if ((++rxen_check & 0xFF) == 0 && !(rd_stable(ECON1) & 0x04)) {
    op_write(BFS, ECON1, 0x04);
    rxen_kicks++;
  }
#endif

  if (!rd_stable(EPKTCNT)) {
    /* Nothing queued, but the chip flagged a receive error or a full buffer.
     * Both flags are sticky, so they may simply be history from an overflow
     * the ring has since drained - clearing them is then enough, and a full
     * reset would needlessly drop reception for the duration. Only a pointer
     * that cannot be real means the chain is broken and the ring is wedged.
     */
    if ((rd(EIR) & 0x01) || (rd(ESTAT) & 0x40)) {
      if ((next_pkt & 1) || next_pkt > ENC_RXSTOP) {
        rx_recover();
      } else {
        op_write(BFC, EIR, 0x01);
        op_write(BFC, ESTAT, 0x40);
      }
    }
    return 0;
  }
#if ENC_LINK_DEFENCES
  /* Read the header TWICE and require agreement. The header is the one read
   * whose corruption is unrecoverable - it carries the pointer to the next
   * packet - and re-reading is free because ERDPT can simply be rewound.
   * A mismatch means the transfer was corrupted, not that the chain is
   * broken, so resync and let the validation below decide.
   */
  for (attempt = 0; attempt < 2; attempt++) {
    wr16(ERDPTL, next_pkt);
    SCK = 0;
    CS = 0;
    spi_byte(RBM);
    for (i = 0; i < 6; i++)
      h[i] = spi_byte(0);
    CS = 1;
    if (attempt == 0) {
      for (i = 0; i < 6; i++)
        h2[i] = h[i];
      continue;
    }
    for (i = 0; i < 6; i++)
      if (h[i] != h2[i])
        break;
    if (i == 6)
      break;                             /* both reads agree */
    spi_errors++;
    spi_resync();
    attempt = 0;                         /* one retry, then trust validation */
    if (++retries > 2)
      break;
  }
#else
  wr16(ERDPTL, next_pkt);
  SCK = 0;
  CS = 0;
  spi_byte(RBM);
  for (i = 0; i < 6; i++)
    h[i] = spi_byte(0);
  CS = 1;
#endif
  next_pkt = h[0] | ((uint16_t)h[1] << 8);
  count = h[2] | ((uint16_t)h[3] << 8);
  /* Validate before trusting it: the ENC aligns packets to even addresses
   * inside the ring, and a frame cannot exceed the buffer we sized for. A
   * header failing this means the chain is broken, and following it would
   * write a garbage ERXRDPT and wedge the ring for good.
   */
  if ((next_pkt & 1) || next_pkt > ENC_RXSTOP ||
      count < 4 || count > NET_MAX_FRAME + 4) {
    last_bad_next = next_pkt;
    last_bad_count = count;
    rx_recover();
    return 0;
  }
  cur_len = count - 4;                              /* drop the FCS */
  if (!(h[4] & 0x80))                               /* not "received OK" */
    cur_len = 0xFFFF;
  return cur_len;
}

void enc_rx_read(uint8_t __xdata *dst, uint8_t n)
{
  SCK = 0;
  CS = 0;
  spi_byte(RBM);
  while (n--)
    *dst++ = spi_byte(0);
  CS = 1;
}

void enc_rx_done(void)
{
  /* Errata: ERXRDPT must be odd; next_pkt is always even. */
  wr16(ERXRDPTL, next_pkt == ENC_RXSTART ? ENC_RXSTOP : next_pkt - 1);
  op_write(BFS, ECON2, 0x40);           /* PKTDEC */
}

static void tx_idle(void)
{
  uint16_t guard = 40000;               /* ~seconds of SPI reads, then give up */

  /* Bounded, not a spin. On a half-duplex link the ENC28J60 can leave
   * TXRTS set forever after a transmit error (late collision), and an
   * unbounded wait here means the board receives normally but never answers
   * again - traced on hardware 2026-09-26. Falling through to the TXRST
   * pulse below is exactly the documented recovery.
   */
  while ((rd(ECON1) & 0x08) && --guard)  /* previous frame still going */
    ;
  op_write(BFS, ECON1, 0x80);           /* errata: reset TX logic ... */
  op_write(BFC, ECON1, 0x80);
  op_write(BFC, EIR, 0x0A);             /* ... and clear TXIF/TXERIF */
  /* The TXRST pulse also clears ECON1.RXEN, and RXEN was only ever set
   * once, in enc_init(). Without this the board answers for a while and then
   * goes permanently deaf: traced on hardware 2026-09-26 as ECON1 stepping
   * 04 -> 84 -> 00 and staying at 00, with no errors, no buffer overflow and
   * PKTCNT 0 - a healthy chip that simply stops receiving.
   */
  op_write(BFS, ECON1, 0x04);           /* re-enable receive */
}

void enc_tx_begin_at(uint16_t base)
{
  tx_idle();
  wr16(EWRPTL, base);
  SCK = 0;
  CS = 0;
  spi_byte(WBM);
  spi_byte(0x00);                       /* per-packet control: MACON3 defaults */
  CS = 1;
}

void enc_tx_write(const uint8_t __xdata *src, uint8_t n)
{
  SCK = 0;
  CS = 0;
  spi_byte(WBM);
  while (n--)
    spi_byte(*src++);
  CS = 1;
}

void enc_tx_fill(uint8_t value, uint16_t n)
{
  SCK = 0;
  CS = 0;
  spi_byte(WBM);
  while (n--)
    spi_byte(value);
  CS = 1;
}

static void tx_go(uint16_t base, uint16_t len)
{
  wr16(ETXSTL, base);
  wr16(ETXNDL, base + len);             /* control byte at base, frame after */
  set_bank(ECON1);
  op_write(BFS, ECON1, 0x08);           /* TXRTS */
}

void enc_tx_send_at(uint16_t base, uint16_t len)
{
  tx_go(base, len);
}

#if NET_RUNG >= 8
void enc_tx_patch(uint16_t base, uint16_t off, const uint8_t __xdata *src, uint8_t n)
{
  uint16_t end = rd(EWRPTL) | ((uint16_t)rd(EWRPTH) << 8);
  wr16(EWRPTL, base + 1 + off);         /* +1: the per-packet control byte */
  enc_tx_write(src, n);
  wr16(EWRPTL, end);
}

void enc_tx_resend(uint16_t base, uint16_t len)
{
  tx_idle();
  tx_go(base, len);
}
#endif
