#include "config.h"
#include "rtc.h"
#include "clock.h"
#include "delay.h"
#include "string.h"
#include "Fill.h"
#include "controls.h"

// Test Files

// Test all pins - A, B, C, D, E, F
//
char TestAllPins()
{
  byte i;
  
	set_pin_a_as_gnd();				//  Set GND on Pin A
	set_pin_f_as_io();
	
	pinMode(PIN_B, OUTPUT);		// make pin input
	pinMode(PIN_C, OUTPUT);		// make pin input
	pinMode(PIN_D, OUTPUT);		// make pin input
	pinMode(PIN_E, OUTPUT);		// make pin input
	pinMode(PIN_F, OUTPUT);		// make pin input

  digitalWrite(PIN_B, LOW);	// Set all pins to LOW
  digitalWrite(PIN_C, LOW);	
  digitalWrite(PIN_D, LOW);	
  digitalWrite(PIN_E, LOW);	
  digitalWrite(PIN_F, LOW);	

  for(i = 0; i < 64; i++)
  {
    digitalWrite(PIN_F, LOW);	
    digitalWrite(PIN_B, HIGH);
    DelayUs(100);

    digitalWrite(PIN_B, LOW);	
    digitalWrite(PIN_C, HIGH);
    DelayUs(100);
    
    digitalWrite(PIN_C, LOW);	
    digitalWrite(PIN_D, HIGH);
    DelayUs(100);

    digitalWrite(PIN_D, LOW);	
    digitalWrite(PIN_E, HIGH);
    DelayUs(100);

    digitalWrite(PIN_E, LOW);	
    digitalWrite(PIN_F, HIGH);
    DelayUs(100);
  }
  
  return 0;
}

// Test RTC functions
// Date 0 - 01 Jan 2012
// Date 1 - 10 Jan 2012
// Date 2 - 26 Feb 2012
// Date 3 - 28 Feb 2012
// Date 4 - 29 Feb 2012
// Date 5 - 01 Mar 2012
// Date 6 - 15 Dec 2012
// Date 7 - 30 Dec 2012
// Date 8 - 31 Dec 2012
// Date 9 - 01 Jan 2013

