#ifndef _i2c_h_
#define _i2c_h_

#include <stdint.h>
#include <mcs51/8051.h>
#include <mcs51/compiler.h>

#define I2C_SDA P2_0
#define I2C_SCL P2_1

#define i2c_start() \
  I2C_SDA = 0;      \
  NOP();            \
  I2C_SCL = 0;      \
  NOP();

#define i2c_stop()  \
  I2C_SCL = 1;      \
  NOP();            \
  I2C_SDA = 1;      \
  NOP();


uint8_t i2c_write(uint8_t data);
uint8_t i2c_read(uint8_t *data);

#endif // _i2c_h_
