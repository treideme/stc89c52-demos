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
 * @file enc28j60.c Driver code for enc28j60, based on AVR Lib (http://www.procyonengineering.com/embedded/avr/avrlib/docs/html/index.html).
 * @author Thomas Reidemeister
 */
#include "enc28j60.h"
#include "enc28j60_cfg.h"
#define _XPRINTF_ // Enable xprintf
#include "xprintf.h"

uint8_t Enc28j60Bank;
uint16_t NextPacketPtr;

#if ENC28J60_SPI_MODE0

/* LSB-first to MSB-first. Mode 0 shifts the low bit of SBUF out first; the
 * ENC28J60 expects the high bit first. One MOVC per byte against 256 bytes
 * of code memory -- cheap here, where the whole image is ~3.2 KB of 8 KB. */
static const uint8_t __code kBitReverse[256] = {
  0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
  0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
  0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
  0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
  0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
  0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
  0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
  0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
  0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
  0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
  0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
  0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
  0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
  0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
  0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
  0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
  0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
  0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
  0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
  0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
  0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
  0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
  0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
  0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
  0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
  0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
  0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
  0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
  0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
  0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
  0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
  0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF,
};

/* Send one byte.
 *
 * The old routine was full duplex and returned what came back. Every WRITE
 * call site discarded that value and every READ call site passed a dummy
 * argument, so the duplex was never used -- and Mode 0 cannot provide it
 * anyway: transmit and receive are separate eight-clock bursts sharing one
 * wire, not one exchange. Splitting the two made the direction explicit
 * instead of leaving half of every transfer silently unused. */
static void spi_tx(uint8_t d) {
  ENC28J60_SO_OE = 1;  /* SO off the bus before we drive the shared pin */
  REN = 0;             /* ...and make sure no reception is armed */
  SBUF = kBitReverse[d];
  while(!TI)
    ;
  TI = 0;
}

static uint8_t spi_rx(void) {
  uint8_t d;

  ENC28J60_SO_OE = 0;  /* hand the shared pin to the ENC */
  RI = 0;
  REN = 1;             /* in Mode 0, REN=1 with RI=0 IS the start trigger */
  while(!RI)
    ;
  /* Clear REN before RI. Leaving REN set with RI clear immediately starts
   * another reception -- the trigger is a condition, not an edge, and the
   * eight surplus clocks it emits would desynchronise the whole transfer. */
  REN = 0;
  RI = 0;
  d = kBitReverse[SBUF];
  ENC28J60_SO_OE = 1;
  return d;
}

#else

static uint8_t bitbang_xfer(uint8_t d) {
  uint8_t res = 0;
  for(uint8_t i = 0; i < 8; i++) {
    ENC28J60_SPI_MOSI = d & 0x80;
    d <<= 1;

    ENC28J60_SPI_SCK = 1;

    res <<= 1;
    res |= ENC28J60_SPI_MISO; // Sample MISO on rising edge (mode 0)

    ENC28J60_SPI_SCK = 0;
  }
  return res;
}

static void spi_tx(uint8_t d) { (void)bitbang_xfer(d); }
static uint8_t spi_rx(void) { return bitbang_xfer(0x00); }

#endif

// NOT microseconds. One iteration is ~20-30 machine cycles, so at 12 MHz
// (1 MIPS on a classic 8051 core) a count is tens of microseconds, not one.
// The old name said us and every call site was read as us; the reset wait
// below is therefore ~1 s rather than the 50 ms it claims. That is harmless
// -- errata #2 asks for at least 1 ms -- but the name was a lie.
static void delay_loops(uint16_t n)
{
  while (n--)
    ;
}

void nicInit(void)
{
	enc28j60Init();
}

void nicSend(unsigned int len, uint8_t* packet)
{
	enc28j60PacketSend(len, packet);
}

unsigned int nicPoll(unsigned int maxlen, uint8_t* packet)
{
	return enc28j60PacketReceive(maxlen, packet);
}

void nicGetMacAddress(uint8_t* macaddr)
{
	// read MAC address registers
	// NOTE: MAC address in ENC28J60 is byte-backward
	*macaddr++ = enc28j60ReadReg(MAADR5);
	*macaddr++ = enc28j60ReadReg(MAADR4);
	*macaddr++ = enc28j60ReadReg(MAADR3);
	*macaddr++ = enc28j60ReadReg(MAADR2);
	*macaddr++ = enc28j60ReadReg(MAADR1);
	*macaddr++ = enc28j60ReadReg(MAADR0);
}

void nicSetMacAddress(uint8_t* macaddr)
{
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	enc28j60WriteReg(MAADR5, *macaddr++);
	enc28j60WriteReg(MAADR4, *macaddr++);
	enc28j60WriteReg(MAADR3, *macaddr++);
	enc28j60WriteReg(MAADR2, *macaddr++);
	enc28j60WriteReg(MAADR1, *macaddr++);
	enc28j60WriteReg(MAADR0, *macaddr++);
}

void nicRegDump(void)
{
	enc28j60RegDump();
}

uint8_t enc28j60ReadOp(uint8_t op, uint8_t address)
{
	uint8_t data;
   
	// assert CS
	// Idle SCK before asserting CS. ENC28J60_SCK_IDLE is not always 0: the
	// Mode 0 wiring puts an inverter between P3.1 and the part, so the
	// level meaning "SCK low at the ENC" is 1 there. A literal 0 written
	// through the inverter starts every transaction with SCK already HIGH,
	// the burst's first clock edge then does nothing, and exactly one bit
	// is lost per transfer -- which on a bench reads as a flaky adapter
	// rather than as a polarity error. (Caught in simulation: the model
	// assembled 0x7C where the firmware had sent 0xBE, which is
	// (0xBE << 1) & 0xFF -- one bit short and one zero too many.)
	ENC28J60_SPI_SCK = ENC28J60_SCK_IDLE;
    ENC28J60_CONTROL_CS = 0;

	// issue read command
    spi_tx(op | (address & ADDR_MASK));
	// do dummy read if needed
	if(address & 0x80) {
      data = spi_rx();
	}
    data = spi_rx();
	// release CS
    ENC28J60_CONTROL_CS = 1;

	return data;
}

void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data)
{
	// assert CS
	// Idle SCK before asserting CS. ENC28J60_SCK_IDLE is not always 0: the
	// Mode 0 wiring puts an inverter between P3.1 and the part, so the
	// level meaning "SCK low at the ENC" is 1 there. A literal 0 written
	// through the inverter starts every transaction with SCK already HIGH,
	// the burst's first clock edge then does nothing, and exactly one bit
	// is lost per transfer -- which on a bench reads as a flaky adapter
	// rather than as a polarity error. (Caught in simulation: the model
	// assembled 0x7C where the firmware had sent 0xBE, which is
	// (0xBE << 1) & 0xFF -- one bit short and one zero too many.)
	ENC28J60_SPI_SCK = ENC28J60_SCK_IDLE;
    ENC28J60_CONTROL_CS = 0;

	// issue write command
    spi_tx(op | (address & ADDR_MASK));
    spi_tx(data);

	// release CS
    ENC28J60_CONTROL_CS = 1;
}

