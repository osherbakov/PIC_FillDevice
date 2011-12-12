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

// Check serial port if there is a request to send DES keys
char CheckSerial()
{
	if( RCSTA1bits.SPEN == 0)
	{
		TRIS_PIN_GND = INPUT;	// Make Ground
		ON_GND = 1;				//  on Pin B
		open_eusart_rx();
		start_eusart_rx(SerialBuffer, sizeof(SerialBuffer));
	}

	if(rx_count >= sizeof(SerialBuffer))
	{
		if( is_equal(SerialBuffer, SN_REQ, sizeof(SN_REQ)) )
		{
			tx_eusart_buff(SN_RESP);
			start_eusart_rx(SerialBuffer, sizeof(SerialBuffer));
		}else if(is_equal(SerialBuffer, OPT_REQ, sizeof(OPT_REQ)))
		{
			send_options();
			while( tx_count || TXSTA1bits.TRMT ) {};	// Wait to finish previous Tx
			open_eusart_rxtx();
			start_eusart_rx(SerialBuffer, sizeof(SerialBuffer));
			return MODE4;
		}
	}
	return 0;
}


// Open EUSART for reading only, TX is not affected
void open_eusart_rx()
{
	TRIS_Tx = INPUT;

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
	TRIS_Rx = INPUT;
	TRIS_Tx = INPUT;

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
	RCSTA = 0;				// Disable EUSART
}

void PCInterface()
{
	if( rx_eusart(&data_cell[0], 6) == 0 ) return;

	if( is_equal(&data_cell[0], KEY_FILL, sizeof(KEY_FILL)))
	{
		GetStoreFill(data_cell[5]);
	}else if(is_equal( &data_cell[0], KEY_DUMP, sizeof(KEY_DUMP)))
	{
		SendStoredFill(data_cell[5]);
	}
}


byte rx_eusart(unsigned char *p_data, byte ncount)
{
  byte	nrcvd = 0;	
  set_timeout(RX_TIMEOUT1_PC);
	while( (ncount > nrcvd) && is_not_timeout() )
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

volatile byte *tx_data;
volatile byte tx_count;

volatile byte *rx_data;
volatile byte rx_count;
volatile byte rx_count_1;


void tx_eusart(unsigned char *p_data, byte ncount)
{
	TXSTA1bits.TXEN = 1; // Enable Tx	
	while( tx_count || !TXSTA1bits.TRMT ) {};	// Wait to finish previous Tx

	tx_data = (volatile byte *) p_data;
	tx_count = ncount;
	PIE1bits.TX1IE = 1;	// Interrupt will be generated
}

//
// Soft UART to communicate with MBITR
//
byte rx_mbitr(unsigned char *p_data, byte ncount)
{
	byte bitcount, data;
	byte	nrcvd = 0;	

	TRIS_Rx = INPUT;
	PR6 = TIMER_MBITR;
  set_timeout(RX_TIMEOUT1_MBITR);
	
	while( (ncount > nrcvd) && is_not_timeout() )
	{
		// Start conditiona was detected - count 1.5 cell size	
		if(RxBIT )
		{
			TMR6 = TIMER_START;
			PIR5bits.TMR6IF = 0;	// Clear overflow flag
			for(bitcount = 0; bitcount < 8 ; bitcount++)
			{
				// Wait until timer overflows
				while(!PIR5bits.TMR6IF){} ;
				PIR5bits.TMR6IF = 0;	// Clear overflow flag
				data = (data >> 1) | (RxBIT ? 0x80 : 0x00);
			}
			*p_data++ = ~data;
			nrcvd++;
		  set_timeout(RX_TIMEOUT2_MBITR);
			while(RxBIT) {};	// Wait for stop bit
		}
	}
	return nrcvd;
}



void tx_mbitr(byte *p_data, byte ncount)
{
	byte 	bitcount;
	byte 	data;

  DelayMs(TX_MBITR_DELAY_MS);
	
	TRIS_Tx = OUTPUT;
	PR6 = TIMER_MBITR;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag
	while(ncount-- )
	{
		TxBIT = START;        // Issue the start bit
		data = ~(*p_data++);  // Get the symbol and advance pointer
    	// send 8 data bits and 4 stop bits
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
