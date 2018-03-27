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
	I_RESET_SETUP,
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
static unsigned int	command;
static unsigned int	num_words;
static unsigned int	flags;

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
#define	NACD				(0x0100)
#define	CONN				(0x0040)
#define	DISC				(0x0020)

static const byte DisableKbdCmd[] = {		// 0x1F words
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const byte EnableKbdCmd[] = {		// 0x16 words
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

static const byte SetupDAGRCmd[] = {		// 0x17 words
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,	// Words 1 - 4
	0x71,0x7E,	// Word 5 - Validity bits - 0x7E71,All ,excpt CodeType(6), MagVarType (13), NavMode(16),  
	0x00,0x00,
	0x00,0x00,	// Word 7 -  00  - Grid MGRS-Old
	0x00,0x00,	// Word 8 -  00  - Distance Metric
	0x00,0x00,	// Word 9 -  00  - Elev Metric
	0x00,0x00,	// Word 10 - 00  - Elev Ref MSL
	0x01,0x00,	// Word 11 - 00  - Angular deg
	0x00,0x00,	// Word 12 - 00  - North Ref True
	0x00,0x00,
	0x00,0x00,
	0x00,0x00,
	0x00,0x00,
	0x00,0x00,
	0x00,0x00,
	0x01,0x00,	// Word 19 - 01 - Error Units FOM
	0x00,0x00,
	0x03,0x00,	// Word 21 - 03 - Timer OFF
	0x00,0x00,
	0x01,0x00	// Word 23 - 01 COM1 PPS 1 PPS UTC
};


static unsigned int curr_word;		// Current assembled word
static unsigned int checksum;		// Running checksum

static byte	valid_start;	// TRUE if proper byte sequence is detected
static byte	valid_header;	// TRUE if valid Header with valid Checksum
static byte	valid_data;		// TRUE if valid Data with valid Checksum 


#define DATA_POLARITY_DAGR	(DATA_POLARITY_RXTX)	// PLGR/DAGR is connected without Level Shifter

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

static 	byte 			*pBytes;			// Pointer to the individual bytes of the word
static 	byte 			*pData;				// Pointer to the individual bytes of the word
static	unsigned int 	*pWords;			// Pointer to the words

static	void CalculateWord(byte symbol)
{
	// Make sure that the packet starts with proper sequence
	// If no start is detected - do not proceed any further
	if(valid_start == 0) {
		FindMessageStart(symbol);
		return;
	}

	// The valid start sequence was detected - now pack all received data in words and process them
	// Place the received byte into a proper half of the word
	pBytes = (byte *) &curr_word;
	if(byte_counter & 0x01) {
		pBytes[1] = symbol;
	}else {
		pBytes[0] = symbol;
	}	

	// Process words differently depending on where they are - in the header or in the data block	
	if(byte_counter & 0x01) {
		checksum += curr_word;
		if(valid_header == 0) {		// Header block
			word_counter++;
			if(word_counter == 1 ) 		{ command = curr_word; } 	// First word is command
			else if(word_counter == 2 ) { num_words = curr_word; }	// Second - number of data words that follow
			else if(word_counter == 3 ) { flags = curr_word; }		// Third - Flag (ACK, REQ, etc)
			else if(word_counter == 4 ) {							// Fourth - Checksum
				// Last word in the header - check for checksum
				if(checksum != 0) {
					 valid_start = 0;	// Start searching for the packet again
				}else {
					// Check if this is a "header-only" message
					if(!num_words )  {
						valid_data = 1;
						valid_start = 0;	// Start searching for the packet again
					}
					valid_header = 1;	// Mark as valid Header received
					word_counter = 0;	// It will be incremented later on
					checksum = 0;		// For Data block the Checksum initial value is 0x0000
				}	 
			}	  
		}else {		// Data block that follows valid Header
			pWords = (unsigned int *)data_buffer;
			pWords[word_counter] = curr_word;
			// Here is the checksum after last word - Check for data validity
			if( word_counter == num_words ) {
				if(checksum == 0) {
					valid_data = 1;
				}	 
				valid_start = 0;	// Start searching for the packet again
			}
			word_counter++;		
		}	
	}
	byte_counter++;
}

static  unsigned int CalculateDAGRChecksum(byte *pBuffer, unsigned char nWords)
{
	int cs = 0;
	pWords = (unsigned int *) pBuffer;
	while(nWords--) {
		cs += *pWords++;
	}
	return -cs;
}	

// Send ACK for the specified command
static  void SendDAGRACK(int Command, int Flags)
{
	Flags |= RRDY;

	tx_eusart_flush();	// Make sure that the data is out before placing anything else in the buffer

	pData = tx_buffer;
	pData[0] = 	DEL;
	pData[1] = 	SOH;
	pData[2] = 	Command;
	pData[3] = 	Command >> 8;
	pData[4] = 	0;		// No data
	pData[5] = 	0;		//		follows...
	pData[6] = 	Flags;
	pData[7] = 	Flags >> 8;
	checksum = CalculateDAGRChecksum(pData, 4);
	pData[8] = 	checksum;
	pData[9] = 	checksum >> 8;

	DelayMs(12);
	tx_eusart_async(pData, 4*2 + 2);
}

// Send the Command 
static byte idx;
static  void SendDAGRCmd(int Command, int Flags, const byte *pCommandData, byte nWords)
{
	Flags |= RRDY;

	tx_eusart_flush();	// Make sure that the data is out before placing anything else in the buffer

	pData = tx_buffer;
	pData[0] = 	DEL;
	pData[1] = 	SOH;
	pData[2] = 	Command;
	pData[3] = 	Command >> 8;
	pData[4] = 	nWords;		// Number of words
	pData[5] = 	0;			//		to follow...
	pData[6] = 	Flags;
	pData[7] = 	Flags >> 8;
	checksum = CalculateDAGRChecksum(pData, 4);
	pData[8] = 	checksum;
	pData[9] = 	checksum >> 8;
	if(nWords) {
		for(idx = 0; idx < nWords * 2; idx++) {
			pData[10 + idx] = *pCommandData++;
		}
		checksum = CalculateDAGRChecksum(&pData[10], nWords);
		pData[10 + nWords*2] = 	checksum;
		pData[10 + nWords*2 + 1] = 	checksum >> 8;

		nWords++;		// Extra word is the calculated checksum
	}
	DelayMs(12);
	tx_eusart_async(pData, (4*2 + 2) + nWords*2);
}


// Function to convert HEX number in the range of 0-99 to the BCD
static byte BinToBCD(byte data_bin) {
	byte data_bcd = 0;
	while(data_bin > 10) {
		data_bcd += 0x10;
		data_bin -= 10;
	}
	return (data_bcd + data_bin);
}	

// Get the specific DAGR word (DAGR Words are numbered starting from 1 !!!)
static	int	GetDAGRWord(byte WordNumber)
{
	idx = (WordNumber - 1) * 2;	// Words in DAGR world are numbered from 1
	pData = data_buffer;
	pWords = (unsigned int *) &pData[idx];		
	return *pWords;
}

static void  ExtractDAGRDate(void)
{
	pBytes = (byte *) 	&dagr_time;
	rtc_date.Seconds	= *pBytes++;
	rtc_date.Minutes	= *pBytes++;
	rtc_date.Hours		= *pBytes++;

	pBytes = (byte *) 	&dagr_date;
	rtc_date.Century	= 0x20;
	rtc_date.Year		= *pBytes++;
	rtc_date.Month		= *pBytes++;
	rtc_date.Day		= *pBytes++;
  	CalculateJulianDay();
  	CalculateWeekDay();
}

void process_dagr_init(byte initial_state)
{
	valid_start = 0;
	valid_header = 0;
	valid_data = 0;
	prev_symbol = 0;
	dagr_state = initial_state;
}	

void process_dagr_symbol(byte new_symbol)
{
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
		process_dagr_init(I_SCAN_BUFF_BOX);
		break;

	case I_SCAN_BUFF_BOX:
		if((command == BUFF_BOX_STATUS) && (flags & REQD)){
  			set_led_off();													// Set LED off
			SendDAGRACK(command, (RCVD | ACKD) );							// Flag = 0x8A00
			SendDAGRCmd(LOCK_KBD_COMMAND, REQD, &DisableKbdCmd[0], 0x1F); 	// Flag = 0x9000
			set_timeout(DAGR_PROCESS_TIMEOUT_MS);  	// Setup new 2 Seconds timeout
			dagr_state = I_RESET_SETUP;
		}
		break;

	case I_RESET_SETUP:
		if((command == BUFF_BOX_STATUS) && (flags & REQD)) SendDAGRACK(command, (RCVD | ACKD));
		if((command == LOCK_KBD_COMMAND) && (flags & REQD)) { 
			SendDAGRACK(command, (RCVD | ACKD));							// Flag = 0x8A00
			SendDAGRCmd(RESET_COMMAND, (REQD | DISC), 0, 0 );				// Flag = 0x9020
			SendDAGRCmd(SETUP_COMMAND, (REQD), &SetupDAGRCmd[0], 0x17 );	// Flag = 0x9000
			SendDAGRCmd(STATUS_MSG, (REQD | CONN), 0, 0 );					// Flag = 0x9040
			SendDAGRCmd(LOCK_KBD_COMMAND, (REQD), &EnableKbdCmd[0], 0x16); 	// Flag = 0x9000
			set_timeout(DAGR_PROCESS_TIMEOUT_MS);  	// Setup new 2 Seconds timeout
			dagr_state = I_ENABLE_KBD;
		}
		break;

	case I_ENABLE_KBD:
		if((command == BUFF_BOX_STATUS) && (flags & REQD)) SendDAGRACK(command, (RCVD | ACKD));
		if((command == RESET_COMMAND) && (flags & REQD)) SendDAGRACK(command, (RCVD | ACKD));
		if((command == SETUP_COMMAND) && (flags & REQD)) SendDAGRACK(command, (RCVD | ACKD));
		if((command == STATUS_MSG) && (flags & REQD)) SendDAGRACK(command, (RCVD | ACKD));
		if((command == LOCK_KBD_COMMAND) && (flags & REQD)) {
			SendDAGRACK(command, (RCVD | ACKD));
			set_timeout(DAGR_PROCESS_TIMEOUT_MS);  	// Setup new 2 Seconds timeout
			dagr_state = I_COLLECT_TIME_TFR;  		// Check if there are 5101 Messages
		}
		break;

	case I_COLLECT_TIME_TFR:
		if((command == BUFF_BOX_STATUS) && (flags & REQD)) SendDAGRACK(command, (RCVD | ACKD));
		if((command == TIME_TRANSFER_MSG) && (flags & RRDY)) {
		
			// Extract the Time info from the last 3 words of the message
			pBytes = (byte *) &dagr_time;
			pData = data_buffer;

			*pBytes++ = pData[4 * 2 + 3];			// Seconds
			*pBytes++ = pData[4 * 2 + 0];			// Minutes
			*pBytes++ = pData[4 * 2 + 1];			// Hours
			set_timeout(DAGR_PROCESS_TIMEOUT_MS);  	// Setup new 2 Seconds timeout
			dagr_state = I_COLLECT_STATUS;			// Check if there are 5040 Messages
		}
		break;

	case I_COLLECT_STATUS:
		if((command == BUFF_BOX_STATUS) && (flags & REQD)) SendDAGRACK(command, (RCVD | ACKD));
		if((command == STATUS_MSG) && (flags & RRDY)) {
			// Extract the Time and Date info from the last specified words of the message
			pBytes = (byte *) &dagr_time;
			*pBytes++ = BinToBCD(GetDAGRWord(22));		// Seconds
			*pBytes++ = BinToBCD(GetDAGRWord(21));		// Minutes
			*pBytes = BinToBCD(GetDAGRWord(20));		// Hours

			pBytes = (byte *) &dagr_date;
			*pBytes++ = BinToBCD(GetDAGRWord(26));		// Year
			*pBytes++ = BinToBCD(GetDAGRWord(25));		// Month
			*pBytes = BinToBCD(GetDAGRWord(24));		// Day
			dagr_state = I_DONE;				// All messages are here - report success
		}
		break;
		
	case I_DONE:
		if((command == BUFF_BOX_STATUS) && (flags & REQD)) SendDAGRACK(command, (RCVD | ACKD));
		break;

	// State to get the time 1PPS tick data
	case COLLECT_TIME_TFR:
		if((command == BUFF_BOX_STATUS) && (flags & REQD)) SendDAGRACK(command, (RCVD | ACKD));
		if((command == TIME_TRANSFER_MSG) && (flags & RRDY)) {
			// Extract the Time info from the last 3 words of the message
			pBytes = (byte *) &dagr_time;
			pData = data_buffer;

			*pBytes++ = pData[4 * 2 + 3];			// Seconds
			*pBytes++ = pData[4 * 2 + 0];			// Minutes
			*pBytes++ = pData[4 * 2 + 1];			// Hours
			dagr_state = COLLECT_TIME_DONE;
		}
		break;

	case COLLECT_TIME_DONE:
		if((command == BUFF_BOX_STATUS) && (flags & REQD)) SendDAGRACK(command, (RCVD | ACKD));
		break;

	// State to get the full status, including time
	case COLLECT_STATUS:
		if((command == BUFF_BOX_STATUS) && (flags & REQD)) SendDAGRACK(command, (RCVD | ACKD));
		if((command == STATUS_MSG) && (flags & RRDY)) {
			// Extract the Time and Date info from the last specified words of the message
			pBytes = (byte *) &dagr_time;
			*pBytes++ = BinToBCD(GetDAGRWord(22));		// Seconds
			*pBytes++ = BinToBCD(GetDAGRWord(21));		// Minutes
			*pBytes = BinToBCD(GetDAGRWord(20));		// Hours

			pBytes = (byte *) &dagr_date;
			*pBytes++ = BinToBCD(GetDAGRWord(26));		// Year
			*pBytes++ = BinToBCD(GetDAGRWord(25));		// Month
			*pBytes = BinToBCD(GetDAGRWord(24));		// Day
			dagr_state = COLLECT_STATUS_DONE;
		}
		break;

	case COLLECT_STATUS_DONE:
		if((command == BUFF_BOX_STATUS) && (flags & REQD)) SendDAGRACK(command, (RCVD | ACKD));
		break;
		
	default:
		if((command == BUFF_BOX_STATUS) && (flags & REQD)) SendDAGRACK(command, (RCVD | ACKD));
		break;
	}
	valid_data = 0;		// Done processing, start a new one..

}

static unsigned char ch;
static unsigned char *p_date;
static unsigned char *p_time;

static char SetupDAGR(void)
{
	// Configure the EUSART module
  	open_eusart(BRREG_DAGR, DATA_POLARITY_DAGR);	

	rx_eusart_async(&RxTx_buff[0], FILL_MAX_SIZE, DAGR_DETECT_TIMEOUT_MS);

	process_dagr_init(INIT);
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
	return ST_TIMEOUT;
}

static char GetDAGRTime(void)
{
	set_timeout(DAGR_PROCESS_TIMEOUT_MS);  
	set_led_off();		// Set LED off

	process_dagr_init(COLLECT_TIME_TFR);
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
	return ST_TIMEOUT;
}

static char GetDAGRStatus(void)
{
	set_timeout(DAGR_PROCESS_TIMEOUT_MS);  
	set_led_off();		// Set LED off

	process_dagr_init(COLLECT_STATUS);
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
	return ST_TIMEOUT;
}

static char FindRisingEdge(void)
{
	// Config the 1PPS pin as input
	pinMode(GPS_1PPS, INPUT);

	set_timeout(DAGR_PROCESS_TIMEOUT_MS);  

	while(is_not_timeout()){	
		if(!pinRead(GPS_1PPS)) break;
	}
	while(is_not_timeout()){	
		if(pinRead(GPS_1PPS)) return ST_OK;
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

	while(pinRead(GPS_1PPS));	// wait for LOW
	while(!pinRead(GPS_1PPS));	// wait for HIGH

	//  8. Finally, set up the RTC clock - according to the spec,
	//	the RTC chain is reset on ACK after writing to seconds register.
	I2C_CLOCK_LOW();
	I2C_DATA_HI();	
	DelayI2C();
	I2C_CLOCK_HI();
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
