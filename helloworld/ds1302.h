#ifndef _ds1302_h_
#define _ds1302_h_

#include <mcs51/compiler.h>
#include <mcs51/8051.h>
#include <stdint.h>

#define DS1302_CE   P3_5
#define DS1302_SCLK P3_6
#define DS1302_IO   P3_4

uint8_t DS1302_read(uint8_t address);
void DS1302_write(uint8_t address, uint8_t data);

#endif // _ds1302_h_
