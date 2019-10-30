/**
 * Various helper shift routines.
 */
#ifndef _shift_h_
#define _shift_h_

#include <mcs51/compiler.h>
#include <stdint.h>

#define SHIFT_OUT(dp, cp, data)             \
    {                                       \
    uint8_t data_tmp = data;                \
    for(volatile uint8_t i=0; i < 8; i++) { \
        dp = data_tmp & 0x01;               \
        cp = 1;                             \
        data_tmp>>=1;                       \
        cp = 0;                             \
    } \
    } \
    NOP()


#endif // _shift_h_
