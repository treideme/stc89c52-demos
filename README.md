# A Collection of STC89C52 Demos using SDCC

See this [blog entry](https://reidemeister.com/blog/2022.06.04) for the start of the series and background for this code.
And the entire [STC89C52 SDCC series](https://reidemeister.com/blog/category/8051).

# Prerequisites
 * [SDCC Compiler](http://sdcc.sourceforge.net/)
 * [Meson Build System](https://mesonbuild.com/)
 * [STC89C52RC development kit](doc/HC6800-ES%20Schematic.pdf) or [bootstrap circuits explained here](https://reidemeister.com/blog/2022.06.04)
 * [stcgal for flashing](https://github.com/nrife/stcgal)

# Building

```shell
meson build
ninja -v -C ./build
```

# Flashing

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

# Demos

The demos span multiple topics and are grouped by my respective blog posts.

 0. [Hello World](#0-hello-world)
 1. [Basic LED and Button Interfacing](#1-basic-led-and-button-interfacing)

## 0. Hello World

![Hello World](00_hello/00_hello.gif)

This demo is a simple hello world application that toggles a LED on pin
P2.0. This is covered in the [first blog post of the series](https://reidemeister.com/blog/2022.06.04).

```shell
# Flash using ...
ninja -v -C ./build flash_00_hello
# Adjust the meson.build file to point to the COM port your serial flasher enumerates to. And power-cycle the target after
# issuing the flash command.
```

Manual boostrap of this example without a dev-kit should be doable as follows.

![Hello World](00_hello/connections.png)
[Drawing source](00_hello/connections.fzz).

## 1. Basic LED and Button Interfacing

These examples are covered in this [blog post](https://reidemeister.com/blog/2025.11.01) of the series.

### LED Button

![LED Button](01_led_button/01_led_button.gif)

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

### LED Button Timer

![LED Button Timer](01_led_button_timer/01_led_button_timer.gif)

Uses timer interrupts to poll the button states and update the LED states accordingly in `50 ms` intervals.

```shell
# Flash using ...
ninja -v -C ./build flash_01_led_button_timer
# Adjust the meson.build file to point to the COM port your serial flasher enumerates to. And power-cycle the target after
# issuing the flash command.
```

### LED Button Timer with Debouncing

Same as above but adds simple debouncing to the button handling to avoid bouncing effects so the effective time is `1 s`.

![LED Button Timer](01_led_button_timer/01_led_button_timer.gif)

### LED and Buzzer

![LED Buzzer](01_led_buzzer/01_led_buzzer.gif)

This demo extends the button and LED demo by adding a buzzer that sounds whenever any button is pressed.

```shell
# Flash using ...
ninja -v -C ./build flash_01_led_buzzer
# Adjust the meson.build file to point to the COM port your serial flasher enumerates to. And power-cycle the target after
# issuing the flash command.
``` 

### LED Multiplexing With 74HC595 Array

![LED 74HC595 Array](01_led_74H595/01_led_74H595.gif)

This demo shows how to use a 74HC595 shift register to drive an array of 8 LEDs using only 3 pins from the microcontroller.

Note on the HC6800-ES development kit, `JP595` and `JPOE` need to be set to connect the 74HC595.

```shell
# Flash using ...
ninja -v -C ./build flash_01_led_74H595
# Adjust the meson.build file to point to the COM port your serial flasher enumerates to. And power-cycle the target after
# issuing the flash command.
```

### LED Matrix 8x8 Display with 74HC595 Column Driver
![LED Matrix 8x8 with 74HC595](01_led_matrix/01_led_matrix.gif)

Illustration how to use a 8x8 LED matrix display.

```shell
# Flash using ...
ninja -v -C ./build flash_01_led_matrix
# Adjust the meson.build file to point to the COM port your serial flasher enumerates to. And power-cycle the target after
# issuing the flash command.
```

### LED Matrix and Hex Keypad
![LED Matrix and Hex Keypad](01_button_led_matrix/button_led_matrix.gif)
Expansion of the previous demo by adding a hex keypad to control the display.

```shell
# Flash using ...
ninja -v -C ./build flash_01_button_led_matrix
# Adjust the meson.build file to point to the COM port your serial flasher enumerates to. And power-cycle the target after
# issuing the flash command.
```

## 02 Seven Segment Displays

See the second blog post in the series [here](https://reidemeister.com/blog/2025.11.15).

### 02 7-Segment Display

![7-Segment Display](02_7_segment/02_7seg_single.gif)

This demo shows how to interface a single 7-segment display to the STC89C52 microcontroller for displaying
a single digit number with a lookup table.

```shell
# Flash using ...
ninja -v -C ./build flash_02_7_segment
# Adjust the meson.build file to point to the COM port your serial flasher enumerates to. And power-cycle the target after
# issuing the flash command.
```

### 02 7-Segment Multiplexed Display

![7-Segment Multiplexed Display](02_7_segment_dyn/02_7seg_dyn.gif)

This demo shows how to interface a multiplexed 4-digit 7-segment display to the STC89C52 microcontroller
for displaying a 8-digit number with a lookup table and multiplexing.

```shell
# Flash using ...
ninja -v -C ./build flash_02_7_segment_dyn
# Adjust the meson.build file to point to the COM port your serial flasher enumerates to. And power-cycle the target after
# issuing the flash command.
```

At speed it looks like this:
![7-Segment Multiplexed Display Fast](02_7_segment_dyn/02_7seg_dyn.png)


----
[(C) 2025](LICENSE) [Thomas Reidemeister](https://reidemeister.com)
