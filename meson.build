project('STC89C52 Examples',
        'c',
        version : '0.1',
        license : ['Apache-2.0']
)

# Use SDCC as compiler and linker
cc = find_program('sdcc', required : true)
stcgal = find_program('stcgal', required : true)

# Compile commands for sdcc
cc_args = ['-mmcs51', '--Werror', '--std-c23', '--out-fmt-ihx']
# Link commands for sdcc
cc_incs = []

# Flashing arguments for STCGAL
stcgal_args = ['-P', 'stc89a', '-p', '/dev/ttyUSB0', '-b', '9600'] # Force 12T mode

# Since SDCC is not natively supported in meson, make it a generator...
compiler = generator(cc,
    output : '@BASENAME@.rel',
    arguments : cc_args + cc_incs + ['-c', '@INPUT@'] + ['-o', '@OUTPUT@'],
)

# The same compiler, with the ENC28J60 driver's SPI transport switched from
# bit-banging to the UART's synchronous Mode 0 (see 09_ethernet/enc28j60_cfg.h).
# A second generator rather than a per-target argument because meson declares a
# generator once and reuses it, and there are exactly two transports -- a
# general mechanism would be more code than the case needs.
mode0_args = cc_args + ['-DENC28J60_SPI_MODE0=1']
compiler_mode0 = generator(cc,
    output : '@BASENAME@.rel',
    arguments : mode0_args + cc_incs + ['-c', '@INPUT@'] + ['-o', '@OUTPUT@'],
)

# Laundry list of example
#
# [flash_target_name, output_name, sources, description, mode0?]
#
# The first field names this entry's flash_* run_target and so has to be unique
# across the list -- it is usually the directory, but two images can come out of
# one directory. The fifth field is optional and defaults to false; true builds
# that image with -DENC28J60_SPI_MODE0=1.
progs = [
    ['00_hello', '00_hello.hex', ['00_hello/hello.c'], 'Hello World Example'],

    ['01_led_button', '01_led_button.hex', ['01_led_button/led_button.c'], 'LED Button Example'],
    ['01_led_button_timer', '01_led_button_timer.hex', ['01_led_button_timer/led_button.c'], 'LED Button Example Timer'],
    ['01_led_button_debounce', '01_led_button_hyst.hex', ['01_led_button_debounce/led_button.c'], 'LED Button Example Timer with Hysteresis'],
    ['01_led_buzzer', '01_led_buzzer.hex', ['01_led_buzzer/led_buzzer.c'], 'LED and Buzzer Example Timer with Hysteresis'],
    ['01_led_74H595', '01_led_74H595.hex', ['01_led_74H595/led_74H595.c'], '74H595 Shift Register Example'],
    ['01_led_matrix', '01_led_matrix.hex', ['01_led_matrix/led_matrix.c'], '8x8 Matrix Example'],
    ['01_button_led_matrix', '01_button_led_matrix.hex', ['01_button_led_matrix/button_led_matrix.c'], '8x8 Matrix Example'],

    ['02_7_segment', '02_7_segment.hex', ['02_7_segment/segment.c'], '7 Segment Example'],
    ['02_7_segment_dyn', '02_7_segment_dyn.hex', ['02_7_segment_dyn/segment.c'], '7 Segment Example Dynamic'],

    ['03_hd44780_lcd', '03_hd44780_lcd.hex', ['03_hd44780_lcd/lcd.c'], '1602 Display Example'],

    ['04_st7920_lcd', '04_st7920_lcd.hex', ['04_st7920_lcd/lcd.c'], '128x64 Display Example'],
    ['04_st7920_graph', '04_st7920_graph.hex', ['04_st7920_graph/lcd.c'], '128x64 Display Example Drawing'],

    ['06_DS18B20_1wire', '06_DS18B20_1wire.hex', ['06_DS18B20_1wire/wire.c'], 'Dallas 1 Wire Temperature Sensor Example'],

    ['07_at24c02_i2c', '07_at24c02_i2c.hex', ['07_at24c02_i2c/i2c.c'], 'I2C EEPROM Example'],

    ['08_irda', '08_irda.hex', ['08_irda/irda.c'], 'Infrared transmission Example'],

    ['09_ethernet', '09_ethernet.hex', ['09_ethernet/ethernet.c', '09_ethernet/xprintf.c', '09_ethernet/enc28j60.c'], 'Ethernet Communication Example'],

    # The same bring-up, with somewhere to report to. The SPI moves onto the
    # UART's Mode 0 shift register -- one machine cycle per bit instead of 26 --
    # which costs P3.0/P3.1 and with them the CH340 console, so the console
    # becomes a bit-banged transmitter on P1.7. It needs an inverter and a
    # tri-state gate in the adapter harness, which is why this is a separate
    # image and not a replacement; see 09_ethernet/enc28j60_cfg.h.
    ['09_ethernet_console', '09_ethernet_console.hex',
        ['09_ethernet/enc28j60.c', '09_ethernet/xprintf.c',
         '09_ethernet/softuart.c', '09_ethernet/diag_console.c'],
        'ENC28J60 over UART Mode 0 with a bit-banged debug console', true],
]

# Build automation
foreach p : progs
    use_mode0 = p.length() > 4 and p[4]
    obj = use_mode0 ? compiler_mode0.process(p[2]) : compiler.process(p[2])
    exe = custom_target(p[1],
        input : obj,
        output : p[1],
        install : true,
        install_dir: 'firmware',
        command : [cc, use_mode0 ? mode0_args : cc_args, '-o', '@OUTPUT@', '@INPUT@'],
    )
    fls = run_target('flash_@0@'.format(p[0]),
        command : [stcgal] + stcgal_args + ['@0@'.format(exe.full_path())],
        depends : exe,
    )

endforeach
