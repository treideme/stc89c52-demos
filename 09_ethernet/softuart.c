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
 * @file softuart.c See softuart.h for the wiring and why this exists.
 * @author Thomas Reidemeister
 */
#include "softuart.h"

static void softuart_wait_bit(void)
{
	/* Timer 1 in mode 2 reloads itself, so polling TF1 gives an exact
	 * period with no cumulative drift. The jitter is the few cycles
	 * between the flag setting and this loop noticing -- about 2% of a
	 * 192-cycle bit, against the ~5% an 8-N-1 receiver tolerates. */
	while (!TF1)
		;
	TF1 = 0;
}

void softuart_init(void)
{
	SOFTUART_TX = 1; /* an idle line is high; a receiver reads low as a start bit */

	/* Timer 1, mode 2 (8-bit auto-reload). Leaves Timer 0 alone -- it is
	 * wanted for counting the ENC28J60's CLKOUT to measure the crystal. */
	TMOD = (TMOD & 0x0F) | 0x20;
	TH1 = (uint8_t)(256 - SOFTUART_TICKS_PER_BIT);
	TL1 = TH1;
	TR1 = 1;
	TF1 = 0;
}

void softuart_putc(char c)
{
	uint8_t d = (uint8_t)c;
	uint8_t i;

	/* Start on a fresh timer period so the start bit is a full bit wide;
	 * without this it is however much of the current period was left. */
	TF1 = 0;
	softuart_wait_bit();

	SOFTUART_TX = 0; /* start bit */
	for (i = 0; i < 8; i++) {
		softuart_wait_bit();
		SOFTUART_TX = d & 1; /* data bits are LSB first, unlike SPI */
		d >>= 1;
	}
	softuart_wait_bit();
	SOFTUART_TX = 1; /* stop bit */
	softuart_wait_bit();
}

void softuart_puts(const char *s)
{
	while (*s)
		softuart_putc(*s++);
}

void softuart_hex8(uint8_t v)
{
	static const char __code kHex[] = "0123456789abcdef";

	softuart_putc(kHex[(v >> 4) & 0x0F]);
	softuart_putc(kHex[v & 0x0F]);
}

void softuart_hex16(uint16_t v)
{
	softuart_hex8((uint8_t)(v >> 8));
	softuart_hex8((uint8_t)(v & 0xFF));
}