void enc28j60ReadBuffer(uint16_t len, uint8_t* data)
{
	// assert CS
	// Idle SCK before asserting CS. ENC28J60_SCK_IDLE is not always 0: the
	// Mode 0 wiring puts an inverter between P3.1 and the part, so the
	// level meaning "SCK low at the ENC" is 1 there. A literal 0 written
	// through the inverter starts every transaction with SCK already HIGH,
	// the burst's first clock edge then does nothing, and exactly one bit
	// is lost per transfer -- which on a bench reads as a flaky adapter
	// rather than as a polarity error. (Caught in simulation: the model
	// assembled 0x7C where the firmware had sent 0xBE, which is
	// (0xBE << 1) & 0xFF -- one bit short and one zero too many.)
	ENC28J60_SPI_SCK = ENC28J60_SCK_IDLE;
    ENC28J60_CONTROL_CS = 0;

	// issue read command
    spi_tx(ENC28J60_READ_BUF_MEM);
	while(len--)
	{
		// read data
		*data++ = spi_rx();
	}	
	// release CS
    ENC28J60_CONTROL_CS = 1;
}

void enc28j60WriteBuffer(uint16_t len, uint8_t* data)
{
	// assert CS
	// Idle SCK before asserting CS. ENC28J60_SCK_IDLE is not always 0: the
	// Mode 0 wiring puts an inverter between P3.1 and the part, so the
	// level meaning "SCK low at the ENC" is 1 there. A literal 0 written
	// through the inverter starts every transaction with SCK already HIGH,
	// the burst's first clock edge then does nothing, and exactly one bit
	// is lost per transfer -- which on a bench reads as a flaky adapter
	// rather than as a polarity error. (Caught in simulation: the model
	// assembled 0x7C where the firmware had sent 0xBE, which is
	// (0xBE << 1) & 0xFF -- one bit short and one zero too many.)
	ENC28J60_SPI_SCK = ENC28J60_SCK_IDLE;
    ENC28J60_CONTROL_CS = 0;

	// issue write command
    spi_tx(ENC28J60_WRITE_BUF_MEM);
	while(len--)
	{
		// write data
    spi_tx(*data++);
	}
	// release CS
    ENC28J60_CONTROL_CS = 1;
}

