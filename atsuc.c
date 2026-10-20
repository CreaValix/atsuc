#include <stdint.h>

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

#define STATE_SEND_DATA 0
#define STATE_WAIT_TIMER 1
#define STATE_RESTART_TIMER 2

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
    0x10, // 0 0 0 1 1
    0x50, // 0 0 1 0 5
    0x00, // 0 0 1 1 0
    0x70, // 0 1 0 0 7
    0x80, // 0 1 0 1 8
    0x60, // 0 1 1 0 6
    0x70, // 0 1 1 1 7
    0x30, // 1 0 0 0 3
    0x20, // 1 0 0 1 2
    0x40, // 1 0 1 0 4
    0x30, // 1 0 1 1 3
    0x00, // 1 1 0 0 0
    0x10, // 1 1 0 1 1
    0x50, // 1 1 1 0 5
    0x00, // 1 1 1 1 0
};

PROGMEM const char usbHidReportDescriptor[150] = {
#else
PROGMEM const char usbHidReportDescriptor[116] = {
#endif
    // There are only buttons. So all input is digital.
    0x14,       // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x34,       // PHYSICAL MINIMUM (0)
    0x45, 0x01, // PHYSICAL MAXIMUM (1)
    0x75, 0x01, // REPORT_SIZE (1)

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
    0x95, 0x04, //   REPORT_COUNT (4)
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
    0x81, 0x02, //     INPUT (Data,Var,Abs)
#endif
    0xc0,       //   END COLLECTION

#ifdef BTN_DIRECTIONS_AS_DPAD
    0x05, 0x09, //   USAGE_PAGE (Button)
    0x14,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x45, 0x01, //   PHYSICAL MAXIMUM (1)
    0x75, 0x01, //   REPORT_SIZE (1)
    0x95, 0x04, //   REPORT_COUNT (4)
    0x64,       //   UNIT (None)
#endif
#ifdef BTN_AB_LAYOUT_XBOX
    0x09, 0x01, //   USAGE (Button 1), A       -> South
#else
    0x09, 0x02, //   USAGE (Button 2), A       -> East
#endif
    0x09, 0x04, //   USAGE (Button 4), X       -> North
    0x09, 0x07, //   USAGE (Button 7), L       -> Trigger Left
    0x09, 0x08, //   USAGE (Button 8), R       -> Trigger Right
    0x81, 0x02, //   INPUT (Data,Var,Abs)
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
    0x95, 0x04, //   REPORT_COUNT (4)
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
    0x81, 0x02, //     INPUT (Data,Var,Abs)
#endif
    0xc0,       //   END COLLECTION

#ifdef BTN_DIRECTIONS_AS_DPAD
    0x05, 0x09, //   USAGE_PAGE (Button)
    0x14,       //   LOGICAL_MINIMUM (0)
    0x25, 0x01, //   LOGICAL_MAXIMUM (1)
    0x45, 0x01, //   PHYSICAL MAXIMUM (1)
    0x75, 0x01, //   REPORT_SIZE (1)
    0x95, 0x04, //   REPORT_COUNT (4)
    0x64,       //   UNIT (None)
#endif
#ifdef BTN_AB_LAYOUT_XBOX
    0x09, 0x01, //   USAGE (Button 1), A       -> South
#else
    0x09, 0x02, //   USAGE (Button 2), A       -> East
#endif
    0x09, 0x04, //   USAGE (Button 4), X       -> North
    0x09, 0x07, //   USAGE (Button 7), L       -> Trigger Left
    0x09, 0x08, //   USAGE (Button 8), R       -> Trigger Right
    0x81, 0x02, //   INPUT (Data,Var,Abs)
    0x81, 0x03, //   INPUT (Cnst,Var,Abs), Padding to reach two bytes
    0xc0,       // END_COLLECTION
};

struct {
    uint8_t report_id;
    uint16_t button_state;
} snes_reports[2] __attribute__((packed));
uint8_t usb_report_id = 0;
static inline void snes_button_read();
uint8_t led = 0;
volatile uint8_t snes_read_status = STATE_SEND_DATA;
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
    OCR0A = 110;
    TIMSK0 = (1 << OCIE0A);

    // Set up SNES pad pins
    PORTB = (PORTB & ~((1 << PORTB0) | (1 << PORTB1))) | (1 << PORTB2) | (1 << PORTB3);
    DDRB = (DDRB | (1 << DDB0) | (1 << DDB1)) & ~((1 << DDB2) | (1 << DDB3));
    LED_DIR_REG |= LED_DIR_PIN;

    snes_reports[0].report_id = 1;
    snes_reports[1].report_id = 2;

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
        if (usbInterruptIsReady() && snes_read_status == STATE_SEND_DATA) {
            // USB TX buffer was just sent. Wait 7 ms and refill it.
#ifdef POWERSAVE
            sei();
#endif
            // 1000 ms : (16.000.000 Hz : 1024 : 110) = 7,04 ms
            TCNT0 = 0;
            TCCR0A = (1 << CS02) | (1 << CS00);
            snes_read_status = STATE_WAIT_TIMER;
        } else if (snes_read_status == STATE_RESTART_TIMER) {
            LED_ON;
            snes_button_read();
            usbSetInterrupt((void *)&(snes_reports[usb_report_id]), sizeof(snes_reports[0]));
            if (!snes_reports[usb_report_id].button_state) {
                LED_OFF;
            }
            usb_report_id = (usb_report_id + 1) % 2;
            snes_read_status = STATE_SEND_DATA;
        }
#ifdef POWERSAVE
        sleep_enable();
        sei();
        sleep_cpu();
        sleep_disable();
        LED_TOGGLE;
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

static inline void snes_button_read() {
    uint16_t *button_state = &snes_reports[usb_report_id].button_state;
    *button_state = 0;
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
        *button_state >>= 1;
        *button_state |= (((uint16_t)(~PINB)) << (11 - PINB2 - usb_report_id)) & 0x0800;
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
#ifdef BTN_DIRECTIONS_AS_DPAD
    *button_state =
        (*button_state & 0xff0f) |
        pgm_read_byte(&dpad_keys_to_rotation[(*button_state >> 4) & 0x000f]);
#endif
}

#ifdef POWERSAVE
ISR(WDT_vect) {
}
#endif

ISR(TIMER0_COMPA_vect) {
    TCCR0A = 0;
    if (snes_read_status == STATE_WAIT_TIMER)
        snes_read_status = STATE_RESTART_TIMER;
}
