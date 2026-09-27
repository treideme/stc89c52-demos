/* Minimal network stack for STC89C5x + ENC28J60.
 *
 * Nothing larger than an Ethernet+IP+TCP header is ever held in RAM:
 * headers are read into `pkt` (and `th` for TCP), rewritten in place into
 * the reply, and payloads are copied ENC-RX -> ENC-TX through `chunk`.
 * Rungs are cumulative (netcfg.h): 1 ARP, 2 ICMP, 3 UDP, 4 DHCP,
 * 5 MTU 1500, 6 full DHCP, 7 ARP client + DNS, 8 TCP echo.
 */
#include <mcs51/8052.h>
#include <stdint.h>
#include "enc.h"
#include "netcfg.h"
#include "net.h"

static __code const uint8_t mac[6] = {NET_MAC};

__xdata uint8_t our_ip[4];
static __xdata uint8_t pkt[42];
static __xdata uint8_t chunk[NET_CHUNK];

/* Frame offsets */
#define ETH_DST 0
#define ETH_SRC 6
#define ETH_TYPE 12
#define ARP_OP 20
#define ARP_SHA 22
#define ARP_SPA 28
#define ARP_THA 32
#define ARP_TPA 38
#define IP_VHL 14
#define IP_LEN 16
#define IP_FRAG 20
#define IP_TTL 22
#define IP_PROTO 23
#define IP_SUM 24
#define IP_SRC 26
#define IP_DST 30
#define L4 34                       /* ICMP / UDP / TCP header */
#define PAYLOAD 42

static void copy(uint8_t __xdata *d, const uint8_t __xdata *s, uint8_t n)
{
  while (n--)
    *d++ = *s++;
}

static void copy_mac(uint8_t __xdata *d)
{
  uint8_t i;
  for (i = 0; i < 6; i++)
    d[i] = mac[i];
}

static uint8_t eq(const uint8_t __xdata *a, const uint8_t __xdata *b, uint8_t n)
{
  while (n--)
    if (*a++ != *b++)
      return 0;
  return 1;
}

static uint16_t be16(const uint8_t __xdata *p)
{
  return ((uint16_t)p[0] << 8) | p[1];
}

static void reply_eth(void)
{
  copy(pkt + ETH_DST, pkt + ETH_SRC, 6);
  copy_mac(pkt + ETH_SRC);
}

#if NET_RUNG >= 2
static void put16(uint8_t __xdata *p, uint16_t v)
{
  p[0] = (uint8_t)(v >> 8);
  p[1] = (uint8_t)v;
}

static uint16_t csum_add(uint16_t s, uint16_t w)
{
  s += w;
  if (s < w)
    s++;
  return s;
}

/* One's-complement sum of n bytes; an odd last byte is padded with zero.
 * Consecutive calls combine correctly only if every call but the last has
 * an even n -- hence NET_CHUNK is even.
 */
static uint16_t sum16(const uint8_t __xdata *p, uint8_t n, uint16_t s)
{
  for (; n > 1; n -= 2, p += 2)
    s = csum_add(s, be16(p));
  if (n)
    s = csum_add(s, (uint16_t)*p << 8);
  return s;
}

static void ip_checksum(void)
{
  pkt[IP_SUM] = 0;
  pkt[IP_SUM + 1] = 0;
  put16(pkt + IP_SUM, ~sum16(pkt + IP_VHL, 20, 0));
}

/* Swap IP addresses, fresh TTL, new header checksum. */
static void reply_ip(void)
{
  uint8_t i, t;
  reply_eth();
  for (i = 0; i < 4; i++) {
    t = pkt[IP_SRC + i];
    pkt[IP_SRC + i] = pkt[IP_DST + i];
    pkt[IP_DST + i] = t;
  }
  pkt[IP_TTL] = 64;
  ip_checksum();
}

/* Send pkt[0..41] followed by n payload bytes copied from the current RX frame. */
static void send_with_payload(uint16_t n)
{
  uint8_t c;
  uint16_t total = PAYLOAD + n;
  enc_tx_begin();
  enc_tx_write(pkt, PAYLOAD);
  while (n) {
    c = n > sizeof(chunk) ? sizeof(chunk) : (uint8_t)n;
    enc_rx_read(chunk, c);
    enc_tx_write(chunk, c);
    n -= c;
  }
  enc_tx_send(total);
}
#endif /* NET_RUNG >= 2 */

#if NET_RUNG >= 6
static uint32_t be32(const uint8_t __xdata *p)
{
  return ((uint32_t)be16(p) << 16) | be16(p + 2);
}

/* Timer 0, 50 ms at 12 MHz / 12T: the clock for DHCP lease timers (rung 6),
 * ARP/DNS retries (rung 7) and the TCP retransmit timer (rung 8).
 */
volatile __data uint8_t net_ticks;
static __data uint8_t sub50;

void t0_isr(void) __interrupt(1)
{
  TH0 = (uint8_t)((65536UL - 50000UL) >> 8);
  TL0 = (uint8_t)(65536UL - 50000UL);
  net_ticks++;
}
#endif

