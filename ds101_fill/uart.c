#include "config.h"
#include "delay.h"
#include "HDLC.h"

#define DTD_BAUDRATE  (9600)
#define DS101_BAUDRATE	(64000)

#define TIMER_DTD 	( ( (XTAL_FREQ * 1000000L) / (4L * 16L * DTD_BAUDRATE)) - 1 )
#define TIMER_DTD_START 	( -(TIMER_DTD/2) )
#define TIMER_DTD_EDGE 	( (TIMER_DTD/2) )
#define TIMER_DTD_CTRL (1<<2 | 2)

#define TIMER_DS101 	( ( (XTAL_FREQ * 1000000L) / ( 4L * DS101_BAUDRATE)) - 1 )
#define TIMER_DS101_START 	( -(TIMER_DS101/2) )
#define TIMER_DS101_EDGE 	( (TIMER_DS101/2) )
#define TIMER_DS101_CTRL (1<<2 | 0)

void (*WriteCharDS101)(char ch);
int (*ReadCharDS101)(void);


//  Simulate UARTs for DTD RS-232 and DS-101 RS-485 communications
//  All interrupts should be disabled during Tx and Rx at 64kBits
//  TMR6 is used to count bits in Tx and Rx
//  TMR1 is used as the timeout counter
//  The biggest timeout is (8 * 2^16)/ (16M /4)= 2^(16 + 3 + 2)/2^4 = 
//  2^17/1M = 0.131072 sec 

//
// Soft UART to communicate with DTD
// Returns:
//  -1  - if no symbol within timeout
//  >=0 - if symbol was detected 
int RxRS232Char()
{
	byte bitcount, data;

	TRIS_RxDTD = INPUT;

	PR6 = TIMER_DTD;
	T6CON = TIMER_DTD_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag
      	
    TMR1H = 0;
  	TMR1L = 0;	// Reset the timeout timer
	PIR1bits.TMR1IF = 0;	// Clear Interrupt
	
	while( !PIR1bits.TMR1IF )	// Until timeout
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
 			while(RxDTD) {};	// Wait for stop bit
			return data;
		}
	}
	return -1;
}

int RxRS232Flag()
{
	char data;
  	char prev, curr;

	TRIS_RxDTD = INPUT;

	PR6 = TIMER_DTD;
	T6CON = TIMER_DTD_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag
      	
  	TMR1H = 0;
  	TMR1L = 0;	// Reset the timeout timer
	PIR1bits.TMR1IF = 0;	// Clear Interrupt
	
  	prev = RxDTD;
	while( !PIR1bits.TMR1IF )	// Loop until timeout
	{
		curr = RxDTD;
		// If transition is detected - reset counter
		if(curr != prev)
		{
			 prev = curr;
			 TMR6 = TIMER_DTD_EDGE;
			 continue;
		}
		// If bit counter expired - shift in the data
		if(PIR5bits.TMR6IF)
		{
			data = (data << 1) | (curr ? 0x00 : 0x01);
			PIR5bits.TMR6IF = 0;	// Clear overflow flag			
			// Check for the FLAG + STOP bit 111111101
			if(data == 0xFD)
			{
				return FLAG;
			}	
		}
	}
	return -1;
}


void TxRS232Char(char data)
{
	byte 	bitcount;
	
	TRIS_TxDTD = OUTPUT;

	PR6 = TIMER_DTD;
	T6CON = TIMER_DTD_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag

	TxDTD = 1;        		// Issue the start bit
	data = ~data;  			// Invert sysmbol
   	// send 8 data bits and 3 stop bits
	for(bitcount = 0; bitcount < 12; bitcount++)
	{
		while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
		PIR5bits.TMR6IF = 0;	// Clear timer overflow bit
		TxDTD = data & 0x01;	// Set the output
		data >>= 1;				// We use the fact that 
						        // "0" bits are STOP bits
	}
} 


//
// Soft UART to communicate with the DS101 via RS-485
// Returns:
//  -1  - if no symbol within timeout
//  >=0 - if symbol was detected 
int RxRS485Char()
{
	byte bitcount, data;

	TRIS_Data_P = INPUT;
	TRIS_Data_N = INPUT;

	PR6 = TIMER_DS101;
	T6CON = TIMER_DS101_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag
      	
    TMR1H = 0;
  	TMR1L = 0;	// Reset the timeout timer
	PIR1bits.TMR1IF = 0;	// Clear Interrupt
	
	while( !PIR1bits.TMR1IF )	// Until timeout
	{
		// Start conditiona was detected - count 1.5 cell size	
		if(Data_N && !Data_P )
		{
			TMR6 = TIMER_DS101_START;
			PIR5bits.TMR6IF = 0;	// Clear overflow flag
			for(bitcount = 0; bitcount < 8 ; bitcount++)
			{
				// Wait until timer overflows
				while(!PIR5bits.TMR6IF){} ;
				PIR5bits.TMR6IF = 0;	// Clear overflow flag
				data = (data >> 1) | ((Data_P && !Data_N) ? 0x80 : 0x00);
			}
 			while(Data_N && !Data_P ) {};	// Wait for stop bit
			return data;
		}
	}
	return -1;
}


int RxRS485Flag()
{
	char data;
  char prev, curr;

	TRIS_Data_P = INPUT;
	TRIS_Data_N = INPUT;

	PR6 = TIMER_DS101;
	T6CON = TIMER_DS101_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag
      	
  	TMR1H = 0;
  	TMR1L = 0;	// Reset the timeout timer
	PIR1bits.TMR1IF = 0;	// Clear Interrupt
	
 	prev = Data_P && !Data_N;
	while( !PIR1bits.TMR1IF )	// Loop until timeout
	{
		curr = Data_P && !Data_N;
		// If transition is detected - reset counter
		if(curr != prev)
		{
			 prev = curr;
			 TMR6 = TIMER_DS101_EDGE;
			 continue;
		}
		// If bit counter expired - shift in the data
		if(PIR5bits.TMR6IF)
		{
			data = (data << 1) | (curr ? 0x01 : 0x00);
			PIR5bits.TMR6IF = 0;	// Clear overflow flag			
			// Check for the FLAG + STOP bit 111111101
			if(data == 0xFD)
			{
				return FLAG;
			}	
		}
	}
	return -1;
}

void TxRS485Char(char data)
{
	byte 	bitcount;
	
	TRIS_Data_P = OUTPUT;
	TRIS_Data_N = OUTPUT;

	PR6 = TIMER_DS101;
	T6CON = TIMER_DS101_CTRL;	
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag

	Data_P = 0;  Data_N = 1;      // Issue the start bit
   	// send 8 data bits
	for(bitcount = 0; bitcount < 9; bitcount++)
	{
		while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
		PIR5bits.TMR6IF = 0;	// Clear timer overflow bit
		Data_P = data & 0x01;	Data_N = !(data & 0x01);// Set the output
		data >>= 1;				
	}
	// Delay for the last data bit
	while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
	PIR5bits.TMR6IF = 0;	// Clear timer overflow bit

	Data_P = 1;	Data_N = 0;// Set the STOP bit 1
	while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
	PIR5bits.TMR6IF = 0;	// Clear timer overflow bit

	Data_P = 1;	Data_N = 0;// Set the STOP bit 2	
	while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
	PIR5bits.TMR6IF = 0;	// Clear timer overflow bit

	Data_P = 1;	Data_N = 0;// Set the STOP bit 3	
	while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
	PIR5bits.TMR6IF = 0;	// Clear timer overflow bit
} 
