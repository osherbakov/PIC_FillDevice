#include "config.h"
#include "crcmodel.h"
#include "delay.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"
#include "controls.h"


static char WaitPCReq(byte req_type)
{
	byte char_received;
	byte char_to_expect;
	
	if(req_type == REQ_LAST) return ST_OK;

	char_to_expect = KEY_ACK; 
	// wait in the loop until receive the ACK character, or timeout
  	while( rx_eusart(&char_received, 1) && (char_received != char_to_expect) ) {}; 
	return ( char_received == char_to_expect ) ? ST_OK : ST_TIMEOUT; 
}

char WaitReqSendPCFill(byte stored_slot)
{
  	byte  	fill_type, records;
	byte    bytes, byte_cnt;
	char    wait_result;
	unsigned short long base_address;

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
		wait_result = WaitPCReq( records ? REQ_NEXT : REQ_LAST );
		
	    // If all records were sent - ignore timeout
		if(records == 0)
		{
			wait_result = ST_OK;
			break;
		}
	    if(wait_result) break;
	}	
	return wait_result;	
}


char ReadMemSendPCFill(byte stored_slot)
{
	unsigned int bytes, byte_cnt;
  	unsigned short long base_address;	

	base_address = get_eeprom_address(stored_slot & 0x0F);
	
	bytes = get_eeprom_address(1);  // That's how we calculate the size 
	while(bytes)	
	{
		byte_cnt = MIN(bytes, FILL_MAX_SIZE);
		array_read(base_address, &data_cell[0], byte_cnt);
		tx_eusart(&data_cell[0], byte_cnt);
    	flush_eusart();
    	// Adjust counters and pointers
		base_address += byte_cnt;
		bytes -= byte_cnt;
	}
	return ST_OK;
}
