#include "config.h"
#include "rtc.h"
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

  pinWrite(PIN_B, LOW);	// Set all pins to LOW
  pinWrite(PIN_C, LOW);	
  pinWrite(PIN_D, LOW);	
  pinWrite(PIN_E, LOW);	
  pinWrite(PIN_F, LOW);	

  for(i = 0; i < 64; i++)
  {
    pinWrite(PIN_F, LOW);	
    pinWrite(PIN_B, HIGH);
    DelayUs(100);

    pinWrite(PIN_B, LOW);	
    pinWrite(PIN_C, HIGH);
    DelayUs(100);
    
    pinWrite(PIN_C, LOW);	
    pinWrite(PIN_D, HIGH);
    DelayUs(100);

    pinWrite(PIN_D, LOW);	
    pinWrite(PIN_E, HIGH);
    DelayUs(100);

    pinWrite(PIN_E, LOW);	
    pinWrite(PIN_F, HIGH);
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

// Test DAGR message decoding
const char TestInput[] = {
0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x84,0x3C,0x62,0x00,0x00,0x00,0x47,
0x57,0x20,0x44,0x20,0x20,0x9B,0xD2,0xFF,0x81,0xED,0x13,0x07,0x00,0x00,0x80,0x0D,0xEA,0x66,0x47,0x01,
0x50,0xA3,0xE5,0x00,0x00,0x05,0x04,0x35,0x22,0x17,0x13,0xA5,0x49,0xFF,0x81,0xFD,0x00,0x00,0x00,0x00,
0x90,0x04,0xED,0xFF,0x81,0xB0,0x13,0x58,0x00,0x00,0x80,0xF9,0xE9,0x66,0x47,0x25,0x52,0xBF,0xBB,0x00,
0x00,0x00,0x00,0x0A,0x00,0x20,0x53,0x20,0x46,0x20,0x47,0xC8,0x14,0x00,0x00,0x16,0xAA,0x00,0x00,0x89,
0x6A,0x13,0x7D,0x00,0x00,0x00,0x00,0x2F,0x00,0x00,0x00,0x04,0x00,0x05,0x00,0x16,0x00,0x00,0x00,0x11,
0x00,0x0C,0x00,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,
0x00,0x01,0x00,0x03,0x00,0x84,0x20,0xD3,0x01,0x00,0x00,0x84,0xBC,0x92,0x36,0x00,0x00,0x01,0x00,0x00,
0x00,0x82,0x43,0x13,0x09,0x00,0x00,0x06,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x3C,0x1B,0x00,0x2E,0x48,
0x1B,0x00,0x2E,0x65,0x1B,0x00,0x25,0x91,0x19,0x20,0x1C,0xBE,0x1B,0x00,0x30,0x01,0x00,0x01,0x00,0x01
};

extern void process_dagr_init(unsigned char initial_state);
extern void process_dagr_symbol(unsigned char new_symbol);

char TestDAGRDecode()
{
	int i;
	process_dagr_init(0);
	for(i = 0; i < sizeof(TestInput); i++) {
		process_dagr_symbol(TestInput[i]);
	}	
	
		
	return 0;
}	