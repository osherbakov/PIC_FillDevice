#include "config.h"
#include "crcmodel.h"
#include "delay.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"
#include "controls.h"

static 	byte char_received;
static	byte char_to_expect;
static const byte CHECK_MBITR[4] = {0x2F, 0x39, 0x38, 0x0D };	// "/98<cr>"

static char WaitMBITRReq(byte req_type)
{
	// This is the DES key load - send serial number request
	char_received = 0;
	if( req_type == REQ_FIRST )
	{
		char_to_expect = KEY_EOL;		// Wait for \n
		tx_mbitr(&CHECK_MBITR[0], 4);
	}else if( req_type == REQ_LAST )  // Do not wait for the ACK on the last one
	{
  		return ST_OK;
	}else 
	{
		char_to_expect = KEY_ACK;		// Wait for 0x06
	}	

	// wait in the loop until receive the ACK character, or timeout
  	while( rx_mbitr(&char_received, 1) && (char_received != char_to_expect) ) {}; 
	return ( char_received == char_to_expect ) ? ST_OK : ST_TIMEOUT ; 
}

char WaitReqSendMBITRFill(byte stored_slot)
{
	byte    bytes, byte_cnt;
  	byte  	fill_type, records;
	char    wait_result;
	unsigned short long base_address;

	// If first fill request was not answered - just return with timeout
	// We will be called again after switches are checked
	open_mbitr();
	if( WaitMBITRReq(REQ_FIRST) < 0 ) return -1;

	wait_result = ST_OK;
	base_address = get_eeprom_address(stored_slot & 0x0F);
	records = byte_read(base_address++);
	if(records == 0xFF) records = 0x00;
	// Get the fill type from the EEPROM
	fill_type = byte_read(base_address++);
	
	while(records)	
	{
		bytes = byte_read(base_address++);
		while(bytes )
		{
			byte_cnt = MIN(bytes, FILL_MAX_SIZE);
			array_read(base_address, &data_cell[0], byte_cnt);
			tx_mbitr(&data_cell[0], byte_cnt);
			// Adjust counters and pointers
			base_address += byte_cnt;
			bytes -= byte_cnt;
		}
		records--;
		
		// After sending a record check for the next request
		wait_result = WaitMBITRReq( records ? REQ_NEXT : REQ_LAST );
    	// If all records were sent - ignore timeout
	  	if(records == 0)
		{
			wait_result = ST_DONE;
			break;
		}
    	if(wait_result) 
			break;
	}	
	close_mbitr();
	return wait_result;
}



//
// Soft UART to communicate with MBITR
//

void open_mbitr(void)
{
	TRIS_RxMBITR = INPUT;
	TRIS_TxMBITR = OUTPUT;
	ANSEL_RxMBITR	= 0;
	ANSEL_TxMBITR	= 0;
	TxMBITR	= STOP;
	DelayMs(50);
}

void close_mbitr(void)
{
	TRIS_RxMBITR = INPUT;
	TRIS_TxMBITR = INPUT;
}
	
byte rx_mbitr(unsigned char *p_data, byte ncount)
{
	byte bitcount, data;
	byte	nrcvd = 0;	

	TRIS_RxMBITR = INPUT;
	T6CON = TIMER_MBITR_CTRL;
	PR6 = TIMER_MBITR;
 	set_timeout(RX_TIMEOUT1_MBITR);
	
	while( (ncount > nrcvd) && is_not_timeout() )
	{
		// Start conditiona was detected - count 1.5 cell size	
		if(RxMBITR )
		{
			TMR6 = TIMER_MBITR_START;
			PIR5bits.TMR6IF = 0;	// Clear overflow flag
		  	set_timeout(RX_TIMEOUT2_MBITR);
			for(bitcount = 0; bitcount < 8 ; bitcount++)
			{
				// Wait until timer overflows
				while(!PIR5bits.TMR6IF){} ;
				PIR5bits.TMR6IF = 0;	// Clear overflow flag
				data = (data >> 1) | (RxMBITR ? 0x80 : 0x00);
			}
			while(!PIR5bits.TMR6IF){} ;
			if(RxMBITR)
			{
  				break;
  			}
			*p_data++ = ~data;
			nrcvd++;
		}
	}
	return nrcvd;
}



void tx_mbitr(const byte *p_data, byte ncount)
{
	byte 	bitcount;
	byte 	data;

  	DelayMs(TX_MBITR_DELAY_MS);
	
	TRIS_TxMBITR = OUTPUT;
	PR6 = TIMER_MBITR;
	T6CON = TIMER_MBITR_CTRL;
	
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag
	while(ncount-- )
	{
		TxMBITR = START;        // Issue the start bit
		data = ~(*p_data++);  // Get the symbol and advance pointer
    	// send 8 data bits and 4 stop bits
		for(bitcount = 0; bitcount < 12; bitcount++)
		{
			while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
			PIR5bits.TMR6IF = 0;	// Clear timer overflow bit
			TxMBITR = data & 0x01;	// Set the output
			data >>= 1;				// We use the fact that 
									// "0" bits are STOP bits
		}
	}
}
