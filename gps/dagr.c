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
	SCAN_BUFF_BOX,
	DISABLE_KBD,
	RESET_DAGR,
	SETUP_DAGR,
	CONNECT_MSG,
	ENABLE_KBD,
	COLLECT_STATUS,
	COLLECT_TIME_TFR,
	DONE
} DAGR_PARSER_STATE;

static byte dagr_state;
static byte byte_counter;
static byte word_counter;


// The header data
static int	command;
static int	num_words;
static int	flags;
static byte	is_header;

static unsigned short long dagr_time;
static unsigned short long dagr_date;

#pragma udata big_buffer   // Select large section
static byte rx_buffer[256];
static byte tx_buffer[256];
#pragma udata               // Return to normal section

#define	DEL		(0xFF)		// ASCII controls with High bit set
#define	SOH		(0x81)

#define	BUFF_BOX_STATUS		(253)
#define	LOCK_KBD_COMMAND	(5001)
#define	RESET_COMMAND		(0)
#define	SETUP_COMMAND		(5030)
#define	STATUS_MSG			(5040)
#define	TIME_TRANSFER_MSG	(5101)

#define	RRDY				(0x8000)
#define	REQD				(0x1000)
#define	RCVD				(0x0800)
#define	ACKD				(0x0200)
#define	CONNECT				(0x0040)

static int curr_word;	// Current assembled word
static int checksum;	// Running checksum

static byte	prev_symbol;
static byte	valid_packet;
static byte	valid_data;

static byte *p;			// Pointer to get LSB or MSB of the word

#define DATA_POLARITY_DAGR	(DATA_POLARITY_RXTX)	// PLGR/DAGR is connected without Level Shifter

static	int 	*pWords;
static  void CalculateDAGRChecksum(byte *pData, unsigned char nWords)
{
	int	checksum = 0;
	pWords = (int *) pData;
	while(nWords--) {
		checksum += *pWords++;
	}
	*pWords = -checksum;
}	

static	void CalculateWord(byte symb)
{
	// Place the received byte into a proper half of the word
	p = (byte *) &curr_word;
	if(byte_counter & 0x01) {
		p[1] = symb;
	}else {
		p[0] = symb;
	}	

	// Process words differently depending on where they are - in header or data block	
	if(byte_counter & 0x01) {
		checksum += curr_word;
		if(is_header) {
			if(word_counter == 0 ) { command = curr_word; }
			else if(word_counter == 1 ) { num_words = curr_word; }
			else if(word_counter == 2 ) { flags = curr_word; }
			else if(word_counter == 3 ) {
				// Last word in the header - check for checksum
				if(checksum != 0) {
					 valid_packet = 0;	// Start searching for the packet again
				}else {
					is_header = 0;
					word_counter = -1;	// It will be incremented later on
					checksum = 0;
					// Check if this is a "header-only" message
					if(num_words == 0) valid_data = 1;
				}	 
			}	  
		}else {
			rx_buffer[word_counter] = curr_word;
			// Here is the checksum after last word - Check for data validity
			if( word_counter == num_words ) {
				if(checksum != 0) {
					valid_packet = 0;	// Start searching for the packet again
				}else {
					valid_data = 1;
				}	 
			}		
		}	
		word_counter++;
	}
	byte_counter++;
}
	
static void  ExtractDAGRDate(void)
{
	p = (byte *) &dagr_time;
	rtc_date.Seconds	= *p++;
	rtc_date.Minutes	= *p++;
	rtc_date.Hours		= *p++;

	p = (byte *) &dagr_date;
	rtc_date.Century	= 0x20;
	rtc_date.Year		= *p++;
	rtc_date.Month		= *p++;
	rtc_date.Day		= *p++;
  	CalculateJulianDay();
  	CalculateWeekDay();
}

static void FindMessageStart(byte new_symbol)
{
	if( (new_symbol == SOH) && (prev_symbol == DEL)) {
		valid_packet = 1;
		byte_counter = 0;
		word_counter = 0;
		num_words = 0;
		checksum = 0x81FF;
		is_header = 1;
		valid_data = 0;
	}
	prev_symbol = new_symbol;
}	

void process_dagr_init(void)
{
	valid_packet = 0;
	valid_data = 0;
	prev_symbol = 0;
	dagr_state = INIT;
}	

