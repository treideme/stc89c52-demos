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
 * @file enc28j60.h Driver code for enc28j60, based on AVR Lib (http://www.procyonengineering.com/embedded/avr/avrlib/docs/html/index.html).
 * @author Thomas Reidemeister
 */
#include "enc28j60.h"
#include "enc28j60_cfg.h"
#define _XPRINTF_ // Enable xprintf
#include "xprintf.h"

uint8_t Enc28j60Bank;
uint16_t NextPacketPtr;

uint8_t spi_byte(uint8_t d) { // CPHA - CPOL SPI-mode 0
  uint8_t res = 0;
  ENC28J60_SPI_SCK = 0; // Reset state
  for(volatile uint8_t i = 0; i < 8; i++) { // MSB first
    ENC28J60_SPI_MOSI = d & 0x80;
    d <<= 1;
    ENC28J60_SPI_SCK = 1; // Valid data on rising edge
    res |= ENC28J60_SPI_MISO; // Read bit
    res <<= 1;
    ENC28J60_SPI_SCK = 0; // Toggle bits on rising edge
  }
  return res;
}

static void delay_us(uint16_t us)
{
  // Simple delay loop (more than 1us at 12MHz)
  while (us--)
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
	*macaddr++ = enc28j60Read(MAADR5);
	*macaddr++ = enc28j60Read(MAADR4);
	*macaddr++ = enc28j60Read(MAADR3);
	*macaddr++ = enc28j60Read(MAADR2);
	*macaddr++ = enc28j60Read(MAADR1);
	*macaddr++ = enc28j60Read(MAADR0);
}

void nicSetMacAddress(uint8_t* macaddr)
{
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	enc28j60Write(MAADR5, *macaddr++);
	enc28j60Write(MAADR4, *macaddr++);
	enc28j60Write(MAADR3, *macaddr++);
	enc28j60Write(MAADR2, *macaddr++);
	enc28j60Write(MAADR1, *macaddr++);
	enc28j60Write(MAADR0, *macaddr++);
}

void nicRegDump(void)
{
	enc28j60RegDump();
}

uint8_t enc28j60ReadOp(uint8_t op, uint8_t address)
{
	uint8_t data;
   
	// assert CS
  ENC28J60_CONTROL_CS = 0;

	// issue read command
  spi_byte(op | (address & ADDR_MASK));
  data = spi_byte(0x00);
	// do dummy read if needed
	if(address & 0x80)
	{
    data = spi_byte(0x00);
	}

	// release CS
  ENC28J60_CONTROL_CS = 1;

	return data;
}

void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data)
{
	// assert CS
  ENC28J60_CONTROL_CS = 0;

	// issue write command
  spi_byte(op | (address & ADDR_MASK));
  spi_byte(data);

	// release CS
  ENC28J60_CONTROL_CS = 1;
}

void enc28j60ReadBuffer(uint16_t len, uint8_t* data)
{
	// assert CS
  ENC28J60_CONTROL_CS = 0;
	
	// issue read command
  spi_byte(ENC28J60_READ_BUF_MEM);
	while(len--)
	{
		// read data
		*data++ = spi_byte(0x00);
	}	
	// release CS
  ENC28J60_CONTROL_CS = 1;
}

void enc28j60WriteBuffer(uint16_t len, uint8_t* data)
{
	// assert CS
  ENC28J60_CONTROL_CS = 0;
	
	// issue write command
  spi_byte(ENC28J60_WRITE_BUF_MEM);
	while(len--)
	{
		// write data
    spi_byte(*data++);
	}
	// release CS
  ENC28J60_CONTROL_CS = 1;
}

void enc28j60SetBank(uint8_t address)
{
	// set the bank (if needed)
	if((address & BANK_MASK) != Enc28j60Bank)
	{
		// set the bank
		enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, (ECON1_BSEL1|ECON1_BSEL0));
		enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, (address & BANK_MASK)>>5);
		Enc28j60Bank = (address & BANK_MASK);
	}
}

uint8_t enc28j60Read(uint8_t address)
{
	// set the bank
	enc28j60SetBank(address);
	// do the read
	return enc28j60ReadOp(ENC28J60_READ_CTRL_REG, address);
}

void enc28j60Write(uint8_t address, uint8_t data)
{
	// set the bank
	enc28j60SetBank(address);
	// do the write
	enc28j60WriteOp(ENC28J60_WRITE_CTRL_REG, address, data);
}

uint16_t enc28j60PhyRead(uint8_t address)
{
	uint16_t data;

	// Set the right address and start the register read operation
	enc28j60Write(MIREGADR, address);
	enc28j60Write(MICMD, MICMD_MIIRD);

	// wait until the PHY read completes
	while(enc28j60Read(MISTAT) & MISTAT_BUSY);

	// quit reading
	enc28j60Write(MICMD, 0x00);
	
	// get data value
	data  = enc28j60Read(MIRDL);
	data |= enc28j60Read(MIRDH);
	// return the data
	return data;
}

