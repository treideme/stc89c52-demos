#ifndef	__MAIN_H__
#define __MAIN_H__

#include <mcs51/8051.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include "delay.h"

void init_mcu(void);
uint8_t get_keyval(void);
void t0_init(void);


#endif
