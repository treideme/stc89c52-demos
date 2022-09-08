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

### 01 Click Counter

This demo shows how to interface multiplexed 7-segment displays and
how to read and debounce button presses. Each button ```K1``` to ```K4```
control two segments of an array of 8 7-segment numbers. A single button
press will increase the counter by one step.

![7 Segment Driver](01_button_segment/8051_7segment-driver.png)

```U2``` is used as driver for the individual segments. This can be 
implemented with a Tristate latch, such as the [74HCT573](https://www.nexperia.com/products/analog-logic-ics/synchronous-interface-logic/latches-registered-drivers/series/74HC573-74HCT573.html).
(The latch is used to buffer the outputs of the microcontroller, since
it has a higher drive stength. Since the latch is persistent after ```OE```
is disabled the GPIOs mapped to the segments can also be used for other
tasks.
Because ICs of the 7400-series HCT grade have a logic margin of 0.8 and 2.0V
respectively they can also be used as level translator to drive 5V loads
from a microcontroller that has less than 5V input voltage.
The resistor array ```RP4``` provides current limiting, such that
the segment diodes of the [segment arrays](http://www.xlitx.com/datasheet/CL3641AH.pdf)
do not get overloaded.)

![7 Segment Driver](01_button_segment/8051_7segment_selector.png)

The segment selection can be implemented with an inverting 3 to 8 
multiplexer such as the [74HCT138](https://www.nexperia.com/products/analog-logic-ics/i-o-expansion-logic/decoders-demultiplexers/series/74HC138-74HCT138.html).
(Again this provides additional current sinking capability and
 can act as a level-shifter).

The pins (not shown) of the buttons are pulled up with weak pull up
resistors, such that a short to ground signals a button press. 

![Button Interface](01_button_segment/8051_simple_button_interface.png).

Also note that pull-ups are needed on P0, as the pins act by default
as open-drain outputs.

![Pull Ups](01_button_segment/8051_pullups_for_signal_stability.png)


----
[(C) 2022](LICENSE) [Thomas Reidemeister](https://reidemeister.com)
