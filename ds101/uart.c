#include "config.h"
#include "delay.h"
#include "HDLC.h"
#include "Fill.h"
#include "serial.h"

#define TIMER_DTD 			( ( (XTAL_FREQ * 1000000L) / (4L * 16L * DTD_BAUDRATE)) - 1 )
#define TIMER_DTD_START 	( -(TIMER_DTD/2) )
#define TIMER_DTD_EDGE 		( (TIMER_DTD/2) )
#define TIMER_DTD_CTRL 		( (1<<2) | 2)     // ENA, 1:16

// #define TIMER_DS101 		( ((XTAL_FREQ * 1000000L) / ( 4L * DS101_BAUDRATE)) - 1 )
// #define TIMER_DS101			(62)
#define TIMER_DS101 		( ((64L * 1000000L) / ( 4L * DS101_BAUDRATE)) - 1 )

#define TIMER_DS101_CTRL 	( (1<<2) | 0)   // ENA, 1:1


void OpenRS232()
{
  	close_eusart();
	TRIS_RxPC = INPUT;
	TRIS_TxPC = OUTPUT;
  	TxPC  = 0;  // Set up the stop bit
}

//  Simulate UARTs for DTD RS-232 and DS-101 RS-485 communications
//  TMR6 is used to count bits in Tx and Rx
//
// Soft UART to communicate with RS232 port
// Returns:
//  -1  - if no symbol within timeout
//  >=0 - if symbol was detected 
int RxRS232Char()
{
	byte bitcount, data;
	byte prev;
	int  result;

	TRIS_RxPC = INPUT;

	PR6 = TIMER_DTD;
	T6CON = TIMER_DTD_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag
      	
	result = -1;
  	
	prev = INTCONbits.GIE;
  	INTCONbits.GIE = 0;

  	while( is_not_timeout() )
	{
		// Start conditiona was detected - count 1.5 cell size	
		if(RxPC )
		{
			TMR6 = TIMER_DTD_START;
			PIR5bits.TMR6IF = 0;	// Clear overflow flag
			for(bitcount = 0; bitcount < 8 ; bitcount++)
			{
				// Wait until timer overflows
				while(!PIR5bits.TMR6IF){} ;
				PIR5bits.TMR6IF = 0;	// Clear overflow flag
				data = (data >> 1) | (RxPC ? 0x00 : 0x80);
			}
 			while(!PIR5bits.TMR6IF){} ;	// Wait for stop bit
			result =  RxPC ? -1 : ((int)data) & 0x00FF;
			break;
		}
	}
	INTCONbits.GIE = prev;

	return result;
}


void TxRS232Char(char data)
{
	byte 	bitcount;
	data = ~data;  			// Invert sysmbol

	TRIS_TxPC = OUTPUT;

	PR6 = TIMER_DTD;
	T6CON = TIMER_DTD_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag

	TxPC = 1;        		// Issue the start bit
 	// send 8 data bits and 3 stop bits
	for(bitcount = 0; bitcount < 12; bitcount++)
	{
		while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
		PIR5bits.TMR6IF = 0;	// Clear timer overflow bit
		TxPC = data & 0x01;		// Set the output
		data >>= 1;				// We use the fact that 
						        // "0" bits are STOP bits
	}
} 



void OpenDTD()
{
  	close_eusart();
	TRIS_RxDTD = INPUT;
	TRIS_TxDTD = OUTPUT;
  	TxDTD  = 0;  // Set up the stop bit
}


//
// Soft UART to communicate with another DTD
// Returns:
//  -1  - if no symbol within timeout
//  >=0 - if symbol was detected 
int RxDTDChar()
{
	byte 	bitcount, data;
	byte 	prev;
	int		result;

	TRIS_RxDTD = INPUT;

	PR6 = TIMER_DTD;
	T6CON = TIMER_DTD_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag
      	
	prev = INTCONbits.GIE;
  	INTCONbits.GIE = 0;

	result = -1;
  	while( is_not_timeout() )
	{
		// Start conditiona was detected - count 1.5 cell size	
		if(RxDTD )
		{
			TMR6 = TIMER_DTD_START;
			PIR5bits.TMR6IF = 0;	// Clear overflow flag
			for(bitcount = 0; bitcount < 8 ; bitcount++)
			{
				// Wait until timer overflows
				while(!PIR5bits.TMR6IF){} ;
				PIR5bits.TMR6IF = 0;	// Clear overflow flag
				data = (data >> 1) | (RxDTD ? 0x00 : 0x80);
			}
			while(!PIR5bits.TMR6IF){} ; // Wait for stop bit
			result = RxDTD ? -1 : ((int)data) & 0x00FF;
			break;
		}
	}
  	INTCONbits.GIE = prev;

	return result;
}