#if NET_RUNG >= 7
static __xdata uint8_t th[60];      /* TCP header (rung 8) / DNS query (rung 7) */
static uint16_t pseudo(const uint8_t __xdata *src, const uint8_t __xdata *dst,
                       uint8_t proto, uint16_t len)
{
  uint16_t s = sum16(src, 4, 0);
  s = sum16(dst, 4, s);
  s = csum_add(s, proto);
  return csum_add(s, len);
}
#endif

/* ---- Rung 7: ARP client + DNS ---------------------------------------------- */
#if NET_RUNG >= 7
#define DNS_IDLE 0
#define DNS_ARP 1
#define DNS_QUERY 2
#define DNS_DONE 3
#define DNS_FAILED 4
#define DNS_ID 0xD5A1

__xdata uint8_t dns_server[4];
__xdata uint8_t dns_result[4];
__xdata uint8_t dns_state;
static __xdata uint8_t hop_ip[4];
static __xdata uint8_t hop_mac[6];
static __xdata uint8_t dns_tries;
static __xdata uint8_t dns_timer;
static __xdata uint16_t dleft;
static __code const char dns_name[] = NET_DNS_NAME;   /* + the root label 0 */
extern __xdata uint8_t net_mask[4];
extern __xdata uint8_t net_router[4];

static void arp_query(void)
{
  uint8_t i;
  for (i = 0; i < 6; i++) {
    pkt[ETH_DST + i] = 0xFF;
    pkt[ARP_THA + i] = 0;
  }
  copy_mac(pkt + ETH_SRC);
  pkt[ETH_TYPE] = 0x08;
  pkt[ETH_TYPE + 1] = 0x06;
  put16(pkt + 14, 1);
  put16(pkt + 16, 0x0800);
  pkt[18] = 6;
  pkt[19] = 4;
  put16(pkt + ARP_OP, 1);
  copy_mac(pkt + ARP_SHA);
  copy(pkt + ARP_SPA, our_ip, 4);
  copy(pkt + ARP_TPA, hop_ip, 4);
  enc_tx_begin();
  enc_tx_write(pkt, 42);
  enc_tx_send(42);
  dns_timer = 0;
}

static void dns_start(void)
{
  uint8_t i, onlink = 1;
  for (i = 0; i < 4; i++)
    if ((dns_server[i] ^ our_ip[i]) & net_mask[i])
      onlink = 0;
  copy(hop_ip, onlink ? dns_server : net_router, 4);
  dns_state = DNS_ARP;
  dns_tries = 1;
  arp_query();
}

static void dns_query(void)
{
  uint8_t i, n = 0;
  uint16_t s;

  /* DNS message, built in th[] (not in use outside TCP processing) */
  put16(th, DNS_ID);
  th[2] = 0x01;                      /* RD */
  th[3] = 0;
  put16(th + 4, 1);                  /* QDCOUNT */
  for (i = 6; i < 12; i++)
    th[i] = 0;
  n = 12;
  for (i = 0; i < sizeof(dns_name); i++)
    th[n++] = dns_name[i];
  put16(th + n, 1);                  /* QTYPE A */
  put16(th + n + 2, 1);              /* QCLASS IN */
  n += 4;

  copy(pkt + ETH_DST, hop_mac, 6);
  copy_mac(pkt + ETH_SRC);
  pkt[ETH_TYPE] = 0x08;
  pkt[ETH_TYPE + 1] = 0x00;
  pkt[IP_VHL] = 0x45;
  pkt[IP_VHL + 1] = 0;
  put16(pkt + IP_LEN, 28 + n);
  put16(pkt + IP_LEN + 2, 0);
  put16(pkt + IP_FRAG, 0);
  pkt[IP_TTL] = 64;
  pkt[IP_PROTO] = 17;
  copy(pkt + IP_SRC, our_ip, 4);
  copy(pkt + IP_DST, dns_server, 4);
  ip_checksum();
  put16(pkt + L4, NET_DNS_PORT);
  put16(pkt + L4 + 2, 53);
  put16(pkt + L4 + 4, 8 + n);
  put16(pkt + L4 + 6, 0);
  s = pseudo(pkt + IP_SRC, pkt + IP_DST, 17, 8 + n);
  s = sum16(pkt + L4, 8, s);
  s = ~sum16(th, n, s);
  put16(pkt + L4 + 6, s ? s : 0xFFFF);
  enc_tx_begin();
  enc_tx_write(pkt, PAYLOAD);
  enc_tx_write(th, n);
  enc_tx_send(PAYLOAD + n);
  dns_timer = 0;
}

/* Discard n bytes of the current RX frame. */
static void rx_skip(uint16_t n)
{
  uint8_t c;
  while (n) {
    c = n > sizeof(chunk) ? sizeof(chunk) : (uint8_t)n;
    enc_rx_read(chunk, c);
    n -= c;
  }
}

static uint8_t dns_take(uint8_t n)
{
  if (dleft < n)
    return 0;
  enc_rx_read(chunk, n);
  dleft -= n;
  return 1;
}

/* Skip a (possibly compressed) name. Returns 0 on a malformed message. */
static uint8_t dns_skip_name(void)
{
  uint8_t c;
  for (;;) {
    if (!dns_take(1))
      return 0;
    c = chunk[0];
    if (c == 0)
      return 1;
    if ((c & 0xC0) == 0xC0)          /* compression pointer: 2 bytes, ends the name */
      return dns_take(1);
    if (c > 63 || dleft < c)
      return 0;
    rx_skip(c);
    dleft -= c;
  }
}

