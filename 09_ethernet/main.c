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
 * @file main.c ENC28J60 bring-up and network stack on an STC89C5x.
 * @author Thomas Reidemeister
 */
#include "net.h"
#include "netcfg.h"

#if NET_CONSOLE
#include <mcs51/8052.h>
#include <stdint.h>
#define _XPRINTF_               /* enable xprintf */
#include "xprintf.h"
#include "enc.h"

/*
 * The console is optional because it is not free: xprintf and the strings
 * below cost about 1.5 KB, which is a fifth of the STC89C52RC's flash and
 * would push the upper rungs out of the part. Build with -DNET_CONSOLE=1 for
 * bring-up on a new board, and without it once the wiring is known good.
 */

/* UART output, 9600 baud, 12 MHz crystal. See https://reidemeister.com/tools */
static void uart_init(void)
{
  PCON &= 0x7F;                 /* SMOD: baud rate not doubled */
  SCON = 0x50;                  /* 8 bits, variable baud rate */
  T2CON &= ~0x03;               /* Timer 2 as a timer */
  T2CON |= 0x30;                /* Timer 2 as UART TCLK and RCLK */
  RCAP2L = 0xd9;
  RCAP2H = 0xff;
  ET2 = 0;                      /* no Timer 2 interrupt */
  TR2 = 1;
}

/* xprintf writes characters through this. */
int putchar(int ch)
{
  for (SBUF = ch; !TI; )
    ;
  TI = 0;
  return ch;
}

/*
 * Say what we are talking to before trusting anything else it says. EREVID is
 * read-only and must read 0x06 on current silicon, which makes it both an
 * identity check and a canary for the SPI link: a wrong value here means the
 * bus is lying, not that the part is exotic.
 *
 * DS80349C Table 1: 0x02 = B1, 0x04 = B4, 0x05 = B5, 0x06 = B7. The erratum
 * about MAC registers below an 8 MHz SPI clock is marked against B1 and B4
 * only.
 */
static void report_silicon(void)
{
  uint8_t rev = enc_read_revid();

  PUTS("ENC28J60 bring-up\r\n");
  PRINTF("EREVID: 0x%02X  ", rev);
  if (rev == 0x02 || rev == 0x04)
    PUTS("B1/B4 -- errata #1 APPLIES\r\n");
  else if (rev == 0x05 || rev == 0x06)
    PUTS("B5/B7 -- errata #1 does NOT apply\r\n");
  else
    PUTS("unexpected -- check the SPI wiring\r\n");
}

/*
 * Link health, printed periodically. A climbing recovery count while the board
 * still answers is the wedge diagnosis confirmed and the recovery working; SPI
 * errors and RXEN kicks are the marginal link being caught in the act. All
 * three staying zero on jumper wiring is worth a second look at the counters
 * rather than celebration.
 */
static void report_link(void)
{
  uint8_t eir, estat, pktcnt, econ1;

  enc_status(&eir, &estat, &pktcnt, &econ1);
  PRINTF("EIR %02X ESTAT %02X PKT %02X ECON1 %02X", eir, estat, pktcnt, econ1);
  PRINTF(" REC %02X SPI %02X RXEN %02X\r\n",
         enc_recover_count(), enc_spi_errors(), enc_rxen_kicks());
}
#endif  /* NET_CONSOLE */

void main(void)
{
#if NET_CONSOLE
  uint16_t ticks = 0;

  uart_init();
#endif

  net_init();

#if NET_CONSOLE
  report_silicon();
#endif

  for (;;) {
    net_poll();
#if NET_CONSOLE
    /* Roughly once a second. The mask is measured, not guessed: an idle poll
       still costs several SPI register reads, about 7 ms on the bench, so 128
       polls is a second and 1024 would be 7.5. A bare `== 0` wraps a uint16
       only every 65,536 polls - the line then never appears at all, measured
       as 100 s under load with only the banner printed.

       The cadence is not free: a line is ~70 characters, and putchar() spins
       on TI, so at 9600 baud it blocks the poll loop for ~70 ms. Raise the
       mask if a run cares more about not missing frames than about watching
       the counters. */
    if ((++ticks & 0x007F) == 0)
      report_link();
#endif
  }
}
