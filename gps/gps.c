#include "config.h"
#include "gps.h"
#include "rtc.h"
#include "delay.h"
#include "i2c_sw.h"
#include "fill.h"
#include "controls.h"

enum {
	INIT = 0,
	SENTENCE,
	TIME_SEC,
	VALID,
	DELIMITERS,
	DATE,
	DONE
} GPS_PARSER_STATE;

enum {
	GPRMC = 0,
	GPGGA
} SENTENCE_TYPE;

static byte gps_state;
static byte counter;
static unsigned short long gps_time;
static unsigned short long gps_date;

#define DATA_POLARITY  (0x20)

static byte RMS_SNT[] = "GPRMC,";
static byte  symb_buffer[6];	// Buffer to keep the symbols
static byte data_polarity = DATA_POLARITY ;

byte is_equal(byte *p1, byte *p2, byte n)
{
	while(n-- )
	if( *p1++ != *p2++) return 0;
	return 1;
}

static void  ExtractGPSDate(void)
{
	byte *p;	// Pointer to get LSB and MSB of the 24-bit word

	p = (byte *) &gps_time;
	rtc_date.Seconds	= *p++;
	rtc_date.Minutes	= *p++;
	rtc_date.Hours		= *p++;

	p = (byte *) &gps_date;
	rtc_date.Century	= 0x20;
	rtc_date.Year		= *p++;
	rtc_date.Month	= *p++;
	rtc_date.Day		= *p++;
  CalculateJulianDay();
  CalculateWeekDay();
}

static void process_gps_symbol(byte new_symbol)
{
	switch(gps_state)
	{
	case INIT:
		if(new_symbol == '$')
		{
			gps_state = SENTENCE;
			gps_date = 0;	
			gps_time = 0;
			counter = 0;
		}
		break;
	case SENTENCE:
		symb_buffer[counter++] = new_symbol;
		if(counter >= 6)
		{
			if(is_equal(symb_buffer, RMS_SNT, 6))
			{
				gps_state = TIME_SEC;
			}else
			{
				gps_state = INIT;
			}
		}
		break;
	case TIME_SEC:
		if(new_symbol == '.')
		{
			// do nothing - skip
		}else if(new_symbol == ',')
		{
			gps_state = VALID;
		}else
		{
			gps_time =  (gps_time << 4) | (new_symbol - '0');
		}
		break;

	case VALID:
		if(new_symbol == 'A')
		{
			counter = 0;
			gps_state = DELIMITERS;
		}else
		{
			gps_state = INIT;
		}
		break;
		
	case DELIMITERS:
		if(new_symbol == ',')
		{
			counter++;
			if(counter  >= 7)
			{
				gps_state = DATE;
			}
		}
		break;

	case DATE:
		if(new_symbol != ',')
		{
			gps_date =  (gps_date << 4) + (new_symbol - '0');
		}else
		{
			gps_state = DONE;
		}
		break;
	}
}

static char GetGPSTime(void)
{
	// Configure the EUSART module
	SPBRGH1 = 0x00;
	SPBRG1 = BRREG_GPS;
	BAUDCON1 = data_polarity;
	TRIS_GPS_DATA = INPUT;
	TXSTA1 = 0x00;
	RCSTA1 = 0x90;
	gps_state = INIT;
	while(is_not_timeout())
	{
		if(PIR1bits.RC1IF)	// Data is avaiable
		{
			// Get and process received symbol
			process_gps_symbol(RCREG1);
			// overruns? clear it
			if(RCSTA1 & 0x06)
			{
				RCSTA1bits.SPEN = 0;
				RCSTA1bits.SPEN = 1;
			}
			// All data collected - fill RTC struct
			if(gps_state == DONE)
			{
				RCSTA1bits.SPEN = 0;
				ExtractGPSDate();
				return 0;
			}
		}
	}
	RCSTA1bits.SPEN = 0;
	data_polarity ^= DATA_POLARITY;
	return -1;
}

static char FindRisingEdge(void)
{
	while(is_not_timeout())
	{	
		if(!GPS_1PPS) break;
	}
	while(is_not_timeout())
	{	
		if(GPS_1PPS) return 0;
	}
	return -1;
}

char ReceiveGPSTime()
{
	byte *p_date;
	byte *p_time;
	// Config the pin as input
	TRIS_GPS_1PPS = INPUT;

	set_timeout(GPS_DETECT_TIMEOUT_MS);	// try to detect the GPS stream
	//	1. Find the 1PPS rising edge
	if(FindRisingEdge()) 
		  return ST_TIMEOUT;
	//  2. Start collecting GPS time/date
	if( GetGPSTime() )	
		  return ST_TIMEOUT;
	//  3. Calculate the next current time.
	CalculateNextSecond();
	
	INTCONbits.GIE = 0;		// Disable interrupts
	SetRTCDataPart1();

//	4. Find the 1PPS rising edge
	while(GPS_1PPS);	// wait for LOW
	while(!GPS_1PPS);	// wait for HIGH

//  5. Finally, set up the RTC clock - according to the spec,
//	the RTC chain is reset on ACK after writing to seconds register.
	CLOCK_LOW();
	DATA_HI();	
	DelayI2C();
	CLOCK_HI();
	DelayI2C();

	SetRTCDataPart2();
	
  // Reset the 10 ms clock
  rtc_date.MilliSeconds_10 = 0;
 	TMR2 = 0;

	INTCONbits.RBIF = 0;	// Clear bit
	INTCONbits.GIE = 1;		// Enable interrupts
  
//  6. Get the GPS time again and compare with the current RTC
	set_timeout(GPS_DETECT_TIMEOUT_MS);	// try to detect the GPS stream
	if( GetGPSTime() )	
	  return ST_ERR;
	  
	GetRTCData();
	p_time = (byte *) &gps_time;
	p_date = (byte *) &gps_date;

	if(rtc_date.Hours)
	return ( 
		(*p_date++ == rtc_date.Year) &&
  		(*p_date++ == rtc_date.Month) &&
	    	(*p_date++ == rtc_date.Day) &&
      		(*p_time++ == rtc_date.Seconds) &&
        		(*p_time++ == rtc_date.Minutes) &&
          		(*p_time++ == rtc_date.Hours)  ) ? ST_DONE : ST_ERR;
}
