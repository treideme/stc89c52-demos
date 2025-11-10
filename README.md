# A Collection of STC89C52 Demos using SDCC

See this [blog entry](https://reidemeister.com/blog/2022.06.04) for the start of the series and background for this code.

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
# Adjust the meson.build file to point to the COM port your serial flasher enumerates to. And power-cycle the target after
# issuing the flash command.
```

Manual boostrap of this example without a dev-kit should be doable as follows.

![Hello World](00_hello/connections.png)
[Drawing source](00_hello/connections.fzz).

### 01 LED Button

This is a simple example that lights up an LED `D1`-`D4` whenever the corresponding buttons `K1`-`K4` are pressed on
the HC6800-ES development kit. The circuit connections are shown below note `RxD` and `TxD` map to `P3.0` and `P3.1` respectively.

![Button interface](01_led_button/button_interface_direct.png)
![LED interface](01_led_button/led_interface_direct.png)

```shell
# Flash using ...
ninja -v -C ./build flash_01_led_button
# Adjust the meson.build file to point to the COM port your serial flasher enumerates to. And power-cycle the target after
# issuing the flash command.
```

### 01 LED Button Timer

Uses timer interrupts to poll the button states and update the LED states accordingly in `50 ms` intervals.

```shell
# Flash using ...
ninja -v -C ./build flash_01_led_button_timer
# Adjust the meson.build file to point to the COM port your serial flasher enumerates to. And power-cycle the target after
# issuing the flash command.
```

### 01 LED Buzzer

This demo extends the button and LED demo by adding a buzzer that sounds whenever any button is pressed.

```shell
# Flash using ...
ninja -v -C ./build flash_01_led_buzzer
# Adjust the meson.build file to point to the COM port your serial flasher enumerates to. And power-cycle the target after
# issuing the flash command.
``` 


----
[(C) 2025](LICENSE) [Thomas Reidemeister](https://reidemeister.com)