void enc28j60PhyWrite(uint8_t address, uint16_t data)
{
	// set the PHY register address
	enc28j60Write(MIREGADR, address);
	
	// write the PHY data
	enc28j60Write(MIWRL, data);	
	enc28j60Write(MIWRH, data>>8);

	// wait until the PHY write completes
	while(enc28j60Read(MISTAT) & MISTAT_BUSY);
}

void enc28j60Init(void)
{
	// initialize I/O
  ENC28J60_SPI_SCK = 0;    // set SCK hi
  ENC28J60_CONTROL_CS = 1; // set CS hi
  delay_us(50);

	// perform system reset
	enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	// check CLKRDY bit to see if reset is complete
	delay_us(50);
	while(!(enc28j60Read(ESTAT) & ESTAT_CLKRDY));

	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	NextPacketPtr = RXSTART_INIT;
	enc28j60Write(ERXSTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERXSTH, RXSTART_INIT>>8);
	// set receive pointer address
	enc28j60Write(ERXRDPTL, RXSTART_INIT&0xFF);
	enc28j60Write(ERXRDPTH, RXSTART_INIT>>8);
	// set receive buffer end
	// ERXND defaults to 0x1FFF (end of ram)
	enc28j60Write(ERXNDL, RXSTOP_INIT&0xFF);
	enc28j60Write(ERXNDH, RXSTOP_INIT>>8);
	// set transmit buffer start
	// ETXST defaults to 0x0000 (beginnging of ram)
	enc28j60Write(ETXSTL, TXSTART_INIT&0xFF);
	enc28j60Write(ETXSTH, TXSTART_INIT>>8);

	// do bank 2 stuff
	// enable MAC receive
	enc28j60Write(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
	// bring MAC out of reset
	enc28j60Write(MACON2, 0x00);
	// enable automatic padding and CRC operations
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
//	enc28j60Write(MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
	// set inter-frame gap (non-back-to-back)
	enc28j60Write(MAIPGL, 0x12);
	enc28j60Write(MAIPGH, 0x0C);
	// set inter-frame gap (back-to-back)
	enc28j60Write(MABBIPG, 0x12);
	// Set the maximum packet size which the controller will accept
	enc28j60Write(MAMXFLL, MAX_FRAMELEN&0xFF);
	enc28j60Write(MAMXFLH, MAX_FRAMELEN>>8);

	// do bank 3 stuff
	// write MAC address
	// NOTE: MAC address in ENC28J60 is byte-backward
	enc28j60Write(MAADR5, ENC28J60_MAC0);
	enc28j60Write(MAADR4, ENC28J60_MAC1);
	enc28j60Write(MAADR3, ENC28J60_MAC2);
	enc28j60Write(MAADR2, ENC28J60_MAC3);
	enc28j60Write(MAADR1, ENC28J60_MAC4);
	enc28j60Write(MAADR0, ENC28J60_MAC5);

	// no loopback of transmitted frames
	enc28j60PhyWrite(PHCON2, PHCON2_HDLDIS);

	// switch to bank 0
	enc28j60SetBank(ECON1);
	// enable interrutps
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, EIE, EIE_INTIE|EIE_PKTIE);
	// enable packet reception
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);
/*
	enc28j60PhyWrite(PHLCON, 0x0AA2);

	// setup duplex ----------------------

	// Disable receive logic and abort any packets currently being transmitted
	enc28j60WriteOp(ENC28J60_BIT_FIELD_CLR, ECON1, ECON1_TXRTS|ECON1_RXEN);

	{
		uint16_t temp;
		// Set the PHY to the proper duplex mode
		temp = enc28j60PhyRead(PHCON1);
		temp &= ~PHCON1_PDPXMD;
		enc28j60PhyWrite(PHCON1, temp);
		// Set the MAC to the proper duplex mode
		temp = enc28j60Read(MACON3);
		temp &= ~MACON3_FULDPX;
		enc28j60Write(MACON3, temp);
	}

	// Set the back-to-back inter-packet gap time to IEEE specified
	// requirements.  The meaning of the MABBIPG value changes with the duplex
	// state, so it must be updated in this function.
	// In full duplex, 0x15 represents 9.6us; 0x12 is 9.6us in half duplex
	//enc28j60Write(MABBIPG, DuplexState ? 0x15 : 0x12);

	// Reenable receive logic
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_RXEN);

	// setup duplex ----------------------
*/
}

void enc28j60PacketSend(uint16_t len, uint8_t* packet)
{
	// Set the write pointer to start of transmit buffer area
	enc28j60Write(EWRPTL, TXSTART_INIT);
	enc28j60Write(EWRPTH, TXSTART_INIT>>8);
	// Set the TXND pointer to correspond to the packet size given
	enc28j60Write(ETXNDL, (TXSTART_INIT+len));
	enc28j60Write(ETXNDH, (TXSTART_INIT+len)>>8);

	// write per-packet control byte
	enc28j60WriteOp(ENC28J60_WRITE_BUF_MEM, 0, 0x00);

	// copy the packet into the transmit buffer
	enc28j60WriteBuffer(len, packet);
	
	// send the contents of the transmit buffer onto the network
	enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, ECON1, ECON1_TXRTS);
}