static void dns_input(uint16_t left)
{
  uint8_t qd, an;
  uint16_t rdlen;

  dleft = left;
  if (!dns_take(12) || be16(chunk) != DNS_ID || !(chunk[2] & 0x80))
    return;                                  /* not our answer */
  if ((chunk[3] & 0x0F) || chunk[4] || chunk[6]) {
    dns_state = DNS_FAILED;                  /* NXDOMAIN etc., or absurd counts */
    return;
  }
  qd = chunk[5];
  an = chunk[7];
  while (qd--)
    if (!dns_skip_name() || !dns_take(4))
      return;
  while (an--) {
    if (!dns_skip_name() || !dns_take(10))
      return;
    rdlen = be16(chunk + 8);
    if (be16(chunk) == 1 && be16(chunk + 2) == 1 && rdlen == 4) {
      if (!dns_take(4))
        return;
      copy(dns_result, chunk, 4);
      dns_state = DNS_DONE;
      return;
    }
    if (dleft < rdlen)                       /* CNAME etc.: skip its data */
      return;
    rx_skip(rdlen);
    dleft -= rdlen;
  }
  dns_state = DNS_FAILED;                    /* answered, but no A record */
}

static void dns_second(void)
{
  if (dns_state == DNS_ARP && ++dns_timer >= 1) {
    if (++dns_tries > NET_ARP_TRIES)
      dns_state = DNS_FAILED;
    else
      arp_query();
  } else if (dns_state == DNS_QUERY && ++dns_timer >= NET_DNS_RETRY_S) {
    if (++dns_tries > NET_DNS_TRIES)
      dns_state = DNS_FAILED;
    else
      dns_query();
  }
}

/* UDP to NET_STATUS_PORT: reply with the resolved address + dns_state. */
static void status_reply(void)
{
  uint16_t s;
  put16(pkt + IP_LEN, 28 + 5);
  reply_ip();
  s = be16(pkt + L4);
  put16(pkt + L4, be16(pkt + L4 + 2));
  put16(pkt + L4 + 2, s);
  put16(pkt + L4 + 4, 8 + 5);
  put16(pkt + L4 + 6, 0);
  copy(chunk, dns_result, 4);
  chunk[4] = dns_state;
  s = pseudo(pkt + IP_SRC, pkt + IP_DST, 17, 8 + 5);
  s = sum16(pkt + L4, 8, s);
  s = ~sum16(chunk, 5, s);
  put16(pkt + L4 + 6, s ? s : 0xFFFF);
  enc_tx_begin();
  enc_tx_write(pkt, PAYLOAD);
  enc_tx_write(chunk, 5);
  enc_tx_send(PAYLOAD + 5);
}
#endif /* NET_RUNG >= 7 */

/* ---- Rung 1: ARP ---------------------------------------------------------- */
static void handle_arp(void)
{
  /* Ethernet/IPv4 only */
  if (be16(pkt + 14) != 1 || be16(pkt + 16) != 0x0800)
    return;
#if NET_RUNG >= 7
  if (be16(pkt + ARP_OP) == 2) {             /* reply to our query? */
    if (dns_state == DNS_ARP && eq(pkt + ARP_SPA, hop_ip, 4)) {
      copy(hop_mac, pkt + ARP_SHA, 6);
      dns_state = DNS_QUERY;
      dns_tries = 1;
      dns_query();
    }
    return;
  }
#endif
  /* request, for our (non-zero) address */
  if (be16(pkt + ARP_OP) != 1)
    return;
  if (!our_ip[0] || !eq(pkt + ARP_TPA, our_ip, 4))
    return;
  reply_eth();
  pkt[ARP_OP + 1] = 2;
  copy(pkt + ARP_THA, pkt + ARP_SHA, 10);   /* tha,tpa <- sha,spa */
  copy_mac(pkt + ARP_SHA);
  copy(pkt + ARP_SPA, our_ip, 4);
  enc_tx_begin();
  enc_tx_write(pkt, 42);
  enc_tx_send(42);                           /* the MAC pads to 60 */
}

#if NET_RUNG >= 4
/* ---- Rung 4: DHCP client (rung 6: lease timer, renew, rebind) ------------- */
#define DHCP_INIT 0
#define DHCP_SELECTING 1
#define DHCP_REQUESTING 2
#define DHCP_BOUND 3
#define DHCP_RENEWING 4
#define DHCP_REBINDING 5

__xdata uint8_t dhcp_state;
__xdata uint8_t dhcp_server[4];
__xdata uint8_t dhcp_offer[4];
__xdata uint8_t net_mask[4];
__xdata uint8_t net_router[4];
static __code const uint8_t xid[4] = {0x5A, 0xD1, 0x00, 0x01};
#define BOOTP_LEN 300                        /* RFC 1542 minimum message */
#if NET_RUNG >= 6
#define LEASE_INFINITE 0xFFFFFFFFUL
static __xdata uint8_t retry;                /* seconds */
static __xdata uint32_t lease, t1, t2;       /* seconds left */
static __xdata uint32_t opt_lease, opt_t1, opt_t2;
static __xdata uint8_t srv_mac[6];
#else
static __xdata uint16_t retry;               /* main-loop polls */
#endif

