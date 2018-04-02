/*
 *	Delay functions
 *	See delay.h for details
 *
 *	Make sure this code is compiled with full optimization!!!
 */
#include 	"config.h"
#include	"delay.h"

void
Delay1K(unsigned int cnt)
{
	while(cnt > 256)
	{
		Delay1KTCYx(0);
		cnt -= 256;
	}
	if(cnt)
		Delay1KTCYx(cnt);
}

volatile signed int timeout_counter;
volatile char timeout_flag;
volatile char ms_10;

static char  prev;
void set_timeout(int timeout_ms)
{
	DISABLE_IRQ(prev);
	timeout_counter = timeout_ms;
	timeout_flag = 0;
	ENABLE_IRQ(prev);
}

char is_timeout(void)
{
	if(IS_IRQ_DISABLED() && PIR1bits.TMR2IF) {
		ms_10++;
		timeout_counter -= 10;
		if(timeout_counter <= 0) timeout_flag = 1;
		PIR1bits.TMR2IF = 0;	// Clear overflow flag
	}
	return timeout_flag;
}

char is_not_timeout(void)
{
	if(IS_IRQ_DISABLED() && PIR1bits.TMR2IF) {
		ms_10++;
		timeout_counter -= 10;
		if(timeout_counter <= 0) timeout_flag = 1;
		PIR1bits.TMR2IF = 0;	// Clear overflow flag
	}
	return !timeout_flag;
}
     

