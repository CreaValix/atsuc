#include <stdint.h>

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/pgmspace.h>  // required by usbdrv.h

#include "usbdrv/usbdrv.h"

/*
Oscilloscope readings show that the controler response times are somwhere
at 100 nS. We can ignore Nintendo's original timing an respond to the USB
interrupt way faster.
Settings TIMING_QUICK sets the clock cycle to approximately 1 µS, reducing one
button readout from ~160 µS to ~40 µS.
*/
#define TIMING_QUICK 1
#define POWERSAVE 1

#define LED_DIR_REG DDRD
#define LED_DIR_PIN (1 << DDD0)
#define LED_OUT_REG PORTD
#define LED_OUT_PIN (1 << PORTD0)
#define LED_ON LED_OUT_REG |= LED_OUT_PIN
#define LED_OFF LED_OUT_REG &= ~LED_OUT_PIN
#define LED_TOGGLE LED_OUT_REG ^= LED_OUT_PIN

PROGMEM const char usbHidReportDescriptor[27] = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x04, // USAGE (Gamepad)
    0xA1, 0x01, // COLLECTION (Application)
    0x05, 0x09, //   USAGE_PAGE (Button)
    0x19, 0x01, //   USAGE_MINIMUM
    0x29, 0x0C, //   USAGE_MAXIMUM
    0x15, 0x00, //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x95, 0x0C, //   REPORT_COUNT (12)
    0x75, 0x01, //   REPORT_SIZE (1)
    0x81, 0x02, //   INPUT (Data,Var,Abs)
    0x95, 0x04, //   REPORT_COUNT (4), padding to reach 2 bytes
    0x81, 0x03, //   INPUT (Cnst,Var,Abs)
    0xC0,       // END_COLLECTION
};

uint16_t snes_button_state = 0;
static inline void snes_button_read();
uint8_t led = 0;
int __attribute__((noreturn)) main(void) {
    wdt_enable(WDTO_1S);
#ifdef POWERSAVE
    ADCSRA &= (1 << ADEN);
    power_all_disable();
    sleep_enable();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
#endif

    // Set up SNES pad pins
    PORTB = (PORTB & ~((1 << PORTB0) | (1 << PORTB1))) | (1 << PORTB2);
    DDRB = (DDRB | (1 << DDB0) | (1 << DDB1)) & ~(1 << DDB2);
    LED_DIR_REG |= LED_DIR_PIN;

    usbInit();
    // Enforce re-enumeration, do this while interrupts are disabled.
    usbDeviceDisconnect();
    // Fake USB disconnect for > 250 ms
    _delay_ms(260);
    usbDeviceConnect();
    sei();
    for(;;) {
        wdt_reset();
#ifdef POWERSAVE
        WDTCSR |= (1 << WDIE);
#endif
        usbPoll();
#ifdef POWERSAVE
        cli();
#endif
        if (usbInterruptIsReady()) {
#ifdef POWERSAVE
            sei();
#endif
            LED_ON;
            snes_button_read();
            usbSetInterrupt((void *)&snes_button_state, sizeof(snes_button_state));
            if (!snes_button_state) {
                LED_OFF;
            }
#ifdef POWERSAVE
        } else {
            sleep_enable();
            sei();
            sleep_cpu();
            sleep_disable();
            LED_TOGGLE;
#endif
        }
    }
}

uint8_t idle_rate = 0;
usbMsgLen_t usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data;

    // This code is taken from the V-USB HID mouse example.
    if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        if (rq->bRequest == USBRQ_HID_GET_REPORT) {
            usbMsgPtr = (void *)&snes_button_state;
            return sizeof(snes_button_state);
        } else if (rq->bRequest == USBRQ_HID_GET_IDLE) {
            usbMsgPtr = &idle_rate;
            return 1;
        } else if (rq->bRequest == USBRQ_HID_SET_IDLE) {
            idle_rate = rq->wValue.bytes[1];
        }
    }
    return 0;
}

static inline void snes_button_read() {
    snes_button_state = 0;
    // Latch high, clock high. First button available on data.
    PORTB |= (1 << PORTB0) | (1 << PORTB1);
#ifndef TIMING_QUICK
    _delay_us(12);
#else
    _delay_us(1);
#endif
    PORTB &= ~(1 << PORTB0);
    for (int i = 0; i < 12; i++) {
#ifndef TIMING_QUICK
        // Loop code runs approximately 2 µS.
        // Waiting 5 µS twice gets very close to 12 µS clock cycle.
        _delay_us(5);
#endif
        snes_button_state >>= 1;
        snes_button_state |= (uint16_t)(~PINB & (1 << PINB2)) << (11 - PINB2);
        // Clock low
        PORTB &= ~(1 << PORTB1);
#ifndef TIMING_QUICK
        // Same as above.
        _delay_us(5);
#else
        _delay_us(1);
#endif
        // Clock high. Next button will be available on data.
        PORTB |= (1 << PORTB1);
    }
}

#ifdef POWERSAVE
ISR(WDT_vect) {
}
#endif
