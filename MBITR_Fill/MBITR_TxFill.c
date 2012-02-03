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


// Generic cell that can keep all the data
byte	 data_cell[FILL_MAX_SIZE];

void  FillTODData(void)
{
	byte ms;
	GetRTCData();
	TOD_cell[0] = TOD_TAG_0;
	TOD_cell[1] = TOD_TAG_1;
	TOD_cell[2] = rtc_date.Century;
	TOD_cell[3] = rtc_date.Year;
	TOD_cell[4] = rtc_date.Month;
	TOD_cell[5] = rtc_date.Day;
	TOD_cell[6] = rtc_date.JulianDayH;
	TOD_cell[7] = rtc_date.JulianDayL;
	TOD_cell[8] = rtc_date.Hours;
	TOD_cell[9] = rtc_date.Minutes;
	TOD_cell[10] = rtc_date.Seconds;
	TOD_cell[11] = 0;

	ms = rtc_date.MilliSeconds_10;
	while(ms >= 10)
	{
		TOD_cell[11] += 0x10;
		ms -= 10; 
	}
}

// Check the equipment type
// If the Type2/3 equipment is detected, then return MODE2 or MODE3
// For Type 1 - return MODE1 right away
// If Key is Type 4 - then send the /98 and wait for the response
char CheckEquipment()
{
  char Equipment = 0;
  if((fill_type == MODE1))
  {
	  AcquireMode1Bus();
	  Equipment = MODE1;
  }else if(fill_type == MODE4)
  {
	  // Connect ground on PIN A
        set_pin_a_as_gnd();
	  if( p_ack(REQ_FIRST) == 0 )  
	  {
		  Equipment = MODE4;
	  }
  }else		// MODE 2 or MODE 3
  {
	  set_pin_a_as_power();
	  AcquireMode23Bus();
	  StartMode23Handshake();
  	SendMode23Query(MODE3);
	  Equipment = GetEquipmentMode23Type();
   	EndMode23Handshake();
	  if(Equipment < 0)	// Timeout occured
	  {
		  ReleaseMode23Bus();
	  }
  }
  return Equipment;
}


static unsigned short long base_address;
byte  	fill_type;
byte 	records;

// Set up functions that will be called depending on the Key type
// Type 1, 2,3 - will be sent thru DS102 interface
// Type 4 - will be sent thru MBITR
// If the high byte of the parameter "stored_slot" is not 0 - send the key to PC
char CheckFillType(byte stored_slot)
{
	base_address = get_eeprom_address(stored_slot & 0x0F);
	records = byte_read(base_address++); 
	if(records == 0xFF) records = 0x00;
	
	// Get the fill type from the EEPROM
	fill_type = byte_read(base_address++);
	if(stored_slot >> 4)	// Send fill to the PC
	{
		p_rx = rx_eusart;
		p_tx = tx_eusart;
		p_ack = WaitPCReq;
	}else					// Send Fill to the MBITR
	{
		close_eusart();
		if(fill_type == MODE4) // DES fill mode
		{
			p_rx = rx_mbitr;
			p_tx = tx_mbitr;
			p_ack = WaitMBITRReq;
		}else					// Regular MODE 1, 2, 3 fills
		{
			p_tx = SendDS102Cell;
			p_ack = WaitDS102Req;
		}
	}
	return records ? fill_type : 0;
}

byte CHECK_MBITR[4] = {0x2F, 0x39, 0x38, 0x0D };	// "/98<cr>"

char WaitMBITRReq(byte req_type)
{
	byte char_received;
	byte char_to_expect;

	// This is the DES key load - send serial number request
	char_received = 0;
	if( req_type == REQ_FIRST )
	{
		tx_mbitr(&CHECK_MBITR[0], 4);
		char_to_expect = KEY_EOL;		// Wait for \n
	}else
	{
		char_to_expect = KEY_ACK;		// Wait for 0x06
	}	

	// wait in the loop until receive the ACK character, or timeout
  while( p_rx(&char_received, 1) && (char_received != char_to_expect) ) {}; 
	return ( char_received == char_to_expect ) ? 0 : -1 ; 
}

char WaitPCReq(byte req_type)
{
	byte char_received;
	byte char_to_expect;

	char_to_expect = KEY_ACK; 
	// wait in the loop until receive the ACK character, or timeout
  while( p_rx(&char_received, 1) && (char_received != char_to_expect) ) {}; 
	return ( char_received == char_to_expect ) ? 0 : -1 ; 
}

char SendMBITRFill()
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


char SendStoredMBITRFill(byte stored_slot)
{
	// If first fill request was not answered - just return with timeout
	// We will be called again after switches are checked
	if( p_ack(REQ_FIRST) < 0 ) return -1;
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
