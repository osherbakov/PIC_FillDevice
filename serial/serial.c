#include "config.h"
#include "delay.h"
#include "controls.h"
#include "Fill.h"
#include "serial.h"
#include "gps.h"
#include "string.h"

#define tx_eusart_str(a) tx_eusart((a), strlen((char *)a))
#define tx_eusart_buff(a) tx_eusart((a), NUM_ELEMS(a))

// The command from the PC to fill the key or to dump it
// The byte after the command specifies the key number and type
//  /FILL<0x34> means fill key 4 of type 3
static byte KEY_FILL[] 		= "/FILL";	// Fill the key N
static byte KEY_DUMP[] 		= "/DUMP";	// Dump the key N

/****************************************************************/
// Those are commands and responses that MBITR receives and sends
// When programmed from the PC
/****************************************************************/
// Command to request a serial number
static byte SN_REQ[]		= {0x2F, 0x39, 0x38, 0x0D}; // "/98\n"	
static byte SN_RESP[]		= {0x53, 0x4E, 0x20, 0x3D, 0x20, 0x00, // "SN = "
 0x31, 0x32, 0x33, 0x34, 0x35, 0x0D, 					// "12345\n"
 0x4F, 0x4B, 0x0D, 0x00 };								// "OK\n"

// The pattern of 2400 baud 0x7E flags as seen from 9600 baud view
static byte HDLC_FLAGS1[] = {0x80, 0x78};
static byte HDLC_FLAGS2[] = {0x78, 0x78};
static byte HDLC_FLAGS3[] = {0x78, 0xF8};

// Command to request the radio capabilities
static byte OPT_REQ[] 		= {0x2F, 0x38, 0x34, 0x0D};	// "/84\n" ;

// Capabilities options
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

static byte OPT_ENABLED[] 	= "Radio Option Enabled";
static byte OK_RESP[] 		= {0x4F, 0x4B, 0x0D};		// "OK\n"

// The list of all options enabled in the radio
static byte *OPTS[] = 
{
	OPT_BASIC, OPT_RETRAN, OPT_SINC1, OPT_SINC2, OPT_HQ2, OPT_ANDVT,
	OPT_DES, OPT_AES, OPT_LDRM, OPT_OPT_X, OPT_MELP, OPT_DIGITAL, OPT_GPS
};

static void send_options(void)
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

static unsigned char SerialBuffer[4];

char CheckFillDTD232Type5()
{
	TRIS_RxDTD = INPUT;
	
	// Only process data when in SPACE
	if(RxDTD == 0)
	{
		 return MODE5;
	}	
	return -1;
}	


char CheckFillRS232Type5()
{
	TRIS_RxPC = INPUT;
	
	// Only process data when in SPACE
	if(RxPC == 0)
	{
	  // Coming in first time - enable eusart and setup buffer
		if( RCSTA1bits.SPEN == 0)
		{
			open_eusart_rx();
			start_eusart_rx(SerialBuffer, 4);
			return -1;
		}
		
		if( (rx_count >= 2) && 
				(is_equal(SerialBuffer, HDLC_FLAGS1, 2) ||
				 	is_equal(SerialBuffer, HDLC_FLAGS2, 2) ||
				  	is_equal(SerialBuffer, HDLC_FLAGS3, 2) ) )
		{
			 close_eusart();
			 return MODE5;
		}
	}	
	return -1;
}	

// Check serial port if there is a request to send DES keys
char CheckFillType4()
{
	TRIS_RxPC = INPUT;
	
	// only process when line is in SPACE
	if(RxPC == 0)
	{
	  // Coming in first time - enable eusart and setup buffer
		if( !RCSTA1bits.SPEN)
		{
			open_eusart_rx();
			start_eusart_rx(SerialBuffer, 4);
    	return -1;
		}
	
	  // Four characters are collected - check if it is 
	  //  /98 - serial number request, or
	  //  /84 - the capabilities request
		if(rx_count >= 4)
		{
			if( is_equal(SerialBuffer, SN_REQ, 4) )
			{
	  		// SN request - send a fake SN = 123456
				tx_eusart_buff(SN_RESP);
				start_eusart_rx(SerialBuffer, 4);
			}else if(is_equal(SerialBuffer, OPT_REQ, 4))
			{
				// Options request - send all options
				send_options();
				while( tx_count || !TXSTA1bits.TRMT ) {};	// Wait to finish previous Tx
				// Prepare to receive all next data as the Type 4 fill
				open_eusart_rxtx();
				start_eusart_rx(SerialBuffer, 4);
				return MODE4;
			}
		}
	}
	return -1;
}


