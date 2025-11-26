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

# Laundry list of example
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

    ['09_ethernet', '09_ethernet.hex', ['09_ethernet/ethernet.c', '09_ethernet/xprintf.c'], 'Ethernet Communication Example'],
]

# Build automation
foreach p : progs
    obj = compiler.process(p[2])
    exe = custom_target(p[1],
        input : obj,
        output : p[1],
        install : true,
        install_dir: 'firmware',
        command : [cc, cc_args, '-o', '@OUTPUT@', '@INPUT@'],
    )
    fls = run_target('flash_@0@'.format(p[0]),
        command : [stcgal] + stcgal_args + ['@0@'.format(exe.full_path())],
        depends : exe,
    )

endforeach
