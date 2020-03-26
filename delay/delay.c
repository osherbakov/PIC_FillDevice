/*
 *	Delay functions
 *	See delay.h for details
 *
 *	Make sure this code is compiled with full optimization!!!
 */
#include 	"config.h"
#include	"delay.h"

static unsigned int cnt;
unsigned char	use_pll;

void	DelayMs(unsigned int delay_ms) {
	cnt = use_pll ? (XTAL_FREQ*delay_ms) :((XTAL_FREQ/4)*delay_ms);
	while(cnt > 256)
	{
		Delay1KTCYx(0);
		cnt -= 256;
	}
	if(cnt)
		Delay1KTCYx(cnt);
}	

void	DelayUs(unsigned int delay_us) {
	cnt = use_pll ? (XTAL_FREQ*delay_us)/10 : ((XTAL_FREQ/4)*delay_us)/10;
	Delay10TCYx(cnt);
}	

volatile unsigned int timeout_counter;
volatile unsigned int timeout_limit;
volatile char timeout_flag;
volatile char ms_10;

void set_timeout(unsigned int timeout_ms)
{
	static char  prev;
	DISABLE_IRQ(prev);
	timeout_limit = use_pll ? (timeout_ms * 4)/10 : timeout_ms/10;
	timeout_counter = 0;
	timeout_flag = 0;
	ENABLE_IRQ(prev);
}

void reset_timeout(void)
{
	static char  prev;
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
    
#define 	TIMER_CTRL0 					( (0 << 3) | (1<<2) | (0<<1) )     		// Post = 1:1, Pre = 1:1, ENA 
#define 	TIMER_CTRL1 					( (0 << 3) | (1<<2) | (0<<1) )     		// Post = 1:1, Pre = 1:1, ENA 
#define 	TIMER_CTRL2 					( (0 << 3) | (1<<2) | (1<<1) )     		// Post = 1:1, Pre = 1:16, ENA 
#define 	TIMER_CTRL3 					( (9 << 3) | (1<<2) | (1<<1) )			// Post = 1:10, Pre=1:16, ENA

static unsigned int limit_reg;
void timerSetupBaudrate(unsigned int baudrate)
{
	limit_reg = ((XTAL_FREQ * 1000000L)/baudrate) - 1;
	
	// Highest settings for the timer - 64 MHz with PLL, no scaling
	if(limit_reg <= 255){
		pllEnable(1);
		timerLimit(limit_reg);
		timerSetup(TIMER_CTRL0);
		return;
	}
	// Next settings for the timer - 16 MHz, no scaling
	limit_reg /= 4; 
	if(limit_reg <= 255){
		pllEnable(0);
		timerLimit(limit_reg);
		timerSetup(TIMER_CTRL1);
		return;
	}

	// Next settings for the timer - 16 MHz, 1:16 prescaler
	limit_reg /= 16; 
	if(limit_reg <= 255){
		pllEnable(0);
		timerLimit(limit_reg);
		timerSetup(TIMER_CTRL2);
		return;
	}

	// Next settings for the timer - 16 MHz, 1:16 prescaler, 1:10 postscaler
	limit_reg /= 10; 
	if(limit_reg <= 255){
		pllEnable(0);
		timerLimit(limit_reg);
		timerSetup(TIMER_CTRL3);
		return;
	}
	while(0) {}	// Error loop
}

void	timerSetupPeriodUs(float period_us)
{
	limit_reg = (unsigned int)(XTAL_FREQ * period_us) - 1;
	
	// Highest settings for the timer - 64 MHz with PLL, no scaling
	if(limit_reg <= 255){
		pllEnable(1);
		timerLimit(limit_reg);
		timerSetup(TIMER_CTRL0);
		return;
	}
	// Next settings for the timer - 16 MHz, no scaling
	limit_reg /= 4; 
	if(limit_reg <= 255){
		pllEnable(0);
		timerLimit(limit_reg);
		timerSetup(TIMER_CTRL1);
		return;
	}

	// Next settings for the timer - 16 MHz, 1:16 prescaler
	limit_reg /= 16; 
	if(limit_reg <= 255){
		pllEnable(0);
		timerLimit(limit_reg);
		timerSetup(TIMER_CTRL2);
		return;
	}

	// Next settings for the timer - 16 MHz, 1:16 prescaler, 1:10 postscaler
	limit_reg /= 10; 
	if(limit_reg <= 255){
		pllEnable(0);
		timerLimit(limit_reg);
		timerSetup(TIMER_CTRL3);
		return;
	}
	while(0) {}	// Error loop
}	
