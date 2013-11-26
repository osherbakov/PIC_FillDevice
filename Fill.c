#include "config.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"
#include "gps.h"
#include "delay.h"
#include <ctype.h>

// Generic cell that can keep all the data
byte	  data_cell[FILL_MAX_SIZE];

// Time cell
byte    TOD_cell[MODE2_3_CELL_SIZE];

static byte month_names[] = "XXXJANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";
static byte weekday_names[] = "XXXMONTUEWEDTHUFRISATSUN";


static void GetCurrentDayTime(byte *p_buffer)
{
  byte  ms_100, ms_10, month, weekday;
  
  
  GetRTCData();
  
  *p_buffer++ = '0' + (rtc_date.Hours >> 4);
  *p_buffer++ = '0' + (rtc_date.Hours & 0x0F);
  *p_buffer++ = '0' + (rtc_date.Minutes >> 4);
  *p_buffer++ = '0' + (rtc_date.Minutes & 0x0F);
  *p_buffer++ = '0' + (rtc_date.Seconds >> 4);
  *p_buffer++ = '0' + (rtc_date.Seconds & 0x0F);
  *p_buffer++ = '.';

  ms_100 = 0;
	ms_10 = rtc_date.MilliSeconds_10;
	while(ms_10 >= 10)
	{
		ms_100++;
		ms_10 -= 10; 
	}
  *p_buffer++ = '0' + ms_100;
  
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
  *p_buffer++ = 0x0D;
  *p_buffer++ = 0x00;
}  


char ClearFill(byte stored_slot)
{
	 unsigned short long base_address = get_eeprom_address(stored_slot & 0x0F);
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
	TOD_cell[11] = 0;
	TOD_cell[12] = 0;
	TOD_cell[13] = 0;
	TOD_cell[14] = 0;

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

char IsValidYear()
{
	return (rtc_date.Century	>= 0x20) &&
	          (rtc_date.Year	>= 0x12);
}

// Time is the sequence of 6 digits
static char  ExtractTime(byte *p_buff, byte n_count)
{
  // find the block that has 6 digits
  while(1)
  {
    while ( !isdigit(*p_buff) && (n_count > 6))
    {
      p_buff++; n_count--;
    }
    if(n_count >= 6)
    {
      if(isdigit(p_buff[0]) && isdigit(p_buff[1]) && isdigit(p_buff[2]) && 
          isdigit(p_buff[3]) && isdigit(p_buff[4]) && isdigit(p_buff[5]) )
      {
  	    rtc_date.Hours		= ((p_buff[0] & 0x0F) << 4) + (p_buff[1] & 0x0F);
        rtc_date.Minutes	= ((p_buff[2] & 0x0F) << 4) + (p_buff[3] & 0x0F);
        rtc_date.Seconds	= ((p_buff[4] & 0x0F) << 4) + (p_buff[5] & 0x0F);
        return TRUE;
      } 
      while ( isdigit(*p_buff) && (n_count > 6))
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
      	    rtc_date.Day		= ((p_buff[0] & 0x0F) << 4) + (p_buff[1] & 0x0F);
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
      if( isdigit(p_buff[0]) && isdigit(p_buff[1]) && 
          isdigit(p_buff[2]) && isdigit(p_buff[3]) && 
            ((n_count == 4) || !isdigit(p_buff[4])) )
      {
  	    rtc_date.Century		= ((p_buff[0] & 0x0F) << 4) + (p_buff[1] & 0x0F);
        rtc_date.Year	=   ((p_buff[2] & 0x0F) << 4) + (p_buff[3] & 0x0F);
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

void GetSetCurrentDayTime(byte *p_buffer)
{
	byte byte_cnt;
	byte_cnt = rx_eusart_cont(p_buffer, FILL_MAX_SIZE);
  
  if( (byte_cnt >= 17) &&
        ExtractYear(p_buffer, byte_cnt) &&
          ExtractTime(p_buffer, byte_cnt) &&
            ExtractDate(p_buffer, byte_cnt) )
  {
    CalculateJulianDay();
    CalculateWeekDay();
		SetRTCData();		
	}
	GetCurrentDayTime(p_buffer);
}

