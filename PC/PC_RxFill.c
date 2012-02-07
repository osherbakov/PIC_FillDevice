#include "config.h"
#include "crcmodel.h"
#include "delay.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"
#include "clock.h"
#include "controls.h"

//--------------------------------------------------------------
// Delays in ms
//--------------------------------------------------------------

//--------------------------------------------------------------
// Delays for the appropriate timings in usecs
//--------------------------------------------------------------

//--------------------------------------------------------------
// Timeouts in ms
//--------------------------------------------------------------
#define tE  	5000   // REQ -> Fill		(0 - 2.3 sec)

static byte key_ack;

// ACK the received key from PC
char SendPCAck(byte ack_type)
{
	key_ack = KEY_ACK;
	tx_eusart(&key_ack, 1);			// ACK the previous packet
}

// NACK the received key from PC
char SendPCNak(byte ack_type)
{
	key_ack = KEY_NAK;
	tx_eusart(&key_ack, 1);			// NAK the previous packet
}

static void SetTimeFromPC(void)
{
	ExtractTODData();
	CalculateNextSecond();
	if( !rtc_date.Valid )
	{
		SendPCNak(0);
	}else
	{
		// The time is in chunks of 1/10 sec
		char ms_100 =  (10 - (data_cell[11] >> 4) ) & 0x0F; 
		while(ms_100-- > 0) delay(100);
		SetRTCData();		
	}
}


static byte GetPCFill(void)
{
	byte records, byte_cnt, record_size;
	unsigned short long saved_base_address;

	records = 0;
	record_size = 0;

	saved_base_address = base_address++;

	SendPCAck(REQ_FIRST);	// REQ the first packet
  while(1)
	{
		byte_cnt = rx_eusart(&data_cell[0], FILL_MAX_SIZE);
		// We can get byte_cnt
		//  = 0  - no data received --> finish everything
		//  == FILL_MAX_SIZE --> record and continue
		//  0 < byte_cnt < FILL_MAX_SIZE --> record and issue new request
		record_size += byte_cnt;
		if(record_size == 0)
		{
			break;	// No data provided - exit
		}
		if(byte_cnt)
		{
			array_write(base_address, &data_cell[0], byte_cnt);
			base_address += byte_cnt;
		}
		if(byte_cnt < FILL_MAX_SIZE)
		{
			byte_write(saved_base_address, record_size);
			records++; 
			record_size = 0;
			saved_base_address = base_address++;
			SendPCAck(REQ_NEXT);	// ACK the previous and REQ the next packet
		}
	}
	return records;
}


// Receive and store the fill data into the specified slot
char StorePCFill(byte stored_slot, byte required_fill)
{
	char result = ST_ERR;
	byte records;
	unsigned short long saved_base_addrress;
	base_address = get_eeprom_address(stored_slot & 0x0F);

	saved_base_addrress = base_address;	
	base_address += 2;	// Skip the fill_type and records field
											// .. to be filled at the end
	// All data are stored in 1K bytes (8K bits) slots
	// The first byte of the each slot has the number of the records (0 - 255)
	// The first byte of the record has the number of bytes that should be sent out
	// so each record has no more than 255 bytes as well
	// Empty slot has first byte as 0x00
	fill_type = required_fill;
	
	records = GetPCFill();
	if( records > 0)
	{
  	// All records were received - put final info into EEPROM
	  // Mark the slot as valid slot containig data
		byte_write(saved_base_addrress, records);
		byte_write(saved_base_addrress + 1, fill_type);
		result = ST_DONE;
	}
  return result;
}
