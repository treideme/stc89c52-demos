project('AT89S52 Example (avrdude)',
        'c',
        version : '0.1',
        license : ['Apache-2.0']
)

# Use SDCC as compiler and linker
cc = find_program('sdcc', required : true)

# Compile commands for sdcc
cc_args = ['-mmcs51', '--Werror', '--std-c23', '--out-fmt-ihx']
# Link commands for sdcc
cc_incs = []

# NOTE: this project deliberately has no flash_* targets. Flashing an AT89S52
# over ISP is a manual step -- with a USBtinyISP it takes two avrdude
# invocations, because avrdude's USBtiny driver rejects the part's Chip Erase
# reply and aborts if erase and write are combined. See README.md "Flashing".

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
endforeach
