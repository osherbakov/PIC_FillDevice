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
	INIT = 0,			// DAGR Init states
	I_SCAN_BUFF_BOX,
	I_DISABLE_KBD,
	I_RESET_DAGR,
	I_SETUP_DAGR,
	I_CONNECT_MSG,
	I_ENABLE_KBD,
	I_COLLECT_STATUS,
	I_COLLECT_TIME_TFR,
	I_DONE,

	COLLECT_TIME_TFR,	// Time Transfer (1PPS) message states
	COLLECT_TIME_DONE,

	COLLECT_STATUS,		// Status (5040) message states
	COLLECT_STATUS_DONE
} DAGR_PARSER_STATE;

static byte dagr_state;
static byte byte_counter;
static byte word_counter;


// The header data
static int	command;
static int	num_words;
static int	flags;

static unsigned short long dagr_time;
static unsigned short long dagr_date;

#pragma udata big_buffer   // Select large section
static byte data_buffer[256];
static byte tx_buffer[256];
#pragma udata               // Return to normal section

#define	DEL		(0xFF)		// ASCII controls with High bit set
#define	SOH		(0x81)
#define	SOHDEL	(0x81FF)

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

static byte DisableKbdCmd[] = {		// 0x1F words
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static byte EnableKbdCmd[] = {
};

static byte SetupDAGRCmd[] = {
};

static byte ConnectDAGRCmd[] = {
};

static int curr_word;		// Current assembled word
static int checksum;		// Running checksum

static byte	valid_start;	// TRUE if proper byte sequence is detected
static byte	valid_header;	// TRUE if valid Header with valid Checksum
static byte	valid_data;		// TRUE if valid Data with valid Checksum 


#define DATA_POLARITY_DAGR	(DATA_POLARITY_RXTX)	// PLGR/DAGR is connected without Level Shifter

static 	byte 	*p;			// Pointer to get LSB or MSB of the word
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
		if(valid_header == 0) {		// Header block
			if(word_counter == 0 ) 		{ command = curr_word; } 	// First word is command
			else if(word_counter == 1 ) { num_words = curr_word; }	// Second - number of data words that follow
			else if(word_counter == 2 ) { flags = curr_word; }		// Third - Flag (ACK, REQ, etc)
			else if(word_counter == 3 ) {							// Fourth - Checksum
				// Last word in the header - check for checksum
				if(checksum != 0) {
					 valid_start = 0;	// Start searching for the packet again
				}else {
					valid_header = 1;	// Mark as valid Header received
					word_counter = -1;	// It will be incremented later on
					checksum = 0;		// For Data block the Checksum initial value is 0x0000
					// Check if this is a "header-only" message
					if(num_words == 0) valid_data = 1;
				}	 
			}	  
		}else {		// Data block that follows valid Header
			data_buffer[word_counter] = curr_word;
			// Here is the checksum after last word - Check for data validity
			if( word_counter == num_words ) {
				if(checksum != 0) {
					valid_start = 0;	// Start searching for the packet again
				}else {
					valid_data = 1;
				}	 
			}		
		}	
		word_counter++;
	}
	byte_counter++;
}

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

// Send ACK for the specified command
static  void SendDAGRACK(int Command, int Flags)
{
	tx_buffer[0] = 	DEL;
	tx_buffer[1] = 	SOH;
	tx_buffer[2] = 	Command & 0x00FF;
	tx_buffer[3] = 	(Command >> 8) & 0x00FF;
	tx_buffer[4] = 	0;		// No data
	tx_buffer[5] = 	0;		//		follows...
	tx_buffer[6] = 	Flags & 0x00FF;
	tx_buffer[7] = 	(Flags >> 8) & 0x00FF;
	CalculateDAGRChecksum(&tx_buffer[0], 4)
	tx_eusart_flush();
	DelayMs(12);
	tx_eusart_async(tx_buffer, 4 * 2 + 2);
}

