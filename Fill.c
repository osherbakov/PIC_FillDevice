#include "config.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"

unsigned short long base_address;
byte  	fill_type;
byte 	  records;

// Generic cell that can keep all the data
byte	  data_cell[FILL_MAX_SIZE];

// Time cell
byte    TOD_cell[MODE2_3_CELL_SIZE];

char ClearFill(byte stored_slot)
{
	 unsigned short long base_address = get_eeprom_address(stored_slot & 0x0F);
   	 byte_write(base_address, 0x00);
   	 return ST_OK;
}

// Detect the fill type and set up global variables
// Type 1, 2,3 - will be sent thru DS102 interface
char CheckFillType(byte stored_slot)
{
	base_address = get_eeprom_address(stored_slot & 0x0F);
	records = byte_read(base_address++); 
	if(records == 0xFF) records = 0x00;
	
	// Get the fill type from the EEPROM
	fill_type = byte_read(base_address++);
	if( (records == 0) || (fill_type == 0xFF))
	{
  	fill_type = 0;
  }
	return fill_type;
}

void  FillTODData()
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

void  ExtractTODData()
{
	rtc_date.Century	= data_cell[2];
	rtc_date.Year		= data_cell[3];
	rtc_date.Month		= data_cell[4];
	rtc_date.Day		= data_cell[5];
	rtc_date.JulianDayH = data_cell[6];
	rtc_date.JulianDayL = data_cell[7];
	rtc_date.Hours		= data_cell[8];
	rtc_date.Minutes	= data_cell[9];
	rtc_date.Seconds	= data_cell[10];
	CalculateWeekDay();
}

