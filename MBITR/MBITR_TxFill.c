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
	pinMode(RxMBITR, INPUT);
	pinMode(TxMBITR, OUTPUT);
	pinWrite(TxMBITR, STOP_BIT);
	DelayMs(50);
}

void close_mbitr(void)
{
	pinMode(RxMBITR, INPUT);
	pinMode(TxMBITR, INPUT);
}
	
byte rx_mbitr(unsigned char *p_data, byte ncount)
{
	byte bitcount, data;
	byte	nrcvd = 0;	

	pinMode(RxMBITR, INPUT);
	
	timerSetupBaudrate(MBITR_BAUDRATE);
	timerClear();

	timerDisableIRQ();

 	set_timeout(RX_TIMEOUT1_MBITR);
	
	while( (ncount > nrcvd) && is_not_timeout() )
	{
		// Start conditiona was detected 
		if(pinRead(RxMBITR) )
		{
			// Delay by 1/2 cell
			DelayUs(1000000/(MBITR_BAUDRATE * 2));
			timerClear();		// Clear counter and start counting cells
		  	set_timeout(RX_TIMEOUT2_MBITR);
			for(bitcount = 0; bitcount < 8 ; bitcount++)
			{
				// Wait until timer overflows
				while(!timerFlag()){} ;	// Wait for the next cell
				timerClearFlag();		// Clear overflow flag
				data = (data >> 1) | (pinRead(RxMBITR) ? 0x80 : 0x00);
			}
			while(!timerFlag()){} ;		// Wait for STOP
			if(pinRead(RxMBITR))		//   no stop - exit
			{
  				break;
  			}
			*p_data++ = DATA_BYTE(data);			// Invert data
			nrcvd++;
		}
	}
	return nrcvd;
}



void tx_mbitr(const byte *p_data, byte ncount)
{
	byte 	bitcount;
	byte 	data;

	pinMode(TxMBITR, OUTPUT);
	pinWrite(TxMBITR, STOP_BIT) ;        // Issue the STOP bit
	
  	DelayMs(TX_MBITR_DELAY_MS);

	timerSetupBaudrate(MBITR_BAUDRATE);
	timerClear();
	
	timerDisableIRQ();

	while(ncount-- )
	{
		pinWrite(TxMBITR, START_BIT); // Issue the start bit
		data = DATA_BYTE(*p_data++);  // Get the symbol and advance pointer
    	// send 8 data bits and 4 stop bits
		for(bitcount = 0; bitcount < 12; bitcount++)
		{
			while(!timerFlag()) {	/* wait until timer overflow bit is set*/};
			timerClearFlag();		// Clear timer overflow bit
			pinWrite(TxMBITR, data & 0x01);	// Set the output
			data >>= 1;				// We use the fact that 
									// "0" bits are STOP bits
		}
	}
}
