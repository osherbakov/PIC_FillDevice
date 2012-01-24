#include "config.h"
#include "rtc.h"
#include "i2c_sw.h"
#include "clock.h"


#define RTC_I2C_ADDRESS			(0xD0)
#define RTC_I2C_CONTROL_REG		(0x0E)

static unsigned int month_day[] =  {0, 31, 59, 90,120,151,181,212,243,273,304,334};
static unsigned char week_day[] =  {0,  3,  3,  6,  1,  4,  6,  2,  5,  0,  3,  5};

static unsigned char month, day, year;
void CalculateDay(void)
{
	unsigned int day;
	unsigned char weekday, day10;
	day = (rtc_date.Day >> 4) * 10 + (rtc_date.Day & 0x0F);
	month = (rtc_date.Month & 0x0F); if(rtc_date.Month & 0x10) month += 10;
	year = (rtc_date.Year >> 4) * 10 + (rtc_date.Year & 0x0F);
	
	weekday = 6 + day + year + (year >> 2);

	day += month_day[month-1];
	weekday += week_day[month-1];

	// Check for the leap year
	if ( (year & 0x03) == 0)
	{
		if(month > 2)
		{
			day++;
		}else
		{
			weekday--;
		}
	}
	// Calculate the Julian Day
	rtc_date.JulianDayH = 0;
	while (day >= 100)
	{
		day -= 100;
		rtc_date.JulianDayH++;
	}
  // Present the Julian day in BCD format
	day10 = 0;
	while (day >= 10)
	{
		day -= 10;
		day10 += 0x10;
	}
	rtc_date.JulianDayL =   (day10 + (byte) day);
	
  // caclulat weekday as number from 1 to 7
  while( weekday > 7 )
	{
		weekday -= 7;
	}
	rtc_date.WeekDay = weekday;
}

void SetNextSecond()
{
	// The next transition HIGH->LOW will happen at that time
	  byte sec;
    byte min;
    byte hour;

	// Use current time
  sec = rtc_date.Seconds;
	min = rtc_date.Minutes;
	hour = rtc_date.Hours;

	if(sec >= 0x59)
	{
		sec = 0x00;
    if(min >= 0x59)
		{
			min = 0x00;
			if(hour >= 23)
			{
				// Skip the incrementing - bad time (once a day)
				rtc_date.Valid = 0;
			}else
			{
				hour += 1;
				if( (hour & 0x0F) == 0x0A) hour += 0x06;
			}
		}else
		{
			min += 1;
			// This is a cool way to increment BCD numbers!!!
			if( (min & 0x0F) == 0x0A) min += 0x06;
		}
	}else
	{
		sec += 1;
		// This is a cool way to increment BCD numbers!!!
		if( (sec & 0x0F) == 0x0A) sec += 0x06;
	}
	rtc_date.Hours = hour;
	rtc_date.Minutes = min;
	rtc_date.Seconds = sec;
}

void GetRTCData()
{
	unsigned int 	day;
	unsigned char	day10;
	// Initialize the pointer to 0
	SWStartI2C();
	SWWriteI2C(RTC_I2C_ADDRESS | I2C_WRITE);
	if(SWAckI2C(READ))	{ SWStopI2C(); return; }
	SWWriteI2C(0);	// Register address - 0
	if(SWAckI2C(READ))	{ SWStopI2C(); return; }
	SWStopI2C();

	// Request data from the chip
	SWStartI2C();
	SWWriteI2C(RTC_I2C_ADDRESS | I2C_READ);
	if(SWAckI2C(READ))	{ SWStopI2C(); return; }

	// Address 0 - seconds
	rtc_date.Seconds = SWReadI2C();
	SWAckI2C(ACK);

	// Address 1 - minutes
	rtc_date.Minutes = SWReadI2C();
	SWAckI2C(ACK);

	// Address 2 - hours
	rtc_date.Hours = SWReadI2C();
	SWAckI2C(ACK);

	// Address 3 - Day of the week
	rtc_date.WeekDay = SWReadI2C();
	SWAckI2C(ACK);

	// Address 4 - Date
	rtc_date.Day = SWReadI2C();
	SWAckI2C(ACK);

	// Address 5 - Century/Month
	rtc_date.Month = SWReadI2C() & 0x1F;
	SWAckI2C(ACK);
	rtc_date.Century = 0x20;

	// Address 6 - Year
	rtc_date.Year = SWReadI2C();
	SWAckI2C(NACK);

	SWStopI2C();

	CalculateDay();
	// mark date as valid
	rtc_date.Valid = 1;
}

void SetRTCData()
{
	SetRTCDataPart1();
	SWAckI2C(READ);
	SetRTCDataPart2();
}

void SetRTCDataPart1()
{
  // Disable clock adjustments because we are about to reset RTC
  InitClockData();
  
	SWStartI2C();
	SWWriteI2C(RTC_I2C_ADDRESS | I2C_WRITE);
	if(SWAckI2C(READ))	{ SWStopI2C(); return; }
	SWWriteI2C(0);	// Register address - 0
	if(SWAckI2C(READ))	{ SWStopI2C(); return; }
	// Address 0 - seconds
	SWWriteI2C(rtc_date.Seconds);
}

void SetRTCDataPart2()
{
	
	// Address 1 - minutes
	SWWriteI2C(rtc_date.Minutes);
	SWAckI2C(READ);

	// Address 2 - hours
	SWWriteI2C(rtc_date.Hours);
	SWAckI2C(READ);

	// Address 3 - Day of the week
	SWWriteI2C(rtc_date.WeekDay);
	SWAckI2C(READ);

	// Address 4 - Date
	SWWriteI2C(rtc_date.Day);
	SWAckI2C(READ);

	// Address 5 - Century/Month
	SWWriteI2C(rtc_date.Month);
	SWAckI2C(READ);

	// Address 6 - Year
	SWWriteI2C(rtc_date.Year);
	SWAckI2C(READ);

	SWStopI2C();
}


void SetupRTC()
{
	SWStartI2C();

	SWWriteI2C(RTC_I2C_ADDRESS | I2C_WRITE);
	if(SWAckI2C(READ))	{ SWStopI2C(); return; }
	SWWriteI2C(RTC_I2C_CONTROL_REG);// Control Register address
	if(SWAckI2C(READ))	{ SWStopI2C(); return; }

	// Control Reg - /EOSC No BBSQW, 1HZ wave, SQ wave, no INT
	SWWriteI2C(0);
	SWAckI2C(READ);
	// Status Reg - Disable 32KHz, clear OSF
	SWWriteI2C(0);
	SWAckI2C(READ);

	SWStopI2C();
}