// Date 10 - 10 Jan 2013
// Date 11 - 26 Feb 2013
// Date 12 - 28 Feb 2013
// Date 13 - 01 Mar 2012
// Date 14 - 15 Dec 2012
// Date 15 - 30 Dec 2012
// Date 15 - 31 Dec 2012
// Date 17 - 01 Jan 2014
// void CalculateWeekDay()     // Year, Month, Day -> Week Day
// void CalculateMonthAndDay() // Year, Julian Day -> Month, Day
// void CalculateJulianDay()   // Year, Month, Day -> Julian Day
char TestRTCFunctions()
{
  memset((void *)&rtc_date, 0, sizeof(rtc_date));
  rtc_date.Century = 0x20;
  rtc_date.Year = 0x12;   // Year = 2012
  
  // Date 0
  rtc_date.Month = 0x01;  // January
  rtc_date.Day = 0x01;    // 01
  CalculateWeekDay();
  if(rtc_date.WeekDay !=  0x07)  return 1;  // Sunday
  CalculateJulianDay();
  if( (rtc_date.JulianDayH != 0x00) && 
    (rtc_date.JulianDayL != 0x01)) return 1; // JD = 01
  // Convert back from JD to Month and Day
  CalculateMonthAndDay();
  if( (rtc_date.Month != 0x01) && 
    (rtc_date.Day != 0x01)) return 1; // Jan 01

  // Date 1
  rtc_date.Month = 0x01;  // January
  rtc_date.Day = 0x10;    // 10
  CalculateWeekDay();
  if(rtc_date.WeekDay !=  0x02)  return 1;  // Tuesday
  CalculateJulianDay();
  if( (rtc_date.JulianDayH != 0x00) && 
    (rtc_date.JulianDayL != 0x10)) return 1; // JD = 01
  // Convert back from JD to Month and Day
  CalculateMonthAndDay();
  if( (rtc_date.Month != 0x01) && 
    (rtc_date.Day != 0x10)) return 1; // Jan 01

  // Date 2
  rtc_date.Month = 0x02;  // February
  rtc_date.Day = 0x26;    // 26
  CalculateWeekDay();
  if(rtc_date.WeekDay !=  0x07)  return 1;  // Sunday
  CalculateJulianDay();
  if( (rtc_date.JulianDayH != 0x00) && 
    (rtc_date.JulianDayL != 0x57)) return 1; // JD = 57
  // Convert back from JD to Month and Day
  CalculateMonthAndDay();
  if( (rtc_date.Month != 0x02) && 
    (rtc_date.Day != 0x26)) return 1; // Feb 26

  // Date 3
  rtc_date.Month = 0x02;  // February
  rtc_date.Day = 0x28;    // 28
  CalculateWeekDay();
  if(rtc_date.WeekDay !=  0x02)  return 1;  // Tuesday
  CalculateJulianDay();
  if( (rtc_date.JulianDayH != 0x00) && 
    (rtc_date.JulianDayL != 0x59)) return 1; // JD = 59
  // Convert back from JD to Month and Day
  CalculateMonthAndDay();
  if( (rtc_date.Month != 0x02) && 
    (rtc_date.Day != 0x28)) return 1; // Feb 28

  // Date 4
  rtc_date.Month = 0x02;  // February
  rtc_date.Day = 0x29;    // 29
  CalculateWeekDay();
  if(rtc_date.WeekDay !=  0x03)  return 1;  // Wednesday
  CalculateJulianDay();
  if( (rtc_date.JulianDayH != 0x00) && 
    (rtc_date.JulianDayL != 0x60)) return 1; // JD = 60
  // Convert back from JD to Month and Day
  CalculateMonthAndDay();
  if( (rtc_date.Month != 0x02) && 
    (rtc_date.Day != 0x29)) return 1; // Feb 29

  // Date 5
  rtc_date.Month = 0x03;  // March
  rtc_date.Day = 0x01;    // 01
  CalculateWeekDay();
  if(rtc_date.WeekDay !=  0x04)  return 1;  // Thursday
  CalculateJulianDay();
  if( (rtc_date.JulianDayH != 0x00) && 
    (rtc_date.JulianDayL != 0x61)) return 1; // JD = 61
  // Convert back from JD to Month and Day
  CalculateMonthAndDay();
  if( (rtc_date.Month != 0x03) && 
    (rtc_date.Day != 0x01)) return 1; // Mar 01

  // Date 6
  rtc_date.Month = 0x12;  // December
  rtc_date.Day = 0x15;    // 15
  CalculateWeekDay();
  if(rtc_date.WeekDay !=  0x06)  return 1;  // Sat
  CalculateJulianDay();
  if( (rtc_date.JulianDayH != 0x03) && 
    (rtc_date.JulianDayL != 0x50)) return 1; // JD = 350
  // Convert back from JD to Month and Day
  CalculateMonthAndDay();
  if( (rtc_date.Month != 0x12) && 
    (rtc_date.Day != 0x15)) return 1; // Dec 15

  // Date 7
  rtc_date.Month = 0x12;  // December
  rtc_date.Day = 0x30;    // 30
  CalculateWeekDay();
  if(rtc_date.WeekDay !=  0x07)  return 1;  // Sun
  CalculateJulianDay();
  if( (rtc_date.JulianDayH != 0x03) && 
    (rtc_date.JulianDayL != 0x65)) return 1; // JD = 365
  // Convert back from JD to Month and Day
  CalculateMonthAndDay();
  if( (rtc_date.Month != 0x12) && 
    (rtc_date.Day != 0x30)) return 1; // Dec 30

  // Date 8
  rtc_date.Month = 0x12;  // December
  rtc_date.Day = 0x31;    // 31
  CalculateWeekDay();
  if(rtc_date.WeekDay !=  0x01)  return 1;  // Mon
  CalculateJulianDay();
  if( (rtc_date.JulianDayH != 0x03) && 
    (rtc_date.JulianDayL != 0x66)) return 1; // JD = 366
  // Convert back from JD to Month and Day
  CalculateMonthAndDay();
  if( (rtc_date.Month != 0x12) && 
    (rtc_date.Day != 0x31)) return 1; // Dec 31


  return 0;  
}



