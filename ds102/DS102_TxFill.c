#include "config.h"
#include "crcmodel.h"
#include "delay.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"
#include "controls.h"

//--------------------------------------------------------------
// Delays for the appropriate timings in millisecs
//--------------------------------------------------------------
#define tM 		50	  	// D LOW -> F LOW 	(-5us - 100ms)
#define tA  	50	  	// F LOW -> D HIGH	(45us - 55us)
#define tE  	10   	// REQ -> Fill		(0 - 2.3 sec)
#define tZ  	500   	// End -> New Fill	
//--------------------------------------------------------------
// Delays for the appropriate timings in usecs
//--------------------------------------------------------------
#define tJ  	25		// D HIGH -> First data bit on B
#define tK1  	425 	// First Data bit on B -> E (CLK) LOW
#define tK2  	425		// Last E (CLK) LOW -> TRISTATE E
#define tL  	50		// C (REQ) LOW -> F (MUX) HIGH for the last bit

//--------------------------------------------------------------
// Timeouts in ms
//--------------------------------------------------------------
#define tB  1000    	// Query -> Response from Radio (0.8ms - 5ms)
#define tD  1000     	// PIN_C Pulse Width (0.25ms - 75ms)
#define tG  1000     	// PIN_B Pulse Wodth (0.25ms - 80ms)
#define tH  1000     	// BAD HIGH - > REQ LOW (0.25ms - 80ms)
#define tF  4000    	// End of fill - > response (4ms - 2sec)
#define tC  (30000)  	// .5 minute - > until REQ (300us - 5min )

// Sends byte data on PIN_B with clocking on PIN_E
static void SendMode23Query(byte Data)
{
  byte i;
  // Set up pins mode and levels
  pinMode(PIN_B, OUTPUT);    // make pin active
  pinMode(PIN_E, OUTPUT);    // make pin active
  digitalWrite(PIN_E, HIGH);  // Set clock to High
  delayMicroseconds(tJ);      // Setup time for clock high

  // Output the first data bit of the request
  digitalWrite(PIN_B, (Data & 0x01) ? LOW : HIGH);  // Output data bit
  Data >>= 1;
  delayMicroseconds(tK1);    // Satisfy Setup time tK1

    // Pulse the clock
  digitalWrite(PIN_E, LOW);  // Drop Clock to LOW 
  delayMicroseconds(tT);     // Hold Clock in LOW for tT
  digitalWrite(PIN_E, HIGH);  // Bring Clock to HIGH

  // Output the rest of the data
  for(i = 0; i < 7; i++)
  {
    // Send next data bit    
    digitalWrite(PIN_B, (Data & 0x01) ? LOW : HIGH);  // Output data bit
    Data >>= 1;
    delayMicroseconds(tT);     // Hold Clock in HIGH for tT (setup time)

    digitalWrite(PIN_E, LOW);  // Drop Clock to LOW 
    delayMicroseconds(tT);     // Hold Clock in LOW for tT
    digitalWrite(PIN_E, HIGH);  // Bring Clock to HIGH
  }
  	delayMicroseconds(tK2 - tT);  // Wait there
  	// Release PIN_B and PIN_E - the Radio will drive it
  	digitalWrite(PIN_B, HIGH);  // Turn on 20 K Pullup
  	digitalWrite(PIN_E, LOW);  // Turn on 20 K Pullup
	pinMode(PIN_B, INPUT);    // Tristate the pin
	pinMode(PIN_E, INPUT);    // Tristate the pin
  	WPUB_PIN_B = 1;
  	WPUB_PIN_E = 0;
}

