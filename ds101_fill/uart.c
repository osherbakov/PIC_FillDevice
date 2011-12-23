#include "config.h"
#include "delay.h"

#define DTD_BAUDRATE  (9600)
#define DS101_BAUDRATE	(64000)

#define TIMER_DTD 	( ( (XTAL_FREQ * 1000000L) / ( 64L * DTD_BAUDRATE)) - 1 )
#define TIMER_DTD_START 	( -(TIMER_DTD/2) )

#define TIMER_DS101 	( ( (XTAL_FREQ * 1000000L) / ( 64L * DS101_BAUDRATE)) - 1 )
#define TIMER_DS101_START 	( -(TIMER_DS101/2) )

// Use negative logic
#define  START	(1)
#define  STOP	(0)

void (*WriteCharDS101)(char ch);
int (*ReadCharDS101)(void);


//  Simulate UARTs for DTD RS-232 and DS-101 RS-485 communications
//  All interrupts are disabled during Tx and Rx
//  TMR6 is used to count bits in Tx and Rx
//  TMR1 is used as the timeout counter
//  The biggest timeout is (8 * 2^16)/ (16M /4)= 2^(16 + 3 + 2)/2^4 = 
//  2^17/1M = 0.131072 sec 

//
// Soft UART to communicate with DTD
// Returns:
//  -1  - if no symbol within timeout
//  >=0 - if symbol was detected 
int RxDTDChar()
{
	byte bitcount, data;

	TRIS_RxDTD = INPUT;
	PR6 = TIMER_DTD;
      	
    TMR1H = 0;
  	TMR1L = 0;	// Reset the timer
    T1GCONbits.TMR1GE = 0;	// No gating
    T1CON = (0x3 << 4) | (1<<1) | 1; // 8-prescalar, 16-bits, enable
	PIR1bits.TMR1IF = 0;	// Clear Interrupt
	
	while( !PIR1bits.TMR1IF )	// Until timeout
	{
		// Start conditiona was detected - count 1.5 cell size	
		if(RxDTD )
		{
			TMR6 = TIMER_DTD_START;
			PIR1bits.TMR1IF = 0;	// Clear overflow flag
			for(bitcount = 0; bitcount < 8 ; bitcount++)
			{
				// Wait until timer overflows
				while(!PIR1bits.TMR1IF){} ;
				PIR1bits.TMR1IF = 0;	// Clear overflow flag
				data = (data >> 1) | (RxDTD ? 0x00 : 0x80);
			}
 			while(RxDTD) {};	// Wait for stop bit
			return data;
		}
	}
	return -1;
}



void TxDTDChar(char data)
{
	byte 	bitcount;
	
	TRIS_TxDTD = OUTPUT;
	
	PR6 = TIMER_DTD;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag
	TxDTD = START;        // Issue the start bit
	data = ~data;  		// invert sysmbol
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
// Soft UART to communicate with the DS101
// Returns:
//  -1  - if no symbol within timeout
//  >=0 - if symbol was detected 
int RxDSChar()
{
	byte bitcount, data;

	TRIS_Data_P = INPUT;
	TRIS_Data_N = INPUT;
	PR6 = TIMER_DS101;
      	
    TMR1H = 0;
  	TMR1L = 0;	// Reset the timer
    T1GCONbits.TMR1GE = 0;	// No gating
    T1CON = (0x3 << 4) | (1<<1) | 1; // 8-prescalar, 16-bits, enable
	PIR1bits.TMR1IF = 0;	// Clear Interrupt
	
	while( !PIR1bits.TMR1IF )	// Until timeout
	{
		// Start conditiona was detected - count 1.5 cell size	
		if(Data_N && !Data_P )
		{
			TMR6 = TIMER_DS101_START;
			PIR1bits.TMR1IF = 0;	// Clear overflow flag
			for(bitcount = 0; bitcount < 8 ; bitcount++)
			{
				// Wait until timer overflows
				while(!PIR1bits.TMR1IF){} ;
				PIR1bits.TMR1IF = 0;	// Clear overflow flag
				data = (data >> 1) | ((Data_P && !Data_N) ? 0x80 : 0x00);
			}
 			while(Data_N && !Data_P ) {};	// Wait for stop bit
			return data;
		}
	}
	return -1;
}



void TxDSChar(char data)
{
	byte 	bitcount;
	
	TRIS_Data_P = OUTPUT;
	TRIS_Data_N = OUTPUT;
	PR6 = TIMER_DS101;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag
	Data_P = 0;  Data_N = 1;      // Issue the start bit
   	// send 8 data bits and 3 stop bits
	for(bitcount = 0; bitcount < 12; bitcount++)
	{
		while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
		PIR5bits.TMR6IF = 0;	// Clear timer overflow bit
		Data_P = data & 0x01;	Data_N = !(data & 0x01);// Set the output
		data >>= 1;				// We use the fact that 
						        // "0" bits are STOP bits
	}
} 
