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

  	OSCTUNEbits.PLLEN = 1;    	// *4 PLL (64MHZ)
	DelayMs(80);				// Wait for PLL to become stable
}

// DS-101 64000bps Differential Manchester/Bi-phase coding
typedef enum {
	INIT = 0,
	GET_EDGE,
	GET_FLAG_EDGE,
	GET_FLAG,
	GET_SAMPLE_EDGE,
	GET_SAMPLE,
	GET_DATA_EDGE,
	GET_DATA,
	SEND_START_FLAG,
	SEND_DATA,
	SEND_END_FLAG,
	DONE,
	TIMEOUT
} HDLC_STATES;



#define TIMER_CTRL			(TIMER_DS101_CTRL)
#define PERIOD_CNTR   		(TIMER_DS101)
#define HALF_PERIOD_CNTR   	(PERIOD_CNTR/2)
#define QTR_PERIOD_CNTR   	(PERIOD_CNTR/4)
#define EIGTH_PERIOD_CNTR   (PERIOD_CNTR/8)
#define SAMPLE_CNTR 		(HALF_PERIOD_CNTR + EIGTH_PERIOD_CNTR)	// PERIOD_CNTR * 0.66

#define	FLAG				(0x7E)
#define	PRE_FLAG			(0xFC)	// Flag one bit before

#define 	Timer_Period	(PR6)
#define 	Timer_Counter	(TMR6)
#define 	Timer_Ctrl		(T6CON)
#define 	TimerFlag		(PIR5bits.TMR6IF)
#define 	PIN_P			(Data_P)
#define 	PIN_N			(Data_N)
#define 	PIN_IN			(Data_P)
#define 	TRIS_PIN_P		(TRIS_Data_P)
#define 	TRIS_PIN_N		(TRIS_Data_N)

static		HDLC_STATES			st;
static		int 				byte_count;
static		unsigned char		bit_count;
static		unsigned char		stuff_count;
static		unsigned char		rcvd_byte;
	
static		unsigned char		rcvd_Sample;		// The current Sample at 0.75T
static		unsigned char		prev_Sample;		// The Sample at the 0.0T

static		byte 				prevIRQ;

