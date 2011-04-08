#include "config.h"
#include "delay.h"
#include "controls.h"
#include "Fill.h"
#include "serial.h"
#include "gps.h"
#include "string.h"

#define tx_eusart_str(a) tx_eusart((a), strlen((char *)a))
#define tx_eusart_buff(a) tx_eusart((a), NUM_ELEMS(a))

static byte KEY_FILL[] 		= "/FILL";
static byte KEY_DUMP[] 		= "/DUMP";

static byte SN_REQ[]		= {0x2F, 0x39, 0x38, 0x0D}; // "/98\n"	
static byte SN_RESP[]		= {0x53, 0x4E, 0x20, 0x3D, 0x20, 0x00, // "SN = "
 0x31, 0x32, 0x33, 0x34, 0x35, 0x0D, 					// "12345\n"
 0x4F, 0x4B, 0x0D, 0x00 };								// "OK\n"

static byte OPT_REQ[] 		= {0x2F, 0x38, 0x34, 0x0D};	// "/84\n" ;
static byte OK_RESP[] 		= {0x4F, 0x4B, 0x0D};		// "OK\n"

static byte OPT_ENABLED[] 	= "Radio Option Enabled";
static byte OPT_BASIC[]  	= "Basic ";
static byte OPT_RETRAN[] 	= "Retransmit ";
static byte OPT_SINC1[]  	= "Sincgar ";
static byte OPT_SINC2[] 	= "SincFH2 ";
static byte OPT_HQ2[] 		= "HqII ";
static byte OPT_ANDVT[] 	= "Andvt ";
static byte OPT_DES[] 		= "Des ";
static byte OPT_AES[] 		= "Aes ";
static byte OPT_LDRM[] 		= "LDRM ";
static byte OPT_OPT_X[] 	= "Option X ";
static byte OPT_MELP[] 		= "Andvt MELP ";
static byte OPT_DIGITAL[] 	= "Clear Digital ";
static byte OPT_GPS[] 		= "Enhanced GPS ";

static byte OPT_END[] 		= {0x0D, 0x00};

static byte *OPTS[] = 
{
	OPT_BASIC, OPT_RETRAN, OPT_SINC1, OPT_SINC2, OPT_HQ2, OPT_ANDVT,
	OPT_DES, OPT_AES, OPT_LDRM, OPT_OPT_X, OPT_MELP, OPT_DIGITAL, OPT_GPS
};

void send_options()
{
	byte idx;
	// Go thru the list of all options and send them out.
	//  Each option is terminated by 0x0D 0x00 sequence
	for( idx = 0; idx < NUM_ELEMS(OPTS); idx++)
	{
		tx_eusart_str(OPTS[idx]);
		tx_eusart_str(OPT_ENABLED);
		tx_eusart_buff(OPT_END);
	}
	// Terminate everything with "OK\n"
	tx_eusart_buff(OK_RESP);
}

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
