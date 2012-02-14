#include "config.h"
#include "crcmodel.h"
#include "delay.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"
#include "clock.h"
#include "controls.h"


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


void StoreCurrentDayTime(byte *p_buffer)
{
	byte byte_cnt;
	byte_cnt = rx_eusart(p_buffer, FILL_MAX_SIZE);
  
  if( (byte_cnt >= 17) &&
        ExtractYear(p_buffer, byte_cnt) &&
          ExtractTime(p_buffer, byte_cnt) &&
            ExtractDate(p_buffer, byte_cnt) )
  {
    CalculateJulianDay();
    CalculateWeekDay();
		SetRTCData();		
	}
}


static byte GetPCFill(unsigned short long base_address)
{
	byte records, byte_cnt, record_size;
	byte  *p_data;
	unsigned short long saved_base_address;

	p_data = &data_cell[0];

	records = 0;
	record_size = 0;

	saved_base_address = base_address++;

	SendPCAck(REQ_FIRST);	// REQ the first packet
  while(1)
	{
		byte_cnt = rx_eusart(p_data, FILL_MAX_SIZE);
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
			array_write(base_address, p_data, byte_cnt);
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
	unsigned short long base_address;
	unsigned short long saved_base_address;
	base_address = get_eeprom_address(stored_slot & 0x0F);

	saved_base_address = base_address;	
	base_address += 2;	// Skip the fill_type and records field
											// .. to be filled at the end
	// All data are stored in 1K bytes (8K bits) slots
	// The first byte of the each slot has the number of the records (0 - 255)
	// The first byte of the record has the number of bytes that should be sent out
	// so each record has no more than 255 bytes as well
	// Empty slot has first byte as 0x00
	
	records = GetPCFill(base_address);
	if( records > 0)
	{
  	// All records were received - put final info into EEPROM
	  // Mark the slot as valid slot containig data
		byte_write(saved_base_address, records);
		byte_write(saved_base_address + 1, required_fill);
		result = ST_DONE;
	}
  return result;
}
