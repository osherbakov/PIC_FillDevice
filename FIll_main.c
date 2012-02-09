#include "config.h"
#include "delay.h"
#include "controls.h"
#include "spi_eeprom.h"
#include "rtc.h"
#include "gps.h"
#include "serial.h"
#include "Fill.h"
#include "DS101.h"

extern void SetNextState(char nextState);


static enum 
{
	INIT = 0,
	BIST,
	BIST_ERR,
	FILL_TX,
	FILL_TX_RS232,
	FILL_TX_DTD232,
	FILL_TX_RS485, 
	FILL_TX_DS102,
	FILL_RX,
	FILL_RX_TYPE23,
	FILL_RX_DS102,
	FILL_RX_PC,
	FILL_RX_RS232,
	FILL_RX_DTD232,
	FILL_RX_RS485,
	ZERO_FILL,
	HQ_TX,
	HQ_RX,
	FILL_TX_TIME,
	FILL_TX_TIME_PROC, 
	PC_CONN,
	ERROR,
	DONE,
	WAIT_BTN_PRESS
} MAIN_STATES;



#define IDLE_SECS (600)

unsigned int idle_counter;


byte 	current_state;
byte 	button_pos;
byte 	prev_button_pos;

//
// Test if the button was depressed and released
//  Returns 1 if transition from LOW to HIGH was detected
byte TestButtonPress(void)
{
	button_pos = get_button_state();
	if(button_pos != prev_button_pos)
	{
		prev_button_pos = button_pos;
		if(button_pos == UP_POS)
		{
			return 1;
		}
	}
	return 0;
}

// Test the resutlt of the fill and set next state.
//   if result is 0 - go back to INIT state
//		result = 1 - error happened - set error blink 
//				and wait for the button press in ERROR state
//		result = 2 - loaded sucessfully - set "Key Valid" blink 
//				and wait for the button press in DONE state
//		result = -1 - timeout, continue working
void TestFillResult(char result)
{
	if(result == ST_OK)					// OK return value
	{
		SetNextState(INIT);
	}else if(result == ST_ERR)			// ERROR return value
	{
		SetNextState(ERROR);
	}else if(result == ST_DONE )			// DONE return value
	{
		SetNextState(DONE);
	}
	// Timeout - stay in the same state
}


// Set the blink pattern depending on the next state specified
void SetNextState(char nextState)
{
	switch(nextState)
	{
		case INIT:
			set_led_state(100, 0);		// Steady Light
			break;

		case ZERO_FILL:
			set_led_state(5, 5);		// About to zero-out pattern
			break;

		case FILL_RX :
		case HQ_RX :
			set_led_state(15, 15);		// "Key empty" blink pattern
			break;

		case FILL_TX :
		case FILL_TX_TIME :
			if(fill_type == MODE4)
				set_led_state(5, 150);	// "Connect Serial" blink pattern	
			else			
				set_led_state(50, 150);	// "Key valid" blink pattern
			break;

		case HQ_TX:
		case PC_CONN:
			set_led_state(5, 150);		// "Connect Serial" blink pattern
			break;
			
		case FILL_TX_RS232:
			set_led_state(20, 80);		// "Try RS232" blink pattern
			break;

		case FILL_TX_DTD232:
			set_led_state(60, 60);		// "Try RS232" blink pattern
			break;

		case FILL_TX_RS485:
			set_led_state(80,20);		// "Try RS485" blink pattern
			break;

		case FILL_RX_PC:
			set_led_state(5, 150);		// "Connect Serial" blink pattern
			break;

		case FILL_RX_RS232:
			set_led_state(200, 50);		// "Try RS232" blink pattern
			break;

		case FILL_RX_DTD232:
			set_led_state(200, 50);		// "Try RS232" blink pattern
			break;

		case FILL_RX_RS485:
			set_led_state(80,20);		// "Try RS485" blink pattern
			break;
	
		case ERROR:
			set_led_state(15, 5);		// "Fill error" blink pattern
			break;

		case DONE:
			set_led_state(100, 100);	// "Done - key valid" blink pattern
			break;
	
		default:
			set_led_state(150, 5);		// "Key loading" blink pattern
			break;
	}
	current_state = nextState;
}

void  PinsToDefault(void)
{
	disable_tx_hqii();
	close_eusart();
  set_pin_f_as_io();
	set_pin_a_as_power(); // Remove ground from pin A
	TRIS_PIN_B = 1;
	TRIS_PIN_C = 1;
	TRIS_PIN_D = 1;
	TRIS_PIN_E = 1;
	TRIS_PIN_F = 1;
}