// Open EUSART for reading only, TX is not affected
void open_eusart_rx()
{
	TRIS_RxPC = INPUT;

	RCSTA1bits.SPEN = 0; // Disable EUSART
	SPBRGH1 = 0x00;
	SPBRG1 = BRREG_CMD;
	BAUDCON1 = DATA_POLARITY;

	rx_count = 0;
	PIE1bits.RC1IE = 0;	 // Disable RX interrupt
	RCSTA1bits.CREN = 1; // Enable Rx
	RCSTA1bits.SPEN = 1; // Enable EUSART
}


void open_eusart_rxtx()
{
	TRIS_RxPC = INPUT;
	TRIS_TxPC = INPUT;

	SPBRGH1 = 0x00;
	SPBRG1 = BRREG_CMD;
	BAUDCON1 = DATA_POLARITY;

	rx_count = 0;
	tx_count = 0;
	PIE1bits.RC1IE = 0;	 // Disable RX interrupt
	PIE1bits.TX1IE = 0;	 // Disable TX Interrupts
	TXSTA1bits.TXEN = 1; // Enable Tx	
	RCSTA1bits.CREN = 1; // Enable Rx
	RCSTA1bits.SPEN = 1; // Enable EUSART
}

// Set up interrupt-driven Rx buffers and counters
void start_eusart_rx(unsigned char *p_data, byte ncount)
{
	rx_data = (volatile byte *) p_data;
	rx_count = 0;
	rx_count_1 = ncount - 1;
	PIE1bits.RC1IE = 1;	 // Enable Interrupts
}


void close_eusart()
{
	PIE1bits.RC1IE = 0;	 	// Disable RX interrupt
	PIE1bits.TX1IE = 0;		// Disable TX Interrupts
	TXSTA1bits.TXEN = 0; 	// Disable Tx	
	RCSTA = 0;					  // Disable EUSART
}

void PCInterface()
{
	// If entering the first time - enable eusart
	// and initialize the buffer to get chars
	if( RCSTA1bits.SPEN == 0)
	{
		open_eusart_rxtx();
		start_eusart_rx(&data_cell[0], 6);
	}
	
	// Wait to receive 6 characters
	if(rx_count < 6) return;
	
	// Six or more characters received - chaeck if
	// this is a /DUMPN request to dump keys to PC
	//  or it is a /FILLN request to load key from PC
	if( is_equal(&data_cell[0], KEY_FILL, 5))
	{
  	// The last char in /FILLN specifies Type(high nibble) 
  	//    and Slot Number (low nibble)
		StorePCFill(data_cell[5] & 0x0F, (data_cell[5] >> 4) & 0x0F);
	}else if(is_equal( &data_cell[0], KEY_DUMP, 5))
	{
  	// The last char in /DUMPN is the slot number
		WaitReqSendPCFill(data_cell[5] & 0x0F);
	}
}


// Receive the specified number of characters with timeout
// RX_TIMEOUT1_PC - timeout until 1st char received
// RX_TIMEOUT2_PC - timeout for all consequtive chars
byte rx_eusart(unsigned char *p_data, byte ncount)
{
  byte	nrcvd = 0;

	// We collect chars in the loop - no need for interrupts
	PIE1bits.RC1IE = 0;	 // Disable RX interrupt

  set_timeout(RX_TIMEOUT1_PC);
	while( (nrcvd < ncount ) && is_not_timeout() )
	{
		if(PIR1bits.RC1IF)	// Data is avaiable
		{
			// Get data byte and save it
			*p_data++ = RCREG1;
		  set_timeout(RX_TIMEOUT2_PC);
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


volatile byte *tx_data; // Pointer to the current output data
volatile byte tx_count; // currect output counter

volatile byte *rx_data; // Pointer to the current receive data
volatile byte rx_count; // Number of characters received
volatile byte rx_count_1; // Max index in the buffer


void tx_eusart(unsigned char *p_data, byte ncount)
{
	TXSTA1bits.TXEN = 1; // Enable Tx	
	while( tx_count || !TXSTA1bits.TRMT ) {};	// Wait to finish previous Tx

	tx_data = (volatile byte *) p_data;
	tx_count = ncount;
	PIE1bits.TX1IE = 1;	// Interrupt will be generated
}

