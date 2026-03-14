project('AT89S52 Example (avrdude)',
        'c',
        version : '0.1',
        license : ['Apache-2.0']
)

# Use SDCC as compiler and linker
cc = find_program('sdcc', required : true)
avrdude = find_program('avrdude', required : true)

# Compile commands for sdcc
cc_args = ['-mmcs51', '--Werror', '--std-c23', '--out-fmt-ihx']
# Link commands for sdcc
cc_incs = []

# Flashing arguments for avrdude, over the AT89S52's SPI ISP interface.
# Adjust -c (programmer type) and -P (port) for your hardware/OS, e.g.:
#   Arduino as ISP, Windows: -c avrisp -P COM3        -b 19200
#   Arduino as ISP, Linux:   -c avrisp -P /dev/ttyACM0 -b 19200
#   USBasp (either OS, libusb-based, no -P needed):    -c usbasp
avrdude_args = ['-c', 'avrisp', '-p', '89s52', '-P', 'COM3', '-b', '19200']

# Since SDCC is not natively supported in meson, make it a generator...
compiler = generator(cc,
    output : '@BASENAME@.rel',
    arguments : cc_args + cc_incs + ['-c', '@INPUT@'] + ['-o', '@OUTPUT@'],
)

# Laundry list of examples.
# Only 00_hello has been ported to this AT89S52/avrdude sub-project so far — see
# CLAUDE.md for the status of porting the rest of the root demos.
progs = [
    ['09_hello', '09_hello.hex', ['hello.c'], 'Hello World Example'],
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
        command : [avrdude] + avrdude_args + ['-U', 'flash:w:@0@:i'.format(exe.full_path())],
        depends : exe,
    )

endforeach
