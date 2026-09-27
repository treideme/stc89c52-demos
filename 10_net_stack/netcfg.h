/* Network-stack experiment configuration. The *memory budget* (flash, IRAM,
 * XRAM) is not here: it is a link-time limit set per target in build.py,
 * which also passes NET_XRAM_SIZE so RAM-dependent choices follow the part.
 */
#ifndef NETCFG_H
#define NETCFG_H

/* Which rungs to compile, cumulatively:
 * 1 = ARP reply, 2 = + IPv4/ICMP echo, 3 = + UDP echo, 4 = + DHCP client,
 * 5 = + MTU 1500, 6 = + full DHCP (lease timer, renew, rebind, expiry),
 * 7 = + ARP client and DNS A query, 8 = + minimal TCP echo (one connection).
 */
#ifndef NET_RUNG
#define NET_RUNG 8
#endif

#ifndef NET_XRAM_SIZE
#define NET_XRAM_SIZE 256
#endif

/* IP MTU. 576 is the smallest datagram every IPv4 host must accept, and the
 * smallest DHCP message size (RFC 2131). From rung 5, full Ethernet: 1500.
 * Frames are streamed through the ENC's buffer, so neither is bounded by
 * XRAM -- the MTU costs ENC buffer and time, not MCU RAM.
 */
#ifndef NET_MTU
#if NET_RUNG >= 5
#define NET_MTU 1500
#else
#define NET_MTU 576
#endif
#endif
#define NET_MAX_FRAME (NET_MTU + 14 + 4)      /* + Ethernet header + FCS */

/* Copy chunk between ENC RX and TX. Each chunk costs a CS cycle and an RBM
 * and a WBM opcode, so a larger one saves SPI overhead -- one of the few
 * places extra XRAM can buy time. Must be EVEN: TCP sums the payload chunk
 * by chunk, and 16-bit one's-complement partial sums only combine if every
 * chunk but the last starts on an even offset.
 */
#if NET_XRAM_SIZE >= 1024
#define NET_CHUNK 254
#else
#define NET_CHUNK 32
#endif

/* Defences against a marginal SPI link, in enc.c. On this bench the link is
 * electrically poor: a read-only EREVID canary misreads 1-3 times per run
 * under traffic, and corrupted headers arrive carrying frame payload. Set to
 * 0 on wiring you trust -- a soldered board rather than jumper leads -- and
 * the driver stops paying for the paranoia:
 *
 * spi_resync()     CS pulse to unstick a slave caught mid-opcode
 * rd_stable()      read a register twice, believe only a repeat
 * header re-read   read each packet header twice and require agreement
 * RXEN watchdog    re-assert receive, since even control writes corrupt
 *
 * Header validation and ring recovery are NOT behind this flag. A frame that
 * overruns the receive buffer is ordinary traffic, not bad wiring, and the
 * sticky RXERIF/BUFER path has to be handled on any link.
 */
#ifndef ENC_LINK_DEFENCES
#define ENC_LINK_DEFENCES 1
#endif

/* Locally administered unicast MAC (02:...), not 09_ethernet's multicast 01:... */
#define NET_MAC 0x02, 0x00, 0x00, 0x00, 0x00, 0x01

/* Static address for rungs 1-3; rung 4 starts at 0.0.0.0 and asks DHCP. */
#ifndef NET_STATIC_IP
#define NET_STATIC_IP 192, 168, 7, 2
#endif

#define NET_UDP_ECHO_PORT 7

/* DHCP retransmit. Rungs 4-5 count main-loop polls (~1 ms each); from rung 6
 * a Timer 0 tick gives real seconds.
 */
#define NET_DHCP_RETRY_POLLS 3000
#define NET_DHCP_RETRY_S 4

/* Rung 7: the name looked up via the DHCP-provided DNS server, and a UDP
 * port that reports the result (4-byte address + 1 status byte).
 */
#define NET_DNS_NAME "\x04" "time" "\x07" "example" "\x03" "com"
#define NET_DNS_PORT 50000                    /* our source port */
#define NET_DNS_RETRY_S 2
#define NET_DNS_TRIES 4
#define NET_ARP_TRIES 5
#define NET_STATUS_PORT 7777

/* Rung 8: TCP echo on port 7, one connection. RTO in 50 ms ticks. */
#define NET_TCP_PORT 7
#define NET_TCP_RTO_TICKS 20                  /* 1 s */
#define NET_TCP_RETRIES 5

#endif
