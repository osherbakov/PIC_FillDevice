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

void set_timeout(int timeout_in_ms)
{
  INTCONbits.GIE = 0;
  timeout_counter = timeout_in_ms;
  timeout_flag = 0;
  INTCONbits.GIE = 1;
}
     

