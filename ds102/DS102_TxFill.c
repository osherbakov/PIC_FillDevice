#include "config.h"
#include "crcmodel.h"
#include "delay.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"
#include "controls.h"


static void AcquireMode1Bus(void);
static void AcquireMode23Bus(void);
static void ReleaseMode23Bus(void);

static char WaitDS102Req(byte req_type);

static char GetEquipmentMode23Type(void);

static void StartMode23Handshake(void);
static void EndMode23Handshake(void);

static void SendMode23Query(byte Data);

//--------------------------------------------------------------
// Delays for the appropriate timings in millisecs
//--------------------------------------------------------------
#define tM 		10	  // D LOW -> F LOW 	(-5us - 100ms)
#define tA  	50	  // F LOW -> D HIGH	(45us - 55us)
#define tE  	100   // REQ -> Fill		(0 - 2.3 sec)

#define tZ  	300   // End -> New Fill	
//--------------------------------------------------------------
// Delays for the appropriate timings in usecs
//--------------------------------------------------------------
#define tJ  	25		// D HIGH -> First data bit on B
#define tK1  	200 	// First Data bit on B -> E (CLK) LOW
#define tK2  	200		// Last E (CLK) LOW -> TRISTATE E
#define tL  	50		// C (REQ) LOW -> F (MUX) HIGH for the last bit

//--------------------------------------------------------------
// Timeouts in ms
//--------------------------------------------------------------
#define tB  1000    // Query -> Response from Radio (0.8ms - 5ms)
#define tD  500     // PIN_C Pulse Width (0.25ms - 75ms)
#define tG  500     // PIN_B Pulse Wodth (0.25ms - 80ms)
#define tH  500     // BAD HIGH - > REQ LOW (0.25ms - 80ms)
#define tF  3000    // End of fill - > response (4ms - 2sec)
#define tC  (30000)  // .5 minute - > until REQ (300us - 5min )

static byte  PreviousState;

// Sends byte data on PIN_B with clocking on PIN_E
void SendMode23Query(byte Data)
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
  pinMode(PIN_B, INPUT);    // Tristate the pin
  pinMode(PIN_E, INPUT);    // Tristate the pin
}


// Sends byte data on PIN_D with clocking on PIN_E
void SendDS102Byte(byte Data)
{
  byte i;
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
void SendDS102Cell(byte *p_cell, byte count)
{
  byte i;
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, HIGH);
  delay(tE);     // Delay before sending first bit
  for (i = 0; i < count; i++)
  {
    SendDS102Byte(*p_cell++);
  }
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, HIGH);
}

// Receive the equipment data that is sent on PIN_B with clocking on PIN_E
//   The total number of clocks is 40 (41), but only the last bit matters
char GetEquipmentMode23Type()
{
  byte i;
  byte  NewState;	

  i = 0;
  pinMode(PIN_B, INPUT);
  pinMode(PIN_C, INPUT); 
  pinMode(PIN_E, INPUT);
  
  WPUB_PIN_B = 1;
  WPUB_PIN_E = 1;
  
  PreviousState = LOW;
  set_timeout(tB);
  while(is_not_timeout())
  {
    NewState = digitalRead(PIN_E);
	// Find the state change
    if( PreviousState != NewState  )
    {
      if( NewState == LOW )
      {
        i++;
        if( i >= 40)
        {
          return (digitalRead(PIN_B) == LOW) ? MODE2 : MODE3;
        }
      }
      PreviousState = NewState;
    }
  }
  return -1;
}

// Wait for the Fill request on PinC
// Returns 	-1 	- timeout
//			0 	- Request received
//			1 	- PinB was asserted - error fill
char WaitDS102Req(byte req_type)
{
  byte  NewState;	
  char   Result = 0;

	// For MODE1 fill we keep PIN_B low and don't read it
  if(fill_type != MODE1)
  {
  	pinMode(PIN_B, INPUT);
	  WPUB_PIN_B = 1;
  }
  pinMode(PIN_C, INPUT); 

  if( req_type == REQ_FIRST)
  {
	    set_timeout(tD);	// Return every 500 ms to check for switch position
  }else
  {
      set_timeout(tF);	// If not first - wait 3 seconds until timeout
  }

  PreviousState = HIGH;
  while( is_not_timeout() )  
  {
    NewState = digitalRead(PIN_C);
    if(PreviousState != NewState)  
    {
      // check if we should return now or wait for the PIN_C to go HIGH
      // If this is the last request - return just as PinC goes LOW	
      if( (NewState == HIGH) || (req_type == REQ_LAST)) 
      {
        return Result;
 	  } 
      PreviousState = NewState;
    }

	// Do not check for pin B in MODE1 fill
    if( (fill_type != MODE1) && (digitalRead(PIN_B) == LOW) ) 
    {
      Result = 1;  // Bad CRC
    }
  }
  // Timeout occured - must return -1, escept some special cases
  // For Type 1 Fill the first request may be just a long pull of PIN_C
  // For Type 1 there may be no ACK/REQ byte - treat timeout as OK
  if( (fill_type == MODE1) && 
          ( (PreviousState == LOW) || (req_type == REQ_LAST) ) )
  {
		Result = 0;
  }else
  {
		Result = -1;
  }
  return Result;
}


