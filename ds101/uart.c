#include "config.h"
#include "delay.h"
#include "HDLC.h"
#include "Fill.h"
#include "serial.h"

#define TIMER_DTD 			( ( (XTAL_FREQ * 1000000L) / (4L * 16L * DTD_BAUDRATE)) - 1 )
#define TIMER_DTD_START 	( -(TIMER_DTD/2) )
#define TIMER_DTD_EDGE 		( (TIMER_DTD/2) )
#define TIMER_DTD_CTRL 		( (1<<2) | 2)     // ENA, 1:16

#define TIMER_DS101 		( ( (XTAL_FREQ * 1000000L) / ( 4L * DS101_BAUDRATE)) - 1 )
#define TIMER_DS101_START 	( -(TIMER_DS101/2) )
#define TIMER_DS101_EDGE 	( (TIMER_DS101/2) )
#define TIMER_DS101_CTRL 	((1<<2) | 0)   // ENA, 1:1

void (*OpenDS101)();
void (*WriteCharDS101)(char ch);
int (*ReadCharDS101)(void);


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
		TxPC = data & 0x01;	// Set the output
		data >>= 1;				  // We use the fact that 
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
	TRIS_Data_P = INPUT;
  	WPUB_Data_P = 1;
  	WPUB_Data_N = 1;
}

//
// Soft UART to communicate with the DS101 via RS-485 at 64Kbd
// Returns:
//  -1  - if no symbol within timeout
//  >=0 - if symbol was detected 
int RxRS485Char()
{
	byte 	bitcount, data;
	byte	prev;
	int		result;

	TRIS_Data_N = INPUT;
	TRIS_Data_P = INPUT;

	PR6 = TIMER_DS101;
	T6CON = TIMER_DS101_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag
      	
	prev = INTCONbits.GIE;
  	INTCONbits.GIE = 0;

  	while( is_not_timeout() )
	{
		// Start conditiona was detected - count 1.5 cell size	
		if(Data_N && !Data_P)
		{
			TMR6 = TIMER_DS101_START;
			PIR5bits.TMR6IF = 0;	// Clear overflow flag
			for(bitcount = 0; bitcount < 8 ; bitcount++)
			{
				// Wait until timer overflows
				while(!PIR5bits.TMR6IF){} ;
				PIR5bits.TMR6IF = 0;	// Clear overflow flag
				data = (data >> 1) | ((Data_N) ? 0x00 : 0x80);
			}
			while(!PIR5bits.TMR6IF){} ;
			result = (Data_N && !Data_P) ? -1 : ((int)data) & 0x00FF;
		}
	}

  	INTCONbits.GIE = prev;
	return result;
}

void TxRS485Char(char data)
{
	byte 	bitcount;
	TRIS_Data_N = OUTPUT;
	TRIS_Data_P = OUTPUT;

	data = ~data;

	PR6 = TIMER_DS101;
	T6CON = TIMER_DS101_CTRL;	
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag

	Data_N = 1;      // Issue the start bit
	Data_P = 0;      // Issue the start bit
	while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
	PIR5bits.TMR6IF = 0;	// Clear timer overflow bit

	// send 8 data bits
	for(bitcount = 0; bitcount < 8; bitcount++)
	{
		Data_N = data & 0x01; // Set the output N
		Data_P = ~Data_N;     // Set the output P
		data >>= 1;				
		while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
		PIR5bits.TMR6IF = 0;	// Clear timer overflow bit
	}

	Data_N = 0;// Set the STOP bit 1
	Data_P = 1;// Set the STOP bit 1
	while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
	PIR5bits.TMR6IF = 0;	// Clear timer overflow bit

	Data_N = 0;// Set the STOP bit 2	
	Data_P = 1;// Set the STOP bit 1
	while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
	PIR5bits.TMR6IF = 0;	// Clear timer overflow bit

	Data_N = 0;// Set the STOP bit 3	
	Data_P = 1;// Set the STOP bit 1
	while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
	PIR5bits.TMR6IF = 0;	// Clear timer overflow bit
} 