static void dhcp_send(uint8_t type)
{
  uint8_t i, n;
#if NET_RUNG >= 6
  /* RENEWING/REBINDING: we have an address (ciaddr); RENEWING is unicast */
  uint8_t cia = dhcp_state == DHCP_RENEWING || dhcp_state == DHCP_REBINDING;
  uint8_t renew = dhcp_state == DHCP_RENEWING;
#endif

  for (i = 0; i < 6; i++)
    pkt[ETH_DST + i] = 0xFF;
#if NET_RUNG >= 6
  if (renew)                                 /* RENEWING: unicast to the server */
    copy(pkt + ETH_DST, srv_mac, 6);
#endif
  copy_mac(pkt + ETH_SRC);
  pkt[ETH_TYPE] = 0x08;
  pkt[ETH_TYPE + 1] = 0x00;
  pkt[IP_VHL] = 0x45;
  pkt[IP_VHL + 1] = 0;
  put16(pkt + IP_LEN, 20 + 8 + BOOTP_LEN);
  put16(pkt + IP_LEN + 2, 0);                /* id */
  put16(pkt + IP_FRAG, 0);
  pkt[IP_TTL] = 64;
  pkt[IP_PROTO] = 17;
  for (i = 0; i < 4; i++) {
    pkt[IP_SRC + i] = 0;
    pkt[IP_DST + i] = 0xFF;
  }
#if NET_RUNG >= 6
  if (cia)
    copy(pkt + IP_SRC, our_ip, 4);
  if (renew)
    copy(pkt + IP_DST, dhcp_server, 4);
#endif
  ip_checksum();
  put16(pkt + L4, 68);
  put16(pkt + L4 + 2, 67);
  put16(pkt + L4 + 4, 8 + BOOTP_LEN);
  put16(pkt + L4 + 6, 0);                    /* UDP checksum optional on IPv4 */

  enc_tx_begin();
  enc_tx_write(pkt, PAYLOAD);
  /* BOOTP: op htype hlen hops, xid, secs, flags (broadcast reply unless we
   * already have an address to be answered at)
   */
  chunk[0] = 1; chunk[1] = 1; chunk[2] = 6; chunk[3] = 0;
  for (i = 0; i < 4; i++)
    chunk[4 + i] = xid[i];
  chunk[8] = 0; chunk[9] = 0; chunk[10] = 0x80; chunk[11] = 0;
#if NET_RUNG >= 6
  if (cia)
    chunk[10] = 0;
#endif
  enc_tx_write(chunk, 12);
#if NET_RUNG >= 6
  if (cia)
    enc_tx_write(our_ip, 4);                 /* ciaddr */
  else
#endif
    enc_tx_fill(0, 4);
  enc_tx_fill(0, 12);                        /* yiaddr siaddr giaddr */
  copy_mac(chunk);
  enc_tx_write(chunk, 6);
  enc_tx_fill(0, 10 + 64 + 128);             /* chaddr pad, sname, file */
  /* magic cookie + options */
  chunk[0] = 99; chunk[1] = 130; chunk[2] = 83; chunk[3] = 99;
  chunk[4] = 53; chunk[5] = 1; chunk[6] = type;
  n = 7;
#if NET_RUNG >= 6
  if (type == 3 && !cia) {                   /* SELECTING -> REQUESTING only */
#else
  if (type == 3) {
#endif
    chunk[n++] = 50; chunk[n++] = 4;
    copy(chunk + n, dhcp_offer, 4); n += 4;
    chunk[n++] = 54; chunk[n++] = 4;
    copy(chunk + n, dhcp_server, 4); n += 4;
  }
#if NET_RUNG >= 7
  chunk[n++] = 55; chunk[n++] = 3; chunk[n++] = 1; chunk[n++] = 3; chunk[n++] = 6;
#else
  chunk[n++] = 55; chunk[n++] = 2; chunk[n++] = 1; chunk[n++] = 3; /* ask mask, router */
#endif
  chunk[n++] = 255;
  enc_tx_write(chunk, n);
  enc_tx_fill(0, BOOTP_LEN - 236 - n);       /* pad to 300 */
  enc_tx_send(PAYLOAD + BOOTP_LEN);
  retry = 0;
}

/* Parse a BOOTP reply streamed from the RX buffer. `left` = UDP payload bytes. */
static void dhcp_input(uint16_t left)
{
  uint8_t code, len, type = 0, i;
  uint8_t ok_xid, ok_mac;

  if (left < 240)
    return;
  enc_rx_read(chunk, 20);                    /* op .. yiaddr */
  if (chunk[0] != 2)
    return;
  ok_xid = 1;
  for (i = 0; i < 4; i++)
    if (chunk[4 + i] != xid[i])
      ok_xid = 0;
  copy(dhcp_offer, chunk + 16, 4);
  enc_rx_read(chunk, 14);                    /* siaddr giaddr chaddr[0..5] */
  ok_mac = 1;
  for (i = 0; i < 6; i++)
    if (chunk[8 + i] != mac[i])
      ok_mac = 0;
  if (!ok_xid || !ok_mac)
    return;
  for (i = 0; i < 7; i++)                    /* chaddr pad, sname, file: 202 B */
    enc_rx_read(chunk, i < 6 ? 32 : 10);
  enc_rx_read(chunk, 4);
  if (chunk[0] != 99 || chunk[1] != 130 || chunk[2] != 83 || chunk[3] != 99)
    return;
  left -= 240;
#if NET_RUNG >= 6
  opt_lease = opt_t1 = opt_t2 = 0;
#endif

  while (left) {
    enc_rx_read(chunk, 1);
    code = chunk[0];
    left--;
    if (code == 255)
      break;
    if (code == 0 || !left)
      continue;
    enc_rx_read(chunk, 1);
    len = chunk[0];
    left--;
    if (len > left)
      return;
    left -= len;
    if (len > 32) {                          /* long option we don't use: skip */
      while (len) {
        i = len > 32 ? 32 : len;
        enc_rx_read(chunk, i);
        len -= i;
      }
      continue;
    }
    enc_rx_read(chunk, len);
    if (code == 53 && len == 1)
      type = chunk[0];
    else if (code == 54 && len == 4)
      copy(dhcp_server, chunk, 4);
    else if (code == 1 && len == 4)
      copy(net_mask, chunk, 4);
    else if (code == 3 && len >= 4)
      copy(net_router, chunk, 4);
#if NET_RUNG >= 6
    else if (code == 51 && len == 4)
      opt_lease = be32(chunk);
    else if (code == 58 && len == 4)
      opt_t1 = be32(chunk);
    else if (code == 59 && len == 4)
      opt_t2 = be32(chunk);
#endif
#if NET_RUNG >= 7
    else if (code == 6 && len >= 4)
      copy(dns_server, chunk, 4);
#endif
  }

  if (dhcp_state == DHCP_SELECTING && type == 2) {          /* OFFER */
    dhcp_state = DHCP_REQUESTING;
    dhcp_send(3);                                           /* REQUEST */
    return;
  }
#if NET_RUNG >= 6
  if (dhcp_state < DHCP_REQUESTING)
    return;
  if (type == 5) {                                          /* ACK */
    copy(our_ip, dhcp_offer, 4);
    copy(srv_mac, pkt + ETH_SRC, 6);                        /* for unicast renew */
    lease = opt_lease ? opt_lease : LEASE_INFINITE;
    t1 = opt_t1 ? opt_t1 : lease / 2;
    t2 = opt_t2 ? opt_t2 : lease - lease / 8;               /* 0.875 * lease */
    dhcp_state = DHCP_BOUND;
  } else if (type == 6) {                                   /* NAK: start over */
    our_ip[0] = our_ip[1] = our_ip[2] = our_ip[3] = 0;
    dhcp_state = DHCP_INIT;
  }
#else
  if (dhcp_state == DHCP_REQUESTING && type == 5) {         /* ACK */
    copy(our_ip, dhcp_offer, 4);
    dhcp_state = DHCP_BOUND;
  } else if (dhcp_state == DHCP_REQUESTING && type == 6) {  /* NAK */
    dhcp_state = DHCP_INIT;
  }
#endif
}

#if NET_RUNG >= 6
static void dhcp_second(void)
{
  if (dhcp_state >= DHCP_BOUND && lease != LEASE_INFINITE) {
    if (!--lease) {                          /* expired: drop the address */
      our_ip[0] = our_ip[1] = our_ip[2] = our_ip[3] = 0;
      dhcp_state = DHCP_INIT;
#if NET_RUNG >= 7
      dns_state = DNS_IDLE;
#endif
      return;
    }
    if (t1)
      t1--;
    if (t2)
      t2--;
    if (!t2 && dhcp_state != DHCP_REBINDING) {
      dhcp_state = DHCP_REBINDING;           /* broadcast REQUEST, ciaddr set */
      dhcp_send(3);
      return;
    }
    if (!t1 && dhcp_state == DHCP_BOUND) {
      dhcp_state = DHCP_RENEWING;            /* unicast REQUEST, ciaddr set */
      dhcp_send(3);
      return;
    }
  }
  if (dhcp_state != DHCP_BOUND && dhcp_state != DHCP_INIT &&
      ++retry >= NET_DHCP_RETRY_S)
    dhcp_send(dhcp_state == DHCP_SELECTING ? 1 : 3);
}
#endif
#endif /* NET_RUNG >= 4 */

/* ---- Rung 8: TCP echo, one connection ------------------------------------- */
#if NET_RUNG >= 8
#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10

#define TCP_LISTEN 0
#define TCP_SYN_RCVD 1
#define TCP_ESTABLISHED 2
#define TCP_LAST_ACK 3

__xdata uint8_t tcp_state;
static __xdata uint8_t peer_ip[4], peer_mac[6];
static __xdata uint16_t peer_port;
static __xdata uint8_t r_ip[4], r_mac[6];    /* where the segment being sent goes */
static __xdata uint16_t r_port;
static __xdata uint16_t r_lport;             /* ...and our port it comes from: an RST
              * must come from the port the refused
              * segment was sent to, not always 7
              */
static __xdata uint32_t rcv_nxt, snd_una, snd_nxt, isn_clock;
static __xdata uint16_t slot_len;            /* frame length in the retransmit slot */
static __xdata uint8_t rto, tries, slot_lost;
/* SDCC's small model keeps the locals and parameters of any non-leaf
 * function in the 128 B directly addressable data space, and the 32-bit
 * sequence arithmetic overflowed it ("Could not get 32 consecutive bytes in
 * internal RAM for area DSEG") -- on the RD+ too, whose extra 1 KB is XRAM,
 * not data space. So the 32-bit values live in XRAM and headers take their
 * seq/ack from o_seq/o_ack instead of parameters.
 */
static __xdata uint32_t seg_seq, seg_ack, o_seq, o_ack;
static __xdata uint16_t t_plen, t_n, t_psum;

static void put32(uint8_t __xdata *p, uint32_t v)
{
  put16(p, (uint16_t)(v >> 16));
  put16(p + 2, (uint16_t)v);
}

/* Build Ethernet+IP into pkt[0..33] and TCP into th[0..hl-1] for r_*. */
static void tcp_hdr(uint8_t flags, uint8_t hl, uint16_t plen)
{
  copy(pkt + ETH_DST, r_mac, 6);
  copy_mac(pkt + ETH_SRC);
  pkt[ETH_TYPE] = 0x08;
  pkt[ETH_TYPE + 1] = 0x00;
  pkt[IP_VHL] = 0x45;
  pkt[IP_VHL + 1] = 0;
  put16(pkt + IP_LEN, 20 + hl + plen);
  put16(pkt + IP_LEN + 2, 0);
  put16(pkt + IP_FRAG, 0);
  pkt[IP_TTL] = 64;
  pkt[IP_PROTO] = 6;
  copy(pkt + IP_SRC, our_ip, 4);
  copy(pkt + IP_DST, r_ip, 4);
  ip_checksum();
  put16(th, r_lport);
  put16(th + 2, r_port);
  put32(th + 4, o_seq);
  put32(th + 8, o_ack);
  th[12] = (hl / 4) << 4;
  th[13] = flags;
  put16(th + 14, NET_MTU - 40);              /* window: one MSS */
  put16(th + 16, 0);
  put16(th + 18, 0);
  if (hl == 24) {                            /* SYN|ACK carries our MSS */
    th[20] = 2;
    th[21] = 4;
    put16(th + 22, NET_MTU - 40);
  }
}

static uint16_t tcp_sum(uint8_t hl, uint16_t plen, uint16_t psum)
{
  uint16_t s = pseudo(pkt + IP_SRC, pkt + IP_DST, 6, hl + plen);
  s = sum16(th, hl, s);
  return ~csum_add(s, psum);
}

/* Header-only segment from the general TX region (ACK, RST). */
static void tcp_send(uint8_t flags)
{
  tcp_hdr(flags, 20, 0);
  put16(th + 16, tcp_sum(20, 0, 0));
  enc_tx_begin();
  enc_tx_write(pkt, 34);
  enc_tx_write(th, 20);
  enc_tx_send(54);
}

/* Header-only segment into the retransmit slot (SYN|ACK, FIN|ACK). */
static void tcp_send_slot(uint8_t flags, uint8_t hl)
{
  o_seq = snd_una;
  o_ack = rcv_nxt;
  tcp_hdr(flags, hl, 0);
  put16(th + 16, tcp_sum(hl, 0, 0));
  enc_tx_begin_at(ENC_TCPSTART);
  enc_tx_write(pkt, 34);
  enc_tx_write(th, hl);
  slot_len = 34 + hl;
  enc_tx_send_at(ENC_TCPSTART, slot_len);
  rto = tries = slot_lost = 0;
}

static void tcp_to_peer(void)
{
  copy(r_ip, peer_ip, 4);
  copy(r_mac, peer_mac, 6);
  r_port = peer_port;
  r_lport = NET_TCP_PORT;
}

static void tcp_abort(void)
{
  tcp_to_peer();
  o_seq = snd_nxt;
  o_ack = rcv_nxt;
  tcp_send(TCP_RST | TCP_ACK);
  tcp_state = TCP_LISTEN;
  snd_una = snd_nxt;
}

/* One 50 ms tick: the retransmit timer. */
static void tcp_tick(void)
{
  isn_clock += 25000;
  if (tcp_state == TCP_LISTEN || snd_una == snd_nxt)
    return;
  if (++rto < NET_TCP_RTO_TICKS)
    return;
  rto = 0;
  if (slot_lost || ++tries > NET_TCP_RETRIES)
    tcp_abort();                             /* give up: RST, back to LISTEN */
  else
    enc_tx_resend(ENC_TCPSTART, slot_len);
}

static void handle_tcp(uint16_t tcp_len)
{
  uint8_t hl, flags, c, echo, match;
  t_psum = 0;

  if (tcp_len < 20)
    return;
  copy(th, pkt + L4, 8);
  enc_rx_read(th + 8, 12);
  hl = (th[12] >> 4) * 4;
  if (hl < 20 || hl > tcp_len)
    return;
  if (hl > 20)
    enc_rx_read(th + 20, hl - 20);
  t_plen = tcp_len - hl;
  flags = th[13];
  seg_seq = be32(th + 4);
  seg_ack = be32(th + 8);
  copy(r_ip, pkt + IP_SRC, 4);
  copy(r_mac, pkt + ETH_SRC, 6);
  r_port = be16(th);
  r_lport = be16(th + 2);
  match = tcp_state != TCP_LISTEN && eq(r_ip, peer_ip, 4) && r_port == peer_port;

  /* Echo only if, once this segment's ACK is applied, the slot is free:
   * then the payload can go straight into the slot while it is summed.
   */
  echo = t_plen && match && be16(th + 2) == NET_TCP_PORT &&
         (tcp_state == TCP_SYN_RCVD || tcp_state == TCP_ESTABLISHED) &&
         (flags & (TCP_ACK | TCP_SYN | TCP_RST)) == TCP_ACK &&
         seg_seq == rcv_nxt && seg_ack == snd_nxt;
  if (echo) {
    if (snd_una != snd_nxt)
      slot_lost = 1;                         /* overwriting unacked data; cleared
                          * below if the checksum holds
                          */
    enc_tx_begin_at(ENC_TCPSTART);
    enc_tx_fill(0, 54);                      /* header, patched in afterwards */
  }
  for (t_n = t_plen; t_n; t_n -= c) {
    c = t_n > sizeof(chunk) ? sizeof(chunk) : (uint8_t)t_n;
    enc_rx_read(chunk, c);
    t_psum = sum16(chunk, c, t_psum);
    if (echo)
      enc_tx_write(chunk, c);
  }
  /* Verify: pseudo-header + header + payload must sum to 0xFFFF. */
  t_n = pseudo(pkt + IP_SRC, pkt + IP_DST, 6, tcp_len);
  t_n = sum16(th, hl, t_n);
  if (csum_add(t_n, t_psum) != 0xFFFF)
    return;                                  /* corrupt: as if never received */

  if (flags & TCP_RST) {
    if (match)
      tcp_state = TCP_LISTEN;
    return;
  }
  if (be16(th + 2) != NET_TCP_PORT || (tcp_state != TCP_LISTEN && !match)) {
    /* closed port, or a second peer while busy: refuse (RFC 793 3.4) */
    if (flags & TCP_ACK) {
      o_seq = seg_ack;
      o_ack = 0;
      tcp_send(TCP_RST);
    } else {
      o_seq = 0;
      o_ack = seg_seq + t_plen + ((flags & TCP_SYN) ? 1 : 0) + ((flags & TCP_FIN) ? 1 : 0);
      tcp_send(TCP_RST | TCP_ACK);
    }
    return;
  }

  if (tcp_state == TCP_LISTEN) {
    if ((flags & (TCP_SYN | TCP_ACK)) == TCP_SYN) {
      copy(peer_ip, r_ip, 4);
      copy(peer_mac, r_mac, 6);
      peer_port = r_port;
      rcv_nxt = seg_seq + 1;
      snd_una = isn_clock;
      snd_nxt = snd_una + 1;
      tcp_state = TCP_SYN_RCVD;
      tcp_send_slot(TCP_SYN | TCP_ACK, 24);
    } else if (flags & TCP_ACK) {
      o_seq = seg_ack;
      o_ack = 0;
      tcp_send(TCP_RST);
    }
    return;
  }

  if (flags & TCP_SYN) {                     /* our SYN|ACK was lost: resend it */
    if (tcp_state == TCP_SYN_RCVD && seg_seq + 1 == rcv_nxt)
      enc_tx_resend(ENC_TCPSTART, slot_len);
    return;
  }
  if (!(flags & TCP_ACK))
    return;
  if (seg_ack == snd_nxt && snd_una != snd_nxt) {
    snd_una = snd_nxt;                       /* everything we sent is acked */
    if (tcp_state == TCP_SYN_RCVD)
      tcp_state = TCP_ESTABLISHED;
    else if (tcp_state == TCP_LAST_ACK) {
      tcp_state = TCP_LISTEN;
      return;
    }
  }
  if (!t_plen && !(flags & TCP_FIN))
    return;                                  /* pure ACK */
  if (seg_seq != rcv_nxt) {                  /* duplicate / out of order */
    o_seq = snd_nxt;
    o_ack = rcv_nxt;
    tcp_send(TCP_ACK);
    return;
  }
  if (tcp_state != TCP_ESTABLISHED || snd_una != snd_nxt)
    return;                                  /* slot busy: peer will retransmit */

  if (!echo && t_plen)
    return;                                  /* data we did not stage (e.g. a stale
                                   * ACK): drop, the peer retransmits
                                   */
  rcv_nxt += t_plen;
  c = TCP_ACK;
  if (t_plen)
    c |= TCP_PSH;
  if (flags & TCP_FIN) {
    rcv_nxt++;
    c |= TCP_FIN;
    tcp_state = TCP_LAST_ACK;
  }
  if (t_plen) {
    o_seq = snd_nxt;
    o_ack = rcv_nxt;
    tcp_hdr(c, 20, t_plen);
    put16(th + 16, tcp_sum(20, t_plen, t_psum));
    enc_tx_patch(ENC_TCPSTART, 0, pkt, 34);
    enc_tx_patch(ENC_TCPSTART, 34, th, 20);
    slot_len = 54 + t_plen;
    enc_tx_send_at(ENC_TCPSTART, slot_len);
    rto = tries = slot_lost = 0;
  } else {
    tcp_send_slot(c, 20);                    /* FIN|ACK, no data */
  }
  snd_nxt += t_plen + ((c & TCP_FIN) ? 1 : 0);
}

#endif /* NET_RUNG >= 8 */

/* ---- Rungs 2+: IPv4 ---------------------------------------------------------- */
#if NET_RUNG >= 2
static void handle_ip(uint16_t frame_len)
{
  uint16_t len;
  uint16_t s;

  if (pkt[IP_VHL] != 0x45)                   /* no IP options */
    return;
  if (be16(pkt + IP_FRAG) & 0x3FFF)          /* no fragments */
    return;
  if (sum16(pkt + IP_VHL, 20, 0) != 0xFFFF)  /* header checksum */
    return;
  len = be16(pkt + IP_LEN);
  if (len < 28 || len > NET_MTU || len + 14 > frame_len)
    return;

#if NET_RUNG >= 4
  if (pkt[IP_PROTO] == 17 && be16(pkt + L4 + 2) == 68 && dhcp_state != DHCP_BOUND) {
    dhcp_input(len - 28);
    return;
  }
#endif
  if (!our_ip[0] || !eq(pkt + IP_DST, our_ip, 4))
    return;

  if (pkt[IP_PROTO] == 1 && pkt[L4] == 8 && pkt[L4 + 1] == 0) {
    /* ICMP echo -> echo reply. Only the type changes (8 -> 0), so the
     * checksum is updated incrementally (RFC 1624) instead of re-summing a
     * payload that never enters RAM.
     */
    reply_ip();
    pkt[L4] = 0;
    s = be16(pkt + L4 + 2);
    s += 0x0800;
    if (s < 0x0800)
      s++;
    put16(pkt + L4 + 2, s);
    send_with_payload(len - 28);
  }
#if NET_RUNG >= 3
  else if (pkt[IP_PROTO] == 17 && be16(pkt + L4 + 2) == NET_UDP_ECHO_PORT) {
    /* UDP echo. Swapping addresses and ports leaves the pseudo-header sum
     * unchanged, so the request's UDP checksum is also correct for the reply
     * (and 0 = "no checksum" stays 0).
     */
    reply_ip();
    s = be16(pkt + L4);
    put16(pkt + L4, be16(pkt + L4 + 2));
    put16(pkt + L4 + 2, s);
    send_with_payload(len - 28);
  }
#endif
#if NET_RUNG >= 7
  else if (pkt[IP_PROTO] == 17 && be16(pkt + L4 + 2) == NET_DNS_PORT &&
           be16(pkt + L4) == 53 && dns_state == DNS_QUERY &&
           eq(pkt + IP_SRC, dns_server, 4)) {
    dns_input(len - 28);
  } else if (pkt[IP_PROTO] == 17 && be16(pkt + L4 + 2) == NET_STATUS_PORT) {
    status_reply();
  }
#endif
#if NET_RUNG >= 8
  else if (pkt[IP_PROTO] == 6) {
    handle_tcp(len - 20);
  }
#endif
}
#endif

void net_init(void)
{
#if NET_RUNG >= 4
  our_ip[0] = our_ip[1] = our_ip[2] = our_ip[3] = 0;
  dhcp_state = DHCP_INIT;
#else
  static __code const uint8_t ip[4] = {NET_STATIC_IP};
  uint8_t i;
  for (i = 0; i < 4; i++)
    our_ip[i] = ip[i];
#endif
  enc_init(mac);
#if NET_RUNG >= 6
  TMOD = (TMOD & 0xF0) | 0x01;               /* Timer 0, mode 1 */
  TH0 = (uint8_t)((65536UL - 50000UL) >> 8);
  TL0 = (uint8_t)(65536UL - 50000UL);
  ET0 = 1;
  EA = 1;
  TR0 = 1;
#endif
}

void net_poll(void)
{
  uint16_t len;

#if NET_RUNG >= 6
  while (net_ticks) {
    net_ticks--;
#if NET_RUNG >= 8
    tcp_tick();
#endif
    if (++sub50 >= 20) {
      sub50 = 0;
      dhcp_second();
#if NET_RUNG >= 7
      dns_second();
#endif
    }
  }
#endif
#if NET_RUNG >= 4
  if (dhcp_state == DHCP_INIT) {
    dhcp_state = DHCP_SELECTING;
    dhcp_send(1);                            /* DISCOVER */
  }
#if NET_RUNG < 6
  else if (dhcp_state != DHCP_BOUND && ++retry > NET_DHCP_RETRY_POLLS) {
    if (dhcp_state == DHCP_SELECTING)
      dhcp_send(1);
    else
      dhcp_send(3);
  }
#endif
#endif
#if NET_RUNG >= 7
  if (dns_state == DNS_IDLE && dhcp_state == DHCP_BOUND && dns_server[0])
    dns_start();
#endif

  len = enc_rx_begin();
  if (!len)
    return;
  if (len != 0xFFFF && len >= 42) {
    enc_rx_read(pkt, 42);
    if (pkt[ETH_TYPE] == 0x08 && pkt[ETH_TYPE + 1] == 0x06)
      handle_arp();
#if NET_RUNG >= 2
    else if (pkt[ETH_TYPE] == 0x08 && pkt[ETH_TYPE + 1] == 0x00)
      handle_ip(len);
#endif
  }
  enc_rx_done();
}
