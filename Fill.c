#include "config.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"
#include "gps.h"
#include "delay.h"
#include <ctype.h>

// Generic cell that can keep all the data
#pragma udata big_buffer   // Select large section
byte	  data_cell[FILL_MAX_SIZE];
#pragma udata               // Return to normal section

// Time cell
byte    TOD_cell[MODE2_3_CELL_SIZE];

static const byte month_names[] 	= "XXXJANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";
static const byte weekday_names[] 	= "XXXMONTUEWEDTHUFRISATSUN";
static const byte HexToASCII[] 		= "0123456789ABCDEF";
static const byte TimeErrorMsg[] 	= "Error in Time/Date format\n\0";
static const byte TimeOKMsg[] 		= "Time/Date is set\n\0";
static const byte TimeGetMsg[] 		= "/Time=\0";
static const byte KeyErrorMsg[] 	= "Error in Key Data format\n\0";
static const byte KeyOKMsg[] 		= "Key is set\n\0";
static const byte KeyErasedMsg[] 	= "Key is erased\n\0";
static const byte KeyGetMsg[] 		= "/Key\0";

static byte	*p_buffer;		// Dedicated pointer to access large buffers
void GetCurrentDayTime()
{
  byte  month, weekday;
  p_buffer = data_cell; 
  
	tx_eusart_str(TimeGetMsg);
	tx_eusart_flush();

  GetRTCData();
  
  *p_buffer++ = '0' + (rtc_date.Hours >> 4);
  *p_buffer++ = '0' + (rtc_date.Hours & 0x0F);
  *p_buffer++ = '0' + (rtc_date.Minutes >> 4);
  *p_buffer++ = '0' + (rtc_date.Minutes & 0x0F);
  *p_buffer++ = '0' + (rtc_date.Seconds >> 4);
  *p_buffer++ = '0' + (rtc_date.Seconds & 0x0F);
  *p_buffer++ = '.';
  *p_buffer++ = '0' + (ms_10/10);
  
  *p_buffer++ = 'Z';
  *p_buffer++ = ' ';
  
  *p_buffer++ = '0' + (rtc_date.Day >> 4);
  *p_buffer++ = '0' + (rtc_date.Day & 0x0F);

  month = (rtc_date.Month >> 4) * 10 + (rtc_date.Month & 0x0F);
  month *= 3;

  *p_buffer++ = month_names[month++];
  *p_buffer++ = month_names[month++];
  *p_buffer++ = month_names[month++];

  *p_buffer++ = '0' + (rtc_date.Century >> 4);
  *p_buffer++ = '0' + (rtc_date.Century & 0x0F);
  *p_buffer++ = '0' + (rtc_date.Year >> 4);
  *p_buffer++ = '0' + (rtc_date.Year & 0x0F);
  *p_buffer++ = ' ';

  weekday = (rtc_date.WeekDay & 0x0F) * 3;
  *p_buffer++ = weekday_names[weekday++];
  *p_buffer++ = weekday_names[weekday++];
  *p_buffer++ = weekday_names[weekday++];
  *p_buffer++ = ' ';

  *p_buffer++ = 'J';
  *p_buffer++ = 'D';
  *p_buffer++ = '0' + (rtc_date.JulianDayH & 0x0F);
  *p_buffer++ = '0' + (rtc_date.JulianDayL >> 4);
  *p_buffer++ = '0' + (rtc_date.JulianDayL & 0x0F);
  *p_buffer++ = '\n';
  *p_buffer++ = 0x00;
	
   p_buffer = data_cell; 
   tx_eusart_str(p_buffer);
   tx_eusart_flush();
}  

char ClearFill(byte stored_slot)
{
	unsigned short long base_address = get_eeprom_address(stored_slot);
   	byte_write(base_address, 0x00);
   	DelayMs(500);    // Debounce the button
   	return ST_OK;
}

// Detect the fill type and set up global variables
// Type 1, 2,3 - will be sent thru DS102 interface
char CheckFillType(byte stored_slot)
{
  	byte  	fill_type, records;
  	unsigned short long base_address = get_eeprom_address(stored_slot & 0x0F);
	records = byte_read(base_address++); 

	// Get the fill type from the EEPROM
	fill_type = byte_read(base_address++);
	if( (records == 0xFF) || (records == 0) || (fill_type == 0xFF))
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
	TOD_cell[11] = ((ms_10 >= 100) ? 0 : (ms_10/10)) << 4;
	TOD_cell[12] = 0;
	TOD_cell[13] = 0;
	TOD_cell[14] = 0;
}

