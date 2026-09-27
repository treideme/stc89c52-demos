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
 * @file diag_console.c ENC28J60 bring-up that can say what happened.
 * @author Thomas Reidemeister
 *
 * A bring-up with no console has to report through the port pins, which is
 * enough to say whether something worked and never enough to say what. This
 * one has a console: a bit-banged 8-N-1 transmitter on P1.7 (softuart.h),
 * reachable with a USB-serial adapter and two wires.
 *
 * The whole point is the FIRST line it prints. Errata DS80349C #1 -- MAC
 * registers unreliable below an 8 MHz SPI clock, which no 8051 can reach --
 * applies to silicon revisions B1 and B4 and to nothing else. Whether this
 * project was ever hard is decided by one register, EREVID, and until it
 * is read out loud every other conclusion rests on an assumption.
 */
#include "enc28j60.h"
#include "softuart.h"

#define _XPRINTF_
#include "xprintf.h"

/* Gate for the crystal measurement, in Timer 1 bit periods. 50 periods is
 * 50 * SOFTUART_TICKS_PER_BIT machine cycles; at 6.25 MHz the ENC's CLKOUT
 * puts ~32.5k edges into Timer 0 in that window, which fits in 16 bits. */
#define XTAL_GATE_PERIODS 50

/* xprintf/puts resolve putchar at link time whether or not they run. On
 * the other diagnostics this writes SBUF -- which here IS the SPI link to
 * the ENC, so sending console text through it would clock garbage at the
 * chip. Routing it to the soft console instead is both safe and useful:
 * it makes enc28j60RegDump()'s PRINTF output land on the FTDI. */
int putchar(int ch)
{
	softuart_putc((char)ch);
	return ch;
}

static void putdec16(uint16_t v)
{
	char buf[6];
	int8_t i = 0;

	if (!v) {
		softuart_putc('0');
		return;
	}
	while (v) {
		buf[i++] = (char)('0' + (v % 10));
		v /= 10;
	}
	while (i--)
		softuart_putc(buf[i]);
}

/**
 * Count ENC28J60 CLKOUT edges on T0 (P3.4) over a Timer 1-gated window.
 *
 * The ENC's CLKOUT is derived from its own 25 MHz crystal and defaults to
 * 25/4 = 6.25 MHz, so it is a frequency reference this board does not
 * otherwise have. Counting it against Timer 1 turns "which crystal is
 * actually fitted" from an argument into a measurement -- and the
 * disagreement is real: the HC6800-ES schematic's BOM says 12 MHz while
 * the vendor's own course code uses TH1 = 0xFD, which is 9600 baud only
 * at 11.0592 MHz.
 *
 * counts = 6.25e6 * (gate_cycles / f_cycle), so f_cycle = 6.25e6 *
 * gate_cycles / counts. At 9600 baud in 6T the gate is 9600 cycles and
 * 11.0592 MHz gives 32552 counts against 12 MHz's 30000 -- far apart.
 *
 * ⚠️ NOT exercised in simulation. emu8051's finest time unit is one
 * machine cycle (~543 ns at 1.8432 MHz) and a 6.25 MHz clock has a 160 ns
 * period, so the model cannot represent the input at all. The arithmetic
 * is checked; the code path is not. Treat a first reading on real
 * hardware as a measurement of this function as much as of the crystal.
 *
 * \return raw Timer 0 count, or 0 if CLKOUT is not wired to P3.4.
 */
static uint16_t measure_clkout(void)
{
	uint8_t periods;

	TMOD = (TMOD & 0xF0) | 0x05; /* Timer 0: mode 1, COUNTER on T0 */
	TH0 = 0;
	TL0 = 0;
	TF1 = 0;
	softuart_init();             /* Timer 1 is the gate and the console clock */

	TR0 = 1;
	for (periods = 0; periods < XTAL_GATE_PERIODS; periods++) {
		while (!TF1)
			;
		TF1 = 0;
	}
	TR0 = 0;

	return (uint16_t)(((uint16_t)TH0 << 8) | TL0);
}

void main(void)
{
	static uint8_t tx[16];
	static uint8_t rx[40];
	uint16_t len = 0;
	uint16_t wait;
	uint16_t clk;
	uint8_t rev;
	uint8_t i;

	P2 = 0xff; /* boot marker, same convention as every other demo here */

	softuart_init();
	softuart_puts("\r\nENC28J60 bring-up\r\n");

	clk = measure_clkout();
	softuart_puts("CLKOUT count: ");
	putdec16(clk);
	softuart_puts(clk ? "\r\n" : " (CLKOUT not wired to P3.4)\r\n");

	for (i = 0; i < sizeof(tx); i++)
		tx[i] = (uint8_t)(0x50 + i);

	enc28j60Init();

	/* The line that decides the whole project. */
	rev = enc28j60ReadReg(EREVID);
	softuart_puts("EREVID: 0x");
	softuart_hex8(rev);
	if (rev == 0x02 || rev == 0x04)
		softuart_puts("  B1/B4 -- errata #1 applies, MAC writes verified\r\n");
	else if (rev == 0x05 || rev == 0x06)
		softuart_puts("  B5/B7 -- errata #1 does NOT apply\r\n");
	else
		softuart_puts("  unknown die, or the SPI link is not working\r\n");

	enc28j60PacketSend(sizeof(tx), tx);
	enc28j60PacketSend(sizeof(tx), tx);
	softuart_puts("sent 2\r\n");

	for (wait = 0; wait < 20000u; wait++) {
		len = enc28j60PacketReceive(sizeof(rx), rx);
		if (len)
			break;
	}

	softuart_puts("rx len: ");
	putdec16(len);
	if (len) {
		softuart_puts(" first: 0x");
		softuart_hex8(rx[0]);
	}
	softuart_puts("\r\ndone\r\n");

	for (;;)
		;
}
