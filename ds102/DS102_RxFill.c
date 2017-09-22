#include "config.h"
#include "crcmodel.h"
#include "delay.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"
#include "clock.h"
#include "DS101.h"
#include "controls.h"

//--------------------------------------------------------------
// Delays in ms
//--------------------------------------------------------------
#define tB  	2      // Query -> Response from Radio (0.8ms - 5ms)
#define tD  	50     // PIN_C Pulse Width (0.25ms - 75ms)
#define tG  	50     // PIN_B Pulse Wodth (0.25ms - 80ms)
#define tH  	50     // BAD HIGH - > REQ LOW (0.25ms - 80ms)

//--------------------------------------------------------------
// Delays for the appropriate timings in usecs
//--------------------------------------------------------------
#define tJ  	25		// D HIGH -> First data bit on B
#define tK3  	425 	// First Data bit on B -> E (CLK) LOW
#define tK4  	425		// Last E (CLK) LOW -> TRISTATE E


//--------------------------------------------------------------
// Timeouts in ms
//--------------------------------------------------------------
#define tA  	100	   // F LOW -> D HIGH	(45ms - 55ms)
#define tE  	3000   // REQ -> Fill		(0 - 2.3 sec)
#define tZ  	2000   // Query cell duration
#define tF  	200    // End of fill - > response (4ms - 2sec)

static char GetQueryByte(void)
{
	byte  PreviousState, NewState;	
	byte  bit_count;
	byte  Data;
	byte  prev; 
	char  ret;
  
  	pinMode(PIN_B, INPUT);		
  	pinMode(PIN_E, INPUT);
	digitalWrite(PIN_B, HIGH);  // Turn on 20 K Pullup
  	digitalWrite(PIN_E, HIGH);  // Turn on 20 K Pullup
	WPUB_PIN_B = 1;
  	WPUB_PIN_E = 1;

	bit_count = 0;
	PreviousState = LOW;
	set_timeout(tZ);

	prev = INTCONbits.GIE;
	INTCONbits.GIE = 0;

	ret = ST_TIMEOUT;
  	// We exit on timeout or Pin F going high
	while( is_not_timeout() && (pin_F() == LOW) )
  	{
    	NewState = pin_E();
    	if( PreviousState != NewState  )
    	{
      		if( NewState == LOW )
      		{
		    	Data = (Data >> 1 ) | ( pin_B() ? 0 : 0x80);
        		bit_count++; 
    			if((bit_count >= 8) && ((Data & 0xFE) == 0x02) )
    			{
					ret = MODE3;
					break;
    			}
      		}
      		PreviousState = NewState;
    	}
  	}

	INTCONbits.GIE = prev;
  	return ret;
}

static void SendEquipmentType(void)
{
  byte i;
  // Set up pins mode and levels
  delay(tB);
  
  pinMode(PIN_B, OUTPUT);		// make a pin active
  pinMode(PIN_E, OUTPUT);		// make a pin active
  digitalWrite(PIN_E, HIGH);	// Set clock to High
  delayMicroseconds(tJ);		// Setup time for clock high

  // Output the data bit of the equipment code
  digitalWrite(PIN_B, HIGH);	// Output data bit - "0" (remember -6.5V logic)
  delayMicroseconds(tK3);		// Satisfy Setup time tK1

  // Output the data
  for(i = 0; i < 40; i++)
  {
	// Pulse the clock
    delayMicroseconds(tT);		// Hold Clock in HIGH for tT (setup time)
    digitalWrite(PIN_E, LOW);	// Drop Clock to LOW 
    delayMicroseconds(tT);		// Hold Clock in LOW for tT
    digitalWrite(PIN_E, HIGH);  // Bring Clock to HIGH
  }
  delayMicroseconds(tK4);  // Wait there
  
  // Release PIN_E - the Fill device will drive it
  pinMode(PIN_E, INPUT);		// Tristate the pin
  pinMode(PIN_B, INPUT);
  digitalWrite(PIN_B, HIGH);  // Turn on 20 K Pullup
  digitalWrite(PIN_E, HIGH);  // Turn on 20 K Pullup
  WPUB_PIN_B = 1;
  WPUB_PIN_E = 1;
}

