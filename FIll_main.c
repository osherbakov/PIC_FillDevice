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
	FILL_TX_RS485, 
	FILL_TX_PROC,
	FILL_RX,
	FILL_RX_SG,
	FILL_RX_PROC,
	FILL_RX_PC,
	FILL_RX_RS232,
	FILL_RX_RS485,
	ZERO_FILL,
	HQ_TX,
	HQ_RX,
	TIME_TX,
	TIME_TX_PROC, 
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

char  use_rs232_mode;

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
			set_led_state(100, 0);		// Steady light
			break;

		case ZERO_FILL:
			set_led_state(5, 5);		// About to zero-out pattern
			break;

		case FILL_RX :
		case HQ_RX :
			set_led_state(15, 15);		// "Key empty" blink pattern
			break;

		case FILL_TX :
		case TIME_TX :
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

		case FILL_TX_RS485:
			set_led_state(80,20);		// "Try RS485" blink pattern
			break;
	
		case ERROR:
			set_led_state(15, 5);		// "Fill error" blink pattern
			break;

		case DONE:
			set_led_state(150, 150);	// "Done - key valid" blink pattern
			break;
	
		default:
			set_led_state(150, 5);		// "Key loading" blink pattern
			break;
	}
	current_state = nextState;
}


void main()
{
	char  result;
	setup_start_io();
  disable_tx_hqii();
	SetNextState(INIT);

	// Initialize current state of the buttons, switches, etc
	prev_power_pos = get_power_state();
	prev_button_pos = get_button_state();
	prev_switch_pos = get_switch_state();
	// Use RS232 mode to receive ONLY when switch was in S16 position during power-up
  use_rs232_mode = (prev_switch_pos == PC_POS) ? 0xFF : 0x00;
    
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
        remove_gnd_pin_b(); // Remove ground from pin B
				disable_tx_hqii();
				close_eusart();
     		idle_counter = seconds_counter + IDLE_SECS;

				// Switch is in one of the VALID key fill positions
				if( (switch_pos > 0) && (switch_pos <= MAX_NUM_POS))
				{
					// First check the position of the ZERO switch 
					if( power_pos == ZERO_POS)
					{
						SetNextState(ZERO_FILL);
					}else	// Not zero position - one of the S1-S10 positions
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
  				make_gnd_pin_b();
					if(is_bootloader_active())
					{
   				  set_led_state(0, 100);	// Turn off LED
    				INTCONbits.GIE = 0;		// Disable interrupts
    				INTCONbits.PEIE = 0;
   					BootloadMode();
  			  }		
					SetNextState(PC_CONN);
				}
				else if(power_pos == ZERO_POS)		// GPS/HQ time receive
				{
          make_gnd_pin_b();
					SetNextState(HQ_RX);
				}else if(switch_pos == HQ_TIME_POS)	// HQ tmt
				{
          make_gnd_pin_b();
          enable_tx_hqii();
					SetNextState(HQ_TX);
				}else if(switch_pos == SG_TIME_POS)
				{
  				remove_gnd_pin_b();
					fill_type = MODE3;
					SetNextState(TIME_TX);
				}
				break;
				
			case FILL_TX_RS232:
				result = SendRS232Fill(switch_pos);
				// On the timeout - switch to a alternative mode
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
				// On the timeout - switch to a alternative mode
				if(result < 0)
				{
					SetNextState(FILL_TX_RS232);	
				}else
				{
					TestFillResult(result);
				}
				break;
					
			case FILL_TX:
				if(fill_type == MODE5)
				{
					SetNextState(FILL_TX_RS232);	// Start with RS232
				}else if( CheckEquipment() > 0 )
				{
					SetNextState(FILL_TX_PROC);
				}
				break;

			case FILL_TX_PROC:
				// Check if the fill was initialed on the Rx device
				TestFillResult(WaitReqSendFill());
				// For Type1 fills we can simulate KOI18 and start
				//     sending data on button press....
				if( (fill_type == MODE1) && TestButtonPress() )
				{
					TestFillResult(SendFill());
				}
				break;

			case FILL_RX:
			  if(use_rs232_mode){
  				make_gnd_pin_b();
  				
  				result = CheckFillType4();
  				if(result > 0)
  				{
  					fill_type = result;
  					SetNextState(FILL_RX_PC);
  					break;
  				}
  				
  				result = CheckFillRS232Type5();
  				if(result > 0)
  				{
  					fill_type = result;
  					SetNextState(FILL_RX_RS232);
  					break;
  				}
  
  				result = CheckFillRS485Type5();
  				if(result > 0)
  				{
  					fill_type = result;
  					SetNextState(FILL_RX_RS485);
  					break;
  				}
				}else{
  				remove_gnd_pin_b();
  				result = CheckFillType23();
  				if(result > 0){
  					// Process Type 1, 2, and 3 fills
  					fill_type = result;
  					SetNextState(FILL_RX_SG);
  					break;
  				}
  				if( TestButtonPress() ){
  					fill_type = MODE1;
  					SetNextState(FILL_RX_PROC);
  				}
  		  }		
				break;

			case FILL_RX_SG:
				if( TestButtonPress() )
				{
					SetNextState(FILL_RX_PROC);
				}
				break;

			case FILL_RX_PC:
				TestFillResult(GetStoreFill( (MODE4<<4) | switch_pos));
				break;

			case FILL_RX_PROC:
				TestFillResult(GetStoreFill(switch_pos));
				break;

			case FILL_RX_RS232:
				result = GetRS232Fill(switch_pos);
				if(result < 0)
				{
					SetNextState(FILL_RX);
				}else
				{
					TestFillResult(result);
				}
				break;

			case FILL_RX_RS485:
				result = GetRS485Fill(switch_pos);
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

			case TIME_TX:
				if( CheckEquipment() > 0 )
				{
					SetNextState(TIME_TX_PROC);
				}
				break;

			case TIME_TX_PROC:
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
 				remove_gnd_pin_b();
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