// Test RTC functionality


// Test EEPROM chip type


// Test RTC functionality

char SetupCurrentTime()
{
  rtc_date.Century = 0x20;
  rtc_date.Year = 0x12;   // Year = 2012
  
  rtc_date.Month = 0x02;  // February
  rtc_date.Day = 0x07;    // 07
  rtc_date.Hours = 0x06;
  rtc_date.Minutes =0x07;
  rtc_date.Seconds =0x15;
  CalculateWeekDay();
  CalculateJulianDay();
  
  SetRTCData();
  
  return 0;
}


// Test Manchester Encoding/Decoding
typedef enum {
	INIT = 0,
	GET_FLAG_EDGE,
	COMP_FLAG,
	GET_FLAG,
	GET_SAMPLE_EDGE,
	COMP_SAMPLE,
	GET_SAMPLE,
	GET_DATA_EDGE,
	COMP_DATA,
	GET_DATA,
	SEND_START_FLAG,
	SEND_DATA,
	SEND_END_FLAG,
	DONE,
	TIMEOUT
} HDLC_STATES;



#define PERIOD_CNTR   		(200)
#define HALF_PERIOD_CNTR   	(PERIOD_CNTR/2)
#define SAMPLE_CNTR 		(133)	// PERIOD_CNTR * 0.66
#define TIMEOUT_CNTR   		(PERIOD_CNTR * 5)
#define	FLAG				(0x7E)
#define	PRE_FLAG			(0xFC)	// Flag one bit before

volatile int 	Timer;
volatile char	PIN;
volatile char	TimerFlag;