void TxDTDChar(char data)
{
	byte 	bitcount;

	data = ~data;  			// Invert sysmbol
	
	TRIS_TxDTD = OUTPUT;

	PR6 = TIMER_DTD;
	T6CON = TIMER_DTD_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag

	TxDTD = 1;        		// Issue the start bit
   	// send 8 data bits and 3 stop bits
	for(bitcount = 0; bitcount < 12; bitcount++)
	{
		while(!PIR5bits.TMR6IF) {	/* wait until timer overflow bit is set*/};
		PIR5bits.TMR6IF = 0;		// Clear timer overflow bit
		TxDTD = data & 0x01;		// Set the output
		data >>= 1;					// We use the fact that 
						        	// "0" bits are STOP bits
	}
} 

void OpenRS485()
{
	TRIS_Data_N = INPUT;
  	WPUB_Data_N = 1;
	ANSEL_Data_N = 0;

	TRIS_Data_P = INPUT;
  	WPUB_Data_P = 1;
	ANSEL_Data_P = 0;
}

// DS-101 64000bps Differential Manchester/Bi-phase coding
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



#define PERIOD_CNTR   		(TIMER_DS101)
#define HALF_PERIOD_CNTR   	(PERIOD_CNTR/2)
#define SAMPLE_CNTR 		((PERIOD_CNTR * 7)/10)	// PERIOD_CNTR * 0.66
#define TIMEOUT_CNTR   		(0xFF)
#define TIMER_CTRL			(TIMER_DS101_CTRL)

#define	FLAG				(0x7E)
#define	PRE_FLAG			(0xFC)	// Flag one bit before

#define 	Timer_Period	(PR6)
#define 	Timer_Counter	(TMR6)
#define 	Timer_Ctrl		(T6CON)
#define 	TimerFlag		(PIR5bits.TMR6IF)
#define 	PIN_P			(Data_P)
#define 	PIN_N			(Data_N)
#define 	TRIS_PIN_P		(TRIS_Data_P)
#define 	TRIS_PIN_N		(TRIS_Data_N)

