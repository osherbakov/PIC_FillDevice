#include "config.h"
#include "controls.h"
#include "delay.h"



byte switch_pos;
byte prev_switch_pos;

byte power_pos;
byte prev_power_pos;
//
// Function to get the reading of the switch
//   Returns the position numbercurrently selected.
//  Works by:
// 	- reading from S1_8 and S9_16 ports
byte get_switch_state()
{
	byte data;

	TRIS_S1_8 = 0xFF;	// Inputs
	TRIS_S9_16 = 0xFF;	// Inputs
	
	TRIS_VRD = OUTPUT;
	VRD = HIGH;		// Keep it high
	Delay10TCY();	// Let the voltage stabilize
	
	// Data is inverted - selected pin is 0
	data = ~(S1_8);
	if(data != 0x00)
	{	
		data = bit_pos(data);		
	}else
	{
		data = ~(S9_16);
		if(data != 0x00)
		{	
			data = bit_pos(data) + 8;
		}else
		{
			data = -1;
		}
	}
	TRIS_VRD = INPUT;
	return data + 1;
}



byte get_power_state()
{
	TRIS_ZBR = OUTPUT;	// Drive the pin low for 1 cycle
	ZBR = LOW;		// Put LOW on the pin
	Delay1TCY();	// Let the voltage stabilize
	TRIS_ZBR = INPUT;	// Go back to the Input state
	Delay10TCY();	// Let the voltage stabilize
	if(ZBR) return ZERO_POS;
	return ON_POS;
}

byte get_button_state()
{
	return (BTN) ? UP_POS : DOWN_POS;
}

char is_bootloader_active()
{
	ANSELA = 0x00;
	ANSELC = 0x00;
	ANSELD = 0x00;
	
  TRIS_Rx = INPUT;
  TRIS_Tx = INPUT;
	
	// Ground control - output and no pullup
	TRIS_OFFBR = OUTPUT;	// Output
	WPUB_OFFBR = 0;
	OFFBR = 0;
  
  // Make ground on Pin B
	TRIS_PIN_GND = INPUT;
	ON_GND = 1;						
	
	return ( (get_switch_state() == PC_POS) && TxBIT );  
}  