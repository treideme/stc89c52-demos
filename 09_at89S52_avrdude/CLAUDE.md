# 09_at89S52_avrdude

AT89S52 variant of the demo series in the parent repo. Same HC6800-ES dev-kit as
the STC89C52 demos (`../00_hello` etc.), but the socketed MCU is swapped for an
**AT89S52** and flashed over its SPI ISP interface with `avrdude`, instead of the
STC89C52's UART bootloader (`stcgal`, used by the root `meson.build`). This is a
standalone Meson project (its own `project()`), not wired into the root build —
the two demos use unrelated flashing tools/hardware and the root build has no
subdir/subproject structure to integrate into.

## Required tools

- `sdcc` (same as the rest of the repo)
- `meson`, `ninja`
- `avrdude` **>= 7.0** — earlier versions (including any bundled with the Arduino
  IDE) don't know the AT89S52 part and will fail with `AVR Part "89s52" not found`.

See `README.md` in this folder for install commands per OS.

## avrdude specifics (for anyone editing this folder's `meson.build`)

- Part id: `-p 89s52` (signature `1E 52 06`, 8192 B flash). Added to upstream
  avrdude in [PR #1310](https://github.com/avrdudes/avrdude/pull/1310), merged
  2023-03-21, first shipped in avrdude 7.0.
- `prog_modes = PM_ISP | PM_HVPP` in `avrdude.conf`; the part's own comment notes
  it was "Tested with `-c avrisp`" (STK500v1 protocol — matches an Arduino
  flashed with the ArduinoISP sketch) and warns USBasp programmers "may require
  different firmware".
- Flash is written byte-at-a-time (no real paged write) — flashing is slow but
  needs no page-size tuning.
- `avrdude_args` in `meson.build` hardcodes `-c avrisp -P COM3 -b 19200`; edit
  those to match your programmer/OS/port before running a `flash_*` target (same
  convention the root `meson.build` uses for its `stcgal_args`).
- **Known Windows gotcha:** if avrdude was installed via `winget install
  AVRDudes.AVRDUDE`, invoking bare `avrdude` from a shell (PATH resolves through
  `%LOCALAPPDATA%\Microsoft\WinGet\Links\avrdude.exe`, a symlink) fails to find
  `avrdude.conf` and reports an empty programmer list. `meson`/`ninja` are
  unaffected — `find_program('avrdude')` resolves straight to the real install
  directory, and `flash_*` targets run correctly. Only matters for someone running
  `avrdude` directly outside the build; see README for the workaround.

## Hardware

- Connect the ISP programmer to **`ISP1`** on the HC6800-ES board (confirmed from
  `../doc/HC6800-ES Schematic.pdf`) — no dev-kit rewiring needed, just swap the
  socketed STC89C52 for an AT89S52 and wire an AVR-ISP-compatible programmer
  (Arduino-as-ISP, USBasp, AVRISP mkII, ...) to this header:

  | Signal | Connects to |
  |---|---|
  | VCC | AT89S52 VCC |
  | GND | GND |
  | P1.5 | MOSI |
  | P1.6 | MISO |
  | P1.7 | SCK |
  | RST | RST/VPD |

- The schematic doesn't show which corner is physical pin 1 — confirm against
  the board silkscreen (square pad / printed "1" / triangle marker) before
  connecting; a reversed ISP header is a common way to fry MOSI/MISO.

## Build / flash

```shell
meson setup build
ninja -v -C build              # -> build/00_hello.hex
ninja -v -C build flash_00_hello   # needs avrdude + programmer + hardware wired up
```

## Status

Only `00_hello` (`hello.c`, unmodified copy of `../00_hello/hello.c` — AT89S52 is
code-compatible with the existing SDCC `-mmcs51` target) has been ported into this
folder's `progs` list so far. Porting the rest of the root demos (`01_*`-`08_*`)
here is a deliberate follow-up, not done yet. Note for whoever does that:
`01_led_buzzer` is the only existing demo that uses `P1.5`-`P1.7` (confirmed via
grep) — those pins conflict with the `ISP1` header while a programmer is
connected, so that demo needs rewiring or a pin change before it can be ported
here.
