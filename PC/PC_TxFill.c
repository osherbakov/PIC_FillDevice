#include "config.h"
#include "crcmodel.h"
#include "delay.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"
#include "controls.h"


// The command from the PC to fill the key or to dump it
// The byte after the command specifies the key type
static const byte TIME_CMD[] 		= "/TIME=";		// Fill/Dump Time
static const byte DATE_CMD[] 		= "/DATE=";		// Fill/Dump Time
static const byte KEY_CMD[] 		= "/KEY<n>";	// Read/Write the key N


void PCInterface()
{
	byte	i;
 	byte *p_data = &data_cell[0];
	// If entering the first time - enable eusart
	// and initialize the buffer to get chars
	if( !uartIsEnabled())
	{
		open_eusart(PC_BAUDRATE, DATA_POLARITY_RXTX);
 		rx_eusart_async(p_data, 6, INF_TIMEOUT);
	}
	
	// Wait to receive 6 characters
	if(rx_eusart_count() >= 6) 
	{
	  	byte  	slot;
		byte 	type;

		if(is_equal( p_data, TIME_CMD, 5) || is_equal(p_data, DATE_CMD, 5))
	  	{
			if(p_data[5] == '=') {
				SetCurrentDayTime();
			}
	  	  	GetCurrentDayTime();
			rx_eusart_async(p_data, 6, INF_TIMEOUT);  // Restart collecting data
	    }else if(is_equal( p_data, KEY_CMD, 4))
	  	{
	    	// The next char in /KEY<n> is the slot number
		  	slot = p_data[4];
			if(p_data[5] == '=') {
				SetPCKey(slot);
			}else if(slot == '0' || slot == ' ')	{	// Special case - all keys
				for(i = 1; i <=13; i++) {
					GetPCKey(i);
 				}
			}else {			
				GetPCKey(slot);
			}
			rx_eusart_async(p_data, 6, INF_TIMEOUT);  // Restart collecting data
		}
  	}  
}