void  ExtractTODData()
{
    p_buffer = data_cell; 
	rtc_date.Century	= p_buffer[2];
	rtc_date.Year		= p_buffer[3];
	rtc_date.Month		= p_buffer[4];
	rtc_date.Day		= p_buffer[5];
	rtc_date.JulianDayH = p_buffer[6];
	rtc_date.JulianDayL = p_buffer[7];
	rtc_date.Hours		= p_buffer[8];
	rtc_date.Minutes	= p_buffer[9];
	rtc_date.Seconds	= p_buffer[10];
	rtc_date.MilliSeconds10	= (p_buffer[11] >> 4) * 10;
	CalculateWeekDay();
}

char IsValidYear()
{
	return (rtc_date.Century	>= 0x20) &&
	          (rtc_date.Year	>= 0x10);
}

// Time is the sequence of 6 digits
static char  ExtractTime(byte *p_buff, byte n_count)
{
  // find the block that has 6 digits
  while(1)
  {
	// find the next digit and at least 6 characters remaining
    while ( !isdigit(*p_buff) && (n_count > 6))
    {
      p_buff++; n_count--;
    }
    if(n_count >= 6){
      if(isdigit(p_buff[0]) && isdigit(p_buff[1]) && isdigit(p_buff[2]) && 
          isdigit(p_buff[3]) && isdigit(p_buff[4]) && isdigit(p_buff[5]) )
      {
  	    rtc_date.Hours		= ((p_buff[0] & 0x0F) << 4) + (p_buff[1] & 0x0F);
        rtc_date.Minutes	= ((p_buff[2] & 0x0F) << 4) + (p_buff[3] & 0x0F);
        rtc_date.Seconds	= ((p_buff[4] & 0x0F) << 4) + (p_buff[5] & 0x0F);
        return TRUE;
      } 
      while ( isdigit(*p_buff) && (n_count > 6)){
        p_buff++; n_count--;
      }
    }else{
      return FALSE;
    }  
  }  
  return FALSE; 
}

// Date is the block of 2 digits and 3 letters
static char  ExtractDate(byte *p_buff, byte n_count)
{
  byte month;
  // find the block that has 2 Digits and 3 Letters
  while(1)
  {
    while ( !isdigit(*p_buff) && (n_count > 5))
    {
      p_buff++; n_count--;
    }
    if(n_count >= 5)
    {
      if(isdigit(p_buff[0]) && isdigit(p_buff[1]) && isalpha(p_buff[2]) && 
          isalpha(p_buff[3]) && isalpha(p_buff[4]) )
      {
  	    // Find the month in the table
  	    for( month = 1; month <= 12; month++)
  	    {
    	    if(is_equal(&p_buff[2], &month_names[month * 3], 3))
    	    {
        	  	if(month >= 10) month += 0x06;
      	    	rtc_date.Day	= ((p_buff[0] & 0x0F) << 4) + (p_buff[1] & 0x0F);
            	rtc_date.Month	= month;
            	return TRUE;
      	  	}  
    	  }
    	  return FALSE;
      } 
      while ( isdigit(*p_buff) && (n_count > 5))
      {
	      p_buff++; n_count--;
      }
    }else
    {
      return FALSE;
    }  
  }  
  return FALSE; 
}

// Year is the 4 sequential digits
static char  ExtractYear(byte *p_buff, byte n_count)
{
  // find the block that has 4 digits only
  while(1)
  {
    while ( !isdigit(*p_buff) && (n_count > 4))
    {
      p_buff++; n_count--;
    }
    if(n_count >= 4)
    {
      if( (p_buff[0] == '2') && (p_buff[1] == '0') && 
          	isdigit(p_buff[2]) && isdigit(p_buff[3]) && 
            	!isdigit(p_buff[4]) )
      {
  	    rtc_date.Century	= ((p_buff[0] & 0x0F) << 4) + (p_buff[1] & 0x0F);
        rtc_date.Year		= ((p_buff[2] & 0x0F) << 4) + (p_buff[3] & 0x0F);
        return TRUE;
      }
      // Skip digits 
      while ( isdigit(*p_buff) && (n_count > 4))
      {
        p_buff++; n_count--;
      }
    }else
    {
      return FALSE;
    }  
  }  
  return FALSE; 
}

void SetCurrentDayTime()
{
	byte byte_cnt;
	byte	*p_buffer = data_cell;
	
	byte_cnt = rx_eusart_line(p_buffer, FILL_MAX_SIZE, 30*1000);
  	if( (byte_cnt > 0) && 
			ExtractYear(p_buffer, byte_cnt) &&
          	ExtractTime(p_buffer, byte_cnt) &&
            ExtractDate(p_buffer, byte_cnt) )
	{
    	CalculateJulianDay();
    	CalculateWeekDay();
		SetRTCData();
		tx_eusart_str(TimeOKMsg);
		tx_eusart_flush();
	}else
	{
		tx_eusart_str(TimeErrorMsg);
		tx_eusart_flush();
	}	
}

