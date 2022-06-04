# A Collection of STC89C52 Demos using SDCC

See this [blog entry](https://www.reidemeister.com/?p=295) for the start of the series and background for this code.

## Prerequisites
 * [SDCC Compiler](http://sdcc.sourceforge.net/)
 * [Meson Build System](https://mesonbuild.com/)
 * STC89C52RC development kit or [bootstrap circuits explained here](https://www.reidemeister.com/?p=295)

## Building

```shell
meson build
ninja -v -C ./build
```

## Flashing

These examples use [stcgal](https://github.com/nrife/stcgal) as flashing tool.
Flashing for particular versions of the firmware can be done via ninja.

```shell
# Power target off
ninja -v -C ./build flash_<hexname>
# Power target on
# Power-cycle target to see new firmware running
```

## Demos

### 00 Hello World

This demo is a simple hello world application that toggles a LED on pin
P1.0.

----
[(C) 2022](LICENSE) [Thomas Reidemeister](https://reidemeister.com)