void process_dagr_symbol(byte new_symbol)
{
	if(dagr_state == INIT) {
		valid_packet = 0;
		valid_data = 0;
		prev_symbol = 0;
		dagr_state = SCAN_BUFF_BOX;
	}	
	// Make sure that the packet starts with proper sequence
	if(valid_packet == 0) {
		FindMessageStart(new_symbol);
		return;
	}
	
	// The valid packet start sequence was detected
	// Process every symbol until we collect a valid message.
	// In case of "header-only" message all info will be in 
	// static int	command;
	// static int	num_words;
	// static int	flags;

	CalculateWord(new_symbol);
	if(valid_data == 0) return;

	switch(dagr_state)
	{
	case INIT:	
		break;

	case SCAN_BUFF_BOX:
		break;

	case DISABLE_KBD:
		break;

	case RESET_DAGR:
		break;

	case SETUP_DAGR:
		break;
		
	case CONNECT_MSG:
		break;

	case ENABLE_KBD:
		break;

	case COLLECT_STATUS:
		break;

	case COLLECT_TIME_TFR:
		break;
		
	case DONE:
		break;
		
	default:
		break;
	}
	valid_packet = 0;
}

static unsigned char ch;
static unsigned char *p_date;
static unsigned char *p_time;

static char SetupDAGR(void)
{
	set_timeout(DAGR_DETECT_TIMEOUT_MS);  
  	set_led_off();		// Set LED off

	// Configure the EUSART module
  	open_eusart(BRREG_DAGR, DATA_POLARITY_DAGR);	
  		
	dagr_state = INIT;
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
				process_dagr_symbol(ch);
			}
			// All data collected - report success
			if(dagr_state == DONE){
  				set_led_off();		// Set LED off
				return ST_OK;
			}
		}
	}
  	close_eusart();
	set_led_off();		// Set LED off
	return ST_TIMEOUT;
}

static char GetDAGRTime(void)
{
	set_timeout(DAGR_DETECT_TIMEOUT_MS);  
  	set_led_off();		// Set LED off

	dagr_state = COLLECT_STATUS;
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
				process_dagr_symbol(ch);
			}
			// All data collected - report success
			if(dagr_state == DONE){
  				set_led_off();		// Set LED off
				return ST_OK;
			}
		}
	}
  	close_eusart();
	set_led_off();		// Set LED off
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
char ReceiveDAGRTime()
{
	//	1. Find the 1PPS rising edge
	if(FindRisingEdge() != ST_OK) return ST_TIMEOUT;

	//  2. Detect DAGR and set it up
	if( SetupDAGR() != ST_OK) return ST_TIMEOUT;
	
	//  3. Start collecting DAGR time/date - once you've got it - there is no turning back!!!
	if( GetDAGRTime() != ST_OK) return ST_TIMEOUT;

	//  3. Calculate the next current time to see if the timimg is consistent.
	ExtractDAGRDate();		// Convert DAGR_Date and DAGR_Time to RTC params
	CalculateNextSecond();	// Result will be in rtc_date structure

	//  4. Get the GPS time again and compare with calculated next second
	if( GetDAGRTime() != ST_OK)	return ST_ERR;
	
	p_time = (unsigned char *) &dagr_time;
	p_date = (unsigned char *) &dagr_date;
	
	// If the time is incorrect - try again
	if( (*p_date++ != rtc_date.Year) ||
  		(*p_date++ != rtc_date.Month) ||
    	(*p_date++ != rtc_date.Day) ||
      		(*p_time++ != rtc_date.Seconds) ||
        	(*p_time++ != rtc_date.Minutes) ||
          	(*p_time++ != rtc_date.Hours)  ) return ST_ERR;
	
	//  5. Time is consistent - prepare everything for the next 1PPS pulse 
	ExtractDAGRDate();
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
	if( GetDAGRTime() != ST_OK)	return ST_ERR;
	GetRTCData();

	p_time = (unsigned char *) &dagr_time;
	p_date = (unsigned char *) &dagr_date;

	return ( 
		(*p_date++ == rtc_date.Year) &&
  		(*p_date++ == rtc_date.Month) &&
    	(*p_date++ == rtc_date.Day) &&
      		(*p_time++ == rtc_date.Seconds) &&
        	(*p_time++ == rtc_date.Minutes) &&
          	(*p_time++ == rtc_date.Hours)  ) ? ST_DONE : ST_ERR;
}