void enc28j60SetBank(uint8_t address)
{
	// set the bank (if needed)
	//if((address & BANK_MASK) != Enc28j60Bank)
	//{
		// set the bank
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
		Enc28j60Bank = (address & BANK_MASK);
	//}
}

uint8_t enc28j60ReadReg(uint8_t address)
{
	// set the bank
	enc28j60SetBank(address);
	// do the read
	return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

void enc28j60WriteReg(uint8_t address, uint8_t data)
{
	// set the bank
	enc28j60SetBank(address);
	// do the write
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

/* Nonzero when the part is a die that errata #1 applies to. */
static uint8_t Enc28j60MacVerify;

/* Errata DS80349C #1, B1 and B4 ONLY: with an SPI clock below 8 MHz, reads
 * and writes of the MAC registers may be unreliable. No 8051 gets near
 * 8 MHz -- bit-banging measures 35 kHz here and UART Mode 0 reaches
 * 1.8432 MHz -- so on affected silicon this is not an edge case, it is
 * every access.
 *
 * The erratum offers two workarounds and this board can use neither: "run
 * the SPI at 8 MHz" is out of reach, and "clock the host off the ENC28J60's
 * CLKOUT" means taking the crystal off the dev kit.
 *
 * There is a third, and it falls out of reading what the erratum actually
 * restricts. Access is UNRELIABLE, not impossible, and it covers the MAC
 * registers only -- not the ETH registers and not the packet buffer, which
 * is where all the throughput is. MAC registers are written about a dozen
 * times, all of them during init. So write, read back, and retry: an
 * unreliable access becomes a bounded startup cost, with no hardware change
 * and nothing to pay on B5/B7.
 */
static void enc28j60WriteMacReg(uint8_t address, uint8_t data)
{
	uint8_t attempt;

	if (!Enc28j60MacVerify) {
		enc28j60WriteReg(address, data);
		return;
	}
	/* The read-back is itself a MAC access and can be corrupted too, so a
	 * mismatch does not prove the write failed. Retrying covers both. */
	for (attempt = 0; attempt < 8; attempt++) {
		enc28j60WriteReg(address, data);
		if (enc28j60ReadReg(address) == data)
			return;
	}
}

void enc28j60WriteRegPair(uint8_t address, uint16_t data) {
	// set the bank
	enc28j60SetBank(address);
	// do the write
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, (data&0xFF));
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address+1, (data) >> 8);
}


uint16_t enc28j60PhyRead(uint8_t address)
{
	uint16_t data;

	// Set the right address and start the register read operation
	enc28j60WriteReg(MIREGADR, address);
	enc28j60WriteReg(MICMD, MICMD_MIIRD);

	// wait until the PHY read completes
	while(enc28j60ReadReg(MISTAT) & MISTAT_BUSY);

	// quit reading
	enc28j60WriteReg(MICMD, 0x00);
	
	// get data value
	data  = enc28j60ReadReg(MIRDL);
	data |= enc28j60ReadReg(MIRDH) << 8;
	// return the data
	return data;
}

