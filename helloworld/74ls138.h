#ifndef _74ls138_h_
#define _74ls138_h_

#include <mcs51/8051.h>

#define SN74LS138_BIT0        P2_2
#define SN74LS138_BIT1        P2_3
#define SN74LS138_BIT2        P2_4

#define SN74LS138_SET(val) \
    SN74LS138_BIT0 = ((val) & 0x01); \
    SN74LS138_BIT1 = ((val) & 0x02); \
    SN74LS138_BIT2 = ((val) & 0x04)

#endif // _74ls138_h_