byte  ASCIIToHex(byte Symbol)
{
	byte	Data;
	if( (Symbol >= '0') && (Symbol <= '9' )) Data = Symbol - '0';
	else if((Symbol >= 'A') && (Symbol <= 'F' )) Data = Symbol - 'A' + 0x0A;
	else if((Symbol >= 'a') && (Symbol <= 'f' )) Data = Symbol - 'a' + 0x0A;
	else Data = Symbol & 0x0F;
	return Data; 
}

void GetPCKey(byte slot)
{
	// The first byte is the Number of records: 0 or FF - empty
	// Then goes the byte that specifies the Type of the fill
	// After that go all N records
	// Each record starts with the number of bytes in that record
	unsigned short long base_address;
	byte	numRecords;
	byte	numBytes;
	byte	fillType;
	byte	Data;

	p_buffer = data_cell;
	slot = ASCIIToHex(slot);

	tx_eusart_str(KeyGetMsg);
	tx_eusart_flush();
	p_buffer[0] = HexToASCII[slot];
	p_buffer[1] = '=';
	p_buffer[2] = '\n';
	tx_eusart_async(p_buffer, 3);
	tx_eusart_flush();

  	base_address = get_eeprom_address(slot);
	numRecords = byte_read(base_address++); if(numRecords == 0xFF) numRecords = 0;
	if(numRecords > 0) {
		fillType = byte_read(base_address++);
		p_buffer[0] = HexToASCII[(fillType>>4) & 0x0F];
		p_buffer[1] = HexToASCII[fillType & 0x0F];
		p_buffer[2] = '\n';
		tx_eusart_async(p_buffer, 3);
   		tx_eusart_flush();
	}

	while(numRecords > 0)
	{
		numBytes = byte_read(base_address++);
		while(numBytes > 0) {
			Data = byte_read(base_address++);
			p_buffer[0] = ' ';
			p_buffer[1] = HexToASCII[(Data>>4) & 0x0F];
			p_buffer[2] = HexToASCII[Data & 0x0F];
			tx_eusart_async(p_buffer, 3);
    		tx_eusart_flush();
			numBytes--;
		}	
		p_buffer[0] = '\n';
		tx_eusart_async(p_buffer, 1);
    	tx_eusart_flush();
		numRecords --;
	}	
	p_buffer[0] = '\n';
	tx_eusart_async(p_buffer, 1);
   	tx_eusart_flush();
}	

void SetPCKey(byte slot)
{
	byte 					byte_cnt;
	byte					*p_buffer = data_cell;
	byte					idx;
	unsigned short long 	base_address;
	unsigned short long 	current_address;
	byte					numRecords;
	byte					numBytes;
	byte					fillType;
	byte					Ch, Data, DataRdy;;

	slot = ASCIIToHex(slot);

  	base_address = get_eeprom_address(slot);	
	current_address = base_address + 2;		// Keys will go there - 1st byte is Number of Records
											//  2nd Byte is Type, 
	numRecords = 0;
	numBytes = 0;
	fillType = 0;

	// Get the Fill type
	do {
		byte_cnt = rx_eusart_line(p_buffer, FILL_MAX_SIZE, INF_TIMEOUT);
		if(byte_cnt < 0 ) goto KeyErr;
	} while(byte_cnt == 0);

	// Parse the string and extract Fill Type
	for(idx = 0; idx < byte_cnt; idx++) 
	{
		fillType = (fillType << 4) + ASCIIToHex(p_buffer[idx]);
	}
	if(fillType == 0) goto KeyErase;

	// Now gow thru every line and save the appropriate key
	while(1) {
		byte_cnt = rx_eusart_line(p_buffer, FILL_MAX_SIZE, INF_TIMEOUT);
		if(byte_cnt < 0) goto KeyErr;
		if(byte_cnt == 0) goto KeyOK;

		numBytes = 0;
		DataRdy = 0;
		Data = 0;
		for(idx = 0; idx < byte_cnt; idx++) {
			Ch = p_buffer[idx];
			if(isxdigit(Ch)) { 
				Data = (Data << 4) + ASCIIToHex(Ch);
				DataRdy = 1;
			}else if ((ispunct(Ch) || isspace(Ch)) && DataRdy) {
				p_buffer[numBytes++] = Data;
				Data = 0;
				DataRdy = 0;	
			}
		}

		if(DataRdy) p_buffer[numBytes++] = Data;	// Save the last accumulated
		byte_write(current_address, numBytes);
		current_address++;
		array_write(current_address, p_buffer, numBytes);
		current_address += numBytes;
		numRecords++;
	}
	
	KeyErr:
	{
		tx_eusart_str(KeyErrorMsg);
   		tx_eusart_flush();
		return;
	}
	KeyErase:
	{
		byte_write(base_address, 0);
		tx_eusart_str(KeyErasedMsg);
   		tx_eusart_flush();
		return;
	}
	KeyOK:
	{
		byte_write(base_address++, numRecords);
		byte_write(base_address++, fillType);
		tx_eusart_str(KeyOKMsg);
   		tx_eusart_flush();
		return;
	}
}	