void enc28j60PhyWrite(uint8_t address, uint16_t data)
{
	// set the PHY register address
	enc28j60WriteReg(MIREGADR, address);
	
	// write the PHY data
	enc28j60WriteRegPair(MIWRL, data);

	// wait until the PHY write completes
	while(enc28j60ReadReg(MISTAT) & MISTAT_BUSY);
}

// Errata DS80349C #19, all revisions: the SPI System Reset command is
// unavailable while the device is in Power Save mode. The errata gives a
// five-step recovery sequence for "a device in an unknown state", which is
// what a part that has just come out of POR alongside the host is.
//
// The old code did step 3 and nothing else, and had step 5 commented out
// with a reference to errata #2. That reference was half right and the
// conclusion was wrong. #2 says polling CLKRDY *instead of waiting* will
// not detect PHY readiness, because an SPI reset stops the PHY clock
// without clearing the bit -- so a bare poll is indeed useless. #19 step 5
// asks for the same bit to be read *after* the 1 ms wait, as confirmation
// the reset happened at all, with a retry if it did not. Same register,
// opposite verdict, and the difference is entirely in the ordering.
// Deleting the check satisfied #2 and abandoned #19.
//
// Returns 1 if the part confirmed the reset, 0 if it never did.
static uint8_t enc28j60Reset(void)
{
	uint8_t attempt;
	uint8_t estat;

	for (attempt = 0; attempt < 8; attempt++)
	{
		// 1. clear ECON2.PWRSV
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON2, ECON2_PWRSV);
		// 2. wait at least 300 us for power to be restored
		delay_loops(2000);
		// 3. issue the System Reset command
		enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
		// 4. wait 1 ms; errata #2 says there is nothing useful to poll
		//    during this window, so it has to be a blind wait
		delay_loops(50000);
		// 5. confirm: CLKRDY set and the unimplemented bit 3 clear
		estat = enc28j60ReadReg(ESTAT);
		if ((estat & ESTAT_CLKRDY) && !(estat & 0x08))
			return 1;
	}
	return 0;
}

