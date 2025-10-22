#ifndef	__MAIN_H__
#define __MAIN_H__

#include <mcs51/8051.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include "delay.h"

// Keil C51 is a bitch
#define _crol_(val, n) ( (uint8_t)((val) << (n) | (val) >> (8 - (n))) )
#define _cror_(val, n) ( (uint8_t)((val) >> (n) | (val) << (8 - (n))) )

void init_mcu(void);
void dis_cled(void);
void t0_init(void);


#endif
