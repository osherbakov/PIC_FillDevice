#include "config.h"
#include "controls.h"
#include "delay.h"
#include "gps.h"
#include "Fill.h"
#include "clock.h"


byte switch_pos;
byte prev_switch_pos;

byte power_pos;
byte prev_power_pos;
//
// Function to get the reading of the switch
//   Returns the position number currently selected.
//  Works by:
// 	- reading from S1_8 and S9_16 ports
byte get_switch_state()
{
	byte data;

  ANSELA = 0; 
  ANSELD = 0;
	TRIS_S1_8 = 0xFF;
  TRIS_S9_16 = 0xFF;
	
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
	TRIS_ZBR = INPUT;	      // Pin is Input 
	return (ZBR)? ZERO_POS : ON_POS;
}

byte get_button_state()
{
    TRIS_BTN = INPUT;
   	return (BTN) ? UP_POS : DOWN_POS;
}

char is_bootloader_active()
{
  // Check if the switch S16 is selected
  //  and the RxD is in break state  (MARK)
  ANSELA = 0; 
  ANSELC = 0;
  
  TRISAbits.RA7  = INPUT;  // That is a S16 pin
  TRIS_RxPC = INPUT;
 
  //Switch is tied to the GND and Rx is (START)
  return (!PORTAbits.RA7 && RxPC);
}

void set_pin_a_as_gnd()
{
    TRIS_PIN_A_PWR = OUTPUT;
    PIN_A_PWR = 0;
}

void set_pin_a_as_power()
{
    TRIS_PIN_A_PWR = OUTPUT;
    PIN_A_PWR = 1;
}

void set_pin_f_as_io()
{
    TRIS_PIN_F = INPUT;
    TRIS_PIN_F_PWR = OUTPUT;
    PIN_F_PWR = 0;
}

void set_pin_f_as_power()
{
    TRIS_PIN_F = INPUT;
    TRIS_PIN_F_PWR = OUTPUT;
    PIN_F_PWR = 1;
}

void disable_tx_hqii()
{
  hq_active = 0;
  hq_enabled = 0;
}


void enable_tx_hqii()
{
	InitClockData();
  hq_active = 0; 
  hq_enabled = 1;
}  



char pin_B()
{
  TRIS_PIN_B = 1;
  return digitalRead(PIN_B);
}

char pin_C()
{
  TRIS_PIN_C = 1;
  return digitalRead(PIN_C);
}

char pin_D()
{
  TRIS_PIN_D = 1;
  return digitalRead(PIN_D);
}

char pin_E()
{
  TRIS_PIN_E = 1;
  return digitalRead(PIN_E);
}

char pin_F()
{
  TRIS_PIN_F = 1;
  return digitalRead(PIN_F);
}
