#include "config.h"
#include "controls.h"
#include "delay.h"
#include "gps.h"


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
  hq_active = 0;
}


void enable_tx_hqii()
{
  hq_enabled = 1;
}  

char pin_B()
{
  unsigned int  Value;
  // Set up it to be analog input
  TRISBbits.RB1 = 1;
  ANSELBbits.ANSB1 = 1;
  ADCON2 = 0x95;  // Right justified + 4TAD and Fosc/16
  ADCON1 = 0;
  ADCON0 = (10 << 2) | 1;   // Channel 10 and Enable bits
  ADCON0bits.GO = 1;
  while(ADCON0bits.GO) {/* wait for the done */};
  Value = (((unsigned int) ADRESH) << 8) | ((unsigned int) ADRESL);
  ADCON0 = 0;   // Disable ADC logic
  ANSELBbits.ANSB1 = 0;
  return (Value > HI_LO_THRESHOLD) ? HIGH : LOW;
}

char pin_C()
{
  unsigned int  Value;
  // Set up it to be analog input
  TRISCbits.RC6 = 1;
  ANSELCbits.ANSC6 = 1;
  ADCON2 = 0x95;  // Right justified + 4TAD and Fosc/16
  ADCON1 = 0;
  ADCON0 = (18 << 2) | 1;   // Channel 18 and Enable bits
  ADCON0bits.GO = 1;
  while(ADCON0bits.GO) {/* wait for the done */};
  Value = (((unsigned int) ADRESH) << 8) | ((unsigned int) ADRESL);
  ADCON0 = 0;   // Disable ADC logic
  ANSELCbits.ANSC6 = 0;
  return (Value > HI_LO_THRESHOLD) ? HIGH : LOW;
}

char pin_D()
{
  unsigned int  Value;
  // Set up it to be analog input
  TRISCbits.RC7 = 1;
  ANSELCbits.ANSC7 = 1;
  ADCON2 = 0x95;  // Right justified + 4TAD and Fosc/16
  ADCON1 = 0;
  ADCON0 = (19 << 2) | 1;   // Channel 19 and Enable bits
  ADCON0bits.GO = 1;
  while(ADCON0bits.GO) {/* wait for the done */};
  Value = (((unsigned int) ADRESH) << 8) | ((unsigned int) ADRESL);
  ADCON0 = 0;   // Disable ADC logic
  ANSELCbits.ANSC7 = 0;
  return (Value > HI_LO_THRESHOLD) ? HIGH : LOW;
}

char pin_E()
{
  unsigned int  Value;
  // Set up it to be analog input
  TRISBbits.RB2 = 1;
  ANSELBbits.ANSB2 = 1;
  ADCON2 = 0x95;  // Right justified + 4TAD and Fosc/16
  ADCON1 = 0;
  ADCON0 = (8 << 2) | 1;   // Channel 8 and Enable bits
  ADCON0bits.GO = 1;
  while(ADCON0bits.GO) {/* wait for the done */};
  Value = (((unsigned int) ADRESH) << 8) | ((unsigned int) ADRESL);
  ADCON0 = 0;   // Disable ADC logic
  ANSELBbits.ANSB2 = 0;
  return (Value > HI_LO_THRESHOLD) ? HIGH : LOW;
}