char RxHDLCData(unsigned char *pData)
{
	HDLC_STATES			st;
	unsigned char 		byte_count;
	unsigned char		bit_count;
	unsigned char		stuff_count;
	unsigned char		rcvd_bit;
	unsigned char		rcvd_byte;
	
	unsigned char		prev_PIN;
	
	st = INIT;
	
	while(1) {
		switch(st) {
		  case INIT:
			bit_count 	= 0;
			byte_count	= 0;
			rcvd_bit 	= 0;
			rcvd_byte 	= 0;
			stuff_count = 0;
			prev_PIN	= PIN;
			
			Timer	= TIMEOUT_CNTR;
			st = GET_FLAG_EDGE;
		 	break;
			
		case GET_FLAG_EDGE:
			if(PIN != prev_PIN) {
				st = COMP_FLAG;	
			}else if(TimerFlag)	{
				st = TIMEOUT;
			}
			break;

		case COMP_FLAG:
			TimerFlag = 0;
			Timer = SAMPLE_CNTR;
			prev_PIN = PIN;
			rcvd_byte	=  (rcvd_byte >> 1) | rcvd_bit;
			st = (rcvd_byte == FLAG) ? GET_SAMPLE : GET_FLAG;
			break;

		case GET_FLAG:
			while(!TimerFlag) {}
			TimerFlag = 0;
			Timer = TIMEOUT_CNTR;
			rcvd_bit = (PIN == prev_PIN)? 0x80 : 0x00;
			prev_PIN = PIN;
			st = GET_FLAG_EDGE;
			break;

		case GET_SAMPLE_EDGE:
			if(PIN != prev_PIN) {
				st = COMP_SAMPLE;	
			}else if(TimerFlag)	{
				st = TIMEOUT;
			}
			break;

		case COMP_SAMPLE:
			TimerFlag = 0;
			Timer = SAMPLE_CNTR;
			prev_PIN = PIN;
			rcvd_byte	=  (rcvd_byte >> 1) | rcvd_bit;
			if(++bit_count >= 8) {
				bit_count = 0;
				if(rcvd_byte != FLAG) {
					*pData++ = rcvd_byte;
					bit_count = 0;				
					byte_count++;
					st = GET_DATA;
					break;
				}	
			}
			st = GET_SAMPLE;
			break;

		case GET_SAMPLE:
			while(!TimerFlag) {}
			TimerFlag = 0;
			Timer = TIMEOUT_CNTR;
			rcvd_bit = (PIN == prev_PIN)? 0x80 : 0x00;
			prev_PIN = PIN;
			st = GET_SAMPLE_EDGE;
			break;

		case GET_DATA_EDGE:
			if(PIN != prev_PIN) {
				st = COMP_DATA;	
			}else if(TimerFlag)	{
				st = TIMEOUT;
			}
			break;

		case COMP_DATA:
			TimerFlag = 0;
			Timer = SAMPLE_CNTR;
			prev_PIN = PIN;
			
			// Beginning of bit stuffing
			//  basically, every "0" after five consequtive "1" should be ignored 
			if(rcvd_bit == 0) {
				if(stuff_count >= 5) {
					stuff_count = 0;
					st = GET_DATA;
					break;
				}
				stuff_count = 0;
			}else {		// Increment counter it is "1"
				stuff_count++;
			}
			// End of bit stuffing
			
			rcvd_byte	=  (rcvd_byte >> 1) | rcvd_bit;
			if(++bit_count >= 8) {
				*pData++ = rcvd_byte;
				bit_count = 0;				
				byte_count++;
			}
			st = GET_DATA;
			break;

		case GET_DATA:
			while(!TimerFlag) {}
			TimerFlag = 0;
			Timer = TIMEOUT_CNTR;
			rcvd_bit = (PIN == prev_PIN)? 0x80 : 0x00;
			prev_PIN = PIN;
			st = ((rcvd_bit == 0) && (rcvd_byte == PRE_FLAG) ) ? DONE : GET_DATA_EDGE;
			break;
			
		case TIMEOUT:
			return -1;
			break;

		case DONE:
			return byte_count;
			break;
			
		default:
			break;
		}			
	}	
}	

#define		NUM_INITIAL_FLAGS	(20)
#define		NUM_FINAL_FLAGS		(10)

void TxHDLCData(unsigned char *pData, unsigned char nBytes)
{
	unsigned char 		byte_count;
	unsigned char		bit_count;
	unsigned char		stuff_count;
	unsigned char		tx_byte;
	unsigned char		next_bit;
	st = INIT;
	
	while(1) {
		switch(st) {
		  case INIT:
			bit_count 	= 0;
			byte_count	= 0;
			stuff_count = 0;
			prev_bit	= 0;
			
			TimerFlag = 0;
			Timer	= HALF_PERIOD_CNTR;
			st = SEND_START_FLAG;
		 	break;

		  case SEND_START_FLAG:
		  	byte_count	 = NUM_INITIAL_FLAGS;
		  	while(byte_count-- > 0) {
			  	tx_byte = FLAG;
			  	bit_count = 8;
			  	while(bit_count-- > 0)
			  	{
			  		PIN = ~next_bit;	// Flip the pin
				  	next_bit = tx_byte & 0x01 ? next_bit : ~next_bit;	// For "0" flip the bit, for 1" keep in in the middle of the cell
				  	tx_byte >>= 1;
					while(!TimerFlag) {}
					TimerFlag = 0;
					PIN = next_bit;
					while(!TimerFlag) {}
				}	
			}	
		 	break;

		} 	
	}	
}	
