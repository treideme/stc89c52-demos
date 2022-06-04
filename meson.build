project('STC89C52 Examples',
        'c',
        version : '0.1',
        license : ['Apache-2.0']
)

# Use SDCC as compiler and linker
cc = find_program('sdcc', required : true)

# Compile commands for sdcc
cc_args = ['-mmcs51', '--Werror', '--std-c99', '--out-fmt-ihx']
# Link commands for sdcc
cc_incs = []

# Since SDCC is not natively supported in meson, make it a generator...
compiler = generator(cc,
    output : '@BASENAME@.rel',
    arguments : cc_args + cc_incs + ['-c', '@INPUT@'] + ['-o', '@OUTPUT@'],
)

# Laundry list of example
progs = [
    ['00_hello.hex', ['00_hello/hello.c'], 'Hello World Example'],
]

# Build automation
foreach p : progs
    obj = compiler.process(p[1])
    exe = custom_target(p[0],
        input : obj,
        output : p[0],
        install : true,
        install_dir: 'firmware',
        command : [cc, cc_args, '-o', '@OUTPUT@', '@INPUT@'],
    )
endforeach
