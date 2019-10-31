#include "i2c.h"

uint8_t i2c_write(uint8_t data) {
  for(uint8_t i = 0; i < 8; i++) {
    I2C_SDA = data & 0x80;
    I2C_SCL = 1;
    data <<= 1;
    I2C_SCL = 0;
  }
  I2C_SCL = 1;
  NOP();
  uint8_t res = I2C_SDA;
  NOP();
  I2C_SCL = 0;
  return res;
}

uint8_t i2c_read(uint8_t *data) {
  uint8_t rdata = 0;
  for(int8_t i = 7; i >= 0; i--) {
    I2C_SCL = 1;
    rdata |= (I2C_SDA<<i);
    I2C_SCL = 0;
  }
  I2C_SCL = 1;
  NOP();
  uint8_t res = I2C_SDA;
  NOP();
  I2C_SCL = 0;
  *data = rdata;
  return res;
}
