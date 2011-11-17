#include "config.h"
#include "controls.h"
#include "delay.h"

byte get_switch_state()
{
	byte data;
	TRIS_VRD = 0;	// Make VD a drive pin
	VRD = 1;		// Keep it high
	Delay1TCY();	// Let the voltage stabilize
	Delay1TCY();	// Let the voltage stabilize
	Delay1TCY();	// Let the voltage stabilize
	// Data is inverted
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
	TRIS_VRD = 1;	// release the pin - save the energy
	return data + 1;
}



byte get_power_state()
{
	ZBR = 0;		// Put LOW on the pin
	TRIS_ZBR = 0;	// Drive the pin low for 10 cycles
	Delay1TCY();	// Let the voltage stabilize
	TRIS_ZBR = 1;	// Go back to the Input state
	if(ZBR) return ZERO_POS;
	return ON_POS;
}

byte get_button_state()
{
	return (BTN) ? UP_POS : DOWN_POS;
}