int RxRS485Data(char *pData)
{
	// Take care of physical pins
	TRIS_PIN_P = INPUT;
	TRIS_PIN_N = INPUT;

	TRIS_PIN_C	= OUTPUT;
	TRIS_PIN_D	= OUTPUT;

	PIN_C = 0;
	PIN_D = 0;

	st = INIT;
	Timer_Ctrl 		= TIMER_CTRL;
      	
	prevIRQ = INTCONbits.GIE;
  	INTCONbits.GIE = 0;

	// Set the big 16-bit timeout counter
  	TMR1H = 0;
  	TMR1L = 0;				// Reset the timer
  	T1GCONbits.TMR1GE = 0;	// No gating
	PIR1bits.TMR1IF = 0;	// Clear Flag

	while(!PIR1bits.TMR1IF) {

		switch(st) {
		  case INIT:
			bit_count 	= 0;
			byte_count	= 0;
			rcvd_byte 	= 0;
			stuff_count = 0;
			prev_Sample = rcvd_Sample = PIN_IN;
	
			Timer_Period	= SAMPLE_CNTR;	// Start the timer to sample at 0.75T

			st = GET_EDGE;
		 	break;

		// Initial state - we are looking for any edge within big timeout
		case GET_EDGE:			
			if(PIN_IN != rcvd_Sample) {
				PIN_D = 1;
				Timer_Counter 	= 0;
				TimerFlag 		= 0;			// Clear overflow flag

				prev_Sample = PIN_IN;

				st = GET_FLAG;	
				PIN_D = 0;
			}
			break;
		
		// Set of 2 states to catch HDLC Flags to achieve frace sync
		// We exit it when we detect First HDLC flag
		case GET_FLAG_EDGE:		// We stay in this state until we get the edge
			if(rcvd_Sample){while(PIN_IN){}} else{while(!PIN_IN) {}}
//			if(PIN_IN != rcvd_Sample) 
			{
				PIN_D = 1;
				Timer_Counter 	= 0;
				TimerFlag 		= 0;			// Clear overflow flag
				
				// We now have 0.75T time to calclulate everything
				rcvd_byte = (rcvd_byte >> 1) | ((rcvd_Sample == prev_Sample)? 0x80 : 0x00);
				prev_Sample = PIN_IN;

				st = (rcvd_byte == FLAG) ? GET_SAMPLE : GET_FLAG;
				PIN_D = 0;
			}
			break;

		case GET_FLAG:			// In this state we wait for 0.75T mark
			while(!TimerFlag) {}
			PIN_C = 1;
			rcvd_Sample	= PIN_IN;
			st = GET_FLAG_EDGE;
			PIN_C = 0;
			break;

		// Set of 2 states to handle HDLC Flags to achieve frame sync	
		// We exit it when we detect First non-HDLC flag symbol
		case GET_SAMPLE_EDGE:
			if(rcvd_Sample){while(PIN_IN){}} else{while(!PIN_IN) {}}
//			if(PIN_IN != rcvd_Sample) {	// Edge detected
			{
				PIN_D = 1;
				Timer_Counter 	= 0;
				TimerFlag 		= 0;			// Clear overflow flag
				
				// We now have 0.75T time to calclulate everything
				rcvd_byte = (rcvd_byte >> 1) | ((rcvd_Sample == prev_Sample)? 0x80 : 0x00);
				prev_Sample = PIN_IN;
				bit_count++;

				st = GET_SAMPLE;
				// Check if the assembled byte is not FLAG - if yes - save it
				if(bit_count >= 8) {
					bit_count = 0;
					if(rcvd_byte != FLAG) {
						PIN_D = 0;
						*pData++ = rcvd_byte;
						byte_count++;
						st = GET_DATA;
						PIN_D = 1;
					}	
				}
				PIN_D = 0;
			}
			break;

		case GET_SAMPLE:
			while(!TimerFlag) {}
			PIN_C = 1;
			rcvd_Sample = PIN_IN;
			st = GET_SAMPLE_EDGE;
			PIN_C = 0;
			break;

		// Set of 2 states to handle HDLC Data	
		// We exit it when we detect first HDLC flag symbol (or any symbol with 6 and more "1"s
		// We also handle bit stuffing here
		case GET_DATA_EDGE:
			if(rcvd_Sample){while(PIN_IN){}} else{while(!PIN_IN) {}}
//			if(PIN_IN != rcvd_Sample) {
			{
				Timer_Counter 	= 0;
				TimerFlag 		= 0;	// Clear overflow flag

				PIN_D = 1;
				// We now have 0.75T time to calclulate everything

				// Beginning of bit stuffing
				//  basically, every "0" after five consecutive "1" should be ignored 
				if(rcvd_Sample != prev_Sample) {	// is this a "0" bit ?
					if(stuff_count < 5) {
						rcvd_byte = (rcvd_byte >> 1) | ((rcvd_Sample == prev_Sample)? 0x80 : 0x00);
						bit_count++;
					}
					stuff_count = 0;
				}else {		// Increment counter of consecutive "1"
					rcvd_byte = (rcvd_byte >> 1) | ((rcvd_Sample == prev_Sample)? 0x80 : 0x00);
					bit_count++;
					stuff_count++;
				}
				// End of bit stuffing
				st = GET_DATA;

				// We got 6 "1" in a row - report as done
				if(stuff_count > 5) {
					st = DONE;
				}else {
					if(bit_count >= 8) {
						PIN_D = 0;
						bit_count = 0;				
						*pData++ = rcvd_byte;
						byte_count++;
						PIN_D = 1;
					}
				}
				PIN_D = 0;
			}
			break;

		case GET_DATA:
			while(!TimerFlag) {}
			PIN_C = 1;
			rcvd_Sample = PIN_IN;
			st = GET_DATA_EDGE;
			PIN_C = 0;
			break;
			
		case TIMEOUT:
  			INTCONbits.GIE = prevIRQ;
			return -1;
			break;

		case DONE:
  			INTCONbits.GIE = prevIRQ;
			return byte_count;
			break;
			
		default:
			break;
		}			
	}	
	INTCONbits.GIE = prevIRQ;
	return -1;
}	


