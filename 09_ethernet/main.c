/* The network stack: how much of ARP / IPv4+ICMP / UDP / DHCP / DNS / TCP fits on
 * an STC89C52RC driving the ENC28J60 over bit-banged SPI. The pins are not
 * P0 any more -- see enc.c. Which rungs are compiled in is NET_RUNG; see
 * README.md.
 */
#include "net.h"

void main(void)
{
  net_init();
  for (;;)
    net_poll();
}
