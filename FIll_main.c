#include "config.h"
#include "delay.h"
#include "controls.h"
#include "spi_eeprom.h"
#include "rtc.h"
#include "gps.h"
#include "serial.h"
#include "Fill.h"

extern volatile byte hq_enabled; 
extern char ReceiveHQTime(void);

byte 	current_state;

static enum 
{
	INIT = 0,
	FILL_TX, 
	FILL_TX_PROC, 
	FILL_RX,
	FILL_RX_SG,
	FILL_RX_PROC,
	ZERO_FILL,
	HQ_TX,
	HQ_RX,
	TIME_TX,
	TIME_TX_PROC, 
	PC_CONN,
	DONE
} MAIN_STATES;


byte 	button_pos;
byte 	prev_button_pos;

#define MAX_NUM_POS (10)
#define HQ_TIME_POS (14)
#define SG_TIME_POS (15)
#define PC_POS 		(16)


byte TestButtonPress(void)
{
	button_pos = get_button_state();
	if(button_pos != prev_button_pos)
	{
		prev_button_pos = button_pos;
		if(button_pos == UP_POS)
			return 1;
	}
	return 0;
}

void TestFillResult(char result)
{
	if(result == 0)					// OK return value
	{
		current_state = INIT;
	}else if(result == 1)			// ERROR return value
	{
		set_led_state(10 , 5);		// "Fill error" blink pattern
		current_state = DONE;
	}else if(result == 2 )			// DONE return value
	{
		set_led_state(70 , 30);		// "Key valid" blink pattern
		current_state = DONE;
	}
	// Timeout - stay in the same state
}

void SetNextState(char nextState)
{
	switch(nextState)
	{
		case ZERO_FILL:
			set_led_state(5 , 5);		// About to zero-out pattern
			break;

		case FILL_RX :
		case HQ_RX :
			set_led_state(15 , 15);		// "Key empty" blink pattern
			break;

		case FILL_TX :
		case TIME_TX :
			if(fill_type == MODE4)
				set_led_state(5 , 95);
			else			
				set_led_state(70 , 30);		// "Key valid" blink pattern
			break;

		case HQ_TX:
		case PC_CONN:
			set_led_state(5 , 95);		// "Key HQ sending" blink pattern
			break;
		
		default:
			set_led_state(95 , 5);		// "Key loading" blink pattern
			break;
	}
	current_state = nextState;
}


void main()
{
	setup_start_io();
	OSCTUNEbits.TUN = 10;
	current_state = INIT;

	// Initialize current state of the butons, switches, etc
	prev_power_pos = get_power_state();
	prev_button_pos = get_button_state();
	prev_switch_pos = get_switch_state();

	while(1)
	{
		Sleep();	// Will wake up every 10 ms

	  	switch_pos = get_switch_state();
		if(switch_pos && (switch_pos != prev_switch_pos))
		{
			prev_switch_pos = switch_pos;
			current_state = INIT;
		}

		power_pos = get_power_state();
		if( power_pos != prev_power_pos )
		{
			prev_power_pos = power_pos;
			// Reset the state only when switch goes into the ZERO, but not back.
			if( power_pos == ZERO_POS )
			{
				current_state = INIT;
			}
		}

		switch(current_state)
		{
			case INIT:
				// Remove ground from pin B
				ON_GND = 0;
				hq_enabled = 0;
				close_eusart();

				if( (switch_pos > 0) && (switch_pos <= MAX_NUM_POS))
				// Switch is in one of the key fill positions
				{
					if( power_pos == ZERO_POS)
					{
						SetNextState(ZERO_FILL);
					}else
					{
						if( CheckFillType(switch_pos) > 0)
						{
							SetNextState(FILL_TX);
						}else
						{
							SetNextState(FILL_RX) ; // Receive fill
						}
					}
				}else if(switch_pos == PC_POS )
				{
					TRIS_PIN_GND = INPUT;			// Make Ground
					ON_GND = 1;						//  on Pin B
					open_eusart();
					SetNextState(PC_CONN);
				}
				else if(power_pos == ZERO_POS)		// GPS/HQ time receive
				{
					TRIS_PIN_GND = INPUT;			// Make ground
					ON_GND = 1;						//  on Pin B
					SetNextState(HQ_RX);
				}else if(switch_pos == HQ_TIME_POS)	// HQ tmt
				{
					TRIS_PIN_GND = INPUT;
					ON_GND = 1;						// Make ground on Pin B
					hq_enabled = 1;					// Enable HQ output
					SetNextState(HQ_TX);
				}else if(switch_pos == SG_TIME_POS)
				{
					SetNextState(TIME_TX);
				}
				break;

			case FILL_TX:
				if( CheckEquipment() > 0 )
				{
					SetNextState(FILL_TX_PROC);
				}
				break;

			case FILL_TX_PROC:
				TestFillResult(SendStoredFill(switch_pos));
				break;

			case FILL_RX:
				if(GetFillType() > 0)
				{
					SetNextState(FILL_RX_SG);
				}
				if( TestButtonPress() )
				{
					fill_type = MODE1;
					SetNextState(FILL_RX_PROC);
				}
				break;

			case FILL_RX_SG:
				if( TestButtonPress() )
				{
					SetNextState(FILL_RX_PROC);
				}
				break;

			case FILL_RX_PROC:
				TestFillResult(GetStoreFill(switch_pos));
				break;

			case ZERO_FILL:
				if( TestButtonPress() )
				{
					ClearFill(switch_pos);
					SetNextState(INIT);
				}
				break;

			case PC_CONN:
				PCInterface();
				break;

//			case HQ_TX:
//				break;

			case TIME_TX:
				if( CheckEquipment() == MODE3 )
				{
					SetNextState(TIME_TX_PROC);
				}
				break;

			case TIME_TX_PROC:
				TestFillResult(SendTODFill());
				break;

			case HQ_RX:
				TestFillResult(ReceiveGPSTime());
				TestFillResult(ReceiveHQTime());
				break;

			case DONE:
				if( TestButtonPress() )
				{
					SetNextState(INIT);
				}
				break;
		}
	}
}