#define		NUM_INITIAL_FLAGS	(10)
#define		NUM_FINAL_FLAGS		(1)

static		unsigned char		tx_byte;
static		unsigned char		next_bit;
static		unsigned char		bit_to_send;

void TxRS485Data(char *pData, int nBytes)
{

	st = INIT;

	// Take care of physical pin
	TRIS_PIN_P 		= OUTPUT;
	TRIS_PIN_N 		= OUTPUT;
	PIN_P 			= 1;
	PIN_N 			= 0;
	next_bit 		= 1;

	DelayMs(80);				// Wait for PLL to become stable

	Timer_Ctrl 		= TIMER_CTRL;

	prevIRQ = INTCONbits.GIE;
  	INTCONbits.GIE = 0;
	
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
					PIN_N = !next_bit;
					TimerFlag = 0;

				  	next_bit = tx_byte & 0x01 ? next_bit : !next_bit;	// For "0" flip the bit, for 1" keep in in the middle of the cell
				  	tx_byte >>= 1;

					while(!TimerFlag) {}	// Middle of the cell
					PIN_P = next_bit;
					PIN_N = !next_bit;
					TimerFlag = 0;
					next_bit = !next_bit; 	// Always flip the bit at the beginning of the cell 
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
					PIN_N = !next_bit;
					TimerFlag = 0;

					// Start calculating the next bit
					bit_to_send = tx_byte & 0x01;
			  		next_bit = bit_to_send ? next_bit : !next_bit;	// For "0" flip the bit, for 1" keep it the same in the middle of the cell
			  		tx_byte >>= 1;

					// Do bit stuffing calculation - after five consequtive "1"s ALWAYS insert "0"
					if(bit_to_send) {
						stuff_count++;			// Count "1"s 
					} else {
						stuff_count = 0;
					} 

					while(!TimerFlag) {}		// We are now at the middle of the cell
					PIN_P = next_bit;
					PIN_N = !next_bit;
					TimerFlag = 0;
					next_bit = !next_bit;	// Always flip the bit at the beginning of the cell 					

					// OK, the bit was sent, now 
					//  do the actual bit stuffing if needed - insert "0"
					if(stuff_count >= 5) {
						while(!TimerFlag) {}		// Wait for the next cell beginning
			  			PIN_P = next_bit;
						PIN_N = !next_bit;
						TimerFlag = 0;

						next_bit = !next_bit;		// Send "0"

						while(!TimerFlag) {}		// Wait for the middle of the cell
						PIN_P = next_bit;
						PIN_N = !next_bit;
						TimerFlag = 0;

						next_bit = !next_bit;	// Always flip the bit at the beginning of the cell 
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
					PIN_N = !next_bit;
					TimerFlag = 0;

				  	next_bit = tx_byte & 0x01 ? next_bit : !next_bit;	// For "0" flip the bit, for 1" keep in in the middle of the cell
				  	tx_byte >>= 1;

					while(!TimerFlag) {}		// Wait for the middle of the cell
					PIN_P = next_bit;
					PIN_N = !next_bit;
					TimerFlag = 0;
					next_bit = !next_bit;	// Always flip the bit at the beginning of the cell 
					bit_count--;
				}	
				byte_count--;
			}
			// Handle the last cell
			while(!TimerFlag) {}		// Wait for the cell start
	  		PIN_P = next_bit;
			PIN_N = !next_bit;
			TimerFlag = 0;
			st = DONE;
		 	break;

		case DONE:
  			INTCONbits.GIE = prevIRQ;
			TRIS_PIN_P 		= INPUT;
			TRIS_PIN_N 		= INPUT;
			return;
			break;
		} 	
	}	
}	