// Receive the equipment data that is sent on PIN_B with clocking on PIN_E
//   The total number of clocks is 40 (41), but only the last bit matters
static char GetEquipmentMode23Type(void)
{
  byte i;
  byte  NewState;	
  byte  PreviousState;
  char 	prev;
  char	result;

  digitalWrite(PIN_B, HIGH);  		// Turn on 20 K Pullup
  digitalWrite(PIN_E, LOW);      	// Turn_on the pullup register
  pinMode(PIN_B, INPUT);    		// Tristate the pin
  pinMode(PIN_E, INPUT);			// Tristate the pin
  WPUB_PIN_B = 1;
  WPUB_PIN_E = 0;
  
  set_timeout(tB);
//  prev = INTCONbits.GIE;
//  INTCONbits.GIE = 0;

  i = 0;
  result = ST_TIMEOUT;
  PreviousState = LOW;
  while(is_not_timeout())
  {
    NewState = pin_E();
	// Find the state change
    if( PreviousState != NewState  )
    {
      if( NewState == LOW )
      {
        if( ++i >= 40)
        {
          	result = (pin_B() == LOW) ? MODE2 : MODE3;
			break;
        }
      }
      PreviousState = NewState;
    }
  }
//  INTCONbits.GIE = prev;
  return result;
}

// Sends byte data on PIN_D with clocking on PIN_E
static void SendDS102Byte(byte Data)
{
  byte i;
  pinMode(PIN_D, OUTPUT);    // make a pin active
  pinMode(PIN_E, OUTPUT);    // make a pin active
  digitalWrite(PIN_E, HIGH);  // Bring Clock to HIGH
  for(i = 0; i < 8; i++)
  {
    digitalWrite(PIN_D, (Data & 0x01) ? LOW : HIGH);  // Output data bit
    Data >>= 1;
    delayMicroseconds(tT);    // Satisfy Setup time tT
    digitalWrite(PIN_E, LOW);  // Drop Clock to LOW 
    delayMicroseconds(tT);     // Hold Clock in LOW for tT
    digitalWrite(PIN_E, HIGH);  // Bring Clock to HIGH
  }
}

// Send Cell - a collection of bytes
static void SendDS102Cell(byte *p_cell, byte count)
{
  byte i;
  delay(tE);     // Delay before sending first bit
  for (i = 0; i < count; i++)
  {
    SendDS102Byte(*p_cell++);
  }
}


// Wait for the Fill request on PinC
// Returns 	
//      -1 	- Timeout
//		0 	- Request received
//		1 	- PinB was asserted - fill error
static char WaitDS102Req(byte fill_type, byte req_type)
{
	byte  NewState_C;	
	byte  NewState_B;	
  	byte  PreviousState_C;

  	char   Result = ST_TIMEOUT;

  	digitalWrite(PIN_C, HIGH);  // Set pullup 
  	pinMode(PIN_C, INPUT); 
	// For MODE23 fill we:
	//  1. Keep pin B high with pullup
	//  2. read PIN_B 
	if(fill_type != MODE1) {
	  	digitalWrite(PIN_B, HIGH);  // Set pullup 
    	pinMode(PIN_B, INPUT);	
    	WPUB_PIN_B = 1;
	}else {
	  	digitalWrite(PIN_B, LOW);  // Keep pin_B LOW
    	pinMode(PIN_B, OUTPUT);	
    	WPUB_PIN_B = 0;
	}

	delayMicroseconds(tK1);    // Satisfy Setup time tK1

  	if( req_type == REQ_FIRST){
	    set_timeout(tB);	// Return every 500 ms to check for switch position
  	}else{
      	set_timeout(tF);	// If not first - wait 3 seconds until timeout
  	}

  	PreviousState_C = HIGH;
  	while( is_not_timeout() )  
  	{
    	NewState_C = pin_C();
    	if(PreviousState_C != NewState_C)  
    	{
      		PreviousState_C = NewState_C;
      		// check if we should return now or wait for the PIN_C to go HIGH
      		// If this is the last request - return just as PinC goes LOW	
      		if( (NewState_C == HIGH) || (req_type == REQ_LAST) ) 
      		{
        		Result = (Result != ST_ERR) ? ST_OK : ST_ERR;
        		break;
 	    	} 
    	}

	  	// Do not check for pin B in MODE1 fill or on REQ_FIRST
    	if( (req_type != REQ_FIRST) && (fill_type != MODE1) ) 
    	{
    		NewState_B = pin_B();
	    	if(NewState_B == LOW) {
      			Result = ST_ERR;  // Bad CRC
      		}			
    	}
  	}
  	// Timeout occured - must return -1, except some special cases
  	// For Type 1 Fill the first request may be just a long pull of PIN_C
  	// For Type 1 there may be no ACK/REQ byte - treat timeout as OK
  	if( (fill_type == MODE1) && (NewState_C == LOW) )
  	{
		Result = ST_OK;
  	}
  	return Result;
}


