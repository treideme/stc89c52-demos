#include "ds1302.h"
#include "shift.h"

/**
 * Latch a single byte on the DS1302 bus.
 * @param data Databyte to write out.
 */
static void DS1302_out(uint8_t data) {
    SHIFT_OUT(DS1302_IO, DS1302_SCLK, data);
}

/**
 * Read a byte from the DS1302 bus.
 * @return data byte read.
 */
static uint8_t DS1302_in() {
    uint8_t dat=0, dat1;
    for(volatile uint8_t n = 0; n < 8; n++) {
        dat1 = DS1302_IO;
        DS1302_SCLK = 1;
        dat |= (dat1 << n);
        DS1302_SCLK = 0;
    }
    return dat;
}

uint8_t DS1302_read(uint8_t address) {
    uint8_t dat;
    DS1302_CE = 0;
    NOP();

    DS1302_SCLK = 0;
    NOP();
    DS1302_CE = 1;
    DS1302_out(address);
    dat = DS1302_in();

    DS1302_CE = 0;
    NOP();
    DS1302_SCLK = 1;
    NOP();
    DS1302_IO = 0;
    NOP();
    DS1302_IO = 1;
    NOP();
    return dat;
}

void DS1302_write(uint8_t address, uint8_t data) {
    DS1302_CE = 0;
    NOP();

    DS1302_SCLK = 0;
    NOP();
    DS1302_CE = 1;
    DS1302_out(address);
    DS1302_out(data);
    DS1302_CE = 0;
    NOP();
    DS1302_SCLK = 1;
    NOP();
    DS1302_IO = 0;
    NOP();
    DS1302_IO = 1;
    NOP();
}
