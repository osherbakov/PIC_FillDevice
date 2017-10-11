#include "config.h"
#include "delay.h"
#include "controls.h"
#include "spi_eeprom.h"
#include "rtc.h"
#include "gps.h"
#include "serial.h"
#include "Fill.h"
#include "DS101.h"

static enum 
{
	INIT = 0,
	IDLE,
	BIST,
	BIST_ERR,
	FILL_TX,
	FILL_TX_DS102_WAIT,
	FILL_TX_MBITR,
	FILL_TX_RS232,
	FILL_TX_DTD232,
	FILL_TX_RS485, 
	FILL_TX_DS102,
	FILL_RX,
	FILL_RX_DS102_WAIT,
	FILL_RX_RS232_WAIT,
	FILL_RX_TYPE1,
	FILL_RX_TYPE23,
	FILL_RX_DS102,
	FILL_RX_PC,
	FILL_RX_RS232,
	FILL_RX_DTD232,
	FILL_RX_RS485,
	ZERO_FILL,
	HQ_TX,
	HQ_GPS_RX,
	FILL_TX_TIME,
	FILL_TX_TIME_PROC, 
	PC_CONN,
	ERROR,
	DONE,
	WAIT_BTN_PRESS
} MAIN_STATES;



#define IDLE_SECS (600)

static unsigned int idle_counter;

byte 	current_state;
byte 	button_pos;
byte 	prev_button_pos;

//
// Test if the button was depressed and released
//  Returns 1 if transition from LOW to HIGH was detected
static byte TestButtonPress(void)
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


// Set the blink pattern depending on the next state specified
static void SetNextState(char nextState)
{
	switch(nextState)
	{
		case INIT:
			set_led_state(100, 0);		// Steady Light
			break;

		case IDLE:
			set_led_state(0, 100);		// No Light
			break;
			
		case ZERO_FILL:
			set_led_state(5, 5);		// About to zero-out pattern (Fastest Blink)
			break;

		case FILL_RX :
		case HQ_GPS_RX :
		case FILL_RX_DS102_WAIT:
		case FILL_RX_RS232_WAIT:
			set_led_state(15, 15);		// "Key empty" blink pattern (Fast Blink)
			break;

		case FILL_TX_MBITR:
			set_led_state(5, 150);		// "Connect Serial" blink pattern	
			break;

		case FILL_TX :
		case FILL_TX_TIME :
		case FILL_TX_DS102_WAIT:
			set_led_state(50, 150);		// "Key valid" blink pattern
			break;

		case HQ_TX:
		case PC_CONN:
			set_led_state(5, 150);		// "Connect Serial" blink pattern
			break;

		case FILL_TX_RS232:
			set_led_state(20, 100);		// "Connect RS232" blink pattern
			break;
			
		case FILL_TX_DTD232:
			set_led_state(60, 60);		// "Try RS232" blink pattern
			break;

		case FILL_TX_RS485:
			set_led_state(100, 20);		// "Try RS485" blink pattern
			break;

		case FILL_RX_PC:
			set_led_state(200, 50);		// "Try RS232 PC" blink pattern
			break;

	    case FILL_RX_DS102:
		case FILL_TX_DS102:
	    case FILL_TX_TIME_PROC:
	    case FILL_RX_TYPE1:
	    case FILL_RX_TYPE23:
			set_led_state(150, 0);		// "Key loading" blink pattern (Steady light)
			break;
    
		case FILL_RX_RS232:
			set_led_state(200, 50);		// "Try RS232 PC" blink pattern
			break;

		case FILL_RX_DTD232:
			set_led_state(200, 50);		// "Try RS232" blink pattern
			break;

		case FILL_RX_RS485:
			set_led_state(80, 40);		// "Try RS485" blink pattern
			break;
	
		case ERROR:
			set_led_state(15, 5);		// "Fill error" blink pattern
			break;

		case DONE:
			set_led_state(100, 100);	// "Done - key valid" blink pattern
			break;
	
		default:
			break;
	}
	current_state = nextState;
}