uint16_t enc28j60PacketReceive(uint16_t maxlen, uint8_t* packet)
{
	uint16_t rxstat;
	uint16_t len;

	// check if a packet has been received and buffered
//	if( !(enc28j60Read(EIR) & EIR_PKTIF) )
	if( !enc28j60Read(EPKTCNT) )
		return 0;
	
	// Make absolutely certain that any previous packet was discarded	
	//if( WasDiscarded == FALSE)
	//	MACDiscardRx();

	// Set the read pointer to the start of the received packet
	enc28j60Write(ERDPTL, (NextPacketPtr));
	enc28j60Write(ERDPTH, (NextPacketPtr)>>8);
	// read the next packet pointer
	NextPacketPtr  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	NextPacketPtr |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the packet length
	len  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	len |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;
	// read the receive status
	rxstat  = enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0);
	rxstat |= enc28j60ReadOp(ENC28J60_READ_BUF_MEM, 0)<<8;

	// limit retrieve length
	// (we reduce the MAC-reported length by 4 to remove the CRC)
	len = MIN(len, maxlen);

	// copy the packet from the receive buffer
	enc28j60ReadBuffer(len, packet);

	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	enc28j60Write(ERXRDPTL, (NextPacketPtr));
	enc28j60Write(ERXRDPTH, (NextPacketPtr)>>8);

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
//	unsigned char macaddr[6];
//	result = ax88796Read(TR);
	
//	PUTS("Media State: ");
//	if(!(result & AUTOD))
//		PUTS("Autonegotiation\r\n");
//	else if(result & RST_B)
//		PUTS("PHY in Reset   \r\n");
//	else if(!(result & RST_10B))
//		PUTS("10BASE-T       \r\n");
//	else if(!(result & RST_TXB))
//		PUTS("100BASE-T      \r\n");
  PRINTF("RevID: 0x%x\r\n", enc28j60Read(EREVID));

	PUTS("Cntrl: ECON1 ECON2 ESTAT  EIR  EIE\r\n");
	PRINTF("         %02X", enc28j60Read(ECON1));
	PRINTF("    %02X", enc28j60Read(ECON2));
	PRINTF("    %02X", enc28j60Read(ESTAT));
	PRINTF("    %02X", enc28j60Read(EIR));
	PRINTF("   %02X", enc28j60Read(EIE));
  PUTS("\r\n");

  PUTS("MAC  : MACON1  MACON2  MACON3  MACON4  MAC-Address\r\n");
	PRINTF("        0x%02X", enc28j60Read(MACON1));
	PRINTF("    0x%02X", enc28j60Read(MACON2));
	PRINTF("    0x%02X", enc28j60Read(MACON3));
	PRINTF("    0x%02X", enc28j60Read(MACON4));
	PRINTF("   %02X", enc28j60Read(MAADR5));
	PRINTF("%02X", enc28j60Read(MAADR4));
	PRINTF("%02X", enc28j60Read(MAADR3));
	PRINTF("%02X", enc28j60Read(MAADR2));
	PRINTF("%02X", enc28j60Read(MAADR1));
	PRINTF("%02X", enc28j60Read(MAADR0));
  PUTS("\r\n");

  PUTS("Rx   : ERXST  ERXND  ERXWRPT ERXRDPT ERXFCON EPKTCNT MAMXFL\r\n");
	PRINTF("       0x%02X", enc28j60Read(ERXSTH));
	PRINTF("%02X", enc28j60Read(ERXSTL));
	PRINTF(" 0x%02X", enc28j60Read(ERXNDH));
	PRINTF("%02X", enc28j60Read(ERXNDL));
	PRINTF(" 0x%02X", enc28j60Read(ERXWRPTH));
	PRINTF("%02X", enc28j60Read(ERXWRPTL));
	PRINTF("  0x%02X", enc28j60Read(ERXRDPTH));
	PRINTF("%02X", enc28j60Read(ERXRDPTL));
	PRINTF("   0x%02X", enc28j60Read(ERXFCON));
	PRINTF("    0x%02X", enc28j60Read(EPKTCNT));
	PRINTF("  0x%02X", enc28j60Read(MAMXFLH));
	PRINTF("%02X", enc28j60Read(MAMXFLL));
  PUTS("\r\n");

  PUTS("Tx   : ETXST  ETXND  MACLCON1 MACLCON2 MAPHSUP\r\n");
	PRINTF("       0x%02X", enc28j60Read(ETXSTH));
	PRINTF("%02X", enc28j60Read(ETXSTL));
	PRINTF(" 0x%02X", enc28j60Read(ETXNDH));
	PRINTF("%02X", enc28j60Read(ETXNDL));
	PRINTF("   0x%02X", enc28j60Read(MACLCON1));
	PRINTF("     0x%02X", enc28j60Read(MACLCON2));
	PRINTF("     0x%02X", enc28j60Read(MAPHSUP));
  PUTS("\r\n");
}



