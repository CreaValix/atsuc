# Set project specific variables
DEVICE = attiny88
F_CPU = 16000000
OBJS = v-usb/usbdrv/usbdrv.o v-usb/usbdrv/usbdrvasm.o atsuc.o
# See usbdrv.h for USB HID descriptor symbol
HIDDESCSYM = usbDescriptorHidReport

# Set variables used by built-in rules
CC = avr-gcc
CFLAGS = -DF_CPU=$(F_CPU) -DDEBUG_LEVEL=0 -Iv-usb/usbdrv -I. --param=min-pagesize=0 -O2 -Wall -Wstack-usage=10
ASFLAGS = $(CFLAGS)
LDLIBS = -Wl,--print-memory-usage
TARGET_ARCH = -mmcu=$(DEVICE)
TARGET_MACH = $(TARGET_ARCH)

.PHONY: all size hidcheck flash clean mrproper

all: atsuc.elf

v-usb/usbdrv/usbdrv.c:
	git submodule init v-usb
	git submodule update v-usb
	grep -F -q usb_in1_polled v-usb/usbdrv/asmcommon.inc || patch --batch --forward -p1 < vusb-in1polled.patch

# Adopted built-in "%: %.o" rule
atsuc.elf: $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

size: atsuc.elf
	avr-size -B $<
	avr-size --mcu=$(DEVICE) -C $<

hidrdd/rd.rex:
	git submodule init hidrdd
	git submodule update hidrdd

HIDDDPART = $(shell objdump -j .text -t $< | gawk '$$6=="$(HIDDESCSYM)"{print "skip="strtonum("0x"$$1)" count="strtonum("0x"$$5); exit}')
hiddesc.bin: atsuc.elf
	objcopy -j .text -O binary $< >(dd bs=1 $(HIDDDPART) of=$@)

hidcheck: hiddesc.bin hidrdd/rd.rex
	cd hidrdd; rexx rd.rex -b -f $${OLDPWD}/$<

flash: atsuc.elf
	avrdude -v -c micronucleus -p $(DEVICE) -x wait -U flash:w:$<:e -V

clean:
	rm -rfv $(OBJS) atsuc.elf hiddesc.bin

mrproper: clean
	git submodule deinit --all --force
