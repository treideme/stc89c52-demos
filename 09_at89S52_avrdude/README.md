# AT89S52 Example (avrdude)

This is an AT89S52 variant of the [STC89C52 demo series](../README.md), using the
same HC6800-ES development kit but with an **AT89S52** in the CPU socket instead
of the STC89C52. The AT89S52 has no UART bootloader, so it can't be flashed with
`stcgal` like the rest of this repo — instead it's programmed over its 4-wire SPI
ISP interface using [avrdude](https://github.com/avrdudes/avrdude), the same tool
used to flash AVR chips like the ATmega328.

You'll need an AVR-ISP-compatible programmer in addition to the dev-kit itself:
an Arduino running the
[ArduinoISP](https://docs.arduino.cc/built-in-examples/arduino-isp/ArduinoISP/)
sketch, a USBasp, an AVRISP mkII, or similar.

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
> programmer list (`avrdude -c '?'` shows nothing, `-p 89s52` fails). This does
> **not** affect `meson`/`ninja` builds — `find_program('avrdude')` resolves the
> real path correctly and `ninja -C build flash_00_hello` works as-is. If you want
> to run `avrdude` directly from a terminal, add its real install directory to
> PATH instead, e.g.:
> ```shell
> $env:PATH = "$env:LOCALAPPDATA\Microsoft\WinGet\Packages\AVRDudes.AVRDUDE_Microsoft.Winget.Source_8wekyb3d8bbwe;$env:PATH"
> ```

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

| Signal | Connects to |
|---|---|
| VCC | AT89S52 VCC |
| GND | GND |
| P1.5 | MOSI |
| P1.6 | MISO |
| P1.7 | SCK |
| RST | RST/VPD |

1. Power off the board and swap the socketed STC89C52 for an AT89S52.
2. Connect your ISP programmer to the `ISP1` header. The schematic gives the
   signal list but not which corner is pin 1 on the silkscreen — check the board
   itself (usually a square pad or a printed "1"/triangle marker, and most
   AVR-ISP cables have a pin-1 stripe/notch) before connecting. A reversed ISP
   header is a common way to fry the MOSI/MISO lines.

## Building

```shell
meson setup build
ninja -v -C build
```

## Flashing

```shell
ninja -v -C build flash_00_hello
```

`meson.build` hardcodes the programmer type and port in `avrdude_args`
(`-c avrisp -P COM3 -b 19200` by default, for an Arduino-as-ISP on Windows). Edit
that line for your hardware/OS before flashing, e.g.:

* Arduino as ISP, Windows: `-c avrisp -P COM3 -b 19200`
* Arduino as ISP, Linux: `-c avrisp -P /dev/ttyACM0 -b 19200`
* USBasp (either OS, no `-P` needed): `-c usbasp`

## Demos

Only `00_hello` has been ported here so far — same LED-blink-on-P2.0 demo as the
[root `00_hello`](../00_hello), unmodified, since the AT89S52 is code-compatible
with the existing SDCC `-mmcs51` target. See `CLAUDE.md` for the status of
porting the rest of the demo series into this folder.

----
[(C) 2025](../LICENSE) [Thomas Reidemeister](https://reidemeister.com)
