#include "config.h"
#include "gps.h"
#include "rtc.h"
#include "delay.h"
#include "i2c_sw.h"
#include "fill.h"
#include "controls.h"
#include "serial.h"
#include <ctype.h>

enum {
	INIT = 0,
	SENTENCE,
	TIME,
	TIME_MSEC,
	VALID,
	DELIMITERS,
	DATE,
	CHECKSUM,
	VALIDITY,
	DONE
} GPS_PARSER_STATE;


#define DATA_POLARITY_GPS	(DATA_POLARITY_RX)	// GPS is connected without Level Shifter

static byte gps_state;
static byte counter;
static unsigned short long gps_time;
static unsigned short long gps_date;
static char running_checksum, saved_checksum, sent_checksum;
static byte *p;			// Pointer to get LSB and MSB of the 24-bit word

static const byte 	RMS_SNT[] = "GPRMC,";
static byte  	symb_buffer[6];			// Buffer to keep the symbols

byte is_equal(byte *p1, const byte *p2, byte n)
{
	while(n-- ) {
		if( toupper(*p1++) != toupper(*p2++)) return 0;
	}	
	return 1;
}

static void  ExtractGPSDate(void)
{
	p = (byte *) &gps_time;
	rtc_date.Seconds	= *p++;
	rtc_date.Minutes	= *p++;
	rtc_date.Hours		= *p++;

	p = (byte *) &gps_date;
	rtc_date.Century	= 0x20;
	rtc_date.Year		= *p++;
	rtc_date.Month		= *p++;
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
			gps_date = 0;	
			gps_time = 0;
			counter = 0;
			running_checksum = 0;
			gps_state = SENTENCE;
			set_led_off();		// Set LED off - GPS sequence start was detected
		}
		break;

	case SENTENCE:
		symb_buffer[counter++] = new_symbol;
		if(counter >= 6)
		{
			if(is_equal(symb_buffer, RMS_SNT, 6)){
				set_led_on();		// Set LED on - GPS RMS sequence was detected
				gps_state = TIME;
			}else{
				gps_state = INIT;
			}
		}
		break;

	case TIME:
		if(new_symbol == '.'){
			gps_state = TIME_MSEC;
		}else if(new_symbol == ','){
			gps_state = VALID;
		}else{
			gps_time =  (gps_time << 4) | ASCIIToHex(new_symbol);
		}
		break;

	case TIME_MSEC:
		if(new_symbol == ','){
			gps_state = VALID;
		}
		break;

	case VALID:
		if(new_symbol == 'A'){
			counter = 0;
			gps_state = DELIMITERS;
		}else{
			gps_state = INIT;
		}
		break;
		
	case DELIMITERS:
		if(new_symbol == ','){
			counter++;
			if(counter  >= 7){
				gps_state = DATE;
			}
		}
		break;

	case DATE:
		if(new_symbol == ','){
			gps_state = CHECKSUM;
		}else{
			gps_date =  (gps_date << 4) + ASCIIToHex(new_symbol);
		}
		break;

	case CHECKSUM:
		if(new_symbol == '*'){
			saved_checksum = running_checksum;
			sent_checksum = 0;
			gps_state = VALIDITY;
		}
		break;

	case VALIDITY:
		if( (new_symbol == '\n') || (new_symbol == '\r')){
			gps_state = (sent_checksum == saved_checksum) ? DONE : INIT;
		}else {
			sent_checksum = (sent_checksum << 4) + ASCIIToHex(new_symbol);
		}
		break;
	}

	// Calculate running checksum on every incoming symbol
	if((new_symbol != '$') && (new_symbol != '*')) {
		running_checksum ^= new_symbol;
	}
}

static unsigned char ch;
static unsigned char *p_date;
static unsigned char *p_time;

static unsigned char baudrate_idx = 0;
static unsigned char baudrates[] = 
{ 
	BRREG_GPS,
	BRREG_GPS1,
	BRREG_GPS2
};	
	

