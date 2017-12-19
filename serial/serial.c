#include "config.h"
#include "delay.h"
#include "controls.h"
#include "Fill.h"
#include "serial.h"
#include "gps.h"
#include "rtc.h"

// The command from the PC to fill the key or to dump it
// The byte after the command specifies the key number and type
//  /FILL<0x34> means fill key 4 of type 3
static byte KEY_FILL[] 		= "/FILL<n>";	// Fill the key N
static byte KEY_DUMP[] 		= "/DUMP<n>";	// Dump the key N
static byte TIME_CMD[] 		= "/TIME=";		// Fill/Dump Time
static byte DATE_CMD[] 		= "/DATE=";		// Fill/Dump Time
static byte KEY_CMD[] 		= "/KEY<n>";	// Read/Write the key N

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
   	flush_eusart();
}

static unsigned char SerialBuffer[6];

//
//		Functions to check for RS-232 MBITR Type 4, PC and DTD Type 5 fills
//

// Check serial port if there is a request to send DES keys
char CheckFillType4()
{
	if( !RCSTA1bits.SPEN)
	{
		// If RxPC (PIN_D) is LOW then maybe there is RS-232 connected
	  	if(RxPC == LOW)
	  	{
	      // Coming in first time - enable eusart and setup buffer
	  		open_eusart(BRREG_MBITR, DATA_POLARITY_RXTX);
	  		rx_eusart_async(SerialBuffer, 4, RX_TIMEOUT1_PC);
	  	}	
	}else if(rx_idx >= 4)
	{
    // Four characters are collected - check if it is 
    //  /98 - serial number request, or
    //  /84 - the capabilities request
		if( is_equal(SerialBuffer, SN_REQ, 4) )
		{
			rx_idx = 0; // Data consumed
  			// SN request - send a fake SN = 123456
			tx_eusart_buff(SN_RESP);
			flush_eusart();
			set_timeout(RX_TIMEOUT1_PC);	// Restart timeout
		}else if(is_equal(SerialBuffer, OPT_REQ, 4))
		{
			rx_idx = 0; // Data consumed
			// Options request - send all options
			send_options();
			flush_eusart();
			return MODE4;
		}
	}
	return ST_TIMEOUT;
}



// Check if there is the request from the PC to send DS-101/RS-232 keys
char CheckFillRS232Type5()
{
  	// Coming in first time - enable eusart and setup buffer
	if( !RCSTA1bits.SPEN )
	{
		// If RxPC (PIN_D) is LOW, then maybe there is RS-232 connected
	  	if(RxPC == LOW)
	  	{
	      // Coming in first time - enable eusart and setup buffer
	  		open_eusart(BRREG_PC, DATA_POLARITY_RXTX);
	  		rx_eusart_async(SerialBuffer, 4, RX_TIMEOUT1_PC);
	  	}	
	}else if( (rx_idx >= 2) && 
			  (is_equal(SerialBuffer, HDLC_FLAGS1, 2) ||
			 	  is_equal(SerialBuffer, HDLC_FLAGS2, 2) ||
			  	  is_equal(SerialBuffer, HDLC_FLAGS3, 2) ) )
	{
		 close_eusart();
		 return MODE5;
	}
	return ST_TIMEOUT;
}	

// Check if there is the request from the DTD to send DS-101/RS-232 keys
char CheckFillDTD232Type5()
{
	if( !RCSTA1bits.SPEN )
	{
	  	if(RxDTD == LOW)
	  	{
	  		 close_eusart();
	  		 return MODE5;
	  	}	
  	} 	
	return ST_TIMEOUT;
}	

// The Type5 RS485 fill is detected when PIN_P is different from PIN_N
char CheckFillRS485Type5()
{
	TRIS_Data_N	= INPUT;
	TRIS_Data_P	= INPUT;
	WPUB_Data_N = 1;
	WPUB_Data_P = 1;
	DelayMs(10);

	return ( Data_P != Data_N ) ? MODE5 : ST_TIMEOUT;
}	


