# Ethernet : an ENC28J60 on an 8-bit part, and an IP stack that fits

Please see [my blog](https://reidemeister.com/blog/2025.11.29) for details.

Two directories come out of that post. `09_ethernet` is the bring-up demo: it
resets the controller, reads its revision, writes and reads back the MAC, and
reports over the serial console. `../10_net_stack` is the network stack that
sits on the same driver and answers a laptop.

They are separate because they answer separate questions. The first is *does
the bus work at all*, which is where this project spent most of its time. The
second is *how much of TCP/IP fits in 8 KB*, which turned out to be more than
expected.

```
09_ethernet/
  enc28j60.c/.h     the driver: banked registers, buffer, PHY
  enc28j60_cfg.h    SPI pins and MAC address, overridable with -D
  ethernet.c        the bring-up demo
  xprintf.c/.h      third-party tiny printf (Eugene Chaban, GPL-2.0+)
../10_net_stack/
  enc.c/.h          a leaner driver, with the link defences
  net.c/.h          ARP, IPv4, ICMP, UDP, DHCP, DNS, TCP
  netcfg.h          rung selection, MTU, MAC, ports
  main.c            net_init() then net_poll() forever
  hosttest/         a host-side ping and UDP echo checker
```

## Building

```shell
meson build
ninja -v -C ./build
ninja -v -C ./build flash_09_ethernet
```

The stack builds one image per rung per part, so
`build/net_stc89c52rc_rung3.hex` is ARP + ICMP + UDP on the fitted chip.
Flashing follows the same pattern: `flash_net_stc89c52rc_rung3`.

## The pins are the whole story

The first attempt put SPI on P0 and read `EREVID` as `0x0C` where the
datasheet says `0x06`. Byte-identical across three cold boots, which is the
shape of a wiring fault rather than noise: every register read came back
shifted left by one.

**On an 8051, P0 has no internal pull-up.** It is open-drain, and on the
HC6800-ES it is pulled up through a 10k pack into a bus shared with an
always-enabled 74HC245, the LED matrix rows and the LCD data lines. None of
that is hidden; it is on page one of the board schematic.

This is the wiring that works:

| signal              | 8051 pin | ENC28J60 pin |
|---------------------|----------|--------------|
| clock               | P1.7     | `SCK`        |
| host out, device in | P1.6     | `SI`         |
| host in, device out | P1.4     | `SO`         |
| chip select         | P3.3     | `CS`         |

Those four are the only pins on this board that are actively driven and carry
nothing else. All sixteen write-and-read-back patterns came back exact.

**The module needs a 3.3 V supply and its pins tolerate 5 V.** The ENC28J60
core is a 3.3 V part; feeding the module 5 V destroys it.

Both drivers take the pins from `-D` overrides, so the old P0 wiring can still
be selected for comparison rather than deleted.

## What fits

Each rung adds a protocol and keeps the ones below it. Sizes are SDCC's own
`.mem` figures from this tree, measured 2026-09-27.

```
rung  adds                          flash  XRAM   STC89C52RC
  1   ARP reply                      2582    84   fits
  2   IPv4, ICMP echo                3265    84   fits
  3   UDP echo                       3385    84   fits
  4   DHCP client                    4780   103   fits
  5   MTU 1500                       4780   103   fits
  6   lease, renew, rebind, expiry   5892   132   fits
  7   ARP client, DNS A records      7902   215   fits
  8   TCP echo, one connection      11027   507   does NOT fit
```

Rung 5 is the one worth staring at: **going from a 576-byte MTU to 1500 costs
no flash and no RAM at all**, because the MTU is a constant and a different
buffer layout inside the controller. Frames never enter the MCU's RAM. Headers
are read into a 42-byte scratch buffer, rewritten in place, and payloads stream
controller-RX to controller-TX through a small chunk, so the MTU is bounded by
the ENC's own 8 KB and by time, not by XRAM. The idea is `UIPEthernet`'s.

Rung 8 needs about 11 KB against the RC's 8 KB. That is not a near miss, so TCP
is built only for the pin-compatible **STC89C516RD+** with 61 KB, where it fits
at 11,027 B of flash and 507 B of XRAM. Every rung up to 7 fits the small part
with room left, which is why `meson.build` stops the RC at 7: asking for rung 8
there would fail the link and break the build.

### These numbers are 755 bytes above the ones in the blog post

The post's table was measured before the link defences existed. Every rung here
is **exactly 755 B of flash and 6 B of XRAM larger** -- the same constant on all
eight, because that code lives in `enc.c` and links into all of them. The shape
is unchanged and so are both conclusions that hang off it: rungs 1-7 fit the RC,
rung 8 does not, and rung 5 is free.

250 B of that is the defences and can be switched off; the rest cannot. See
[the link defences](#the-link-defences-and-when-you-need-them) below.

## The link defences, and when you need them

The board answered ping and UDP for minutes, then stopped replying and stayed
deaf until a reset. Twelve and then fourteen consecutive runs came back 0 of 5.

Three causes were found, and those fixes are **not** optional or behind a flag,
because none of them is about wiring quality:

- `ECON1.RXEN` was set once at init, and the `TXRST` pulse clears it. Every
  transmit therefore switched the receiver off.
- `tx_idle()` waited on `TXRTS` forever, and a half-duplex transmit error
  leaves that bit set. It is bounded now.
- `ETXST` was never set per frame, so a transmit could send the receive area.

Fixing those revealed the real problem. **The SPI link itself is electrically
marginal**, and that is a property of the jumper wires, not of the code. A
read-only `EREVID` canary, which must return `0x06`, misreads one to three
times per run once traffic flows. Corrupted headers come back carrying frame
payload (`0x4141`, two ASCII `A`s), or pointers that are odd, or pointers
outside the ring. Widening the clock made it worse rather than better, which is
what says this is lost synchronisation rather than edge speed.

So `enc.c` can be built not to trust what it reads. Four defences, all behind
`ENC_LINK_DEFENCES` in `netcfg.h`, which defaults to on:

**`spi_resync()`** raises `CS`, pulses it low with no clocks, and releases it.
A slave that missed or gained a single clock edge is stuck mid-opcode and stays
out of step for every byte after it; ending the transaction is the only
recovery available from the master side. Costs four SPI byte times, so it is
used liberally.

**`rd_stable()`** reads a register twice and believes only a value that
repeats. On disagreement it resyncs and tries once more. Registers are read far
less often than frame bytes, so the second read is affordable where doubling
the payload reads would not be.

**The header is read twice** and the two copies must agree. The packet header
is the one read whose corruption is unrecoverable, because it carries the
pointer to the next packet, and re-reading is free: `ERDPT` just gets rewound.

**An `RXEN` watchdog** re-asserts receive every 256 polls. On this link even
control writes get corrupted, and a lost `ECON1` write leaves the board deaf
while every status register still reads perfectly healthy -- no errors,
`CLKRDY` set, and nothing arriving.

Header validation and ring recovery sit outside the flag on purpose. A frame
that overruns the receive buffer is ordinary traffic rather than bad wiring,
and the sticky `RXERIF`/`BUFER` path has to be handled on any link.

### Which setting to build

**Keep them on for jumper wires.** That is what the photo in the blog post
shows and what every measurement here was taken with. Turn them off only for
wiring you have reason to trust -- a soldered board or a proper PCB -- where
the fault they answer does not exist.

```shell
ninja -C ./build                                 # defended, the default
sdcc ... -DENC_LINK_DEFENCES=0 ...               # bare, for trusted wiring
```

**Measured effect with them on: a permanent wedge becomes graceful
degradation.** The board no longer goes deaf. Under sustained traffic it
answers one to five of five pings and one to four of five UDP echoes. That is
an improvement and it is not a fix; the remaining fault is electrical.

### They are what puts these numbers above the blog post's

The defences cost **250 bytes of flash and 6 bytes of XRAM**, the same on every
rung, and that cost is included in every figure in the table above. **The blog
post's table was measured without them**, so the two do not line up and are not
meant to. Build with `-DENC_LINK_DEFENCES=0` and the XRAM matches the post
exactly; the flash is still 505 B higher, which is the non-optional work listed
at the top of this section.

## Checking it from a host

`hosttest/` is a dependency-free uv project that pings the board and round-trips
UDP payloads.

```shell
cd 10_net_stack/hosttest
uv run hosttest.py                      # exits 0 only if everything passed
uv run hosttest.py --ip 192.168.7.2 --count 20
uv run hosttest.py --no-ping            # UDP only
```

It shells out to the system `ping` because a raw ICMP socket needs
administrator rights on Windows, and it drains the UDP socket before each
exchange. Without that drain, a reply that missed its timeout is handed to the
*next* `recvfrom()` and every later exchange reports a mismatch -- which looks
exactly like a firmware bug and is not one.

**The first ping after a cold boot is usually lost** while ARP resolves. The
board answers the ARP request and replies from the second ping onward.

## Caveats worth knowing before trusting this

- On a direct cable there is no DHCP, so a host self-assigns a link-local
  address. Build with `-DNET_STATIC_IP=169,254,194,20` to land in the same /16.
- The TCP initial sequence number comes from a 50 ms clock, and the DNS query
  ID and source port are fixed. Fine on a bench, not on a hostile network.
- Rung 8 is one connection, one unacknowledged segment, a fixed window of one
  MSS, and no out-of-order buffering.
- Round trips are dominated by the bit-banged SPI, not the network: every byte
  crosses it twice at roughly 250 us each, which works out near 1.8 kB/s.