void main()
{
	char  result;

	setup_start_io();
  PinsToDefault();	
	
#ifdef  DO_TEST
  // Perform BIST (self-test)
  SetupCurrentTime();
  TestAllPins();
  TestRTCFunctions();  
#endif

	SetNextState(INIT);
	// Initialize current state of the buttons, switches, etc
	DelayMs(100);
	prev_power_pos = get_power_state();
	prev_button_pos = get_button_state();
	prev_switch_pos = get_switch_state();
	
  idle_counter = seconds_counter + IDLE_SECS;
	while(1)
	{
		// If no activity was detected for more than 3 minutes - shut down
		if(idle_counter < seconds_counter)
		{
	  	setup_sleep_io();
			while(1) 
			{
				INTCONbits.GIE = 0;		// Disable interrupts
				INTCONbits.PEIE = 0;
				Sleep();
			};
		}

	  // Check the switch position - did it change?
		switch_pos = get_switch_state();
		if(switch_pos && (switch_pos != prev_switch_pos))
		{
      // On any change bump the idle counter
      idle_counter = seconds_counter + IDLE_SECS;
			prev_switch_pos = switch_pos; // Save new state
			SetNextState(INIT);
		}

		power_pos = get_power_state();
		if( power_pos != prev_power_pos )
		{
      // On any change bump the idle counter
      idle_counter = seconds_counter + IDLE_SECS;
			prev_power_pos = power_pos; // Save new state
			// Reset the state only when switch goes into the ZERO, but not back.
			if( power_pos == ZERO_POS )
			{
				SetNextState(INIT);
			}
		}

		switch(current_state)
		{
			// This case when any switch or button changes
			case INIT:
        PinsToDefault();
     		idle_counter = seconds_counter + IDLE_SECS;

				// Switch is in one of the key fill positions
				if( (switch_pos > 0) && (switch_pos <= MAX_NUM_POS))
				{
					// First check the position of the ZERO switch 
					if( power_pos == ZERO_POS)
					{
						SetNextState(ZERO_FILL);
					}else	// Zero is not active - use reqular
					{
						// Type = 0 - empty slot
						//		= 1 - Type 1 fill
						//		= 2,3 - Type 2,3 fill
						//		= 4 - DES Key fill
						//		= 5 - DS-101 fill (both RS232 and RS485)
						if( CheckFillType(switch_pos) > 0)
						{
							SetNextState(FILL_TX);	// Be ready to send
						}else
						{
							SetNextState(FILL_RX) ; // Receive fill
						}
					}
				}else if(switch_pos == PC_POS )		// Talk to PC
				{
					set_pin_a_as_gnd();						//  Set GND on Pin A
          set_pin_f_as_power();
					// Check for the bootloader activity
					if( is_bootloader_active() )
				  {
  				  set_led_state(0, 100);	// Turn off LED
    				INTCONbits.GIE = 0;		// Disable interrupts
    				INTCONbits.PEIE = 0;
   				  BootloadMode();
				  }else
				  {
					  SetNextState(PC_CONN);
					}
				}
				else if(power_pos == ZERO_POS)		// GPS/HQ time receive
				{
					  set_pin_a_as_gnd();
					  set_pin_f_as_power();
            SetNextState(HQ_RX);
				}else if(switch_pos == HQ_TIME_POS)	// HQ tmt
				{
					set_pin_a_as_gnd();			// Make ground on Pin A
				  set_pin_f_as_power();
					enable_tx_hqii();				// Enable HQ output
					SetNextState(HQ_TX);
				}else if(switch_pos == SG_TIME_POS)
				{
          // SINCGARS TIME only fill - will use negative logic 
          set_pin_a_as_power();
				  set_pin_f_as_io();
					fill_type = MODE3;
					SetNextState(FILL_TX_TIME);
				}
				break;
				
			case FILL_TX_RS232:
				result = SendRS232Fill(switch_pos);
				// On the timeout - switch to next mode
				if(result < 0)
				{
					SetNextState(FILL_TX_DTD232);	
				}else
				{
					TestFillResult(result);
				}
				break;	

			case FILL_TX_DTD232:
				result = SendDTD232Fill(switch_pos);
				// On the timeout - switch to next mode
				if(result < 0)
				{
					SetNextState(FILL_TX_RS485);	
				}else
				{
					TestFillResult(result);
				}
				break;	

			case FILL_TX_RS485:
				result = SendRS485Fill(switch_pos);
				// On the timeout - switch to next mode
				if(result < 0)
				{
					SetNextState(FILL_TX_RS232);	
				}else
				{
					TestFillResult(result);
				}
				break;
					
			case FILL_TX:
				if(fill_type == MODE5)          // Any DS-101 fill
				{
					set_pin_a_as_gnd();						//  Set GND on Pin A
          set_pin_f_as_power();
					SetNextState(FILL_TX_RS232);	// Start with RS232 and cycle thru 3 modes
				}else if(fill_type == MODE4)    // MBITR keys
				{
					set_pin_a_as_gnd();						//  Set GND on Pin A
          set_pin_f_as_power();
  				TestFillResult(WaitReqSendMBITRFill());
				}else if( CheckType123Equipment() > 0 )
				{
					set_pin_a_as_power();						//  Set POWER on Pin A for Type 1,2,3
          set_pin_f_as_io();
					SetNextState(FILL_TX_DS102);
				}
				break;

			case FILL_TX_DS102:
				// Check if the fill was initialed on the Rx device
				TestFillResult(WaitReqSendDS102Fill());
				// For Type1 fills we can simulate KOI18 and start
				//     sending data on button press....
				if( (fill_type == MODE1) && TestButtonPress() )
				{
					TestFillResult(SendDS102Fill());
				}
				break;

			case FILL_RX:
			    // For Type 2 and 3 pins D and F should go low
				  result = CheckFillType23();
				  if(result > 0)
				  {
  					// Process Type 2, and 3 fills
  					fill_type = result;
  					SetNextState(FILL_RX_TYPE23);
  					break;
  				}
  				// If button pressed - Type 1!!!
  				if( TestButtonPress() )
  				{
  					fill_type = MODE1;
  					SetNextState(FILL_RX_DS102);
  				}

          // If Pin_D is -5V - that is Type 4 or RS-232 Type 5
   				result = CheckFillType4();
  				if(result > 0)
  				{
  					fill_type = result;
  					SetNextState(FILL_RX_PC);
 					  break;
  				}

          // If Pin_D is -5V - that is Type 4 or RS-232 Type 5
   				result = CheckFillRS232Type5();
  				if(result > 0)
  				{
  					fill_type = result;
  					SetNextState(FILL_RX_RS232);
 					  break;
  				}

          // If Pin_C is -5V - that is DTD-232 Type 5
  				result = CheckFillDTD232Type5();
  				if(result > 0)
  				{
  					fill_type = result;
  					SetNextState(FILL_RX_DTD232);
				    break;
  				}

  				result = CheckFillRS485Type5();
  				if(result > 0)
  				{
  					fill_type = result;
  					SetNextState(FILL_RX_RS485);
				    break;
  				}
				  break;

			case FILL_RX_TYPE23:
			  CheckFillType23();
				if( TestButtonPress() )
				{
					SetNextState(FILL_RX_DS102);
				}
				break;

			case FILL_RX_DS102:
				TestFillResult(StoreDS102Fill(switch_pos, fill_type));
				break;
				
			case FILL_RX_PC:
				TestFillResult(StorePCFill( switch_pos, MODE4));
				break;

			case FILL_RX_RS232:
				result = StoreRS232Fill(switch_pos, MODE5);
				if(result < 0)
				{
					SetNextState(FILL_RX);
				}else
				{
					TestFillResult(result);
				}
				break;

			case FILL_RX_DTD232:
				result = StoreDTD232Fill(switch_pos, MODE6);
				if(result < 0)
				{
					SetNextState(FILL_RX);
				}else
				{
					TestFillResult(result);
				}
				break;

			case FILL_RX_RS485:
				result = StoreRS485Fill(switch_pos, MODE7);
				if(result < 0)
				{
					SetNextState(FILL_RX);
				}else
				{
					TestFillResult(result);
				}
				break;
				
			case ZERO_FILL:
				if( TestButtonPress() )
				{
					TestFillResult(ClearFill(switch_pos));
				}
				break;

			case PC_CONN:
				PCInterface();
				break;

			case HQ_TX:
				break;

			case FILL_TX_TIME:
				if( CheckType123Equipment() > 0 )
				{
					SetNextState(FILL_TX_TIME_PROC);
				}
				break;

			case FILL_TX_TIME_PROC:
				TestFillResult(WaitReqSendTODFill());
				break;

			case HQ_RX:
				// State may change after call to the ReceiveGPSTime() and ReceiveHQTime()
				if( current_state == HQ_RX)
				{
					TestFillResult(ReceiveGPSTime());
				}
				if( current_state == HQ_RX)
				{
					TestFillResult(ReceiveHQTime());
				}
				break;


			case ERROR:
			case DONE:
        current_state = WAIT_BTN_PRESS;
				break;

			case WAIT_BTN_PRESS:
				if( TestButtonPress() )
				{
					SetNextState(INIT);
				}
				break;
		}
	}
}