// Send the Command 
static  void SendDAGRCmd(int Command, int Flags, byte *pData, byte nWords)
{
	tx_buffer[0] = 	DEL;
	tx_buffer[1] = 	SOH;
	tx_buffer[2] = 	Command & 0x00FF;
	tx_buffer[3] = 	(Command >> 8) & 0x00FF;
	tx_buffer[4] = 	nWords;		// Number of words
	tx_buffer[5] = 	0;			//		to follow...
	tx_buffer[6] = 	Flags & 0x00FF;
	tx_buffer[7] = 	(Flags >> 8) & 0x00FF;
	CalculateDAGRChecksum(&tx_buffer[0], 4);
	if(nWords) {
		memcpy((void *)tx_buffer[10],(void *) pData,  nWords * 2);
		CalculateDAGRChecksum(&tx_buffer[10], nWords);
		nWords++;
	}
	tx_eusart_flush();
	DelayMs(12);
	tx_eusart_async(tx_buffer, 4 * 2 + 2 + nWords * 2);
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

static byte	prev_symbol;
static void FindMessageStart(byte new_symbol)
{
	if( (new_symbol == SOH) && (prev_symbol == DEL)) {
		valid_start = 1;
		byte_counter = 0;
		word_counter = 0;
		checksum = SOHDEL;	// Initial value for the checksum
		valid_header = 0;
		valid_data = 0;
	}
	prev_symbol = new_symbol;
}	

void process_dagr_init(void)
{
	valid_start = 0;
	valid_header = 0;
	valid_data = 0;
	prev_symbol = 0;
	dagr_state = I_SCAN_BUFF_BOX;
}	

void process_dagr_symbol(byte new_symbol)
{
	// Make sure that the packet starts with proper sequence
	// If no start is detected - do not proceed further
	if(valid_start == 0) {
		FindMessageStart(new_symbol);
		return;
	}
	
	// The valid packet start sequence was detected
	// Process every symbol until we collect a valid message (Header+Data).
	// In case of "header-only" message all info will be in 
	// static int	command;
	// static int	num_words;
	// static int	flags;
	CalculateWord(new_symbol);
	if(valid_data == 0) return;
	// No need to do the processing if not all valid data is assembled

	switch(dagr_state)
	{
	case INIT:	
		break;

	case I_SCAN_BUFF_BOX:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))){
			SendDAGRACK(BUFF_BOX_STATUS, (RRDY | RCVD | ACKD) );
			SendDAGRCmd(LOCK_KBD_COMMAND, (RRDY | REQD), DisableKbdCmd, 0x1F);
			dagr_state = I_DISABLE_KBD;
		}
		break;

	case I_DISABLE_KBD:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))) SendDAGRACK(command, (RRDY | RCVD | ACKD));
		if((command == LOCK_KBD_COMMAND) && (flags & (REQD | ACKD)) { 
			SendDAGRACK(LOCK_KBD_COMMAND, (RRDY | RCVD | ACKD);
			SendDAGRCmd(RESET_COMMAND, (RRDY | REQD),  ;
			dagr_state = I_RESET_DAGR;
		}
		break;

	case I_RESET_DAGR:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))) SendDAGRACK(command, (RRDY | RCVD | ACKD));
		break;

	case I_SETUP_DAGR:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))) SendDAGRACK(command, (RRDY | RCVD | ACKD));
		break;
		
	case I_CONNECT_MSG:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))) SendDAGRACK(command, (RRDY | RCVD | ACKD));
		break;

	case I_ENABLE_KBD:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))) SendDAGRACK(command, (RRDY | RCVD | ACKD));
		break;

	case I_COLLECT_STATUS:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))) SendDAGRACK(command, (RRDY | RCVD | ACKD));
		break;

	case I_COLLECT_TIME_TFR:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))) SendDAGRACK(command, (RRDY | RCVD | ACKD));
		break;
		
	case I_DONE:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))) SendDAGRACK(command, (RRDY | RCVD | ACKD));
		break;

	// State to get the time 1PPS tick data
	case COLLECT_TIME_TFR:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))) SendDAGRACK(command, (RRDY | RCVD | ACKD));
		break;
	case COLLECT_TIME_DONE:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))) SendDAGRACK(command, (RRDY | RCVD | ACKD));
		break;

	// State to get the full status, including time
	case COLLECT_STATUS:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))) SendDAGRACK(command, (RRDY | RCVD | ACKD));
		break;

	case COLLECT_STATUS_DONE:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))) SendDAGRACK(command, (RRDY | RCVD | ACKD));
		break;
		
	default:
		if((command == BUFF_BOX_STATUS) && (flags == (RRDY | REQD))) SendDAGRACK(command, (RRDY | RCVD | ACKD));
		break;
	}
	valid_start = 0;
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

	process_dagr_init();
	dagr_state = I_SCAN_BUFF_BOX;
	while(is_not_timeout())
	{
		if(rx_eusart_count() > 0) {
			// Get and process received symbol
			ch = rx_eusart_symbol();
			process_dagr_symbol(ch);
			// All data states passed and collected - report success
			if(dagr_state == I_DONE){
  				set_led_on();		// Set LED on - DAGR is setup
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
	set_timeout(DAGR_PROCESS_TIMEOUT_MS);  
  	set_led_off();		// Set LED off

	process_dagr_init();
	dagr_state = COLLECT_TIME_TFR;
	while(is_not_timeout())
	{
		if(rx_eusart_count() > 0) {
			// Get and process received symbol
			ch = rx_eusart_symbol();
			process_dagr_symbol(ch);
			// All data states passed and collected - report success
			if(dagr_state == COLLECT_TIME_DONE){
  				set_led_on();		// Set LED on - DAGR sent us the Time Transfer (5101) message
				return ST_OK;
			}
		}
	}
  	close_eusart();
	set_led_off();		// Set LED off
	return ST_TIMEOUT;
}

static char GetDAGRStatus(void)
{
	set_timeout(DAGR_PROCESS_TIMEOUT_MS);  
  	set_led_off();		// Set LED off

	process_dagr_init();
	dagr_state = COLLECT_STATUS;
	while(is_not_timeout())
	{
		if(rx_eusart_count() > 0) {
			// Get and process received symbol
			ch = rx_eusart_symbol();
			process_dagr_symbol(ch);
			// All data states passed and collected - report success
			if(dagr_state == COLLECT_STATUS_DONE){
  				set_led_on();		// Set LED on - DAGR sent us the Status (5040) message
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

	set_timeout(DAGR_PROCESS_TIMEOUT_MS);  

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
	if( GetDAGRStatus() != ST_OK) return ST_TIMEOUT;

	//  4. Calculate the next current time to see if the timimg is consistent.
	ExtractDAGRDate();		// Convert DAGR_Date and DAGR_Time to RTC params
	CalculateNextSecond();	// Result will be in rtc_date structure

	//  5. Get the DAGR time again and compare with calculated next second
	if( GetDAGRStatus() != ST_OK)	return ST_ERR;
	
	p_time = (unsigned char *) &dagr_time;
	p_date = (unsigned char *) &dagr_date;
	
	// If the time is incorrect - try again
	if( (*p_date++ != rtc_date.Year) ||
  		(*p_date++ != rtc_date.Month) ||
    	(*p_date++ != rtc_date.Day) ||
      		(*p_time++ != rtc_date.Seconds) ||
        	(*p_time++ != rtc_date.Minutes) ||
          	(*p_time++ != rtc_date.Hours)  ) return ST_ERR;
	
	//  6. Time is consistent - prepare everything for the next 1PPS pulse 
	ExtractDAGRDate();
	CalculateNextSecond();
	if(rtc_date.Valid == 0) return ST_TIMEOUT;

	//	7. Find the 1PPS rising edge with interrupts disabled
	prev = INTCONbits.GIE;
	INTCONbits.GIE = 0;		// Disable interrupts

	SetRTCDataPart1();		// Prepare for the RTC Clock set

	while(GPS_1PPS);	// wait for LOW
	while(!GPS_1PPS);	// wait for HIGH

	//  8. Finally, set up the RTC clock - according to the spec,
	//	the RTC chain is reset on ACK after writing to seconds register.
	CLOCK_LOW();
	DATA_HI();	
	DelayI2C();
	CLOCK_HI();
	DelayI2C();

	SetRTCDataPart2();
	
	INTCONbits.GIE = prev;		// Enable interrupts

	//  9. Get the DAGR time again and compare with the current RTC
	if( GetDAGRStatus() != ST_OK)	return ST_ERR;
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
