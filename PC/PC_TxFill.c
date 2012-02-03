#include "config.h"
#include "crcmodel.h"
#include "delay.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"
#include "controls.h"

static char WaitPCReq(byte req_type);

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

// Tag values
#define NO_FILL 0x7D

// The cell is filled with NO_FILL tags
byte nofill_cell[] =
{
  NO_FILL, 
  NO_FILL, NO_FILL, NO_FILL, NO_FILL, NO_FILL, NO_FILL, NO_FILL, 
  NO_FILL, NO_FILL, NO_FILL, NO_FILL, NO_FILL, NO_FILL, NO_FILL, 
  0x77  
};


// Time cell
byte    TOD_cell[MODE2_3_CELL_SIZE];

// Generic cell that can keep all the data
byte	  data_cell[FILL_MAX_SIZE];

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

char WaitPCReq(byte req_type)
{
	byte char_received;
	byte char_to_expect;

	char_to_expect = KEY_ACK; 
	// wait in the loop until receive the ACK character, or timeout
  while( p_rx(&char_received, 1) && (char_received != char_to_expect) ) {}; 
	return ( char_received == char_to_expect ) ? 0 : -1 ; 
}

char SendPCFill()
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
			// Check if the cell that we are about to send is the 
			// TOD cell - replace it with the real Time cell
			if( (data_cell[0] == TOD_TAG_0) && (data_cell[1] == TOD_TAG_1) && 
						(fill_type == MODE3) && (byte_cnt == MODE2_3_CELL_SIZE) )
			{
				FillTODData();
				cm_append(TOD_cell, MODE2_3_CELL_SIZE);
		  		tx_eusart(TOD_cell, MODE2_3_CELL_SIZE);
			}else
			{
				tx_eusart(&data_cell[0], byte_cnt);
			}
			bytes -= byte_cnt;
		}
		records--;
		
		// After sending a record check for the next request
		wait_result = p_ack( records ? REQ_NEXT : REQ_LAST );
    // If all records were sent - ignore timeout
	  if(records == 0)
		{
			wait_result = ST_OK;
		}
    if(wait_result) 
			break;
	}	
	
	// If regular Type 2 3 fill - release the bus
	if( p_ack == WaitDS102Req)
	{
    	ReleaseMode23Bus();
	}
	return (p_ack == WaitMBITRReq) ? ST_DONE : wait_result;	// When send to MBITR - return with DONE flag
}


char SendStoredPCFill(byte stored_slot)
{
	CheckFillType(stored_slot);
	// If first fill request was not answered - just return with timeout
	// We will be called again after switches are checked
	if( p_ack(REQ_FIRST) < 0 ) return -1;
	return SendFill();
}

char WaitReqSendPCFill()
{
	// If first fill request was not answered - just return with timeout
	// We will be called again after switches are checked
	if( p_ack(REQ_FIRST) < 0 ) return -1;
	return SendPCFill();
}
