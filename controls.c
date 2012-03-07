#include "config.h"
#include "controls.h"
#include "delay.h"
#include "gps.h"
#include "Fill.h"


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
	InitClockData();
  hq_enabled = 1;
}  

unsigned char ADC_Max = 0x7F;
unsigned char ADC_Min = 0x00;
unsigned char ADC_Threshold;
unsigned char ADC_Last;

static char ReadPin(void)
{
  unsigned char Delta;
  ADCON2 = 0x0C;  // Left justified + 2TAD and Fosc/4
  ADCON1 = 0;
  ADCON0bits.GO = 1;
// After starting ADC we have plenty of time to do some math
//  ADC_Threshold = (ADC_Max + ADC_Min) >> 1;
//  ADC_Max  = MAX(ADC_Max, ADC_Last);
//  ADC_Min  = MIN(ADC_Min, ADC_Last);
//  if(ADC_Max > ADC_Min)
//  {
//    Delta = (ADC_Max - ADC_Min) >> 6;
//    ADC_Max -= Delta; // Apply decay factor
//    ADC_Min += Delta;
//  }
  while(ADCON0bits.GO) {/* wait for the done */};
  ADC_Last = ADRESH; // (((unsigned char) ADRESH) << 5) | ((unsigned char) ADRESL >> 3);
  ADCON0 = 0;   // Disable ADC logic
  return (ADC_Last > 0x90) ? HIGH : LOW;
}

char pin_B()
{
  char ret;
  if( !digitalRead(PIN_B)) return LOW;
  // Set up it to be analog input
  TRIS_PIN_B = 1;
  ANSEL_PIN_B = 1;
  ADCON0 = (10 << 2) | 1;   // Channel 10 and Enable bits
  ret = ReadPin();
  ANSEL_PIN_B = 0;
  return ret;
}

char pin_C()
{
  char ret;
  if( !digitalRead(PIN_C)) return LOW;
  // Set up it to be analog input
  TRIS_PIN_C = 1;
  ANSEL_PIN_C = 1;
  ADCON0 = (18 << 2) | 1;   // Channel 18 and Enable bits
  ret = ReadPin();
  ANSEL_PIN_C = 0;
  return ret;
}

char pin_D()
{
  char ret;
  if( !digitalRead(PIN_D)) return LOW;
  // Set up it to be analog input
  TRIS_PIN_D = 1;
  ANSEL_PIN_D = 1;
  ADCON0 = (19 << 2) | 1;   // Channel 19 and Enable bits
  ret = ReadPin();
  ANSEL_PIN_D = 0;
  return ret;
}

char pin_E()
{
  char ret;
  if( !digitalRead(PIN_E)) return LOW;
  // Set up it to be analog input
  TRIS_PIN_E = 1;
  ANSEL_PIN_E = 1;
  ADCON0 = (8 << 2) | 1;   // Channel 8 and Enable bits
  ret = ReadPin();
  ANSEL_PIN_E = 0;
  return ret;
}

char pin_F()
{
  return digitalRead(PIN_F);
}


char pin_MAX()
{
  return ADC_Max;
}

char pin_MIN()
{
  return ADC_Min;
}

char pin_Threshold()
{
  return ADC_Threshold;
}