void open_eusart(unsigned char baudrate_reg, unsigned char rxtx_polarity)
{
	TRIS_RxPC 	= INPUT;
	TRIS_TxPC 	= INPUT;
	ANSEL_RxPC	= 0;
	ANSEL_TxPC	= 0;

	RCSTA1bits.CREN = 0; // Disable Rx
 	TXSTA1bits.TXEN = 0; // Disable Tx	
	PIE1bits.RC1IE = 0;	 // Disable RX interrupt
	PIE1bits.TX1IE = 0;	 // Disable TX Interrupts
	RCSTA1bits.SPEN = 0; // Disable EUSART

	// Set up the baud rate
	SPBRGH1 = 0x00;
	SPBRG1 = baudrate_reg ;
	BAUDCON1 = rxtx_polarity;

	rx_idx = 0;
	rx_idx_max = sizeof(SerialBuffer) - 1;
	rx_data = (volatile byte *) &SerialBuffer[0];

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
	rx_idx = 0;
	tx_count = 0;
}

void flush_eusart()
{
  	if(	PIE1bits.TX1IE )	// If Tx is enabled - wait until all chars are sent out
  	{
		while( tx_count || !TXSTA1bits.TRMT ) {};	// Wait to finish previous Tx
	}
}

void rx_eusart_async(unsigned char *p_rx_data, byte max_size, unsigned int timeout)
{
	PIE1bits.RC1IE = 0;	 // Disable RX interrupt
	RCSTA1bits.CREN = 0; // Disable Rx
	rx_data = (volatile byte *) p_rx_data;
	rx_idx = 0;
	rx_idx_max = max_size - 1;
	set_timeout(timeout);
	RCSTA1bits.CREN = 1; // Enable Rx
	PIE1bits.RC1IE = 1;	 // Enable RX interrupt
}

  
void PCInterface()
{
	byte	i;
 	byte *p_data = &data_cell[0];
	// If entering the first time - enable eusart
	// and initialize the buffer to get chars
	if( RCSTA1bits.SPEN == 0)
	{
		open_eusart(BRREG_PC, DATA_POLARITY_RXTX);
 		rx_eusart_async(p_data, 6, INF_TIMEOUT);
	}
	
	// Wait to receive 6 characters
	if(rx_idx >= 6) 
	{
	  	byte  	slot;
		byte 	type;

		if( is_equal(p_data, KEY_FILL, 5))
	  	{
	    	// The last char in /FILLN specifies Type(high nibble) 
	    	//    and Slot Number (low nibble)
	    	slot = p_data[5] & 0x0F;
	    	type = (p_data[5] >> 4) & 0x0F;
	  		StorePCFill(slot, type);
			rx_eusart_async(p_data, 6, INF_TIMEOUT);  // Restart collecting data
	  	}else if(is_equal( p_data, KEY_DUMP, 5))
	  	{
	    	// The last char in /DUMPN is the slot number
	    	slot = p_data[5] & 0x0F;
	  		WaitReqSendPCFill(slot);
			rx_eusart_async(p_data, 6, INF_TIMEOUT);  // Restart collecting data
	  	}else if(is_equal( p_data, TIME_CMD, 5) || is_equal(p_data, DATE_CMD, 5))
	  	{
			if(p_data[5] == '=') {
				SetCurrentDayTime();
			}
	  	  	GetCurrentDayTime();
			rx_eusart_async(p_data, 6, INF_TIMEOUT);  // Restart collecting data
	    }else if(is_equal( p_data, KEY_CMD, 4))
	  	{
	    	// The next char in /KEY<n> is the slot number
		  	slot = p_data[4];
			if(p_data[5] == '=') {
				SetPCKey(slot);
			}else if(slot == '0' || slot == ' ')	{	// Special case - all keys
				for(i = 1; i <=13; i++) {
					GetPCKey(i);
 				}
			}else {			
				GetPCKey(slot);
			}
			rx_eusart_async(p_data, 6, INF_TIMEOUT);  // Restart collecting data
		}
  	}  
}


// Receive the specified number of characters with timeout
// RX_TIMEOUT1_PC - timeout until 1st char received
// RX_TIMEOUT2_PC - timeout for all consequtive chars
byte rx_eusart(unsigned char *p_data, byte ncount, unsigned int timeout)
{
  	byte	symbol;
  	byte  nrcvd = 0;
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
volatile byte rx_idx;     // Number of characters received
volatile byte rx_idx_max; // Max index in the buffer


void tx_eusart(unsigned char *p_data, byte ncount)
{
  	flush_eusart();
	tx_data = (volatile byte *) p_data;
	tx_count = ncount;
	PIE1bits.TX1IE = 1;	// Interrupt will be generated
}

