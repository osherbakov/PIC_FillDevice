#include "config.h"
#include "delay.h"
#include "controls.h"
#include "Fill.h"
#include "serial.h"
#include "gps.h"

static byte KEY_FILL[] = "/FILL";
static byte KEY_DUMP[] = "/DUMP";


void open_eusart()
{
	if(!RCSTA1bits.SPEN)
	{
		TRIS_Rx = INPUT;
		TRIS_Tx = INPUT;

		SPBRGH1 = 0x00;
		SPBRG1 = BRREG_CMD;
		BAUDCON1 = DATA_POLARITY;
	
		tx_count = 0;
		TXSTA1bits.TXEN = 1; // Enable Tx	
		RCSTA1bits.CREN = 1; // Enable Rx
		RCSTA1bits.SPEN = 1; // Enable EUSART
	}
}

void close_eusart()
{
	PIE1bits.TX1IE = 0;		// Disable Interrupts
	TXSTA1bits.TXEN = 0; 	// Disable Tx	
	RCSTA = 0;				// Disable EUSART
}

void PCInterface()
{
	if( rx_eusart(&data_cell[0], 6) == 0 ) return;
	
	if( is_equal(&data_cell[0], KEY_FILL, 5))
	{
		GetStoreFill(data_cell[5]);
	}else if(is_equal( &data_cell[0], KEY_DUMP, 5))
	{
		SendStoredFill(data_cell[5]);
	}
}

byte rx_eusart(unsigned char *p_data, byte ncount)
{
	byte	nrcvd = 0;	
	
    set_timeout(RX_TIMEOUT_PC);
	while( (ncount > nrcvd) && is_not_timeout() )
	{
		if(PIR1bits.RC1IF)	// Data is avaiable
		{
			// Get data byte and save it
			*p_data++ = RCREG1;
		    set_timeout(RX_TIMEOUT_PC);
			// overruns? clear it
			if(RCSTA1 & 0x06)
			{
				RCSTA1bits.CREN = 0;
				RCSTA1bits.CREN = 1;
			}
			nrcvd++;
		}
	}
	return nrcvd;
}

volatile byte *tx_data;
volatile byte tx_count;

void tx_eusart(unsigned char *p_data, byte ncount)
{
	if( tx_count ) {};	// Wait to finish previous Tx

	tx_data = (volatile byte *) p_data;
	tx_count = ncount;
	
	PIE1bits.TX1IE = 1;	// Interrupt will be generated
}

byte rx_mbitr(unsigned char *p_data, byte ncount)
{
	byte bitcount, data;
	byte	nrcvd = 0;	

	TRIS_Rx = INPUT;
	PR6 = TIMER_CMD;
    set_timeout(RX_TIMEOUT_MBITR);
	while( (ncount > nrcvd) && is_not_timeout() )
	{
		// Start conditiona was detected - count 1.5 cell size	
		if(RxBIT )
		{
			TMR6 = TIMER_START_CMD;
			PIR5bits.TMR6IF = 0;	// Clear overflow flag
			for(bitcount = 0; bitcount < 8 ; bitcount++)
			{
				// Wait until timer overflows
				while(!PIR5bits.TMR6IF){} ;
				data = (data >> 1) | (RxBIT ? 0x80 : 0x00);
				PIR5bits.TMR6IF = 0;	// Clear overflow flag
			}
			*p_data++ = ~data;
			nrcvd++;
			while(RxBIT) {};
		}
	}
	return nrcvd;
}



void tx_mbitr(byte *p_data, byte ncount)
{
	byte 	bitcount;
	byte 	data;
	
	TRIS_Tx = OUTPUT;
	PR6 = TIMER_CMD;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag

	while(ncount-- )
	{
		TxBIT = START;
		data = ~(*p_data++);
		for(bitcount = 0; bitcount < 12; bitcount++)
		{
			while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
			PIR5bits.TMR6IF = 0;	// Clear timer overflow bit
			TxBIT = data & 0x01;	// Set the output
			data >>= 1;				// We use the fact that 
									// "0" bits are STOP bits
		}
	}
}
