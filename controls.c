
#include "config.h"
#include "controls.h"
#include "delay.h"
#include "gps.h"
#include "Fill.h"


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
	portMode(S1_8, INPUT_PULLUP);
	portMode(S9_16, INPUT_PULLUP);
  
	// Data is inverted - selected pin is 0
	data = ~(portRead(S1_8));
	if(data != 0x00)
	{	
		data = bit_pos(data);		
	}else
	{
		data = ~(portRead(S9_16));
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
	pinMode(ZBR, INPUT);
	return (pinRead(ZBR)? ZERO_POS : ON_POS);
}

byte get_button_state()
{
	pinMode(BTN, INPUT);
   	return ( pinRead(BTN) ? UP_POS : DOWN_POS);
}

char is_bootloader_active()
{
  // Check if the switch S16 is selected
  //  and the RxD is in break state  (MARK)
  pinMode(S16, INPUT);
  pinMode(RxPC, INPUT);
 
  //Switch is tied to the GND and Rx is (START)
  return (!pinRead(S16) && pinRead(RxPC));
}

void set_pin_a_as_gnd()
{
	pinMode(PIN_A_PWR, OUTPUT);
	pinWrite(PIN_A_PWR, 0);
}

void set_pin_a_as_power()
{
	pinMode(PIN_A_PWR, OUTPUT);
	pinWrite(PIN_A_PWR, 1);
}

void set_pin_f_as_io()
{
	pinMode(PIN_F, INPUT);
	pinMode(PIN_F_PWR, OUTPUT);
	pinWrite(PIN_F_PWR, 0);
}

void set_pin_f_as_power()
{
	pinMode(PIN_F, INPUT);
	pinMode(PIN_F_PWR, OUTPUT);
	pinWrite(PIN_F_PWR, 1);
}

void disable_tx_hqii()
{
  	hq_enabled = 0;
}


void enable_tx_hqii()
{
  	hq_enabled = 1;
}  


byte	NO_WPU;			// Dummy
byte	NO_ANSEL;		// Dummy


unsigned char Threshold = 0xB0;


static char pinAnalogRead(void)
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
  pinMode(PIN_B, INPUT);
  ret = pinRead(PIN_B);
  if(ret == LOW) return ret;
  // Set up it to be analog input
  pinMode(PIN_B, INPUT_ANALOG);
  ADCON0 = (10 << 2) | 1;   // RB1 : Channel 10 and Enable bits
  ret = pinAnalogRead();
  pinMode(PIN_B, INPUT);
  return ret;
}

char pin_C()
{
  char ret;
  pinMode(PIN_C, INPUT);
  ret = pinRead(PIN_C);
  if(ret == LOW) return ret;
  // Set up it to be analog input
  pinMode(PIN_C, INPUT_ANALOG);
  ADCON0 = (18 << 2) | 1;   // RC6 : Channel 18 and Enable bits
  ret = pinAnalogRead();
  pinMode(PIN_C, INPUT);
  return ret;
}

char pin_D()
{
  char ret;
  pinMode(PIN_D, INPUT);
  ret = pinRead(PIN_D);
  if(ret == LOW) return ret;
  // Set up it to be analog input
  pinMode(PIN_D, INPUT_ANALOG);
  ADCON0 = (19 << 2) | 1;   // RC7 : Channel 19 and Enable bits
  ret = pinAnalogRead();
  pinMode(PIN_D, INPUT);
  return ret;
}

char pin_E()
{
  char ret;
  pinMode(PIN_E, INPUT);
  ret = pinRead(PIN_E);
  if(ret == LOW) return ret;
  // Set up it to be analog input
  pinMode(PIN_E, INPUT_ANALOG);
  ADCON0 = (8 << 2) | 1;   // Channel 8 and Enable bits
  ret = pinAnalogRead();
  pinMode(PIN_E, INPUT);
  return ret;
}

char pin_F()
{
  char ret;
  pinMode(PIN_F, INPUT);
  ret = pinRead(PIN_F);
  return ret;
}


//-------------------------------------------------
//  LED support fucntions
//------------------------------------------------

volatile unsigned char led_counter;
volatile unsigned char led_on_time;
volatile unsigned char led_off_time;
volatile unsigned char LED_current_bit; 

void set_led_state(char on_time, char off_time)
{
	led_on_time = on_time;
	led_off_time = off_time;
	LED_current_bit = led_on_time ? HIGH : LOW;	// Turn on/off LED
	pinWrite(LEDP, LED_current_bit);
	led_counter = (led_on_time && led_off_time) ? led_on_time : 0;
}

void set_led_on()
{
	led_counter = 0;
	LED_current_bit = HIGH;				// Turn on LED
	pinWrite(LEDP, LED_current_bit);
}

void set_led_off()
{
	led_counter = 0;
	LED_current_bit = LOW;				// Turn off LED
	pinWrite(LEDP, LED_current_bit);
}

