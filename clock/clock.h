#ifndef	__CLOCK_H_
#define __CLOCK_H_
// The header file for the module that performs the adjustment of the
//  microcontroller clock based on the supplied 1PPS wave from the RTC
// The logic is to use the Counter 0 to count number of instruction
// samples between each falling edge of the RTC 1pps signal.
// The ideal count will be 4,000,000 clocks or 0x3D0900.
// We accumulate 8 consecutive samples and make a majority decision
// The margin is 2 samples.
#include "config.h"

#define ONE_SEC_TICKS ( (XTAL_FREQ/4) * 1000000L)

#define	NUM_SAMPLES		(6)		// Number of samples collected
#define NUM_MARGIN		(4)		// Margin number

extern volatile unsigned char curr_msb;	// MSB for the current sample (8 bits)
extern volatile unsigned int curr_lsb;	// LSB for the current sample (16 bits)

extern void InitClockData(void);
extern void UpdateClockData(void);
extern void ProcessClockData(void);

#endif		// __CLOCK_H_
