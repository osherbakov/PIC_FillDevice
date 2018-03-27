#include "config.h"
#include "delay.h"
#include "controls.h"
#include "Fill.h"
#include "serial.h"
#include "gps.h"
#include "rtc.h"



void open_eusart(unsigned char baudrate_reg, unsigned char rxtx_polarity)
{
	pinMode(RxPC, INPUT);
	pinMode(TxPC, INPUT);

	RCSTA1bits.CREN = 0; // Disable Rx
 	TXSTA1bits.TXEN = 0; // Disable Tx	
	PIE1bits.RC1IE = 0;	 // Disable RX interrupt
	PIE1bits.TX1IE = 0;	 // Disable TX Interrupts
	RCSTA1bits.SPEN = 0; // Disable EUSART

	// Set up the baud rate
	SPBRGH1 = 0x00;
	SPBRG1 = baudrate_reg ;
	BAUDCON1 = rxtx_polarity;

	// Set up the Rx portion
	rx_idx_in = rx_idx_out = 0;
	rx_data = (volatile byte *) &RxTx_buff[0];
	rx_idx_max = sizeof(RxTx_buff) - 1;

	// Set up the Tx portion
	tx_data = (volatile byte *) &RxTx_buff[0];
	tx_count = 0;
	
	RCSTA1bits.CREN = 1; // Enable Rx
 	TXSTA1bits.TXEN = 1; // Enable Tx	
	RCSTA1bits.SPEN = 1; // Enable EUSART
}

void close_eusart()
{
	PIE1bits.RC1IE = 0;	 	// Disable RX Interrupt
	PIE1bits.TX1IE = 0;		// Disable TX Interrupts
	TXSTA = 0x00; 	      	// Disable Tx	
	RCSTA = 0x00;			// Disable EUSART

	// Set up the Rx portion
	rx_idx_in = rx_idx_out = 0;
	rx_data = (volatile byte *) &RxTx_buff[0];
	rx_idx_max = sizeof(RxTx_buff) - 1;

	// Set up the Tx portion
	tx_data = (volatile byte *) &RxTx_buff[0];
	tx_count = 0;
}

void rx_eusart_async(unsigned char *p_rx_data, byte max_size, unsigned int timeout)
{
	PIE1bits.RC1IE = 0;	 // Disable RX interrupt
	RCSTA1bits.CREN = 0; // Disable Rx

	rx_idx_in = rx_idx_out = 0;
	rx_data = (volatile byte *) p_rx_data;
	rx_idx_max = max_size - 1;
	
	set_timeout(timeout);
	RCSTA1bits.CREN = 1; // Enable Rx
	PIE1bits.RC1IE = 1;	 // Enable RX interrupt
}

  

// Receive the specified number of characters with timeout
// RX_TIMEOUT1_PC - timeout until 1st char received
// RX_TIMEOUT2_PC - timeout for all consequtive chars
byte rx_eusart(unsigned char *p_data, byte ncount, unsigned int timeout)
{
  	byte	symbol;
  	byte  nrcvd = 0;
	RCSTA1bits.CREN = 1; // Enable Rx
	PIE1bits.RC1IE = 0;	 // Disable RX interrupt

  	set_timeout(timeout);
	while( (nrcvd < ncount ) && is_not_timeout() )
	{
		if(PIR1bits.RC1IF)	// Data is avaiable
		{
			// Get data byte and save it
			symbol = RCREG1;
			*p_data++ = symbol;
			nrcvd++;
		  	set_timeout(RX_TIMEOUT2_PC);
			// overruns? clear it
			if(RCSTA1 & 0x06)
			{
				RCSTA1bits.CREN = 0;
				RCSTA1bits.CREN = 1;
			}
		}
	}
  	return nrcvd;
}

byte rx_eusart_line(unsigned char *p_data, byte ncount, unsigned int timeout)
{
  	byte	symbol;
  	byte  	nrcvd = 0;
	RCSTA1bits.CREN = 1; // Enable Rx
	PIE1bits.RC1IE = 0;	 // Disable RX interrupt

  	set_timeout(timeout);
	while( (nrcvd < ncount) && is_not_timeout())
	{
		if(PIR1bits.RC1IF)	// Data is avaiable
		{
			// Get data byte and save it
			symbol = RCREG1;
			*p_data++ = symbol;
			if(symbol == '\n' || symbol == '\r')  {
				break;
			}	
			nrcvd++;
  			set_timeout(timeout);
			// overruns? clear it
			if(RCSTA1 & 0x06)
			{
				RCSTA1bits.CREN = 0;
				RCSTA1bits.CREN = 1;
			}
		}
	}
  	return nrcvd;
}


volatile byte *tx_data; // Pointer to the current output data
volatile byte tx_count; // currect output counter

volatile byte *rx_data;   // Pointer to the current receive data
volatile byte rx_idx_in;  // Index of characters received
volatile byte rx_idx_out; // Index of characters consumed
volatile byte rx_idx_max; // Max index in the buffer
static  char prev;

// Return the number of bytes received and stored in the buffer
byte rx_eusart_count(void)
{
	byte	count;

	prev = INTCONbits.GIE;
	INTCONbits.GIE = 0;		// Disable interrupts
	count = rx_idx_in - rx_idx_out;	
	if(count == 0) { rx_idx_in = rx_idx_out = 0; }

	INTCONbits.GIE = prev;

	return	count;
}	

void rx_eusart_reset(void)
{
	prev = INTCONbits.GIE;
	INTCONbits.GIE = 0;		// Disable interrupts
	rx_idx_in = rx_idx_out = 0; 
	INTCONbits.GIE = prev;
}	


int rx_eusart_symbol(void)
{
	int 	result = -1;

	prev = INTCONbits.GIE;
	INTCONbits.GIE = 0;		// Disable interrupts
	
	if (rx_idx_in > rx_idx_out) {
		result = ((int)rx_data[rx_idx_out++]) & 0x00FF;
	}
	// Check for the all symbols taken and adjust the indices
	if(rx_idx_in == rx_idx_out){rx_idx_in = rx_idx_out = 0; }

	INTCONbits.GIE = prev;

	return	result;
}	

byte rx_eusart_data(unsigned char *p_data, byte ncount, unsigned int timeout)
{
	prev = INTCONbits.GIE;
	INTCONbits.GIE = 0;		// Disable interrupts
	
  	set_timeout(timeout);
  	
	ncount = MIN(ncount, (rx_idx_in - rx_idx_out));
	memcpy((void *)p_data, (void *)&rx_data[rx_idx_out], ncount);
	rx_idx_out += ncount;
	// Check for the all symbols taken and adjust the indices
	if(rx_idx_in == rx_idx_out){ rx_idx_in = rx_idx_out = 0; }

	INTCONbits.GIE = prev;

	return	ncount;
}	


void tx_eusart_async(const unsigned char *p_data, byte ncount)
{
	tx_eusart_flush();	// Make sure that the old data is fully flushed and sent...
	tx_data = (volatile byte *) p_data;
	tx_count = ncount;
 	TXSTA1bits.TXEN = 1; // Enable Tx	
	PIE1bits.TX1IE = 1;	// Interrupt will be generated
}

void tx_eusart_flush()
{
  	if(	PIE1bits.TX1IE )	// If Tx is enabled - wait until all chars are sent out
  	{
		while( tx_count || !TXSTA1bits.TRMT ) {};	// Wait to finish previous Tx
	}
}

