# A Collection of STC89C52 Demos using SDCC

See this [blog entry](https://www.reidemeister.com/?p=295) for the start of the series and background for this code.

## Prerequisites
 * [SDCC Compiler](http://sdcc.sourceforge.net/)
 * [Meson Build System](https://mesonbuild.com/)
 * [STC89C52RC development kit](doc/HC6800-ES%20Schematic.pdf) or [bootstrap circuits explained here](https://www.reidemeister.com/?p=295)

## Building

```shell
meson build
ninja -v -C ./build
```

## Flashing

These examples use [stcgal](https://github.com/nrife/stcgal) as flashing tool.
Flashing for particular versions of the firmware can be done via ninja.
My BSL version of the chip [was not supported](https://github.com/grigorig/stcgal/issues/50) by the mainline stcgal, [nrifes
version](https://github.com/nrife/stcgal) seemed to work better.

```shell
# Power target off
ninja -v -C ./build flash_<hexname>
# Power target on
# Power-cycle target to see new firmware running
```

## Demos

### 00 Hello World

![Hello World](00_hello/00_hello.gif)

This demo is a simple hello world application that toggles a LED on pin
P2.0.

```shell
# Flash using ...
ninja -v -C ./build flash_00_hello
# Adjust the meson.build file to point to the COM port your serial flasher enumerates to.
```

Manual boostrap of this example without a dev-kit should be doable as follows.

![Hello World](00_hello/connections.png)
[Drawing source](00_hello/connections.fzz).

----
[(C) 2022](LICENSE) [Thomas Reidemeister](https://reidemeister.com)
