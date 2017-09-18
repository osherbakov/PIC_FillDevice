#include "config.h"
#include "delay.h"
#include "controls.h"
#include "rtc.h"
#include "i2c_sw.h"
#include "gps.h"
#include "fill.h"

#define START_FRAME_SIZE	(400/8)		// 400 SYNC bits of all "1"
#define START_FRAME_DATA 	(0xFF)		// The data to be sent during SYNC phase
#define MIN_DATA_FRAME_SIZE	(112/8)		// 112 bits of actual timing data


#define TIMER_300MS_PERIOD ( ((XTAL_FREQ/4) * (HQ_BIT_TIME_US/2)) / 16)
#define TIMER_DELAY_EDGE ( TIMER_300MS_PERIOD + (TIMER_300MS_PERIOD / 2 ) - 4)
#define TIMER_WAIT_EDGE ( TIMER_300MS_PERIOD - 4 )

// The table to translate the BCS digit into the 
// Hamming code to send all timing data
unsigned char hamming_table[] = 
{
	0b00000000,		// 0  - 0x00
	0b11100001,		// 1  - 0xE1
	0b01110010,		// 2  - 0x72
	0b10010011,		// 3  - 0x93
	0b10110100,		// 4  - 0xB4
	0b01010101,		// 5  - 0x55
	0b11000110,		// 6  - 0xC8
	0b00100111,		// 7  - 0x47
	0b11011000,		// 8  - 0xD8
	0b00111001		// 9  - 0x39
};

unsigned char HQ_Hours;
unsigned char HQ_Minutes;
unsigned char HQ_Seconds;
unsigned char HQ_JulianDayH;
unsigned char HQ_JulianDayL;
unsigned char HQ_Year;


#define SYNC_PATTERN		(0x11e9)
#define DEFAULT_FOM			(0x3)

#define DecodeByte(a) 		((a) & 0x0F)
// Decode the received byte with Hamming error correction bits.
// Current implementation just removes top Hamming bits.
// Needs to be expanded
// char	DecodeByte(byte data)
// {
// 	return data &0x0F;
// }



// HQ data - KKHHMMSSDDDYYM
byte hq_data[14];

#pragma tmpdata HQIITmpData
void CalculateHQDate()
{
	byte dataout;
	
	// HaveQuick time packet is made out of 
	//  400 bits of 1, followed by SYNC pattern
	//

	hq_data[0] = SYNC_PATTERN >> 8;
	hq_data[1] = SYNC_PATTERN;

	// Send the HQ data 
	// Hours
	dataout = rtc_date.Hours;
	hq_data[2] = hamming_table[dataout >> 4];
	hq_data[3] = hamming_table[dataout & 0x0F];
	
	// Minutes
	dataout = rtc_date.Minutes;
	hq_data[4] = hamming_table[dataout >> 4];
	hq_data[5] = hamming_table[dataout & 0x0F];

	// Seconds
	dataout = rtc_date.Seconds;
	hq_data[6] = hamming_table[dataout >> 4];
	hq_data[7] = hamming_table[dataout & 0x0F];

	// Day of the year
	dataout = rtc_date.JulianDayH;
	hq_data[8] = hamming_table[dataout & 0x0F];
	dataout = rtc_date.JulianDayL;
	hq_data[9] = hamming_table[(dataout >> 4) & 0x0F];
	hq_data[10] = hamming_table[dataout & 0x0F];

	// Year
	dataout = rtc_date.Year;
	hq_data[11] = hamming_table[dataout >> 4];
	hq_data[12] = hamming_table[dataout & 0x0F];

	// FOM	- Figure of Merit
	hq_data[13] = hamming_table[DEFAULT_FOM];
}
#pragma tmpdata

static void ExtractHQDate(void)
{
	// HQ Hours
	HQ_Hours = (DecodeByte(hq_data[2]) << 4) | DecodeByte(hq_data[3]);
	
	// Minutes
	HQ_Minutes = (DecodeByte(hq_data[4]) << 4) | DecodeByte(hq_data[5]);
	
	// Seconda
	HQ_Seconds = (DecodeByte(hq_data[6]) << 4) | DecodeByte(hq_data[7]);
  	
  	// 3 digits of Julian day
	HQ_JulianDayH = DecodeByte(hq_data[8]);
	HQ_JulianDayL = (DecodeByte(hq_data[9]) << 4) | DecodeByte(hq_data[10]);
  	
  	// Year
	HQ_Year = (DecodeByte(hq_data[11]) << 4) | DecodeByte(hq_data[12]);

	rtc_date.Hours = HQ_Hours;
	rtc_date.Minutes = HQ_Minutes;
	rtc_date.Seconds = HQ_Seconds;
	rtc_date.Century = 0x20;
	rtc_date.Year = HQ_Year;
	rtc_date.JulianDayH = HQ_JulianDayH;
	rtc_date.JulianDayL = HQ_JulianDayL;
	
  	CalculateMonthAndDay();
  	CalculateWeekDay();
}


//*******************************************************************/
// Set of functions to detect Have Quick stream
// and to decode Manchester encoding

static byte current_pin;		// Current state of HQ_PIN pin
static byte bit_count;			// Bit counter
static byte byte_count;			// Bytes counted/index into the table
static unsigned int sync_word;	// The syncronization word
static byte data_byte;

static char hq_pin;


