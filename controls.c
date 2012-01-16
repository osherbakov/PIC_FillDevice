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
	VRD = HIGH;		// Keep it high
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
	return data + 1;
}



byte get_power_state()
{
	ZBR = LOW;		// Put LOW on the pin
	TRIS_ZBR = OUTPUT;	// Drive the pin low for 1 cycle
	Delay1TCY();	// Let the voltage stabilize
	TRIS_ZBR = INPUT;	// Go back to the Input state
	Delay1TCY();	// Let the voltage stabilize
	if(ZBR) return ZERO_POS;
	return ON_POS;
}

byte get_button_state()
{
	return (BTN) ? UP_POS : DOWN_POS;
}
