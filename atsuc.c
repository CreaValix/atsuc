#include <stdint.h>
#include <string.h>

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/pgmspace.h>  // required by usbdrv.h

#include "config.h"
#include "usbdrv/usbdrv.h"

#define LED_DIR_REG DDRD
#define LED_DIR_PIN (1 << DDD0)
#define LED_OUT_REG PORTD
#define LED_OUT_PIN (1 << PORTD0)
#define LED_ON LED_OUT_REG |= LED_OUT_PIN
#define LED_OFF LED_OUT_REG &= ~LED_OUT_PIN
#define LED_TOGGLE LED_OUT_REG ^= LED_OUT_PIN

#ifdef BTN_DIRECTIONS_AS_DPAD
/*
This is a mapping table from d-pad key presses (as array index) to multiples of 45° angles.
0 Means d-pad is not pressed. North starts at 1 and is increased clockwise.
If two opposite direction keys are pressed, they are ignored.
Output is pre-shifted to the higher nibble to fit in the HID report.
*/
PROGMEM const char dpad_keys_to_rotation[16] = {
    //       → ← ↓ ↑ direction
    0x00, // 0 0 0 0 0
    0x01, // 0 0 0 1 1
    0x05, // 0 0 1 0 5
    0x00, // 0 0 1 1 0
    0x07, // 0 1 0 0 7
    0x08, // 0 1 0 1 8
    0x06, // 0 1 1 0 6
    0x07, // 0 1 1 1 7
    0x03, // 1 0 0 0 3
    0x02, // 1 0 0 1 2
    0x04, // 1 0 1 0 4
    0x03, // 1 0 1 1 3
    0x00, // 1 1 0 0 0
    0x01, // 1 1 0 1 1
    0x05, // 1 1 1 0 5
    0x00, // 1 1 1 1 0
};