int RxRS485Data(char *pData)
{
	HDLC_STATES			st;
	int 				byte_count;
	unsigned char		bit_count;
	unsigned char		stuff_count;
	unsigned char		rcvd_bit;
	unsigned char		rcvd_byte;
	
	unsigned char		prev_PIN;
	byte 				prev;
	
	// Take care of physical pin
	TRIS_PIN_P = INPUT;
	TRIS_PIN_N = INPUT;

	st = INIT;
	Timer_Ctrl 		= TIMER_CTRL;
      	
	prev = INTCONbits.GIE;
  	INTCONbits.GIE = 0;

  	OSCTUNEbits.PLLEN = 1;    	// *4 PLL (64MHZ)
	DelayMs(40);				// Wait for PLL to become stable

	while(1) {
		switch(st) {
		  case INIT:
			bit_count 	= 0;
			byte_count	= 0;
			rcvd_bit 	= 0;
			rcvd_byte 	= 0;
			stuff_count = 0;
			prev_PIN	= PIN_P;
			
			Timer_Period	= TIMEOUT_CNTR;
			Timer_Counter 	= 0;
			TimerFlag 		= 0;	// Clear overflow flag

			st = GET_FLAG_EDGE;
		 	break;
			
		case GET_FLAG_EDGE:
			if(PIN_P != prev_PIN) {
				st = COMP_FLAG;	
			}else if(TimerFlag)	{
				st = TIMEOUT;
			}
			break;

		case COMP_FLAG:
			Timer_Period	= SAMPLE_CNTR;
			Timer_Counter 	= 0;
			TimerFlag 		= 0;	// Clear overflow flag
			prev_PIN = PIN_P;
			rcvd_byte	=  (rcvd_byte >> 1) | rcvd_bit;
			st = (rcvd_byte == FLAG) ? GET_SAMPLE : GET_FLAG;
			break;

		case GET_FLAG:
			while(!TimerFlag) {}
			Timer_Period	= TIMEOUT_CNTR;
			Timer_Counter 	= 0;
			TimerFlag 		= 0;	// Clear overflow flag
			rcvd_bit = (PIN_P == prev_PIN)? 0x80 : 0x00;
			prev_PIN = PIN_P;
			st = GET_FLAG_EDGE;
			break;

		case GET_SAMPLE_EDGE:
			if(PIN_P != prev_PIN) {
				st = COMP_SAMPLE;	
			}else if(TimerFlag)	{
				st = TIMEOUT;
			}
			break;

		case COMP_SAMPLE:
			Timer_Period	= SAMPLE_CNTR;
			Timer_Counter 	= 0;
			TimerFlag 		= 0;	// Clear overflow flag
			prev_PIN = PIN_P;
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
			Timer_Period	= TIMEOUT_CNTR;
			Timer_Counter 	= 0;
			TimerFlag 		= 0;	// Clear overflow flag
			rcvd_bit = (PIN_P == prev_PIN)? 0x80 : 0x00;
			prev_PIN = PIN_P;
			st = GET_SAMPLE_EDGE;
			break;

		case GET_DATA_EDGE:
			if(PIN_P != prev_PIN) {
				st = COMP_DATA;	
			}else if(TimerFlag)	{
				st = TIMEOUT;
			}
			break;

		case COMP_DATA:
			Timer_Period	= SAMPLE_CNTR;
			Timer_Counter 	= 0;
			TimerFlag 		= 0;	// Clear overflow flag
			prev_PIN = PIN_P;
			
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
			Timer_Period	= TIMEOUT_CNTR;
			Timer_Counter 	= 0;
			TimerFlag 		= 0;	// Clear overflow flag
			rcvd_bit = (PIN_P == prev_PIN)? 0x80 : 0x00;
			prev_PIN = PIN_P;
			st = ((rcvd_bit == 0) && (rcvd_byte == PRE_FLAG) ) ? DONE : GET_DATA_EDGE;
			break;
			
		case TIMEOUT:
		  	OSCTUNEbits.PLLEN = 0;    	// No PLL (16MHZ)
			DelayMs(10);				// Wait for PLL to become stable
  			INTCONbits.GIE = prev;
			return -1;
			break;

		case DONE:
		  	OSCTUNEbits.PLLEN = 0;    	// No PLL (16MHZ)
			DelayMs(10);				// Wait for PLL to become stable
  			INTCONbits.GIE = prev;
			return byte_count;
			break;
			
		default:
			break;
		}			
	}	
}	

#define		NUM_INITIAL_FLAGS	(10)
#define		NUM_FINAL_FLAGS		(1)

