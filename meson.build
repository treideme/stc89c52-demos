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
#
# [flash_target_name, output_name, sources, description]
#
# The first field names this entry's flash_* run_target and so has to be unique
# across the list.
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

    ['05_enc', '05_enc.hex', ['05_enc/enc.c'], 'Encoder to LED Example'],
    ['05_enc_stepper', '05_enc_stepper.hex', ['05_enc_stepper/logic.c', '05_enc_stepper/main.c'], 'Encoder to 28BYJ-48 Stepper Example'],

    ['06_DS18B20_1wire', '06_DS18B20_1wire.hex', ['06_DS18B20_1wire/wire.c'], 'Dallas 1 Wire Temperature Sensor Example'],

    ['07_at24c02_i2c', '07_at24c02_i2c.hex', ['07_at24c02_i2c/i2c.c'], 'I2C EEPROM Example'],

    ['08_irda', '08_irda.hex', ['08_irda/irda.c'], 'Infrared transmission Example'],

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

# ---------------------------------------------------------------------------
# The IP stack (net.c/enc.c in 09_ethernet), one image per rung per part.
#
# This does not go through the generator above, for two reasons. SDCC refuses
# more than one source per invocation ("error 120"), so compiling and linking
# are separate steps either way; and a generator bakes its arguments in once,
# while every rung needs a different -DNET_RUNG. A generator per rung would
# also collide, because they would all emit main.rel/net.rel/enc.rel into the
# same place. Explicit custom_targets with unique object names avoid both.
#
# Flags are the staging project's, not cc_args, so the sizes reproduce the
# ones quoted in the blog post rather than drifting with this project's
# defaults.
net_cc_args = ['-mmcs51', '--model-small', '--Werror', '--opt-code-size', '--out-fmt-ihx']

# [part, flash, iram, xram, highest rung that fits]
#
# The last field is a measurement, not a preference. Rung 8 (TCP) needs about
# 11 KB against the RC's 8 KB, so asking for it would fail the link and break
# the whole build. The RD+ is the pin-compatible part with 61 KB that does fit
# it. Raise the number when a rung gets smaller, not to make ninja try harder.
net_targets = [
    ['stc89c52rc', 8192, 256, 256, 7],
    ['stc89c516rd', 62464, 256, 1024, 8],
]

# [image name, flash target, part, flash, iram, xram, rung, console?]
#
# The bring-up image is the same firmware with the serial console compiled in
# and rung 3 (ARP, ICMP, UDP echo) -- what the bench was actually driven with.
# It keeps the flash_09_ethernet name it has always had.
net_images = [['09_ethernet', '09_ethernet', 'stc89c52rc', 8192, 256, 256, 3, true]]

foreach t : net_targets
    foreach r : [1, 2, 3, 4, 5, 6, 7, 8]
        if r <= t[4]
            net_images += [['net_@0@_rung@1@'.format(t[0], r),
                            'net_@0@_rung@1@'.format(t[0], r),
                            t[0], t[1], t[2], t[3], r, false]]
        endif
    endforeach
endforeach

foreach im : net_images
    img_args = net_cc_args + ['-DNET_RUNG=@0@'.format(im[6]),
                              '-DNET_XRAM_SIZE=@0@'.format(im[5])]
    img_srcs = ['main', 'net', 'enc']
    if im[7]
        img_args += ['-DNET_CONSOLE=1']
        img_srcs += 'xprintf'
    endif

    img_objs = []
    foreach src : img_srcs
        img_objs += custom_target('@0@_@1@.rel'.format(im[0], src),
            input : '09_ethernet/@0@.c'.format(src),
            output : '@0@_@1@.rel'.format(im[0], src),
            command : [cc, img_args, '-c', '@INPUT@', '-o', '@OUTPUT@'],
        )
    endforeach

    img_hex = '@0@.hex'.format(im[0])
    img_exe = custom_target(img_hex,
        input : img_objs,
        output : img_hex,
        install : true,
        install_dir : 'firmware',
        command : [cc, img_args,
                   '--code-size', '@0@'.format(im[3]),
                   '--iram-size', '@0@'.format(im[4]),
                   '--xram-size', '@0@'.format(im[5]),
                   '-o', '@OUTPUT@', '@INPUT@'],
    )

    run_target('flash_@0@'.format(im[1]),
        command : [stcgal] + stcgal_args + ['@0@'.format(img_exe.full_path())],
        depends : img_exe,
    )
endforeach