void enc28j60Init(void)
{
	// initialize I/O
	ENC28J60_SPI_SCK = ENC28J60_SCK_IDLE; // idle SCK LOW at the ENC (datasheet 4.1)
	ENC28J60_CONTROL_CS = 1; // set CS hi
	delay_loops(50);

	// Reset per errata #19's sequence. The return value is deliberately
	// ignored here to keep the signature the callers expect -- but it is
	// the honest place for a caller to learn the part never came up, and
	// enc28j60Reset() is exposed so one can.
	(void)enc28j60Reset();

	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	NextPacketPtr = RXSTART_INIT;
	// Rx start
    enc28j60WriteRegPair(ERXSTL, RXSTART_INIT);
    // RX end. Must be odd for the errata #14 workaround below to have an
    // odd value to fall back on; 0x07FF is.
    enc28j60WriteRegPair(ERXNDL, RXSTOP_INIT);
    // Set the RX read pointer. Errata DS80349C #14, all revisions: an EVEN
    // value in ERXRDPT lets the receive hardware corrupt the circular buffer,
    // Next Packet Pointer and status vector included. At init the next packet
    // pointer equals ERXST, which is the first branch of the errata's
    // Example 2: write ERXND.
    enc28j60WriteRegPair(ERXRDPTL, RXSTOP_INIT);
    // TX start and end. Both were commented out, so ETXST kept its reset
    // value of 0x0000 -- an address inside the RX buffer. Every transmit
    // would have put the whole receive area on the wire ahead of the frame.
    enc28j60WriteRegPair(ETXSTL, TXSTART_INIT);
    enc28j60WriteRegPair(ETXNDL, TXSTOP_INIT);
    // do bank 1 stuff, packet filter:
    // For broadcast packets we allow only ARP packtets
    // All other packets should be unicast only for our mac (MAADR)
    //
    // The pattern to match on is therefore
    // Type     ETH.DST
    // ARP      BROADCAST
    // 06 08 -- ff ff ff ff ff ff -> ip checksum for theses bytes=f7f9
    // in binary these poitions are:11 0000 0011 1111
    // This is hex 303F->EPMM0=0x3f,EPMM1=0x30
    enc28j60WriteReg(ERXFCON, ERXFCON_UCEN|ERXFCON_CRCEN|ERXFCON_PMEN);
    enc28j60WriteRegPair(EPMM0, 0x303f);
    enc28j60WriteRegPair(EPMCSL, 0xf7f9);

	//
    //
    // do bank 2 stuff
    // enable MAC receive
    // and bring MAC out of reset (writes 0x00 to MACON2)
    /* Decide the MAC write strategy before writing any MAC register.
     * EREVID is an ETH register, so reading it stays reliable even on the
     * silicon errata #1 affects -- which is the whole reason this test is
     * possible at all. DS80349C Table 1: 0x02 = B1, 0x04 = B4, 0x05 = B5,
     * 0x06 = B7, and the erratum's affected-revisions box carries marks
     * under B1 and B4 only. On anything newer the verified path is pure
     * cost, so it is not paid. */
    {
        uint8_t rev = enc28j60ReadReg(EREVID);
        Enc28j60MacVerify = (rev == 0x02) || (rev == 0x04);
    }

    enc28j60WriteMacReg(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
    /* MACON2 = 0 brings the MAC out of reset. The old code got that as a
     * side effect of writing the pair at MACON1; spelled out because each
     * half now has to be verified on its own. */
    enc28j60WriteMacReg(MACON2, 0x00);
    // Enable automatic padding to 60 bytes and CRC operations.
    // Datasheet 3.2.3/3.2.4: BFS and BFC are valid on ETH registers ONLY.
    // MACON3 is a MAC register -- this file's own header tags it with
    // SPRD_MASK to say so -- so the bit-field form was undefined behaviour.
    // MACON3 reads 0x00 after reset, so a plain WCR sets the same bits.
    enc28j60WriteMacReg(MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
    // set inter-frame gap (non-back-to-back)
    enc28j60WriteMacReg(MAIPGL, 0x12);
    enc28j60WriteMacReg(MAIPGH, 0x0C);
    // set inter-frame gap (back-to-back)
    enc28j60WriteMacReg(MABBIPG, 0x12);
    // Set the maximum packet size which the controller will accept
    // Do not send packets longer than MAX_FRAMELEN:
    enc28j60WriteMacReg(MAMXFLL, MAX_FRAMELEN & 0xFF);
    enc28j60WriteMacReg(MAMXFLH, MAX_FRAMELEN >> 8);
    // do bank 3 stuff
    // write MAC address
    // NOTE: MAC address in ENC28J60 is byte-backward
    enc28j60WriteMacReg(MAADR5, ENC28J60_MAC0);
    enc28j60WriteMacReg(MAADR4, ENC28J60_MAC1);
    enc28j60WriteMacReg(MAADR3, ENC28J60_MAC2);
    enc28j60WriteMacReg(MAADR2, ENC28J60_MAC3);
    enc28j60WriteMacReg(MAADR1, ENC28J60_MAC4);
    enc28j60WriteMacReg(MAADR0, ENC28J60_MAC5);
    // no loopback of transmitted frames
    enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);
    // switch to bank 0
    enc28j60SetBank(ECON1);
    // enable interrutps
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
    // enable packet reception
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
    //Configure leds
    enc28j60PhyWrite(PHLCON,0x476);
}

void enc28j60PacketSend(uint16_t len, uint8_t* packet)
{
	uint8_t eir;
	uint16_t guard;

	// Errata DS80349C #12, first half, affects B1/B4/B5/B7 -- every part ever
	// shipped. A hardware transmit abort stalls the internal transmit logic
	// and TXRTS then stays set indefinitely, so the FIRST failed send wedges
	// every send after it. The errata offers this unconditional reset
	// explicitly, "for simplicity"; take it.
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRST);
	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRST);
	// Clearing TXRST can itself raise TXERIF, so clear the flags after it.
	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, EIR, EIR_TXERIF|EIR_TXIF);

	// Set the write pointer to start of transmit buffer area
	enc28j60WriteRegPair(EWRPTL, TXSTART_INIT);
	// Set the TXND pointer to correspond to the packet size given. ETXND is
	// inclusive and TXSTART holds the control byte, so start+len is right:
	// control at start, payload at start+1 .. start+len.
	enc28j60WriteRegPair(ETXNDL, (TXSTART_INIT+len));

	// write per-packet control byte
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len, packet);

	// send the contents of the transmit buffer onto the network
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);

	// Errata #12, second half, B5/B7: on an aborted transmit TXIF never
	// arrives, so waiting on TXIF alone hangs. Wait on EITHER flag, and if
	// TXIF did not come, force TXRTS down so this send does not wedge the
	// next one. The guard bounds the spin -- a dead chip must not hang the
	// caller, because on this board there is no console to say that it did.
	guard = 0xFFFF;
	do {
		eir = enc28j60ReadReg(EIR);
	} while (!(eir & (EIR_TXIF|EIR_TXERIF)) && --guard);

	if (!(eir & EIR_TXIF))
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS);
}