static byte ReceiveDS102Cell(byte fill_type, byte *p_cell, byte count)
{
  byte  bit_count;
  byte  byte_count;
  byte  PreviousState, NewState;	
  byte  Data;
  byte  prev;

  pinMode(PIN_D, INPUT);		// make pin input DATA
  pinMode(PIN_E, INPUT);		// make pin input CLOCK
  pinMode(PIN_F, INPUT);		// make pin input MUX OVR
  digitalWrite(PIN_D, HIGH);  // Turn on 20 K Pullup
  digitalWrite(PIN_E, HIGH);  // Turn on 20 K Pullup
  digitalWrite(PIN_F, HIGH);  // Turn on 20 K Pullup
  WPUB_PIN_E = 1;
  

  byte_count = 0;
  bit_count = 0;
  PreviousState = pin_E();

  set_timeout(tE);

  prev = INTCONbits.GIE;
  INTCONbits.GIE = 0;

  while( is_not_timeout() &&  (byte_count < count) )
  {
		// Check for the last fill for Mode2 and 3
		if( (fill_type != MODE1) && (pin_F() == HIGH) )
		{
  			byte_count = 0;
			break;	// Fill device had deasserted PIN F - exit
		}

    	NewState = pin_E(); 
	    if( PreviousState != NewState  )
	    {
	      PreviousState = NewState;
	      if( NewState == LOW )	// Transition from HIGH to LOW
	      {
	  	    Data = (Data >> 1) | ( pin_D() ? 0x00 : 0x80);  // Add Input data bit
	        bit_count++; 
			if( bit_count >= 8)
			{
				*p_cell++ = Data;
				bit_count = 0;
				byte_count++;
    		  	set_timeout(tF);
			}
	      }
	    }
  }

  INTCONbits.GIE = prev;
  return byte_count;
}

static char SendFillRequest(void)
{
  pinMode(PIN_C, OUTPUT);		// make pin an output
  digitalWrite(PIN_C, LOW);
  delay(tD);
  digitalWrite(PIN_C, HIGH);
}

static char SendBadFillAck(void)
{
  pinMode(PIN_B, OUTPUT);		// make pin an output
  digitalWrite(PIN_B, LOW);
  delay(tG);
  digitalWrite(PIN_B, HIGH);
  delay(tH);
}


// Detect if there is Type 1 Fill device connected
// It is detected by LOW on PIN B
char CheckFillType1()
{
	return (pin_B() == LOW);
}	

typedef enum {
  DF_INIT = 0,  // Initial state
  DF_HIGH,      // D and F pins in high detected
  DF_LOW        // D and F pins in low detected
} CT23_STATES;

static CT23_STATES t23_state = DF_INIT;
// Detect if there is Type 2/3 PIN_D and PIN_F sequence
// Initially they must be HIGH, then PIN_D and PIN_F go LOW
// PIN_F stays that way, but PIN_D goes HIGH before tA expires
char CheckFillType23()
{
	char type;
	char ret_val = ST_TIMEOUT;

	// Setup pins
	pinMode(PIN_D, INPUT);		// make pin D input
	pinMode(PIN_F, INPUT);		// make pin F input
  	digitalWrite(PIN_D, HIGH);  // Turn on 20 K Pullup
  	digitalWrite(PIN_F, HIGH);  // Turn on 20 K Pullup

  switch(t23_state)
  {
    case DF_INIT:
      if((pin_D() == HIGH) && (pin_F() == HIGH))
      {
        t23_state = DF_HIGH;
      }
      break;

    case DF_HIGH:
      if(( pin_D() == LOW) && (pin_F() == LOW))
      {
        t23_state = DF_LOW;
      }
      break;

    case DF_LOW:
    	// Both PIN_D and PIN_F are LOW - wait for PIN_D to get HIGH
    	// Here we are waiting for some time when F and D are LOW, and then D comes back
	    // to HIGH no later than tA timeout
    	set_timeout(tA);
    	while( is_not_timeout() )
    	{
      	// Pin F went high - return back to normal
    		if( pin_F() == HIGH )
    		{
          		t23_state = DF_INIT;
          		break;
    		}
    		// Pin D went high before timeout expired - wait for query request from the fill device
    		if( pin_D() == HIGH)
    		{
    			type = 	GetQueryByte();
    			if( type > 0 )  
    			{
    				SendEquipmentType();
    				ret_val =  type;
    			}
          		t23_state = DF_INIT;
          		break;
    		}
    	}
      	break;
     
    default:
      	t23_state = DF_INIT;
      	break;
  	}
	return ret_val;
}

