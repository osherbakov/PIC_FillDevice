#ifndef	__DELAY_H_
#define __DELAY_H_
#include <delays.h>

extern void	Delay1K(unsigned int cnt);

extern volatile unsigned int timeout_counter;
extern volatile unsigned int seconds_counter;

extern void set_timeout(int timeout_in_ms);

#define is_timeout() (timeout_counter == 0)
#define is_not_timeout() (!timeout_counter != 0)

#define DelayMs(x) 		Delay1K((XTAL_FREQ * ((int)(x))) / 4 )
#define DelayUs(x) 		Delay10TCYx((XTAL_FREQ * ((int)(x))) / (4 * 10) )


#endif		// __DELAY_H_