uint16_t enc28j60PacketReceive(uint16_t maxlen, uint8_t* packet)
{
	uint16_t rxstat;
	uint16_t len;

	// check if a packet has been received and buffered
//	if( !(enc28j60ReadReg(EIR) & EIR_PKTIF) )
	if( !enc28j60ReadReg(EPKTCNT) )
		return 0;
	
	// Make absolutely certain that any previous packet was discarded	
	//if( WasDiscarded == FALSE)
	//	MACDiscardRx();

	// Set the read pointer to the start of the received packet
	enc28j60WriteReg(ERDPTL, (NextPacketPtr));
	enc28j60WriteReg(ERDPTH, (NextPacketPtr)>>8);
	// read the next packet pointer
	NextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	NextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the packet length
	len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the receive status
	rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;

	// The MAC-reported length includes the 4-byte CRC, which the caller does
	// not want. The comment here said so and the code did not do it.
	len = (len > 4) ? (len - 4) : 0;

	// rxstat bit 7 is Received Ok. A frame that failed CRC or ran long was
	// never checked before, so a corrupt frame reached the caller looking
	// exactly like a good one. Drop the payload -- but keep going, because
	// the buffer space still has to be released either way.
	if (!(rxstat & 0x80))
		len = 0;

	// limit retrieve length
	len = MIN(len, maxlen);

	// copy the packet from the receive buffer
	if (len)
		enc28j60ReadBuffer(len, packet);

	// Move the RX read pointer to the start of the next received packet,
	// which frees the memory just read out. Errata DS80349C #14, all
	// revisions: only ODD values may be written to ERXRDPT, or the receive
	// hardware corrupts the circular buffer. This is the errata's Example 2
	// verbatim; NextPacketPtr is always even because of hardware padding.
	if (NextPacketPtr == RXSTART_INIT)
		enc28j60WriteRegPair(ERXRDPTL, RXSTOP_INIT);
	else
		enc28j60WriteRegPair(ERXRDPTL, NextPacketPtr - 1);

	// decrement the packet counter indicate we are done with this packet
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON2, ECON2_PKTDEC);

	return len;
}

void enc28j60ReceiveOverflowRecover(void)
{
	// receive buffer overflow handling procedure

	// recovery completed
}

