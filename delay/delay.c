/*
 *	Delay functions
 *	See delay.h for details
 *
 *	Make sure this code is compiled with full optimization!!!
 */
#include 	"config.h"
#include	"delay.h"

static unsigned int cnt;
void	DelayMs(unsigned int delay_ms) {
	cnt = ( (XTAL_FREQ/4) * delay_ms);
	while(cnt > 256)
	{
		Delay1KTCYx(0);
		cnt -= 256;
	}
	if(cnt)
		Delay1KTCYx(cnt);
}	

void	DelayUs(unsigned int delay_us) {
	cnt = ((XTAL_FREQ/4) * delay_us) / 10;
	Delay10TCYx(cnt);
}	

volatile unsigned int timeout_counter;
volatile unsigned int timeout_limit;
volatile char timeout_flag;
volatile char ms_10;

static char  prev;
void set_timeout(int timeout_ms)
{
	DISABLE_IRQ(prev);
	timeout_limit = timeout_ms/10;
	timeout_counter = 0;
	timeout_flag = 0;
	ENABLE_IRQ(prev);
}

void reset_timeout(void)
{
	DISABLE_IRQ(prev);
	timeout_counter = 0;
	timeout_flag = 0;
	ENABLE_IRQ(prev);
}

char is_timeout(void)
{
	if(IS_IRQ_DISABLED() && timer10msFlag()) {
		ms_10++;
		if(!timeout_flag) {
			timeout_counter++;
			if(timeout_counter >= timeout_limit) timeout_flag = 1;
		}	
		timer10msClearFlag();	// Clear overflow flag
	}
	return timeout_flag;
}

char is_not_timeout(void)
{
	if(IS_IRQ_DISABLED() && timer10msFlag()) {
		ms_10++;
		if(!timeout_flag) {
			timeout_counter++;
			if(timeout_counter >= timeout_limit) timeout_flag = 1;
		}	
		timer10msClearFlag();	// Clear overflow flag
	}
	return !timeout_flag;
}
    
#define 	TIMER_CTRL0 					( (0 << 3) | (1<<2) | (0<<0) )     		// Post = 1:1, Pre = 1:1, ENA 
#define 	TIMER_CTRL1 					( (0 << 3) | (1<<2) | (0<<0) )     		// Post = 1:1, Pre = 1:1, ENA 
#define 	TIMER_CTRL2 					( (0 << 3) | (1<<2) | (2<<0) )     		// Post = 1:1, Pre = 1:16, ENA 
#define 	TIMER_CTRL3 					( (9 << 3) | (1<<2) | (2<<0) )			// Post = 1:10, Pre=1:16, ENA
#define 	TIMER_FROM_BAUDRATE0(baudrate) 	(((XTAL_FREQ * 1000000L)/(1L * (baudrate))) - 1 )
#define 	TIMER_FROM_BAUDRATE1(baudrate) 	(((XTAL_FREQ * 1000000L / 4L)/(1L * (baudrate))) - 1 )
#define 	TIMER_FROM_BAUDRATE2(baudrate) 	(((XTAL_FREQ * 1000000L / 4L)/(16L * (baudrate))) - 1 )
#define 	TIMER_FROM_BAUDRATE3(baudrate) 	(((XTAL_FREQ * 1000000L / 4L)/(16L * 10L * (baudrate))) - 1 )

#define 	TIMER_FROM_PERIOD_US0(period) 	(((XTAL_FREQ * (period) / 1L)/1L) - 1 )
#define 	TIMER_FROM_PERIOD_US1(period) 	(((XTAL_FREQ * (period) / 4L)/1L) - 1 )
#define 	TIMER_FROM_PERIOD_US2(period) 	(((XTAL_FREQ * (period) / 4L)/16L) - 1 )
#define 	TIMER_FROM_PERIOD_US3(period) 	(((XTAL_FREQ * (period) / 4L)/(16L * 10L)) - 1 )

static unsigned int ctrl_reg;
void timerSetupBaudrate(unsigned int baudrate)
{
	ctrl_reg = ((XTAL_FREQ * 1000000L)/baudrate) - 1;
	
	// Highest settings for the timer - 64 MHz with PLL, no scaling
	if(ctrl_reg <= 255){
		pllEnable(1);
		timerSetup(ctrl_reg, TIMER_CTRL0);
		return;
	}
	// Next settings for the timer - 16 MHz, no scaling
	ctrl_reg /= 4; 
	if(ctrl_reg <= 255){
		pllEnable(0);
		timerSetup(ctrl_reg, TIMER_CTRL1);
		return;
	}

	// Next settings for the timer - 16 MHz, 1:16 prescaler
	ctrl_reg /= 16; 
	if(ctrl_reg <= 255){
		pllEnable(0);
		timerSetup(ctrl_reg, TIMER_CTRL2);
		return;
	}

	// Next settings for the timer - 16 MHz, 1:16 prescaler, 1:10 postscaler
	ctrl_reg /= 10; 
	if(ctrl_reg <= 255){
		pllEnable(0);
		timerSetup(ctrl_reg, TIMER_CTRL3);
		return;
	}
	while(0) {}	// Error loop
}

void	timerSetupPeriodUs(unsigned int period_us)
{
	ctrl_reg = (XTAL_FREQ * period_us) - 1;
	
	// Highest settings for the timer - 64 MHz with PLL, no scaling
	if(ctrl_reg <= 255){
		pllEnable(1);
		timerSetup(ctrl_reg, TIMER_CTRL0);
		return;
	}
	// Next settings for the timer - 16 MHz, no scaling
	ctrl_reg /= 4; 
	if(ctrl_reg <= 255){
		pllEnable(0);
		timerSetup(ctrl_reg, TIMER_CTRL1);
		return;
	}

	// Next settings for the timer - 16 MHz, 1:16 prescaler
	ctrl_reg /= 16; 
	if(ctrl_reg <= 255){
		pllEnable(0);
		timerSetup(ctrl_reg, TIMER_CTRL2);
		return;
	}

	// Next settings for the timer - 16 MHz, 1:16 prescaler, 1:10 postscaler
	ctrl_reg /= 10; 
	if(ctrl_reg <= 255){
		pllEnable(0);
		timerSetup(ctrl_reg, TIMER_CTRL3);
		return;
	}
	while(0) {}	// Error loop
}	
