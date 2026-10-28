DEVICE = attiny88
F_CPU = 16000000
MICRONUCLEUS_DIR = micronucleus/commandline
MICRONUCLEUS = $(MICRONUCLEUS_DIR)/micronucleus
CC = avr-gcc

TARGET_ARCH = -mmcu=$(DEVICE)
TARGET_MACH = $(TARGET_ARCH)
CFLAGS = -Iv-usb/usbdrv -I. -DDEBUG_LEVEL=0 --param=min-pagesize=0 -Wall -Wstack-usage=10 -O2 -DF_CPU=$(F_CPU)
ASFLAGS = $(CFLAGS)
OBJS = v-usb/usbdrv/usbdrv.o v-usb/usbdrv/usbdrvasm.o atsuc.o

atsuc.hex: atsuc.elf
	avr-objcopy -j .text -j .data -O ihex $< $@

# Adopted built-in "%: %.o" rule
atsuc.elf: $(OBJS)
	grep -F -q usb_in1_polled v-usb/usbdrv/asmcommon.inc || patch --batch --forward -p1 < vusb-in1polled.patch
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	avr-size $@

$(MICRONUCLEUS):
	sed -i -E 's/^(\s)STATIC/\1#STATIC/g' $(MICRONUCLEUS_DIR)/Makefile
	cd $(MICRONUCLEUS_DIR) && $(MAKE)

run: $(MICRONUCLEUS) atsuc.hex
	$(MICRONUCLEUS) --run $<

clean:
	rm -rfv $(OBJS) atsuc.elf atsuc.hex

mrproper: clean
	rm -rfv $(MICRONUCLEUS)
	git submodule deinit --all --force
