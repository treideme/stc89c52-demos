# AT89S52 Example (avrdude)

This is an AT89S52 variant of the [STC89C52 demo series](../README.md), using the
same HC6800-ES development kit but with an **AT89S52** in the CPU socket instead
of the STC89C52. The AT89S52 has no UART bootloader, so it can't be flashed with
`stcgal` like the rest of this repo — instead it's programmed over its 4-wire SPI
ISP interface using [avrdude](https://github.com/avrdudes/avrdude), the same tool
used to flash AVR chips like the ATmega328.

You'll need an AVR-ISP-compatible programmer in addition to the dev-kit itself.
An Arduino running the
[ArduinoISP](https://docs.arduino.cc/built-in-examples/arduino-isp/ArduinoISP/)
sketch is the recommended option — see
[Programmer compatibility](#programmer-compatibility) below, because **not every
AVR ISP programmer can drive this part**.

## Prerequisites

* [SDCC Compiler](http://sdcc.sourceforge.net/)
* [Meson Build System](https://mesonbuild.com/)
* [HC6800-ES development kit](../doc/HC6800-ES%20Schematic.pdf) with an AT89S52
  populated instead of the STC89C52
* [avrdude](https://github.com/avrdudes/avrdude) **>= 7.0** — this is the first
  version with a built-in AT89S52 part definition. Anything older (including the
  avrdude bundled with the Arduino IDE) will not recognize `-p 89s52`.
* An AVR-ISP-compatible programmer, wired to the dev-kit's `ISP1` header

### Windows

```shell
pip install meson ninja
winget install --id AVRDudes.AVRDUDE -e
```
sdcc isn't packaged for winget; install it from the
[SDCC downloads page](http://sdcc.sourceforge.net/snap.php) as for the rest of
this repo.

> **Known issue:** the `avrdude` shim winget adds to PATH
> (`%LOCALAPPDATA%\Microsoft\WinGet\Links\avrdude.exe`, a symlink) fails to find
> `avrdude.conf` when invoked directly from a shell — it reports an empty
> programmer list (`avrdude -c '?'` shows nothing, `-p 89s52` fails). Flashing is
> a manual step in this project (the Meson build never invokes avrdude), so this
> *will* affect you. Add the real install directory to PATH instead, e.g.:
> ```shell
> $env:PATH = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\AVRDudes.AVRDUDE_Microsoft.Winget.Source_8wekyb3d8bbwe;$env:PATH"
> ```

Note you need [Zadig](https://zadig.akeo.ie/) on Windows to configure `libusb32` association to the device 
for this to work, see [Here](https://learn.sparkfun.com/tutorials/tiny-avr-programmer-hookup-guide/all).

### Linux (Debian/Ubuntu)

```shell
sudo apt install sdcc meson ninja-build avrdude
```
Debian and Ubuntu 24.04 ("noble") both package avrdude `7.1+dfsg`, new enough for
AT89S52 support out of the box. On an older release, check `avrdude -v` — if it
reports < 7.0, build avrdude from source instead of relying on `apt`.

If your ISP programmer is USB-based (USBasp, etc.), you'll typically need udev
rules granting your user access to the device (e.g. adding yourself to the
`plugdev` group, or a udev rule matching the programmer's vendor/product ID) to
avoid having to run avrdude as root — check the instructions for your specific
programmer.

## Hardware setup

Connect your AVR-ISP programmer to the **`ISP1`** header on the HC6800-ES board.
It's already wired for AT89S51/S52 SPI-ISP — confirmed from
`../doc/HC6800-ES Schematic.pdf` — so no rewiring of the dev-kit is needed:

| Signal | Connects to   |
|--------|---------------|
| VCC    | AT89S52 VCC   |
| GND    | GND           |
| P1.5   | MOSI          |
| P1.6   | MISO          |
| P1.7   | SCK           |
| RST    | RST/VPD       |

1. Power off the board and swap the socketed STC89C52 for an AT89S52.
2. Connect your ISP programmer to the `ISP1` header. The schematic gives the
   signal list but not which corner is pin 1 on the silkscreen — check the board
   itself (usually a square pad or a printed "1"/triangle marker, and most
   AVR-ISP cables have a pin-1 stripe/notch) before connecting. A reversed ISP
   header is a common way to fry the MOSI/MISO lines.

## Programmer compatibility

The AT89S52 enters serial-programming mode while **RST is HIGH**, the opposite of
every AVR, whose RESET is active low. avrdude only handles this on the STK500v1
path: `avrdude.conf` tags the part with `stk500_devcode = 0xe1` and sends that
byte to the programmer, and the stock ArduinoISP sketch flips its reset polarity
on it (`rst_active_high = (param.devicecode >= 0xe0);`). avrdude.conf's own
comment on the part reads "Tested with `-c avrisp`, USBASP programmers may
require different firmware".

| Programmer                   | Works?                                                                                                    |
|------------------------------|-----------------------------------------------------------------------------------------------------------|
| Arduino as ISP (`-c avrisp`) | Yes, out of the box — the stock sketch inverts RST from the device code                                   |
| USBasp (`-c usbasp`)         | Needs firmware that knows AT89S5x, e.g. [dioannidis/usbasp](https://github.com/dioannidis/usbasp)         |
| USBtinyISP (`-c usbtiny`)    | Yes, with an external RST jumper *and* the two-command flashing sequence — verified working on this board |

Symptom when reset polarity is wrong: a signature read appears to work while you
hold the board's RESET button (you are forcing RST high by hand), but other
attempts return `Device signature = 00 00 00` and nothing longer than a single
command completes reliably.

**USBtinyISP setup (verified working):** leave the programmer's RESET pin
*disconnected* from `ISP1`, and jumper the board's **RST to VCC** for the
duration of the session — MOSI/MISO/SCK/GND still come from the programmer.
Remove the jumper afterwards to let the chip run. The tidy version is a 74HC04 or
an NPN + 10 k inverter between the programmer's RESET output and `ISP1`'s RST.

Two things not to do:

* Don't leave the programmer's RESET pin connected *and* jumper RST to VCC. The
  USBtiny holds that pin low push-pull (`usb_control(pgm, USBTINY_POWERUP,
  my.sck_period, RESET_LOW, 0)` in `usbtiny_initialize`), so the jumper shorts
  VCC through the programmer's output FET — on an ATtiny2313 that is roughly
  100-200 mA through a pin rated 40 mA absolute maximum. It appears to work,
  right up until the pin fails.
* Don't hold the board's RESET button as a substitute, for the same reason.

Even with RST correct, `-c usbtiny` still cannot run a chip erase the normal way
— see [Flashing](#flashing).

## Building

```shell
meson setup build
ninja -v -C build
```

## Flashing

Flashing is **not** wired into the Meson build — there is no `flash_*` target.
avrdude can't reliably erase this part in the same invocation that writes it (see
[Why `avrdude -e` reports failure](#why-avrdude--e-reports-failure-usbtinyisp)),
so it's a two-command manual step. With a USBtinyISP:

```shell
avrdude -c usbtiny -p 89s52 -e
avrdude -c usbtiny -p 89s52 -D -U flash:w:build/09_hello.hex:i
```

* The first command **reports `Error: chip erase failed`. Ignore it** — the erase
  really did happen. See the section below for why.
* The second command needs `-D` so that avrdude skips its own automatic
  erase-before-write, which would fail the same way.

Substitute your own programmer/port in place of `-c usbtiny`, e.g.:

* Arduino as ISP, Windows: `-c avrisp -P COM3 -b 19200`
* Arduino as ISP, Linux: `-c avrisp -P /dev/ttyACM0 -b 19200`
* USBasp (either OS, no `-P` needed): `-c usbasp`

With `-c avrisp` the erase works normally, so the two commands collapse into one:

```shell
avrdude -c avrisp -P COM3 -b 19200 -p 89s52 -U flash:w:build/09_hello.hex:i
```

`-B` is not needed. The AT89S52 only requires SCK below f_osc/16 (≈ 690 kHz at
the board's 11.0592 MHz crystal) and every programmer's default is already well
under that.

### Verifying an erase

```shell
avrdude -c usbtiny -p 89s52 -U flash:r:erased.bin:r
```

avrdude trims trailing `0xFF` from raw output, so on a fully erased part this
prints `Writing 0 bytes to output file erased.bin`. **A zero-length file is the
success case** — it means all 8192 bytes read back as `0xFF`.

### Why `avrdude -e` reports failure (USBtinyISP)

avrdude's USBtiny driver validates every SPI transaction by requiring the target
to echo command byte 2 back in response byte 3 (`src/usbtiny.c`, `usbtiny_cmd`):

```c
return nbytes == 4 && res[2] == cmd[1]? LIBAVRDUDE_SUCCESS:
  LIBAVRDUDE_GENERAL_FAILURE;
```

That holds for AVRs, and for most AT89S52 instructions, but not for Chip Erase:
the part begins its self-timed erase as soon as it has received the second byte
and stops echoing. `avrdude -c usbtiny -p 89s52 -e -vvvv` shows it directly:

```
CMD: [ac 53 00 00] [00 ac 53 00]    Programming Enable  res[2] == 0x53, accepted
CMD: [28 00 00 00] [00 28 00 1e]    read signature 0    res[2] == 0x00, accepted
CMD: [ac 80 00 00] [00 ac 00 00]    Chip Erase          res[2] == 0x00 != 0x80, rejected
Error main() main.c 1864: chip erase failed
```

The erase completes regardless; only avrdude's check of the reply fails. `-F`
does not help — `main.c` aborts on any nonzero return from `avr_chip_erase()`
without consulting it. Nor is this a timing problem: the check happens *before*
`usleep(p->chip_erase_delay)`, so raising `chip_erase_delay` (avrdude.conf ships
250 ms; the datasheet says ~500 ms) changes nothing.

The STK500v1 driver used by `-c avrisp` applies no such echo check, which is
consistent with avrdude.conf's note that the part was "Tested with `-c avrisp`".

## Demos

Only `00_hello` has been ported here so far — same LED-blink-on-P2.0 demo as the
[root `00_hello`](../00_hello), unmodified, since the AT89S52 is code-compatible
with the existing SDCC `-mmcs51` target. See `CLAUDE.md` for the status of
porting the rest of the demo series into this folder.

----
[(C) 2025](../LICENSE) [Thomas Reidemeister](https://reidemeister.com)