void StartMode23Handshake()
{
  pinMode(PIN_D, OUTPUT);    // make pin output
  pinMode(PIN_F, OUTPUT);    // make pin output
  // Drop PIN_D first
  digitalWrite(PIN_D, LOW);
  digitalWrite(PIN_F, LOW);   // Drop PIN_F after delay
  delay(tA);                  // Pin D pulse width
  digitalWrite(PIN_D, HIGH);  // Bring PIN_D up again
}

void EndMode23Handshake()
{
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, HIGH);
}

void AcquireMode1Bus()
{
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_C, INPUT);
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  pinMode(PIN_F, OUTPUT);

  digitalWrite(PIN_B, HIGH);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, LOW);
  digitalWrite(PIN_F, HIGH);
  delay(tZ);
}


void AcquireMode23Bus()
{
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_C, INPUT);
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  pinMode(PIN_F, OUTPUT);

  digitalWrite(PIN_B, HIGH);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, HIGH);
  digitalWrite(PIN_F, HIGH);
  delay(tZ);
}


void ReleaseMode23Bus()
{
  delayMicroseconds(tL);
  pinMode(PIN_F, OUTPUT);
  digitalWrite(PIN_F, HIGH);
  delay(tZ);
  pinMode(PIN_B, INPUT);
  pinMode(PIN_C, INPUT);
  pinMode(PIN_D, INPUT);
  pinMode(PIN_E, INPUT);
  pinMode(PIN_F, INPUT);

  delay(tZ);
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
char CheckType123Equipment()
{
  char Equipment = 0;
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
		  ReleaseMode23Bus();
	  }
  }
  return Equipment;
}

// Send only TOD fill info for Type 3 SINCGARS
char WaitReqSendTODFill()
{
  char pos;	
  // On timeout return and check the switches
  if( WaitDS102Req(REQ_FIRST)  < 0 ) return -1;   
  
  for(pos = 1 ; pos <= NUM_TYPE3_CELLS; pos++)
  {
	  if(pos == TOD_CELL_NUMBER)	// Cell 13 is the TOD cell position
	  {
		  FillTODData();
		  cm_append(TOD_cell, MODE2_3_CELL_SIZE);
		  SendDS102Cell(TOD_cell, MODE2_3_CELL_SIZE);
	  }else
	  {
		  SendDS102Cell(nofill_cell, MODE2_3_CELL_SIZE);
	  }
	  WaitDS102Req(pos == NUM_TYPE3_CELLS ? REQ_LAST : REQ_NEXT);
  }
  ReleaseMode23Bus();
  return 0;
}

char SendDS102Fill(void)
{
	byte bytes, byte_cnt;
	char wait_result = ST_OK;
	
	while(records)	
	{
		bytes = byte_read(base_address++);
		while(bytes )
		{
			byte_cnt = MIN(bytes, FILL_MAX_SIZE);
			array_read(base_address, &data_cell[0], byte_cnt);
			base_address += byte_cnt;
			// Check if the cell that we are about to send is the 
			// TOD cell - replace it with the real Time cell
			if( (data_cell[0] == TOD_TAG_0) && (data_cell[1] == TOD_TAG_1) && 
						(fill_type == MODE3) && (byte_cnt == MODE2_3_CELL_SIZE) )
			{
				FillTODData();
				cm_append(TOD_cell, MODE2_3_CELL_SIZE);
	  		SendDS102Cell(TOD_cell, MODE2_3_CELL_SIZE);
			}else
			{
				SendDS102Cell(&data_cell[0], byte_cnt);
			}
			bytes -= byte_cnt;
		}
		records--;
		
		// After sending a record check for the next request
		wait_result = WaitDS102Req( records ? REQ_NEXT : REQ_LAST );

    // If all records were sent - ignore timeout
	  if(records == 0)
		{
			wait_result = ST_DONE;
			break;
		}
    if(wait_result) 
			break;
	}	

  if( (fill_type == MODE2) || (fill_type == MODE3))
  {
    ReleaseMode23Bus();
  }
	return wait_result;	// When send to MBITR - return with DONE flag
}

char WaitReqSendDS102Fill()
{
  char  result;
	// If first fill request was not answered - just return with timeout
	// We will be called again after switches are checked
	if( WaitDS102Req(REQ_FIRST) < 0 ) return -1;

	return SendDS102Fill();
}