void TxRS485Data(char *pData, int nBytes)
{
	HDLC_STATES			st;
	unsigned char 		byte_count;
	unsigned char		bit_count;
	unsigned char		stuff_count;
	unsigned char		tx_byte;
	unsigned char		next_bit;
	unsigned char		bit_to_send;
	byte 				prev;

	st = INIT;

	// Take care of physical pin
	TRIS_PIN_P 		= OUTPUT;
	TRIS_PIN_N 		= OUTPUT;
	PIN_P 			= 1;
	PIN_N 			= 0;
	next_bit 		= 0;

	Timer_Ctrl 		= TIMER_CTRL;

	prev = INTCONbits.GIE;
  	INTCONbits.GIE = 0;
	
  	OSCTUNEbits.PLLEN = 1;    	// *4 PLL (64MHZ)
	DelayMs(40);				// Wait for PLL to become stable

	while(1) {
		switch(st) {
		  case INIT:
			bit_count 	= 0;
			byte_count	= 0;
			stuff_count = 0;
			

			Timer_Period	= HALF_PERIOD_CNTR;
			Timer_Counter 	= 0;
			TimerFlag 		= 0;	// Clear overflow flag
			st 				= SEND_START_FLAG;
		 	break;

		  case SEND_START_FLAG:
		  	byte_count	 = NUM_INITIAL_FLAGS;

		  	while(byte_count > 0) {
			  	tx_byte = FLAG;
			  	bit_count = 8;
			  	while(bit_count > 0)
			  	{
					while(!TimerFlag) {}	// Start of the cell
			  		PIN_P = next_bit;
					PIN_N = ~next_bit;
					TimerFlag = 0;

				  	next_bit = tx_byte & 0x01 ? next_bit : ~next_bit;	// For "0" flip the bit, for 1" keep in in the middle of the cell
				  	tx_byte >>= 1;

					while(!TimerFlag) {}	// Middle of the cell
					PIN_P = next_bit;
					PIN_N = ~next_bit;
					TimerFlag = 0;
					next_bit = ~next_bit; 	// Always flip the bit at the beginning of the cell 
					bit_count--;
				}
				byte_count--;	
			}
			st = SEND_DATA;
		 	break;

		  case SEND_DATA:
		  	byte_count	 	= nBytes;
			stuff_count 	= 0;

		  	while(byte_count > 0) {
			  	tx_byte = *pData;
			  	bit_count = 8;
			  	while(bit_count > 0)
			  	{
					while(!TimerFlag) {}		// Wait for the next cell beginning
			  		PIN_P = next_bit;
					PIN_N = ~next_bit;
					TimerFlag = 0;

					// Start calculating the next bit
					bit_to_send = tx_byte & 0x01;
			  		next_bit = bit_to_send ? next_bit : ~next_bit;	// For "0" flip the bit, for 1" keep it the same in the middle of the cell
			  		tx_byte >>= 1;

					// Do bit stuffing calculation - after five consequtive "1"s ALWAYS insert "0"
					if(bit_to_send) {
						stuff_count++;			// Count "1"s 
					} else {
						stuff_count = 0;
					} 

					while(!TimerFlag) {}		// We are now at the middle of the cell
					PIN_P = next_bit;
					PIN_N = ~next_bit;
					TimerFlag = 0;
					next_bit = ~next_bit;	// Always flip the bit at the beginning of the cell 					

					// OK, the bit was sent, now 
					//  do the actual bit stuffing if needed - insert "0"
					if(stuff_count >= 5) {
						while(!TimerFlag) {}		// Wait for the next cell beginning
			  			PIN_P = next_bit;
						PIN_N = ~next_bit;
						TimerFlag = 0;

						next_bit = ~next_bit;		// Send "0"

						while(!TimerFlag) {}		// Wait for the middle of the cell
						PIN_P = next_bit;
						PIN_N = ~next_bit;
						TimerFlag = 0;

						next_bit = ~next_bit;	// Always flip the bit at the beginning of the cell 
						stuff_count = 0;
					}
					bit_count--;
				}	
				pData++;
				byte_count--;
			}
			st = SEND_END_FLAG;
		 	break;

		  case SEND_END_FLAG:
		  	byte_count	 = NUM_FINAL_FLAGS;

		  	while(byte_count > 0) {
			  	tx_byte = FLAG;
			  	bit_count = 8;
			  	while(bit_count > 0)
			  	{
					while(!TimerFlag) {}		// Wait for the cell start
			  		PIN_P = next_bit;
					PIN_N = ~next_bit;
					TimerFlag = 0;

				  	next_bit = tx_byte & 0x01 ? next_bit : ~next_bit;	// For "0" flip the bit, for 1" keep in in the middle of the cell
				  	tx_byte >>= 1;

					while(!TimerFlag) {}		// Wait for the middle of the cell
					PIN_P = next_bit;
					PIN_N = ~next_bit;
					TimerFlag = 0;
					next_bit = ~next_bit;	// Always flip the bit at the beginning of the cell 
					bit_count--;
				}	
				byte_count--;
			}
			// Handle the last cell
			while(!TimerFlag) {}		// Wait for the cell start
	  		PIN_P = next_bit;
			PIN_N = ~next_bit;
			TimerFlag = 0;
			st = DONE;
		 	break;

		case DONE:
  			OSCTUNEbits.PLLEN = 0;    	// No PLL (16MHZ)
  			INTCONbits.GIE = prev;
			TRIS_PIN_P 		= INPUT;
			TRIS_PIN_N 		= INPUT;
			return;
			break;
		} 	
	}	
}	

