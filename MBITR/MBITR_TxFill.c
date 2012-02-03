#include "config.h"
#include "crcmodel.h"
#include "delay.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"
#include "controls.h"

static char WaitMBITRReq(byte req_type);

//--------------------------------------------------------------
// Delays for the appropriate timings in millisecs
//--------------------------------------------------------------
#define tM 		10	  // D LOW -> F LOW 	(-5us - 100ms)
#define tA  	50	  // F LOW -> D HIGH	(45us - 55us)
#define tE  	250   // REQ -> Fill		(0 - 2.3 sec)

#define tZ  	250   // End -> New Fill	
//--------------------------------------------------------------
// Delays for the appropriate timings in usecs
//--------------------------------------------------------------
#define tJ  	25		// D HIGH -> First data bit on B
#define tK1  	425 	// First Data bit on B -> E (CLK) LOW
#define tK2  	425		// Last E (CLK) LOW -> TRISTATE E
#define tL  	50		// C (REQ) LOW -> F (MUX) HIGH for the last bit

//--------------------------------------------------------------
// Timeouts in ms
//--------------------------------------------------------------
#define tB  1000    // Query -> Response from Radio (0.8ms - 5ms)
#define tD  500     // PIN_C Pulse Width (0.25ms - 75ms)
#define tG  500     // PIN_B Pulse Wodth (0.25ms - 80ms)
#define tH  500     // BAD HIGH - > REQ LOW (0.25ms - 80ms)
#define tF  3000    // End of fill - > response (4ms - 2sec)
#define tC  (30000)  // .5 minute - > until REQ (300us - 5min )

byte CHECK_MBITR[4] = {0x2F, 0x39, 0x38, 0x0D };	// "/98<cr>"

static char WaitMBITRReq(byte req_type)
{
	byte char_received;
	byte char_to_expect;

	// This is the DES key load - send serial number request
	char_received = 0;
	if( req_type == REQ_FIRST )
	{
		char_to_expect = KEY_EOL;		// Wait for \n
		tx_mbitr(&CHECK_MBITR[0], 4);
	}else
	{
		char_to_expect = KEY_ACK;		// Wait for 0x06
	}	

	// wait in the loop until receive the ACK character, or timeout
  while( rx_mbitr(&char_received, 1) && (char_received != char_to_expect) ) {}; 
	return ( char_received == char_to_expect ) ? 0 : -1 ; 
}

static char SendMBITRFill(void)
{
	byte bytes, byte_cnt;
	char wait_result;
	
	while(records)	
	{
		bytes = byte_read(base_address++);
		while(bytes )
		{
			byte_cnt = MIN(bytes, FILL_MAX_SIZE);
			array_read(base_address, &data_cell[0], byte_cnt);
			base_address += byte_cnt;
			tx_mbitr(&data_cell[0], byte_cnt);
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
	return wait_result;
}


char WaitReqSendMBITRFill()
{
	// If first fill request was not answered - just return with timeout
	// We will be called again after switches are checked
	if( WaitMBITRReq(REQ_FIRST) < 0 ) return -1;

	return SendMBITRFill();
}



//
// Soft UART to communicate with MBITR
//
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
			for(bitcount = 0; bitcount < 8 ; bitcount++)
			{
				// Wait until timer overflows
				while(!PIR5bits.TMR6IF){} ;
				PIR5bits.TMR6IF = 0;	// Clear overflow flag
				data = (data >> 1) | (RxMBITR ? 0x80 : 0x00);
			}
			*p_data++ = ~data;
			nrcvd++;
		  set_timeout(RX_TIMEOUT2_MBITR);
			while(RxMBITR) {};	// Wait for stop bit
		}
	}
	return nrcvd;
}



void tx_mbitr(byte *p_data, byte ncount)
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
