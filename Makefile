VUSB_BRANCH = releases/20121206
DEVICE = attiny88
F_CPU = 16000000
MICRONUCLEUS = micronucleus
CC = avr-gcc

TARGET_ARCH = -mmcu=$(DEVICE)
TARGET_MACH = $(TARGET_ARCH)
CFLAGS = -Iusbdrv -I. -DDEBUG_LEVEL=0 --param=min-pagesize=0 -Wall -Wstack-usage=10 -O2 -DF_CPU=$(F_CPU)
ASFLAGS = $(CFLAGS)
OBJS = v-usb/usbdrv/usbdrv.o v-usb/usbdrv/usbdrvasm.o atsuc.o

atsuc.hex: atsuc.elf
	avr-objcopy -j .text -j .data -O ihex $< $@
	avr-size $@

atsuc.elf: $(OBJS)
	$(LINK.c) $(OBJS) $(LOADLIBES) $(LDLIBS) -o $@

run: atsuc.hex
	$(MICRONUCLEUS) --run $<

clean:
	rm -rfv $(OBJS) atsuc.elf atsuc.hex
