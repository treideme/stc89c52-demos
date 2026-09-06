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

uint8_t spi_byte(uint8_t d) {
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
	ENC28J60_SPI_SCK = 0;      // Ensure SCK is low BEFORE CS
    ENC28J60_CONTROL_CS = 0;

	// issue read command
    spi_byte(op | (address & ADDR_MASK));
	// do dummy read if needed
	if(address & 0x80) {
      data = spi_byte(0x00);
	}
    data = spi_byte(0x00);
	// release CS
    ENC28J60_CONTROL_CS = 1;

	return data;
}

void enc28j60WriteOp(uint8_t op, uint8_t address, uint8_t data)
{
	// assert CS
	ENC28J60_SPI_SCK = 0;      // Ensure SCK is low BEFORE CS
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
	ENC28J60_SPI_SCK = 0;      // Ensure SCK is low BEFORE CS
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
	ENC28J60_SPI_SCK = 0;      // Ensure SCK is low BEFORE CS
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

void enc28j60Init(void)
{
	// initialize I/O
	ENC28J60_SPI_SCK = 0;    // set SCK hi
	ENC28J60_CONTROL_CS = 1; // set CS hi
	delay_us(50);

	// perform system reset
	enc28j60WriteOp(ENC28J60_SOFT_RESET, 0, ENC28J60_SOFT_RESET);
	// check CLKRDY bit to see if reset is complete
	delay_us(50000);
	// See See Rev. B4 Silicon Errata point. Just wait.
	//while(!(enc28j60ReadReg(ESTAT) & ESTAT_CLKRDY)); //

	// do bank 0 stuff
	// initialize receive buffer
	// 16-bit transfers, must write low byte first
	// set receive buffer start address
	NextPacketPtr = RXSTART_INIT;
	// Rx start
    enc28j60WriteRegPair(ERXSTL, RXSTART_INIT);
    // set receive pointer address
    enc28j60WriteRegPair(ERXRDPTL, RXSTART_INIT);
    // RX end
    enc28j60WriteRegPair(ERXNDL, RXSTOP_INIT);
    // TX start
    //writeRegPair(ETXSTL, TXSTART_INIT);
    // TX end
    //writeRegPair(ETXNDL, TXSTOP_INIT);
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
    enc28j60WriteRegPair(MACON1, MACON1_MARXEN|MACON1_TXPAUS|MACON1_RXPAUS);
    // enable automatic padding to 60bytes and CRC operations
    enc28j60WriteOp(ENC28J60_BIT_FIELD_SET, MACON3, MACON3_PADCFG0|MACON3_TXCRCEN|MACON3_FRMLNEN);
    // set inter-frame gap (non-back-to-back)
    enc28j60WriteRegPair(MAIPGL, 0x0C12);
    // set inter-frame gap (back-to-back)
    enc28j60WriteReg(MABBIPG, 0x12);
    // Set the maximum packet size which the controller will accept
    // Do not send packets longer than MAX_FRAMELEN:
    enc28j60WriteRegPair(MAMXFLL, MAX_FRAMELEN);
    // do bank 3 stuff
    // write MAC address
    // NOTE: MAC address in ENC28J60 is byte-backward
    enc28j60WriteReg(MAADR5, ENC28J60_MAC0);
    enc28j60WriteReg(MAADR4, ENC28J60_MAC1);
    enc28j60WriteReg(MAADR3, ENC28J60_MAC2);
    enc28j60WriteReg(MAADR2, ENC28J60_MAC3);
    enc28j60WriteReg(MAADR1, ENC28J60_MAC4);
    enc28j60WriteReg(MAADR0, ENC28J60_MAC5);
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
	// Set the write pointer to start of transmit buffer area
	enc28j60WriteReg(EWRPTL, TXSTART_INIT);
	enc28j60WriteReg(EWRPTH, TXSTART_INIT>>8);
	// Set the TXND pointer to correspond to the packet size given
	enc28j60WriteReg(ETXNDL, (TXSTART_INIT+len));
	enc28j60WriteReg(ETXNDH, (TXSTART_INIT+len)>>8);

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

	// limit retrieve length
	// (we reduce the MAC-reported length by 4 to remove the CRC)
	len = MIN(len, maxlen);

	// copy the packet from the receive buffer
	enc28j60ReadBuffer(len, packet);

	// Move the RX read pointer to the start of the next received packet
	// This frees the memory we just read out
	enc28j60WriteReg(ERXRDPTL, (NextPacketPtr));
	enc28j60WriteReg(ERXRDPTH, (NextPacketPtr)>>8);

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

bool enc28j60LinkStatus()
{
  return (enc28j60PhyRead(PHSTAT2) & 0x0400) > 0;
}