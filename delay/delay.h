#ifndef	__DELAY_H_
#define __DELAY_H_
#include <delays.h>

extern void	Delay1K(unsigned int cnt);

extern volatile unsigned int seconds_counter;
extern volatile unsigned int timeout_counter;
extern volatile unsigned int timeout_limit;
extern volatile char timeout_flag;
extern volatile char ms_10;

extern void set_timeout(int a);
extern void reset_timeout(void);
extern char is_timeout(void);
extern char is_not_timeout(void);

extern void DelayMs(unsigned int); 
extern void DelayUs(unsigned int); 

#endif		// __DELAY_H_