static void StartMode23Handshake(void)
{
  pinMode(PIN_B, INPUT);
  pinMode(PIN_C, INPUT);
  pinMode(PIN_D, OUTPUT);    // make pin output
  pinMode(PIN_E, OUTPUT);  
  pinMode(PIN_F, OUTPUT);    // make pin output
  digitalWrite(PIN_B, HIGH);
  digitalWrite(PIN_C, HIGH);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, LOW);
  digitalWrite(PIN_F, HIGH);
  WPUB_PIN_B = 1;
  delay(200);

  // Drop PIN_D first
  digitalWrite(PIN_D, LOW);
  //delay(tM);
  digitalWrite(PIN_F, LOW);   // Drop PIN_F after delay
  delay(tA);                  // Pin D pulse width
  digitalWrite(PIN_D, HIGH);  // Bring PIN_D up again
}

static void EndMode23Handshake(void)
{
  digitalWrite(PIN_B, HIGH);
  digitalWrite(PIN_C, HIGH);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, LOW);
  pinMode(PIN_B, INPUT);
  pinMode(PIN_C, INPUT);
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  WPUB_PIN_B = 1;
}

static void  EndFill(void)
{
  delayMicroseconds(tL);
  digitalWrite(PIN_F, HIGH);
  delay(tZ);
}


static void AcquireMode23Bus(void)
{
  digitalWrite(PIN_B, HIGH);
  digitalWrite(PIN_C, HIGH);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, LOW);
  digitalWrite(PIN_F, HIGH);
  pinMode(PIN_B, INPUT);
  pinMode(PIN_C, INPUT);
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  pinMode(PIN_F, OUTPUT);
  WPUB_PIN_B = 1;
  delayMicroseconds(tJ);
}

static void AcquireMode1Bus(void)
{
  digitalWrite(PIN_B, LOW);
  digitalWrite(PIN_C, HIGH);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, LOW);
  digitalWrite(PIN_F, HIGH);
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_C, INPUT);
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  pinMode(PIN_F, OUTPUT);
  delayMicroseconds(tJ);
}


static void ReleaseBus(void)
{
  pinMode(PIN_B, INPUT);
  pinMode(PIN_C, INPUT);
  pinMode(PIN_D, INPUT);
  pinMode(PIN_E, INPUT);
  pinMode(PIN_F, INPUT);
  delay(tB);
  digitalWrite(PIN_B, HIGH);
  digitalWrite(PIN_C, HIGH);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, HIGH);
  digitalWrite(PIN_F, HIGH);
}



// Tag values
#define NO_FILL 0x7D
// The cell is filled with NO_FILL tags
byte nofill_cell[] =
{
  NO_FILL, 
  NO_FILL, NO_FILL, NO_FILL, NO_FILL, NO_FILL, NO_FILL, NO_FILL, 
  NO_FILL, NO_FILL, NO_FILL, NO_FILL, NO_FILL, NO_FILL, NO_FILL, 
  0x77  
};

// Check the equipment type
// If the Type2/3 equipment is detected, then return MODE2 or MODE3
// For Type 1 - return MODE1 right away
char CheckType123Equipment(byte fill_type)
{
  	char Equipment = NONE;
  	if((fill_type == MODE1))
  	{
	  	AcquireMode1Bus();
	  	Equipment = MODE1;
  	}else		// MODE 2 or MODE 3
  	{
	  	AcquireMode23Bus();
		StartMode23Handshake();
  		SendMode23Query(MODE3);
	  	Equipment = GetEquipmentMode23Type();
   		EndMode23Handshake();
   		if(Equipment < 0)	// Timeout occured
	  	{
		  	ReleaseBus();
	  	}
  	}
  	return Equipment;
}