// Test the resutlt of the fill and set next state.
//   if result is 0 - go back to INIT state
//		result = 1 - error happened - set error blink 
//				and wait for the button press in ERROR state
//		result = 2 - loaded sucessfully - set "Key Valid" blink 
//				and wait for the button press in DONE state
//		result = -1 - timeout, continue working
static void TestFillResult(char result)
{
	if(result == ST_OK)					// OK return value
	{
		SetNextState(INIT);
	}else if(result == ST_ERR)			// ERROR return value
	{
		SetNextState(ERROR);
	}else if(result == ST_DONE )		// DONE return value
	{
		SetNextState(DONE);
	}
	// Timeout - stay in the same state
}


static void  PinsToDefault(void)
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

static void bump_idle_counter(void)
{
	char prev = INTCONbits.GIE;
  	INTCONbits.GIE = 0; 
	idle_counter = seconds_counter + IDLE_SECS;
  	INTCONbits.GIE = prev;
}

void main()
{
	char  result;
  	byte  fill_type;
  	char  allow_type45_fill;
  
	setup_start_io();
  	PinsToDefault();	

	SetNextState(INIT);
	DelayMs(200);
	
	// Initialize current state of the buttons, switches, etc
	prev_power_pos = get_power_state();
	prev_button_pos = get_button_state();
	prev_switch_pos = get_switch_state();

  	allow_type45_fill = (prev_button_pos == DOWN_POS) ? TRUE : FALSE;
	
  	bump_idle_counter();
  
	while(1)
	{
		//
		// If no activity was detected for more than 6 minutes - shut down
		//
		if(idle_counter < seconds_counter)
		{
			SetNextState(IDLE);
	  		setup_sleep_io();
			while(1) 
			{
				INTCONbits.GIE = 0;		// Disable interrupts
				INTCONbits.PEIE = 0;
				Sleep();
			};
		}

	  	//
	  	// Check the switch position - did it change?
	  	//
		switch_pos = get_switch_state();
		if(switch_pos && (switch_pos != prev_switch_pos))
		{
      		// On any change bump the idle counter
      		bump_idle_counter();
			prev_switch_pos = switch_pos; // Save new state
			SetNextState(INIT);
		}

	  	//
	  	// Check the power position - did it change?
	  	//
		power_pos = get_power_state();
		if( power_pos != prev_power_pos )
		{
      		// On any change bump the idle counter
      		bump_idle_counter();
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
			// Also after ending any operation
			//-----------------------------------------------------
			//********************************************
			//-----------INIT-----------------
			case INIT:
        		PinsToDefault();
        		bump_idle_counter();
        
        		// Start all from the beginning.................
				// Switch is in one of the key fill positions
				if( (switch_pos > 0) && (switch_pos <= MAX_NUM_POS))
				{
					// First check the position of the ZERO switch 
					if( power_pos == ZERO_POS)
					{
						SetNextState(ZERO_FILL);
					}else	// Not Zeroizing - we are in a regular mode
					{
						// Type = 0 - empty slot
						//		= 1 - Type 1 fill
						//		= 2,3 - Type 2,3 fill
						//		= 4 - DES MBITR Key fill
						//		= 5 - DS-101 fill (both RS232, DTD and RS485)
						fill_type = CheckFillType(switch_pos);
						if( fill_type != 0)
						{
							SetNextState(FILL_TX);	// Be ready to send fill
						}else
						{
							SetNextState(FILL_RX) ; // Be ready to receive fill
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
   				  		BootloadMode();       // Go to bootloader
				  	}else
				  	{
					  	SetNextState(PC_CONN);// Switch to the PC connection mode
					}
				}else if(
					((switch_pos == HQ_TIME_POS) || (switch_pos == SG_TIME_POS)) 
						&& (power_pos == ZERO_POS))		// GPS/HQ time receive
				{
					set_pin_a_as_gnd();
					set_pin_f_as_power();
          			SetNextState(HQ_GPS_RX);
				}else if(switch_pos == HQ_TIME_POS)	// HQ Transmit
				{
					set_pin_a_as_gnd();			// Make ground on Pin A
				  	set_pin_f_as_power();
					SetNextState(HQ_TX);
				}else if(switch_pos == SG_TIME_POS) // SINCGARS Time only fill
				{
          			// SINCGARS TIME only fill - will use negative logic 
          			set_pin_a_as_power();
				  	set_pin_f_as_io();
					fill_type = MODE3;
					SetNextState(FILL_TX_TIME);
				}
				break;
			//-----------INIT-----------------
			//********************************************
	
			//********************************************
			//-----------FILL_TX--------------	
			case FILL_TX:
				if(fill_type == MODE5)          	// Any DS-101 fill
				{
					set_pin_a_as_gnd();				//  Set GND on Pin A
          			set_pin_f_as_power();
					SetNextState(FILL_TX_RS485);	// Start with RS232 and cycle thru 3 modes
				}else if(fill_type == MODE4)    	// MBITR keys
				{
					set_pin_a_as_gnd();				//  Set GND on Pin A
          			set_pin_f_as_power();
					SetNextState(FILL_TX_MBITR);
				}else 
				{       							// Any type 1,2,3 fill - DS-102
					set_pin_a_as_power();			//  Set +5V on Pin A for Type 1,2,3
          			set_pin_f_as_io();
					SetNextState(FILL_TX_DS102_WAIT);
				}
				break;

			//------------------------------------------------------------
      		// DS-102 Fills	- type 1,2, and 3
			//------------------------------------------------------------
			case FILL_TX_DS102_WAIT:
				result = CheckType123Equipment(fill_type);
				if( (result != ST_TIMEOUT) && (result != NONE) )
				{
				  SetNextState(FILL_TX_DS102);
				}
				break;

			case FILL_TX_DS102:
				// Check if the fill was initialed on the Rx device
				TestFillResult(WaitReqSendDS102Fill(switch_pos, fill_type));
				// For Type1 fills we can simulate KOI18 and start
				//     sending data on button press....
				if( (fill_type == MODE1) && TestButtonPress() )
				{
					TestFillResult(SendDS102Fill(switch_pos));
				}
				break;

			//------------------------------------------------------------
      		// DS-101 Type 5 Fills				
			//------------------------------------------------------------
			case FILL_TX_RS232:
				result = SendRS232Fill(switch_pos);
				// On the timeout - switch to next mode
				if( (result == ST_TIMEOUT) || (result == NONE) ) 
				{
					SetNextState(FILL_TX_DTD232);	
				}else{
					TestFillResult(result);
				}
				break;	

			case FILL_TX_DTD232:
				result = SendDTD232Fill(switch_pos);
				// On the timeout - switch to next mode
				if( (result == ST_TIMEOUT) || (result == NONE) ) 
				{
					SetNextState(FILL_TX_RS485);	
				}else{
					TestFillResult(result);
				}
				break;	

			case FILL_TX_RS485:
				result = SendRS485Fill(switch_pos);
				// On the timeout - switch to next mode
				if( (result == ST_TIMEOUT) || (result == NONE) ) 
				{
					SetNextState(FILL_TX_RS232);	
				}else{
					TestFillResult(result);
				}
				break;

			//------------------------------------------------------------
      		// MBITR Type 4 fill
			//------------------------------------------------------------
			case FILL_TX_MBITR:
 				TestFillResult(WaitReqSendMBITRFill(switch_pos));
				break;

			//------------------------------------------------------------
      		// SINCGARS time fill
			//------------------------------------------------------------
			case FILL_TX_TIME:
				result = CheckType123Equipment(fill_type);
				if( (result != ST_TIMEOUT) && (result != NONE) )
				{
					SetNextState(FILL_TX_TIME_PROC);
				}
				break;

			case FILL_TX_TIME_PROC:
				TestFillResult(WaitReqSendTODFill());
				break;
			//-----------FILL_TX--------------	
			//********************************************
			//
			//********************************************
			//-----------FILL_RX--------------	
			case FILL_RX:
        		PinsToDefault();
			  	if( !allow_type45_fill ){ 
				  	// Only Type 1, 2 and 3 fills are allowed in DS-102 mode
          			set_pin_a_as_power();         // Set +5V on Pin A
          			set_pin_f_as_io();
  			  		SetType123PinsRx();
 					SetNextState(FILL_RX_DS102_WAIT);
        		}else{ 
        			// Only RS-232 and RS-485 fills are allowed 
					set_pin_a_as_gnd();						//  Set GND on Pin A
          			set_pin_f_as_power();
  					SetNextState(FILL_RX_RS232_WAIT);
				}
				break;
      
      		// Wait for Type 1,2,3 DS-102 Fills
      		// Check for each fill type in turn
			case FILL_RX_DS102_WAIT:
			  	result = CheckFillType23();
				if( (result != ST_TIMEOUT) && (result != NONE) )
			  	{
					// Process Type 2, and 3 fills
					fill_type = result;
					SetNextState(FILL_RX_TYPE23);
					break;
				}
			  	result = CheckFillType1();
				if( (result != ST_TIMEOUT) && (result != NONE) )
			  	{
					// Process Type 1 fills
					fill_type = MODE1;
					SetNextState(FILL_RX_TYPE1);
					break;
				}
				if( TestButtonPress() )
				{
					fill_type = MODE1;
					SetNextState(FILL_RX_DS102);
				}
				break;

			case FILL_RX_TYPE1:
			  	if ( !CheckFillType1Connected()) 
				{
					SetNextState(INIT);
					break;
			    }
				if( TestButtonPress() )
				{
					SetNextState(FILL_RX_DS102);
				}
				break;

			case FILL_RX_TYPE23:
			  	if ( !CheckFillType23Connected()) 
				{
					SetNextState(INIT);
					break;
			    }
				if( TestButtonPress() )
				{
					SetNextState(FILL_RX_DS102);
				}
				break;


			case FILL_RX_DS102: // Do actual DS-102 fill
				TestFillResult(StoreDS102Fill(switch_pos, fill_type));
				break;

			//
      		// Wait for serial RS-232 or DS-101 fills				
			//
      		case FILL_RX_RS232_WAIT:
      			// Check if is is a DES keys fill from the PC (Type 4)
 				result = CheckFillType4();
				if( (result != ST_TIMEOUT) && (result != NONE) )
				{
					fill_type = result;
					SetNextState(FILL_RX_PC);
				  	break;
				}

        		// If Pin_D is -5V - that is Type 4 or RS-232 Type 5
 				result = CheckFillRS232Type5();
				if( (result != ST_TIMEOUT) && (result != NONE) )
				{
					fill_type = result;
					SetNextState(FILL_RX_RS232);
				  	break;
				}

        		// If Pin_C is -5V - that is DTD-232 Type 5
//				result = CheckFillDTD232Type5();
//				if( (result != ST_TIMEOUT) && (result != NONE) )
//				{
//					fill_type = result;
//					SetNextState(FILL_RX_DTD232);
//			    	break;
//				}
				
/********************************************************
				result = CheckFillRS485Type5();
				if( (result != ST_TIMEOUT) && (result != NONE) )
				{
					fill_type = result;
					SetNextState(FILL_RX_RS485);
			    break;
				}
*********************************************************/				
        		break;
     
			case FILL_RX_PC:  
				TestFillResult(StorePCFill( switch_pos, MODE4));
				break;

			case FILL_RX_RS232:
				TestFillResult(StoreRS232Fill(switch_pos, MODE5));
				break;

			case FILL_RX_DTD232:
				TestFillResult(StoreDTD232Fill(switch_pos, MODE5));
				break;

			case FILL_RX_RS485:
				TestFillResult(StoreRS485Fill(switch_pos, MODE5));
				break;
			//-----------FILL_RX--------------	
			//********************************************
				
			//********************************************
			//-----------ZERO_FILL--------------	
			case ZERO_FILL:
				if( TestButtonPress() )
				{
					ClearFill(switch_pos);
					SetNextState(INIT);
				}
				break;
			//-----------ZERO_FILL--------------	
			//********************************************

			//********************************************
			//-----------PC_CONN--------------	
			case PC_CONN:
				PCInterface();
				break;
			//-----------PC_CONN--------------	
			//********************************************

			//********************************************
			//-----------HQII TX and RX--------------	
			case HQ_TX:
				enable_tx_hqii();				// Enable HQ output
				break;

			case HQ_GPS_RX:
				// State may change after call to the ReceiveGPSTime() and ReceiveHQTime()
				if( current_state == HQ_GPS_RX)
				{
					TestFillResult(ReceiveGPSTime());
				}
				if( current_state == HQ_GPS_RX)
				{
					TestFillResult(ReceiveHQTime());
				}
				break;
			//-----------HQII TX and RX--------------	
			//********************************************

			//********************************************
			//-----------DONE and ERROR--------------	
			case ERROR:
			case DONE:
		    	PinsToDefault();
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
