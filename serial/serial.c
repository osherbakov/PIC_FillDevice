#include "config.h"
#include "delay.h"
#include "controls.h"
#include "Fill.h"
#include "serial.h"
#include "gps.h"
#include "rtc.h"
#include "string.h"

#define tx_eusart_str(a) tx_eusart((a), strlen((char *)a))
#define tx_eusart_buff(a) tx_eusart((a), NUM_ELEMS(a))

// The command from the PC to fill the key or to dump it
// The byte after the command specifies the key number and type
//  /FILL<0x34> means fill key 4 of type 3
static byte KEY_FILL[] 		= "/FILL";	// Fill the key N
static byte KEY_DUMP[] 		= "/DUMP";	// Dump the key N
static byte TIME_CMD[]    = "/TIME";	// Fill/Dump Time

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
static byte OPT_DIGITAL[] = "Clear Digital ";
static byte OPT_GPS[] 		= "Enhanced GPS ";
static byte OPT_END[] 		= {0x0D, 0x00};

static byte OPT_ENABLED[] = "Radio Option Enabled";
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

static unsigned char SerialBuffer[6];

char CheckFillDTD232Type5()
{
	if( !RCSTA1bits.SPEN )
	{
  	if( !RxDTD )
  	{
  		 return MODE5;
  	}	
  } 	
	return -1;
}	


char CheckFillRS232Type5()
{
  // Coming in first time - enable eusart and setup buffer
	if( !RCSTA1bits.SPEN )
	{
  	if(!RxPC)
  	{
      // Coming in first time - enable eusart and setup buffer
  		open_eusart_rxtx(SerialBuffer, 4);
  	}	
	}else	if( (rx_count >= 2) && 
			  (is_equal(SerialBuffer, HDLC_FLAGS1, 2) ||
			 	  is_equal(SerialBuffer, HDLC_FLAGS2, 2) ||
			  	  is_equal(SerialBuffer, HDLC_FLAGS3, 2) ) )
	{
		 close_eusart();
		 return MODE5;
	}
	return -1;
}	

// Check serial port if there is a request to send DES keys
char CheckFillType4()
{
	if( !RCSTA1bits.SPEN)
	{
  	if(!RxPC)
  	{
      // Coming in first time - enable eusart and setup buffer
  		open_eusart_rxtx(SerialBuffer, 4);
  	}	
	}else if(rx_count >= 4)
	{
    // Four characters are collected - check if it is 
    //  /98 - serial number request, or
    //  /84 - the capabilities request
		if( is_equal(SerialBuffer, SN_REQ, 4) )
		{
			rx_count = 0; // Data consumed
  		// SN request - send a fake SN = 123456
			tx_eusart_buff(SN_RESP);
		}else if(is_equal(SerialBuffer, OPT_REQ, 4))
		{
			rx_count = 0; // Data consumed
			// Options request - send all options
			send_options();
			while( tx_count || !TXSTA1bits.TRMT ) {};	// Wait to finish previous Tx
			// Prepare to receive all next data as the Type 4 fill
			open_eusart_rxtx(SerialBuffer, 4);
			return MODE4;
		}
	}
	return -1;
}


void open_eusart_rxtx(unsigned char *p_rx_data, byte max_size)
{
	TRIS_RxPC = INPUT;
	TRIS_TxPC = INPUT;

	PIE1bits.RC1IE = 0;	 // Disable RX interrupt
	PIE1bits.TX1IE = 0;	 // Disable TX Interrupts
	RCSTA1bits.SPEN = 0; // Disable EUSART
	SPBRGH1 = 0x00;
	SPBRG1 = BRREG_CMD;
	BAUDCON1 = DATA_POLARITY;

	rx_data = (volatile byte *) p_rx_data;
	rx_count = 0;
	rx_count_1 = max_size - 1;
	tx_count = 0;
	
	RCSTA1bits.CREN = 1; // Enable Rx
 	TXSTA1bits.TXEN = 1; // Enable Tx	
	RCSTA1bits.SPEN = 1; // Enable EUSART

	PIE1bits.RC1IE = 1;	 // Enable RX interrupt
}