// PIN_F should stay LOW during Type23 Fill 
char CheckFillType23Connected()
{
	return (pin_F() == LOW);
}	

// PIN_B should stay LOW during Type1 Fill 
char CheckFillType1Connected()
{
	return (pin_B() == LOW);
}	

static byte GetDS102Fill(unsigned short long base_address, byte fill_type)
{
	byte records, byte_cnt, record_size;
	unsigned short long saved_base_address;
	byte num_tries;

	records = 0;
	record_size = 0;

	saved_base_address = base_address++;  // First byte is record size

  	while(1)
	{
    	// Do retries for Mode 2 and 3 only
    	num_tries = 0; 
    	while(1)
    	{
 			set_led_on();
     		SendFillRequest();	// REQ the data
		  	byte_cnt = ReceiveDS102Cell(fill_type, &data_cell[0], FILL_MAX_SIZE);

      		if( (byte_cnt == 0) || (fill_type == MODE1) ) break;   // No data or Mode 1 - no checks
		  	if( (byte_cnt == MODE2_3_CELL_SIZE) && 
		      cm_check(&data_cell[0], MODE2_3_CELL_SIZE)) break;  // Size is OK and CRC is OK
		  	if(num_tries >= TYPE23_RETRIES) return 0;  // Number of tries exceeded - return Error

      		// Try to get the data couple more times
      		SendBadFillAck();		        // Tell the sender that there was an error
		  	num_tries++;
		}
		
		// Based on the byte_cnt
		//  = 0  - no data received --> finish everything
		//  == FILL_MAX_SIZE --> record and continue collecting
		//  0 < byte_cnt < FILL_MAX_SIZE --> record and issue new request
		record_size += byte_cnt;
		if(record_size == 0)
		{
			break;	// No data provided on the first fill - exit
		}
		// Any data present - save it in EEPROM
		if(byte_cnt)
		{
			array_write(base_address, &data_cell[0], byte_cnt);
			base_address += byte_cnt;
		}
		// Block of data received - save size and request next
		if(byte_cnt < FILL_MAX_SIZE)
		{
			byte_write(saved_base_address, record_size);
			records++; 
			// Prepare for the next record
			record_size = 0;
			saved_base_address = base_address++;
		}
		set_led_off();
	}
	return records;
}


// Receive and store the fill data into the specified slot
// The slot has the following format:
char StoreDS102Fill(byte stored_slot, byte required_fill)
{
	char result = ST_ERR;
	byte records;
	unsigned short long base_address;
	unsigned short long saved_base_address;

	base_address = get_eeprom_address(stored_slot & 0x0F);
	saved_base_address = base_address;	
	base_address += 2;	// Skip the fill_type and records field
						// .. to be filled at the end
	// The first byte of the each slot has the number of the records (0 - 255)
	// The second byte - fill type
	// The first byte of the record has the number of bytes that should be sent out
	// so each record has no more than 255 bytes as well
	// Empty slot has first byte as 0x00
	records = GetDS102Fill(base_address, required_fill);
 	// All records were received - put final info into EEPROM
	// Mark the slot as valid slot containig data
	if( records > 0)
	{
		byte_write(saved_base_address, records);
		byte_write(saved_base_address + 1, required_fill);
		result = ST_DONE;
	}
  return result;
}

void SetType123PinsRx()
{
  pinMode(PIN_B, INPUT);
  pinMode(PIN_C, OUTPUT);
  pinMode(PIN_D, INPUT);
  pinMode(PIN_E, INPUT);
  pinMode(PIN_F, INPUT);

  digitalWrite(PIN_B, HIGH);
  digitalWrite(PIN_C, HIGH);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, HIGH);
  digitalWrite(PIN_F, HIGH);

  WPUB_PIN_B = 1;
  WPUB_PIN_E = 1;
  // Set up pins mode and levels
  delay(tB);
  
}
