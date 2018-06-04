#include "config.h"
#include "controls.h"
#include "delay.h"
#include "gps.h"
#include "Fill.h"


// Variables that keep the current and previous states for 
// channel/slot switch and power switch positions
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

  	// Force all bits of port A and D into Digital IO mode
	// Make all of them Inputs (Tristate)
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
  	hq_enabled = 0;
}


void enable_tx_hqii()
{
  	hq_enabled = 1;
}  




unsigned char Threshold = 0xB0;


static char ReadPin(void)
{
	unsigned char ADC_Value;
  	ADCON2 = 0x15;  // Left justified + 4TAD and Fosc/16
  	ADCON1 = 0;
  	ADCON0bits.GO = 1;

  	while(ADCON0bits.GO) {/* wait for the done flag */};
  	ADC_Value = ADRESH;
  	ADCON0 = 0;   // Disable ADC logic

  	return (ADC_Value >= Threshold) ? HIGH : LOW;
}

char pin_B()
{
  char ret;

  TRIS_PIN_B = 1;
  ANSEL_PIN_B = 0;
  ret = digitalRead(PIN_B);
  if(ret == LOW) return ret;
  // Set up it to be analog input
  ANSEL_PIN_B = 1;
  ADCON0 = (10 << 2) | 1;   // RB1 : Channel 10 and Enable bits
  ret = ReadPin();
  ANSEL_PIN_B = 0;
  return ret;
}

char pin_C()
{
  char ret;

  TRIS_PIN_C = 1;
  ANSEL_PIN_C = 0;
  ret = digitalRead(PIN_C);
  if(ret == LOW) return ret;
  // Set up it to be analog input
  ANSEL_PIN_C = 1;
  ADCON0 = (18 << 2) | 1;   // RC6 : Channel 18 and Enable bits
  ret = ReadPin();
  ANSEL_PIN_C = 0;
  return ret;
}

char pin_D()
{
  char ret;

  TRIS_PIN_D = 1;
  ANSEL_PIN_D = 0;
  ret = digitalRead(PIN_D);
  if(ret == LOW) return ret;
  // Set up it to be analog input
  ANSEL_PIN_D = 1;
  ADCON0 = (19 << 2) | 1;   // RC7 : Channel 19 and Enable bits
  ret = ReadPin();
  ANSEL_PIN_D = 0;
  return ret;
}

char pin_E()
{
  char ret;

  TRIS_PIN_E = 1;
  ANSEL_PIN_E = 0;
  ret = digitalRead(PIN_E);
  if(ret == LOW) return ret;
  // Set up it to be analog input
  ANSEL_PIN_E = 1;
  ADCON0 = (8 << 2) | 1;   // Channel 8 and Enable bits
  ret = ReadPin();
  ANSEL_PIN_E = 0;
  return ret;
}

char pin_F()
{
  char ret;

  TRIS_PIN_F = 1;
  ret = digitalRead(PIN_F);
  return ret;
}

