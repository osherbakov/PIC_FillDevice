#include "config.h"
#include "delay.h"
#include "controls.h"
#include "Fill.h"
#include "serial.h"
#include "gps.h"
#include "rtc.h"


static byte config_reg;
void open_eusart(unsigned int baudrate, unsigned char rxtx_polarity)
{
	pinMode(RxPC, INPUT);
	pinMode(TxPC, INPUT);

// Values for the Baud Rate Control registers
	config_reg = ( ( (XTAL_FREQ * 1000000L / 4L)/(16L * baudrate)) - 1 );
	uartSetup(config_reg, rxtx_polarity);
	uartEnableRx(0);
	uartEnableTx(0);
	uartIRQRx(0);
	uartIRQTx(0);
	uartEnable(0);


	// Set up the Rx portion
	rx_idx_in = rx_idx_out = 0;
	rx_data = (volatile byte *) &RxTx_buff[0];
	rx_idx_max = sizeof(RxTx_buff) - 1;

	// Set up the Tx portion
	tx_data = (volatile byte *) &RxTx_buff[0];
	tx_count = 0;
	
	uartEnableRx(1);	// Enable Rx
	uartEnableTx(1);	// Enable Tx
	uartEnable(1); 		// Enable EUSART

}

void close_eusart()
{
	uartEnable(0);
	uartIRQRx(0);
	uartIRQTx(0);
	uartEnableRx(0);
	uartEnableTx(0);

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
	uartIRQRx(0);		// Disable RX interrupt
	uartEnableRx(0); 	// Disable Rx

	rx_idx_in = rx_idx_out = 0;
	rx_data = (volatile byte *) p_rx_data;
	rx_idx_max = max_size - 1;
	
	set_timeout(timeout);
	uartEnableRx(1); 	// Enable Rx
	uartIRQRx(1);	 	// Enable RX interrupt
}

  

// Receive the specified number of characters with timeout
// RX_TIMEOUT1_PC - timeout until 1st char received
// RX_TIMEOUT2_PC - timeout for all consequtive chars
byte rx_eusart(unsigned char *p_data, byte ncount, unsigned int timeout)
{
  	byte	symbol;
  	byte  nrcvd = 0;
	uartEnableRx(1); 		// Enable Rx
	uartIRQRx(0);	 		// Disable RX interrupt

  	set_timeout(timeout);
	while( (nrcvd < ncount ) && is_not_timeout() )
	{
		if(uartIsRx())	// Data is avaiable
		{
			// Get data byte and save it
			symbol = uartRx();
			*p_data++ = symbol;
			nrcvd++;
		  	set_timeout(RX_TIMEOUT2_PC);
			// overruns? clear it
			if(uartIsError())
			{
				uartEnableRx(0); 	// Disable Rx
				uartEnableRx(1); 	// Enable Rx
			}
		}
	}
  	return nrcvd;
}

byte rx_eusart_line(unsigned char *p_data, byte ncount, unsigned int timeout)
{
  	byte	symbol;
  	byte  	nrcvd = 0;
	uartEnableRx(1); 		// Enable Rx
	uartIRQRx(0);	 		// Disable RX interrupt

  	set_timeout(timeout);
	while( (nrcvd < ncount) && is_not_timeout())
	{
		if(uartIsRx())	// Data is avaiable
		{
			// Get data byte and save it
			symbol = uartRx();
			*p_data++ = symbol;
			if(symbol == '\n' || symbol == '\r')  {
				break;
			}	
			nrcvd++;
  			set_timeout(timeout);
			// overruns? clear it
			if(uartIsError())
			{
				uartEnableRx(0); 		// Disable Rx
				uartEnableRx(1); 		// Enable Rx
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

	DISABLE_IRQ(prev);		// Disable interrupts
	count = rx_idx_in - rx_idx_out;	
	if(count == 0) { rx_idx_in = rx_idx_out = 0; }
	ENABLE_IRQ(prev);
	return	count;
}	

void rx_eusart_reset(void)
{
	DISABLE_IRQ(prev);		// Disable interrupts
	rx_idx_in = rx_idx_out = 0; 
	ENABLE_IRQ(prev);
}	


int rx_eusart_symbol(void)
{
	int 	result = -1;

	DISABLE_IRQ(prev);		// Disable interrupts
	
	if (rx_idx_in > rx_idx_out) {
		result = ((int)rx_data[rx_idx_out++]) & 0x00FF;
	}
	// Check for the all symbols taken and adjust the indices
	if(rx_idx_in == rx_idx_out){rx_idx_in = rx_idx_out = 0; }
	ENABLE_IRQ(prev);
	return	result;
}	

void tx_eusart_async(const unsigned char *p_data, byte ncount)
{
	tx_eusart_flush();	// Make sure that the old data is fully flushed and sent...
	tx_data = (volatile byte *) p_data;
	tx_count = ncount;
 	uartEnableTx(1); 	// Enable Tx	
	uartIRQTx(1);		// Interrupt will be generated
}

void tx_eusart_flush()
{
  	if(	uartIsIRQTx())	// If Tx is enabled - wait until all chars are sent out
  	{
		while( tx_count || !uartIsTxBusy() ) {};	// Wait to finish previous Tx
	}
}