static char GetGPSTime(void)
{
	// Configure the EUSART module
  	open_eusart(baudrates[baudrate_idx], DATA_POLARITY_GPS);
  	// Try the next baudrate
  	baudrate_idx++;
  	if(baudrate_idx >= NUM_ELEMS(baudrates)) baudrate_idx = 0; 
  	
	set_timeout(GPS_DETECT_TIMEOUT_MS);  
  		
	gps_state = INIT;
	while(is_not_timeout())
	{
		if(PIR1bits.RC1IF)	// Data is avaiable
		{
			// Get and process received symbol
			ch = RCREG1;
			// overruns? clear it
			if(RCSTA1 & 0x06){
				RCSTA1bits.CREN = 0;
				RCSTA1bits.CREN = 1;
			}else { // ... or process the symbol
				process_gps_symbol(ch);
			}
			// All data collected - report success
			if(gps_state == DONE){
        		close_eusart();
  				set_led_off();		// Set LED off
				return ST_OK;
			}
		}
	}
  	close_eusart();
	return ST_TIMEOUT;
}

static char FindRisingEdge(void)
{
	// Config the 1PPS pin as input
	ANSEL_GPS_1PPS = 0;
	TRIS_GPS_1PPS = INPUT;
	GPS_1PPS = 0;	

	set_timeout(GPS_DETECT_TIMEOUT_MS);  

	while(is_not_timeout()){	
		if(!GPS_1PPS) break;
	}
	while(is_not_timeout()){	
		if(GPS_1PPS) return ST_OK;
	}
	return ST_TIMEOUT;
}

static  char prev;
char ReceiveGPSTime()
{
	//	1. Find the 1PPS rising edge
	if(FindRisingEdge() != ST_OK) return ST_TIMEOUT;

	//  2. Start collecting GPS time/date - once you've got it - there is no turning back!!!
	if( GetGPSTime() != ST_OK) return ST_TIMEOUT;

	//  3. Calculate the next current time to see if the timimg is consistent.
	ExtractGPSDate();		// Convert GPS_Date and GPS_time to RTC params
	CalculateNextSecond();	// Result will be in rtc_date structure

	//  4. Get the GPS time again and compare with calculated next second
	if( GetGPSTime() != ST_OK)	return ST_ERR;
	
	p_time = (unsigned char *) &gps_time;
	p_date = (unsigned char *) &gps_date;
	
	// If the time is incorrect - try again
	if( (*p_date++ != rtc_date.Year) ||
  		(*p_date++ != rtc_date.Month) ||
    	(*p_date++ != rtc_date.Day) ||
      		(*p_time++ != rtc_date.Seconds) ||
        	(*p_time++ != rtc_date.Minutes) ||
          	(*p_time++ != rtc_date.Hours)  ) return ST_ERR;
	
	//  5. Time is consistent - prepare everything for the next 1PPS pulse 
	ExtractGPSDate();
	CalculateNextSecond();
	if(rtc_date.Valid == 0) return ST_TIMEOUT;

	//	6. Find the 1PPS rising edge with interrupts disabled
	prev = INTCONbits.GIE;
	INTCONbits.GIE = 0;		// Disable interrupts

	SetRTCDataPart1();		// Prepare for the RTC Clock set

	while(GPS_1PPS);	// wait for LOW
	while(!GPS_1PPS);	// wait for HIGH

	//  7. Finally, set up the RTC clock - according to the spec,
	//	the RTC chain is reset on ACK after writing to seconds register.
	CLOCK_LOW();
	DATA_HI();	
	DelayI2C();
	CLOCK_HI();
	DelayI2C();

	SetRTCDataPart2();
	
	INTCONbits.GIE = prev;		// Enable interrupts

	//  8. Get the GPS time again and compare with the current RTC
	if( GetGPSTime() != ST_OK)	return ST_ERR;
	GetRTCData();

	p_time = (unsigned char *) &gps_time;
	p_date = (unsigned char *) &gps_date;

	return ( 
		(*p_date++ == rtc_date.Year) &&
  		(*p_date++ == rtc_date.Month) &&
    	(*p_date++ == rtc_date.Day) &&
      		(*p_time++ == rtc_date.Seconds) &&
        	(*p_time++ == rtc_date.Minutes) &&
          	(*p_time++ == rtc_date.Hours)  ) ? ST_DONE : ST_ERR;
}
