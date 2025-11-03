#ifndef _CONFIG_H
#define _CONFIG_H 1

/*
Oscilloscope readings show that the controler response times are somwhere
at 100 nS. We can ignore Nintendo's original timing an respond to the USB
interrupt way faster.
Settings TIMING_QUICK sets the clock cycle to approximately 1 µS, reducing one
button readout from ~160 µS to ~40 µS.
*/
#define TIMING_QUICK 1
#define POWERSAVE 1
//#define BTN_AB_LAYOUT_XBOX 1
//#define BTN_DIRECTIONS_AS_DPAD 1

#endif
