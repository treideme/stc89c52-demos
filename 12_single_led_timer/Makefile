###############################################################################
#                                                                             #
# IRQ LED blinking                                                            #
# -------------------------                                                   #
###############################################################################

## Tool configuration
CC=sdcc
LD=sdcc
OC=objcopy
SZ=size

CSRC=$(wildcard *.c)
OBJ=$(CSRC:.c=.rel)
INCLUDE=-I. -I../include

CFLAGS=--Werror -mmcs51 --model-small --Werror --opt-code-size

all: app.bin
	

app.bin: app.hex
	$(OC) --input-target=ihex --output-target=binary $< $@
	$(SZ) $<

app.hex: $(OBJ)
	$(LD) -I. -I../include $(CFLAGS) -o $@ $^

%.rel: %.c
	$(CC) -I. -I../include $(CFLAGS) -o $@ -c $<

# Be sure to have stcgal installed and your user has access to /dev/ttyUSB0
# if you get protocol error change to stc89, 115200 might be to fast, pinned to 9600 here
prog:
	stcgal -b 9600 -P stc89a -p /dev/ttyUSB0 app.hex

clean:
	rm -f *.asm *.ihx *.lk *.lst *.bin *.map *.mem *.rel *.rst *.sym *.hex