PROGMEM const char usbHidReportDescriptor[146] = {
#else
PROGMEM const char usbHidReportDescriptor[118] = {
#endif
    // There are only buttons. So all input is digital.

    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x05, // USAGE (Gamepad)
    0xa1, 0x01, // COLLECTION (Application)
    0x85, 0x01, //   REPORT_ID (1)
    0x05, 0x09, //   USAGE_PAGE (Button)
#ifdef BTN_AB_LAYOUT_XBOX
    0x09, 0x02, //   USAGE (Button 2), B       -> East
#else
    0x09, 0x01, //   USAGE (Button 1), B       -> South
#endif
    0x09, 0x05, //   USAGE (Button 5), Y       -> West
    0x09, 0x0b, //   USAGE (Button 11), Select -> Select
    0x09, 0x0c, //   USAGE (Button 12), Start  -> Start
#ifdef BTN_AB_LAYOUT_XBOX
    0x09, 0x01, //   USAGE (Button 1), A       -> South
#else
    0x09, 0x02, //   USAGE (Button 2), A       -> East
#endif
    0x09, 0x04, //   USAGE (Button 4), X       -> North
    0x09, 0x07, //   USAGE (Button 7), L       -> Trigger Left
    0x09, 0x08, //   USAGE (Button 8), R       -> Trigger Right
    0x14,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x34,       //   PHYSICAL_MINIMUM (0)
    0x45, 0x01, //   PHYSICAL_MAXIMUM (1)
    0x75, 0x01, //   REPORT_SIZE (1)
    0x95, 0x08, //   REPORT_COUNT (8)
    0x81, 0x02, //   INPUT (Data,Var,Abs)

    0x05, 0x01, //   USAGE_PAGE (Generic Desktop)
    0x09, 0x01, //   USAGE (Pointer)
    0xa1, 0x00, //   COLLECTION (Physical)
#ifdef BTN_DIRECTIONS_AS_DPAD
    0x09, 0x39, //     USAGE (Hat switch (Dynamic Value))
    0x15, 0x01, //     LOGICAL_MINIMUM (1)
    0x25, 0x08, //     LOGICAL_MAXIMUM (8)
    0x46, 0x3b, 0x01, // PHYSICAL_MAXIMUM (315)
    0x65, 0x14, //     UNIT (English Rotation, Degrees, 1° Units)
    0x75, 0x04, //     REPORT_SIZE (4)
    0x95, 0x01, //     REPORT_COUNT (1)
    0x81, 0x42, //     INPUT (Data,Var,Abs,Null)
#else
    0x05, 0x09, //     USAGE_PAGE (Button)
    0x09, 0x13, //     USAGE (Button 13), Up     -> Trigger Happy 3
    0x09, 0x14, //     USAGE (Button 14), Down   -> Trigger Happy 4
    0x09, 0x11, //     USAGE (Button 15), Left   -> Trigger Happy 1
    0x09, 0x12, //     USAGE (Button 16), Right  -> Trigger Happy 2
    0x95, 0x04, //     REPORT_COUNT (4)
    0x81, 0x02, //     INPUT (Data,Var,Abs)
#endif
    0xc0,       //   END COLLECTION

#ifdef BTN_DIRECTIONS_AS_DPAD
    0x05, 0x09, //   USAGE_PAGE (Button)
    0x14,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x45, 0x01, //   PHYSICAL_MAXIMUM (1)
    0x64,       //   UNIT (None)
    0x75, 0x01, //   REPORT_SIZE (1)
    0x95, 0x04, //   REPORT_COUNT (4)
#endif
    0x81, 0x03, //   INPUT (Cnst,Var,Abs), Padding to reach two bytes
    0xc0,       // END_COLLECTION

    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x05, // USAGE (Gamepad)
    0xa1, 0x01, // COLLECTION (Application)
    0x85, 0x02, //   REPORT_ID (2)
    0x05, 0x09, //   USAGE_PAGE (Button)
#ifdef BTN_AB_LAYOUT_XBOX
    0x09, 0x02, //   USAGE (Button 2), B       -> East
#else
    0x09, 0x01, //   USAGE (Button 1), B       -> South
#endif
    0x09, 0x05, //   USAGE (Button 5), Y       -> West
    0x09, 0x0b, //   USAGE (Button 11), Select -> Select
    0x09, 0x0c, //   USAGE (Button 12), Start  -> Start
#ifdef BTN_AB_LAYOUT_XBOX
    0x09, 0x01, //   USAGE (Button 1), A       -> South
#else
    0x09, 0x02, //   USAGE (Button 2), A       -> East
#endif
    0x09, 0x04, //   USAGE (Button 4), X       -> North
    0x09, 0x07, //   USAGE (Button 7), L       -> Trigger Left
    0x09, 0x08, //   USAGE (Button 8), R       -> Trigger Right
    0x95, 0x08, //   REPORT_COUNT (8)
    0x81, 0x02, //   INPUT (Data,Var,Abs)

    0x05, 0x01, //   USAGE_PAGE (Generic Desktop)
    0x09, 0x01, //   USAGE (Pointer)
    0xa1, 0x00, //   COLLECTION (Physical)
#ifdef BTN_DIRECTIONS_AS_DPAD
    0x09, 0x39, //     USAGE (Hat switch (Dynamic Value))
    0x15, 0x01, //     LOGICAL_MINIMUM (1)
    0x25, 0x08, //     LOGICAL_MAXIMUM (8)
    0x46, 0x3b, 0x01, // PHYSICAL_MAXIMUM (315)
    0x65, 0x14, //     UNIT (English Rotation, Degrees, 1° Units)
    0x75, 0x04, //     REPORT_SIZE (4)
    0x95, 0x01, //     REPORT_COUNT (1)
    0x81, 0x42, //     INPUT (Data,Var,Abs,Null)
#else
    0x05, 0x09, //     USAGE_PAGE (Button)
    0x09, 0x13, //     USAGE (Button 13), Up     -> Trigger Happy 3
    0x09, 0x14, //     USAGE (Button 14), Down   -> Trigger Happy 4
    0x09, 0x11, //     USAGE (Button 15), Left   -> Trigger Happy 1
    0x09, 0x12, //     USAGE (Button 16), Right  -> Trigger Happy 2
    0x95, 0x04, //     REPORT_COUNT (4)
    0x81, 0x02, //     INPUT (Data,Var,Abs)
#endif
    0xc0,       //   END COLLECTION

#ifdef BTN_DIRECTIONS_AS_DPAD
    0x05, 0x09, //   USAGE_PAGE (Button)
    0x14,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x45, 0x01, //   PHYSICAL_MAXIMUM (1)
    0x64,       //   UNIT (None)
    0x75, 0x01, //   REPORT_SIZE (1)
    0x95, 0x04, //   REPORT_COUNT (4)
#endif
    0x81, 0x03, //   INPUT (Cnst,Var,Abs), Padding to reach two bytes
    0xc0,       // END_COLLECTION
};

struct {
    uint8_t report_id;
    uint8_t report_data[2];
} snes_reports[NUM_CONTROLLERS] __attribute__((packed));
uint16_t button_states[NUM_CONTROLLERS], last_states[NUM_CONTROLLERS];
uint8_t usb_report_id = 0;
static inline uint8_t snes_button_read();
volatile uint8_t timer0_alert = 0, usb_in1_polled = 0;
int __attribute__((noreturn)) main(void) {
    wdt_enable(WDTO_1S);
#ifdef POWERSAVE
    power_twi_disable();
    power_timer1_disable();
    power_spi_disable();
    ADCSRA &= (1 << ADEN);
    power_adc_disable();
    sleep_enable();
    set_sleep_mode(SLEEP_MODE_IDLE);
#endif

    // Prepare timer 0 for SNES pad read wakeup
    OCR0A = 120;
    TIMSK0 = (1 << OCIE0A);

    // Set up SNES pad pins
    PORTB = (PORTB & ~((1 << PORTB0) | (1 << PORTB1))) | (1 << PORTB2) | (1 << PORTB3);
    DDRB = (DDRB | (1 << DDB0) | (1 << DDB1)) & ~((1 << DDB2) | (1 << DDB3));
    LED_DIR_REG |= LED_DIR_PIN;

    for (uint8_t i = 0; i < NUM_CONTROLLERS; i++) {
        snes_reports[i].report_id = i + 1;
    }

    sei();
    usbInit();
    // Enforce re-enumeration, do this while interrupts are disabled.
    usbDeviceDisconnect();
    // Fake USB disconnect for > 250 ms
    _delay_ms(260);
    usbDeviceConnect();
    for(;;) {
        wdt_reset();
#ifdef POWERSAVE
        WDTCSR |= (1 << WDIE);
#endif
        usbPoll();
        char unknown_interrupt = 1;
        if (usb_in1_polled) {
            usb_in1_polled = 0;
            // USB endpoint 1 was polled. (Re)set timer to read controller just before next poll.
            // 1000 ms : (16.000.000 Hz : 1024 : 120) = 7,68 ms
            TCNT0 = 0;
            TCCR0A = (1 << CS02) | (1 << CS00);
            if (!last_states[usb_report_id]) {
                LED_OFF;
            }
            unknown_interrupt = 0;
        }
        if (timer0_alert) {
            timer0_alert = 0;
            // Timer rang. Read controller once if buffer is free.
            if (usbInterruptIsReady()) {
                LED_ON;
                if (snes_button_read()) {
                    usbSetInterrupt(
                        (void *)&snes_reports[usb_report_id], sizeof(snes_reports[0]));
                }
            }
            unknown_interrupt = 0;
        }
        if (unknown_interrupt) {
            // Should be nothing else than watchdoch and other USB related wakeups.
            // Creates a slow flashing if idle and rapid flashing on USB traffic.
            LED_TOGGLE;
        }
#ifdef POWERSAVE
        sleep_enable();
        sleep_cpu();
        sleep_disable();
#endif
    }
}

uint8_t idle_rate = 0;
usbMsgLen_t usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data;

    // This code is taken from the V-USB HID mouse example.
    if ((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        if (rq->bRequest == USBRQ_HID_GET_REPORT) {
            usbMsgPtr = (void *)&(snes_reports[usb_report_id]);
            return sizeof(snes_reports[usb_report_id]);
        } else if (rq->bRequest == USBRQ_HID_GET_IDLE) {
            usbMsgPtr = &idle_rate;
            return 1;
        } else if (rq->bRequest == USBRQ_HID_SET_IDLE) {
            idle_rate = rq->wValue.bytes[1];
        }
    }
    return 0;
}

static inline uint8_t snes_button_read() {
    memset(button_states, 0x00, sizeof(button_states));
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
        for (uint8_t i = 0; i < NUM_CONTROLLERS; i++) {
            button_states[i] >>= 1;
            button_states[i] |= (((uint16_t)(~PINB)) << (11 - PINB2 - i)) & 0x0800;
        }
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
    uint8_t i = usb_report_id;
    do {
        i = (i + 1) % NUM_CONTROLLERS;
        if (last_states[i] != button_states[i]) {
            uint8_t *report_data = snes_reports[i].report_data;
            uint16_t button_state = button_states[i];
            report_data[0] = (button_state & 0x000f) | (button_state & 0x0f00) >> 4;
#ifdef BTN_DIRECTIONS_AS_DPAD
            report_data[1] = pgm_read_byte(&dpad_keys_to_rotation[(button_state & 0x00f0) >> 4]);
#else
            report_data[1] = (button_state & 0x00f0) >> 4;
#endif
            last_states[i] = button_state;
            usb_report_id = i;
            return 1;
        }
    } while (i != usb_report_id);
    return 0;
}

#ifdef POWERSAVE
ISR(WDT_vect) {
}
#endif

ISR(TIMER0_COMPA_vect) {
    TCCR0A = 0;
    timer0_alert = 1;
}
