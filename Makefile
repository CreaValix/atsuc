# Set project specific variables
DEVICE = attiny88
F_CPU = 16000000
MICRONUCLEUS_DIR = micronucleus/commandline
MICRONUCLEUS = $(MICRONUCLEUS_DIR)/micronucleus
OBJS = v-usb/usbdrv/usbdrv.o v-usb/usbdrv/usbdrvasm.o atsuc.o

# Set variables used by built-in rules
CC = avr-gcc
CFLAGS = -DF_CPU=$(F_CPU) -DDEBUG_LEVEL=0 -Iv-usb/usbdrv -I. --param=min-pagesize=0 -O2 -Wall -Wstack-usage=10
ASFLAGS = $(CFLAGS)
LDLIBS = -Wl,--print-memory-usage
TARGET_ARCH = -mmcu=$(DEVICE)
TARGET_MACH = $(TARGET_ARCH)

.PHONY: all size run clean mrproper

all: atsuc.hex

v-usb/usbdrv/usbdrv.c:
	git submodule init v-usb
	git submodule update v-usb
	grep -F -q usb_in1_polled v-usb/usbdrv/asmcommon.inc || patch --batch --forward -p1 < vusb-in1polled.patch

# Adopted built-in "%: %.o" rule
atsuc.elf: $(OBJS)
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

atsuc.hex: atsuc.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

size: atsuc.elf
	avr-size -B $<
	avr-size --mcu=$(DEVICE) -C $<

$(MICRONUCLEUS_DIR):
	git submodule init micronucleus
	git submodule update micronucleus

$(MICRONUCLEUS): $(MICRONUCLEUS_DIR)
	sed -i -E 's/^(\s)STATIC/\1#STATIC/g' $(MICRONUCLEUS_DIR)/Makefile
	cd $(MICRONUCLEUS_DIR) && $(MAKE)

run: $(MICRONUCLEUS) atsuc.hex
	$(MICRONUCLEUS) --run $<

clean:
	rm -rfv $(OBJS) atsuc.elf atsuc.hex

mrproper: clean
	rm -rfv $(MICRONUCLEUS)
	git submodule deinit --all --force
