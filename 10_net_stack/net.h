#ifndef NET_H
#define NET_H

#include <stdint.h>

extern __xdata uint8_t our_ip[4];

#include "netcfg.h"
#if NET_RUNG >= 6
/* SDCC needs the ISR prototype visible in the file that defines main(). */
void t0_isr(void) __interrupt(1);
#endif

void net_init(void);
void net_poll(void);

#endif
