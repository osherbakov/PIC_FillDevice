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


#define TIMER_300US_PERIOD ( ((XTAL_FREQ/4) * (HQ_BIT_TIME_US/2)) / 16)
#define TIMER_DELAY_EDGE ( TIMER_300US_PERIOD + (TIMER_300US_PERIOD / 2 ) - 4)
#define TIMER_WAIT_EDGE ( TIMER_300US_PERIOD - 4 )

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

const unsigned char oneBits[] = {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4};
unsigned char CountOnes(unsigned char x)
{
    unsigned char result;
    result = oneBits[x&0x0f];
    result += oneBits[x>>4];
    return result;
}

// Decode the received byte with Hamming error correction bits.
char	DecodeByte(byte data)
{
	char result, value, i, num_ones;
	result = 0;
	value = 8;
	for(i = 0; i < 10; i++)
	{
		num_ones = CountOnes(hamming_table[i] ^ data);
		if(value > num_ones) { 
			value = num_ones; 
			result = i;
			if(value == 0) break;
		}
	}
	return result;
}



// HQ data - KKHHMMSSDDDYYM
byte hq_data[14];

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
static char ret_value;
static char curr_bit;

// WaitEdge - returns immediately when the Edge is detected, or after timeout
//  returns 0 - if LOW->HIGH ("0") is detected
//  returns 1 - if HIGH->LOW ("1") was detected
//  returns -1 - if the timeout occured
static char WaitEdge(unsigned char timeout)
{
	ret_value = current_pin;
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

// WaitTimer - return ONLY after the timeout expired, returning the number of edges that happened within that timeout..
//  returns number of edges detected within a specified timeout
//  returns 0 - if no edges detected during that timeout
static char WaitTimer(unsigned char timeout)
{
	ret_value = 0;
	PR6 = timeout;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;
	while(!PIR5bits.TMR6IF)
	{
		hq_pin = HQ_PIN;
		if(hq_pin != current_pin)
		{
			ret_value++;
			current_pin = hq_pin;
		} 
	}
	return ret_value;
}

typedef enum 
{
	INIT,
	IDLE,
	IDLE_1,
	SYNC,
	DATA
} HQ_STATES;

static HQ_STATES State;

static char GetHQTime(void)
{
	T6CON = HQII_TIMER_CTRL;// 1:1 Post, 16 prescaler, on 

	set_timeout(HQ_DETECT_TIMEOUT_MS);	// try to detect the HQ stream within 4 seconds

	current_pin = HQ_PIN;

	State = INIT;
	while(is_not_timeout() )
	{
		switch(State)
		{
		case INIT:	// Wait for no activity on the line and HQ_PIN == LOW for at least 3 clock periods
			if( (WaitTimer(3 * TIMER_WAIT_EDGE) == 0) && (current_pin == LOW))
			{
				State = IDLE;
			}
			break;
	
		case IDLE:
			if( WaitEdge(TIMER_DELAY_EDGE) == 0)	// LOW->HIGH transition detected
			{
  				set_led_off();		// Set LED off
				State = IDLE_1;
			}
			break;

		case IDLE_1:
			if( WaitEdge(TIMER_DELAY_EDGE) == 1 )	// HIGH->LOW transition detected
			{
				sync_word = 0x0001;
				bit_count = 1;
				byte_count = 0;
  				set_led_on();		// Set LED on
				State = SYNC;
				WaitTimer(TIMER_DELAY_EDGE);
			}
			break;

		case SYNC:
			curr_bit = WaitEdge(TIMER_WAIT_EDGE);
			if(curr_bit >= 0) {
				sync_word = (sync_word << 1) | curr_bit;
				bit_count++;
				if(sync_word == SYNC_PATTERN)
				{
					hq_data[byte_count++] = sync_word >> 8;
					hq_data[byte_count++] = sync_word;
					bit_count = 0;
					data_byte = 0;
					State = DATA;
				}
				WaitTimer(TIMER_DELAY_EDGE);
			}
			break;
		case DATA:
			curr_bit = WaitEdge(TIMER_WAIT_EDGE);
			if(curr_bit >= 0) {
				data_byte = (data_byte << 1) | curr_bit;
				bit_count++;
				if(bit_count >= 8)
				{
					hq_data[byte_count++] = data_byte;
					// All data collected - return
					if(byte_count >= MIN_DATA_FRAME_SIZE)
					{
						set_led_off();		// Set LED off
						return ST_OK;
					}
					bit_count = 0;
					data_byte = 0;
				}
				WaitTimer(TIMER_DELAY_EDGE);
 			}
			break;
		}
	}
	return ST_TIMEOUT;
}

static	char prev;
char ReceiveHQTime(void )
{
	// Config pin as input
	TRIS_HQ_PIN = INPUT;


  	//	1. Find the HQ stream rising edge and
	//  	Start collecting HQ time/date
	if( GetHQTime() != ST_OK)	return ST_TIMEOUT;

	//  2. Find the next time when we will have HQ stream
	ExtractHQDate();
	CalculateNextSecond();

	prev = INTCONbits.GIE;
	INTCONbits.GIE = 0;		// Disable interrupts
	SetRTCDataPart1();

  	//	3. Find the next HQ stream rising edge with interrupts disabled
	while ( WaitTimer(0xF0) != 0 ) {};	// Wait until IDLE
	while(HQ_PIN) {};		// look for the rising edge
	while(!HQ_PIN){};		

  	//  4. Finally, set up the RTC clock on the rising edge
	//	the RTC chain is reset on ACK after writing to seconds register.
	CLOCK_LOW();
	DATA_HI();	
	DelayI2C();
	CLOCK_HI();
	DelayI2C();

	SetRTCDataPart2();
	
	INTCONbits.GIE = prev;		// Enable interrupts
	
	//  5. Get the HQ time again and compare with the current RTC
	if( GetHQTime() != ST_OK)	return ST_ERR;
	ExtractHQDate();
	  
	GetRTCData();
	return ( 
	  		(HQ_Hours == rtc_date.Hours) &&
			(HQ_Minutes == rtc_date.Minutes) &&
		  	(HQ_Seconds == rtc_date.Seconds) && 
  		  		(HQ_JulianDayH == rtc_date.JulianDayH) && 
    			(HQ_JulianDayL == rtc_date.JulianDayL) && 
	    		(HQ_Year == rtc_date.Year) 
	   		) ? ST_DONE : ST_ERR;
}

