# AT89S52 Example (avrdude)

This is an AT89S52 variant of the [STC89C52 demo series](../README.md), using the
same HC6800-ES development kit but with an **AT89S52** in the CPU socket instead
of the STC89C52. The AT89S52 has no UART bootloader, so it can't be flashed with
`stcgal` like the rest of this repo — instead it's programmed over its 4-wire SPI
ISP interface using [avrdude](https://github.com/avrdudes/avrdude), the same tool
used to flash AVR chips like the ATmega328.

See my [blog post](https://reidemeister.com/blog/2026.03.14) for more details.

----
[(C) 2025](../LICENSE) [Thomas Reidemeister](https://reidemeister.com)