void enc28j60RegDump(void)
{
    PRINTF("RevID: 0x%x\r\n", enc28j60ReadReg(EREVID));

	//PUTS("Cntrl: ECON1 ECON2 ESTAT  EIR  EIE\r\n");
	//PRINTF("         %02X", enc28j60ReadReg(ECON1));
	//PRINTF("    %02X", enc28j60ReadReg(ECON2));
	//PRINTF("    %02X", enc28j60ReadReg(ESTAT));
	//PRINTF("    %02X", enc28j60ReadReg(EIR));
	//PRINTF("   %02X", enc28j60ReadReg(EIE));
    //PUTS("\r\n");
    
    PUTS("MAC  : MACON1  MACON2  MACON3  MACON4  MAC-Address\r\n");
	//PRINTF("        0x%02X", enc28j60ReadReg(MACON1));
	//PRINTF("    0x%02X", enc28j60ReadReg(MACON2));
	//PRINTF("    0x%02X", enc28j60ReadReg(MACON3));
	//PRINTF("    0x%02X", enc28j60ReadReg(MACON4));
	PRINTF("   %02X", enc28j60ReadReg(MAADR5));
	PRINTF("%02X", enc28j60ReadReg(MAADR4));
	PRINTF("%02X", enc28j60ReadReg(MAADR3));
	PRINTF("%02X", enc28j60ReadReg(MAADR2));
	PRINTF("%02X", enc28j60ReadReg(MAADR1));
	PRINTF("%02X", enc28j60ReadReg(MAADR0));
    PUTS("\r\n");

	enc28j60WriteReg(MAADR5, ENC28J60_MAC0);
    enc28j60WriteReg(MAADR4, ENC28J60_MAC1);
    enc28j60WriteReg(MAADR3, ENC28J60_MAC2);
    enc28j60WriteReg(MAADR2, ENC28J60_MAC3);
    enc28j60WriteReg(MAADR1, ENC28J60_MAC4);
    enc28j60WriteReg(MAADR0, ENC28J60_MAC5);
    
    //PUTS("Rx   : ERXST  ERXND  ERXWRPT ERXRDPT ERXFCON EPKTCNT MAMXFL\r\n");
	//PRINTF("       0x%02X", enc28j60ReadReg(ERXSTH));
	//PRINTF("%02X", enc28j60ReadReg(ERXSTL));
	//PRINTF(" 0x%02X", enc28j60ReadReg(ERXNDH));
	//PRINTF("%02X", enc28j60ReadReg(ERXNDL));
	//PRINTF(" 0x%02X", enc28j60ReadReg(ERXWRPTH));
	//PRINTF("%02X", enc28j60ReadReg(ERXWRPTL));
	//PRINTF("  0x%02X", enc28j60ReadReg(ERXRDPTH));
	//PRINTF("%02X", enc28j60ReadReg(ERXRDPTL));
	//PRINTF("   0x%02X", enc28j60ReadReg(ERXFCON));
	//PRINTF("    0x%02X", enc28j60ReadReg(EPKTCNT));
	//PRINTF("  0x%02X", enc28j60ReadReg(MAMXFLH));
	//PRINTF("%02X", enc28j60ReadReg(MAMXFLL));
    //PUTS("\r\n");
    //
    //PUTS("Tx   : ETXST  ETXND  MACLCON1 MACLCON2 MAPHSUP\r\n");
	//PRINTF("       0x%02X", enc28j60ReadReg(ETXSTH));
	//PRINTF("%02X", enc28j60ReadReg(ETXSTL));
	//PRINTF(" 0x%02X", enc28j60ReadReg(ETXNDH));
	//PRINTF("%02X", enc28j60ReadReg(ETXNDL));
	//PRINTF("   0x%02X", enc28j60ReadReg(MACLCON1));
	//PRINTF("     0x%02X", enc28j60ReadReg(MACLCON2));
	//PRINTF("     0x%02X", enc28j60ReadReg(MAPHSUP));
    //PUTS("\r\n");
    //
	//PUTS("Link Status: ");
	//PRINTF("%d", enc28j60LinkStatus());
	//PRINTF("\r\nPHSTAT1 0x%02X", enc28j60PhyRead(PHSTAT1));
	//PRINTF("\r\nPHSTAT2 0x%02X", enc28j60PhyRead(PHSTAT2));
	//PUTS("\r\n");
}

bool enc28j60LinkStatus(void)
{
  return (enc28j60PhyRead(PHSTAT2) & 0x0400) > 0;
}