// Send only TOD fill info for Type 3 SINCGARS
char WaitReqSendTODFill()
{
  	byte num_retries;
  	char pos;	// Cell position (1..MAX_TYPE_3_POS)
	char wait_result = ST_TIMEOUT;

  	// On timeout return and check the switches
  	if( WaitDS102Req(MODE3, REQ_FIRST)  != ST_OK ) return ST_TIMEOUT;  
  
	for(pos = 1 ; pos <= NUM_TYPE3_CELLS; pos++)
	{
		num_retries = 0;
		while(1)		// Loop on retries
		{
			set_led_off();
			if(pos == TOD_CELL_NUMBER)	// Cell 13 is the TOD cell position
			{
				FillTODData();
				cm_append(TOD_cell, MODE2_3_CELL_SIZE);
				SendDS102Cell(TOD_cell, MODE2_3_CELL_SIZE);
			}else
			{
				cm_append(nofill_cell, MODE2_3_CELL_SIZE);
				SendDS102Cell(nofill_cell, MODE2_3_CELL_SIZE);
			}
			set_led_on();
			wait_result = WaitDS102Req(MODE3, (pos == NUM_TYPE3_CELLS) ? REQ_LAST : REQ_NEXT);
			if( (wait_result != ST_ERR) || (num_retries >= TYPE23_RETRIES)) break;
	 	  	num_retries++;
		}
   		// If all records were sent - ignore timeout
  		if(pos == NUM_TYPE3_CELLS)
		{
			wait_result = ST_DONE;
			break;
		}
   		if(wait_result != ST_OK) break;
	 }

  	EndFill();
  	ReleaseBus();
  	return wait_result;
}

char SendDS102Fill(byte stored_slot)
{
  	byte    num_retries;
  	byte  	fill_type, records;
	byte    bytes, rec_bytes, byte_cnt;
	unsigned short long base_address;
	unsigned short long rec_base_address;
	
	char    wait_result = ST_TIMEOUT;
	
  	base_address = get_eeprom_address(stored_slot & 0x0F);
	records = byte_read(base_address++);
	if(records == 0xFF) records = 0x00;
	// Get the fill type from the EEPROM
	fill_type = byte_read(base_address++);

	while(records)	
	{
 		rec_bytes = byte_read(base_address++);
 		rec_base_address = base_address;
		num_retries = 0;
	   	while(1)	// Loop on retries
	   	{
	    	bytes = rec_bytes;
	     	base_address = rec_base_address;
	 		while( bytes )	// Loop until all bytes from the record are sent
	 		{
				set_led_off();
	 			byte_cnt = MIN(bytes, FILL_MAX_SIZE);
	 			array_read(base_address, &data_cell[0], byte_cnt);
	
	 			// Check if the cell that we are about to send is the 
	 			// TOD cell - replace it with the real Time cell
	 			if( (data_cell[0] == TOD_TAG_0) && (data_cell[1] == TOD_TAG_1) && 
	 						(fill_type == MODE3) && (byte_cnt == MODE2_3_CELL_SIZE) )
	 			{
	 				FillTODData();
	 				cm_append(TOD_cell, MODE2_3_CELL_SIZE);
	 	  			SendDS102Cell(TOD_cell, byte_cnt);
	 			}else
	 			{
	 				SendDS102Cell(&data_cell[0], byte_cnt);
	 			}
				set_led_on();
	 			// Adjust counters and pointers
	 			base_address += byte_cnt;
	 			bytes -= byte_cnt;
			}
	 		// After sending a record check for the next request
			if( fill_type == MODE1) break;  // No retries for Type 1 fills
	 		wait_result = WaitDS102Req(fill_type, (records > 1) ? REQ_NEXT : REQ_LAST );
			if( wait_result == ST_OK) break;
			if( num_retries >= TYPE23_RETRIES) break;
			num_retries++;
		}
		records--;
	   	// If all records were sent - ignore timeout
	  	if(records == 0){
			wait_result = ST_DONE;
			break;
		}
	   	if(wait_result != ST_OK) break;
	}	
  	EndFill();
   	ReleaseBus();
	return wait_result;	
}

char WaitReqSendDS102Fill(byte stored_slot, byte fill_type)
{
	// If first fill request was not answered - just return with timeout
	// We will be called again after switches are checked
	if( WaitDS102Req(fill_type, REQ_FIRST) != ST_OK ) return ST_TIMEOUT;

	return SendDS102Fill(stored_slot);
}