void close_eusart()
{
	PIE1bits.RC1IE = 0;	 	// Disable RX interrupt
	PIE1bits.TX1IE = 0;		// Disable TX Interrupts
	TXSTA = 0x00; 	      // Disable Tx	
	RCSTA = 0x00;				  // Disable EUSART
}

void PCInterface()
{
	// If entering the first time - enable eusart
	// and initialize the buffer to get chars
	if( RCSTA1bits.SPEN == 0)
	{
		open_eusart_rxtx(&data_cell[0], 6);
	}
	
	// Wait to receive 6 characters
	if(rx_count >= 6) 
	{
  	// Six or more characters received - check if
  	// this is a /DUMPN request to dump keys to PC
  	//  or it is a /FILLN request to load key from PC
  	if( is_equal(&data_cell[0], KEY_FILL, 5))
  	{
    	// The last char in /FILLN specifies Type(high nibble) 
    	//    and Slot Number (low nibble)
  		rx_count = 0; // Data consumed
  		StorePCFill(data_cell[5] & 0x0F, (data_cell[5] >> 4) & 0x0F);
  	}else if(is_equal( &data_cell[0], KEY_DUMP, 5))
  	{
    	// The last char in /DUMPN is the slot number
  		rx_count = 0; // Data consumed
  		WaitReqSendPCFill(data_cell[5] & 0x0F);
  	}else if(is_equal( &data_cell[0], TIME_CMD, 5))
  	{
    	// The last char in /TIMEX is either "D" - Dump, or "F" - Fill
  		rx_count = 0; // Data consumed
    	GetCurrentDayString(&data_cell[0]);
    	tx_eusart_str(data_cell);
    } 	
  }  
}


// Receive the specified number of characters with timeout
// RX_TIMEOUT1_PC - timeout until 1st char received
// RX_TIMEOUT2_PC - timeout for all consequtive chars
byte rx_eusart(unsigned char *p_data, byte ncount)
{
  byte  n_rcvd;
  byte  *rx_data_saved;
  byte  rx_count_1_saved;

  // Save previous buffer setup  
	PIE1bits.RC1IE = 0;	 // Disable RX interrupt
	rx_data_saved = rx_data;
	rx_count_1_saved = rx_count_1;

	rx_data = (volatile byte *) p_data;
	rx_count = 0;
	rx_count_1 = ncount - 1;
	PIE1bits.RC1IE = 1;	 // Enable RX interrupt
  set_timeout(RX_TIMEOUT1_PC);
  // We need 2 while loops - one with initial Timeout,
  // another with the timeout after receiving first symbol 
	while( ( rx_count == 0 ) && is_not_timeout() ) {}
	if(is_not_timeout() )
	{
    set_timeout(RX_TIMEOUT2_PC);
	  while( (rx_count < ncount ) && is_not_timeout() )	{}
	}
	
	// Restore all previous buffer setup
	PIE1bits.RC1IE = 0;	 // Disable RX interrupt
	n_rcvd = rx_count; 
	rx_data = rx_data_saved;
	rx_count_1 = rx_count_1_saved;
	rx_count = 0;        // Clear buffer
	PIE1bits.RC1IE = 1;	 // Enable RX interrupt
  return n_rcvd;
}


volatile byte *tx_data; // Pointer to the current output data
volatile byte tx_count; // currect output counter

volatile byte *rx_data; // Pointer to the current receive data
volatile byte rx_count; // Number of characters received
volatile byte rx_count_1; // Max index in the buffer


void tx_eusart(unsigned char *p_data, byte ncount)
{
 	while( tx_count || !TXSTA1bits.TRMT ) {};	// Wait to finish previous Tx
	tx_data = (volatile byte *) p_data;
	tx_count = ncount;
	PIE1bits.TX1IE = 1;	// Interrupt will be generated
}