// WaitEdge 
//  returns 0 - if LOW->HIGH ("0") is detected
//  returns 1 - if HIGH->LOW ("1") was detected
//  returns -1 - if the timeout occured
static char WaitEdge(unsigned char timeout)
{
	char ret_value = current_pin;
	PR6 = timeout;	
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;
	while(!PIR5bits.TMR6IF)
	{
		hq_pin = HQ_PIN;
		if(hq_pin != current_pin)
		{
			current_pin = hq_pin;
			return ret_value;
		} 
	}	
	return -1;
}

// WaitTimer 
//  returns 0 - if LOW->HIGH ("0") is detected
//  returns 1 - if HIGH->LOW ("1") was detected
//  returns 2 - if 2 edges were detected while waiting for the timeout
//  returns -1 - if the timeout occured and no edges detected
static char WaitTimer(unsigned char timeout)
{
	char ret_value = -1;
	PR6 = timeout;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;
	while(!PIR5bits.TMR6IF)
	{
		hq_pin = HQ_PIN;
		if(hq_pin != current_pin)
		{
			// There was one edge already
			ret_value = (ret_value >= 0) ? 2 : current_pin;
			current_pin = hq_pin;
		} 
	}
	return ret_value;
}

// The state is skewed a little - We expect all "1" to be a SYNC pattern
enum 
{
	INIT,
	IDLE,
	IDLE_1,
	SYNC,
	DATA
} MCODE_STATES;

byte RHQD_State;

static char GetHQTime(void)
{
	T6CON = HQII_TIMER_CTRL;// 1:1 Post, 16 prescaler, on 

	RHQD_State = INIT;
	current_pin = HQ_PIN;

	while(is_not_timeout() )
	{
		switch(RHQD_State)
		{
		case INIT:	// Wait for LOW state for at least 3 clock periods
			if( WaitTimer(3 * TIMER_WAIT_EDGE) < 0 )
			{
				RHQD_State = IDLE;
			}
			break;
	
		case IDLE:
			if( WaitEdge(TIMER_DELAY_EDGE) == 0)
			{
				RHQD_State = IDLE_1;
			}
			break;

		case IDLE_1:
			if( WaitEdge(TIMER_DELAY_EDGE) == 1 )
			{
				sync_word = 0x0001;
				bit_count = 1;
				byte_count = 0;
				RHQD_State = SYNC;
			}
			break;

		case SYNC:
			WaitTimer(TIMER_DELAY_EDGE);
			sync_word = (sync_word << 1) | WaitEdge(TIMER_WAIT_EDGE);
			bit_count++;
			if(sync_word == SYNC_PATTERN)
			{
				// Sanity check - bit_count should be 400 % 256 = 0x90
				hq_data[byte_count++] = sync_word >> 8;
				hq_data[byte_count++] = sync_word;
				bit_count = 0;
				data_byte = 0;
				RHQD_State = DATA;
			}
			break;
		case DATA:
			WaitTimer(TIMER_DELAY_EDGE);
			data_byte = (data_byte << 1) | WaitEdge(TIMER_WAIT_EDGE);
			bit_count++;
			if(bit_count >= 8)
			{
				hq_data[byte_count++] = data_byte;
				// All data collected - return
				if(byte_count >= MIN_DATA_FRAME_SIZE)
				{
					ExtractHQDate();
					return 0;
				}
				bit_count = 0;
				data_byte = 0;
 			}
			break;
		}
	}
	return -1;
}

char ReceiveHQTime(void )
{
	char prev;
	// Config pin as input
	TRIS_HQ_PIN = INPUT;

	set_timeout(HQ_DETECT_TIMEOUT_MS);	// try to detect the HQ stream
  	//	1. Find the HQ stream rising edge and
	//  	Start collecting HQ time/date
	if( GetHQTime() )	
	  return ST_TIMEOUT;
	
	//  2. Find the next time when we will have HQ stream
	CalculateNextSecond();

	prev = INTCONbits.GIE;
	INTCONbits.GIE = 0;		// Disable interrupts
	SetRTCDataPart1();
	
  	//	3. Find the next HQ stream rising edge
	while ( WaitTimer(0xF0) >= 0 ) {};	// Wait until IDLE
	while(HQ_PIN) {};		// look for the rising edge
	while(!HQ_PIN){};		

  	//  4. Finally, set up the RTC clock on the rising edge
	CLOCK_LOW();
	DATA_HI();	
	DelayI2C();
	CLOCK_HI();
	DelayI2C();

	SetRTCDataPart2();
	
  	// Reset the 10 ms clock
  	rtc_date.MilliSeconds_10 = 0;
 
	INTCONbits.RBIF = 0;	// Clear bit
	INTCONbits.GIE = prev;		// Enable interrupts
	
	//  5. Get the HQ time again and compare with the current RTC
	set_timeout(HQ_DETECT_TIMEOUT_MS);	// try to detect the HQ stream again for the final check
	if( GetHQTime() )	
	  return ST_ERR;
	  
	GetRTCData();
	return ( 
  	(HQ_Hours == rtc_date.Hours) &&
		(HQ_Minutes == rtc_date.Minutes) &&
		  (HQ_Seconds == rtc_date.Seconds) && 
  		  	(HQ_JulianDayH == rtc_date.JulianDayH) && 
    		  (HQ_JulianDayL == rtc_date.JulianDayL) && 
    		    (HQ_Year == rtc_date.Year) &&
    		      (0x20 == rtc_date.Century)
		          ) ? ST_DONE : ST_ERR;
}

