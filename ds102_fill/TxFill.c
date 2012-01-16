#include "config.h"
#include "crcmodel.h"
#include "delay.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"


static void AcquireMode1Bus(void);
static void AcquireMode23Bus(void);
static void ReleaseMode23Bus(void);

static char WaitDS102Req(byte req_type);

static char GetEquipmentMode23Type(void);
static char WaitMBITRReq(byte req_type);
static char WaitPCReq(byte req_type);

static void StartMode23Handshake(void);
static void EndMode23Handshake(void);

static void SendMode23Query(byte Data);

//--------------------------------------------------------------
// Delays for the appropriate timings in millisecs
//--------------------------------------------------------------
#define tM 		10	  // D LOW -> F LOW 	(-5us - 100ms)
#define tA  	50	  // F LOW -> D HIGH	(45us - 55us)
#define tE  	250   // REQ -> Fill		(0 - 2.3 sec)

#define tZ  	250   // End -> New Fill	
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
  ON_GND = 0;
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
  ON_GND = 0;
  pinMode(PIN_B, INPUT);
  pinMode(PIN_C, INPUT); 
  pinMode(PIN_E, INPUT);
  
  WPUB_PIN_C = 1;
  
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
  	ON_GND = 0;
  	pinMode(PIN_B, INPUT);
  }
  pinMode(PIN_C, INPUT); 
  WPUB_PIN_C = 1;

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
  ON_GND = 0;
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_C, INPUT);
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  pinMode(PIN_F, OUTPUT);

  WPUB_PIN_C = 1;

  digitalWrite(PIN_B, HIGH);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, LOW);
  digitalWrite(PIN_F, HIGH);
  delay(tZ);
}


void AcquireMode23Bus()
{
  ON_GND = 0;
  pinMode(PIN_B, OUTPUT);
  pinMode(PIN_C, INPUT);
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  pinMode(PIN_F, OUTPUT);

  WPUB_PIN_C = 1;

  digitalWrite(PIN_B, HIGH);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, HIGH);
  digitalWrite(PIN_F, HIGH);
  delay(tZ);
}


void ReleaseMode23Bus()
{
  ON_GND = 0;
  delayMicroseconds(tL);
  pinMode(PIN_F, OUTPUT);
  digitalWrite(PIN_F, HIGH);
  delay(tZ);
  pinMode(PIN_B, INPUT);
  pinMode(PIN_C, INPUT);
  pinMode(PIN_D, INPUT);
  pinMode(PIN_E, INPUT);
  pinMode(PIN_F, INPUT);

  WPUB_PIN_C = 0;
  
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


// Time cell
byte TOD_cell[MODE2_3_CELL_SIZE];

// Generic cell that can keep all the data
byte	 data_cell[FILL_MAX_SIZE];

void  FillTODData(void)
{
	byte ms;
	GetRTCData();
	TOD_cell[0] = TOD_TAG_0;
	TOD_cell[1] = TOD_TAG_1;
	TOD_cell[2] = rtc_date.Century;
	TOD_cell[3] = rtc_date.Year;
	TOD_cell[4] = rtc_date.Month;
	TOD_cell[5] = rtc_date.Day;
	TOD_cell[6] = rtc_date.JulianDayH;
	TOD_cell[7] = rtc_date.JulianDayL;
	TOD_cell[8] = rtc_date.Hours;
	TOD_cell[9] = rtc_date.Minutes;
	TOD_cell[10] = rtc_date.Seconds;
	TOD_cell[11] = 0;

	ms = rtc_date.MilliSeconds_10;
	while(ms >= 10)
	{
		TOD_cell[11] += 0x10;
		ms -= 10; 
	}
}

// Check the equipment type
// If the Type2/3 equipment is detected, then return MODE2 or MODE3
// For Type 1 - return MODE1 right away
// If Key is Type 4 - then send the /98 and wait for the response
char CheckEquipment()
{
  char Equipment = 0;
  if((fill_type == MODE1))
  {
	  AcquireMode1Bus();
	  Equipment = MODE1;
  }else if(fill_type == MODE4)
  {
	  // Connect ground on PIN B
  	TRIS_PIN_GND = INPUT;
	  ON_GND = 1;
	  if( p_ack(REQ_FIRST) == 0 )  
	  {
		  Equipment = MODE4;
	  }
  }else		// MODE 2 or MODE 3
  {
	  ON_GND = 0;
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

static unsigned int base_address;
byte  	fill_type;
byte 	records;

// Set up functions that will be called depending on the Key type
// Type 1, 2,3 - will be sent thru DS102 interface
// Type 4 - will be sent thru MBITR
// If the high byte of the parameter "stored_slot" is not 0 - send the key to PC
char CheckFillType(byte stored_slot)
{
	base_address = ((unsigned int)(stored_slot & 0x0F)) << KEY_MAX_SIZE_PWR;
	records = byte_read(base_address++); 
	if(records == 0xFF) records = 0x00;
	
	// Get the fill type from the EEPROM
	fill_type = byte_read(base_address++);
	if(stored_slot >> 4)	// Send fill to the PC
	{
		p_rx = rx_eusart;
		p_tx = tx_eusart;
		p_ack = WaitPCReq;
	}else					// Send Fill to the MBITR
	{
		close_eusart();
		if(fill_type == MODE4) // DES fill mode
		{
			p_rx = rx_mbitr;
			p_tx = tx_mbitr;
			p_ack = WaitMBITRReq;
		}else					// Regular MODE 1, 2, 3 fills
		{
			p_tx = SendDS102Cell;
			p_ack = WaitDS102Req;
		}
	}
	return records ? fill_type : 0;
}

byte CHECK_MBITR[4] = {0x2F, 0x39, 0x38, 0x0D };	// "/98<cr>"

char WaitMBITRReq(byte req_type)
{
	byte char_received;
	byte char_to_expect;

	// This is the DES key load - send serial number request
	char_received = 0;
	if( req_type == REQ_FIRST )
	{
		char_to_expect = KEY_EOL;		// Wait for \n
		p_tx(&CHECK_MBITR[0], 4);
	}else
	{
		char_to_expect = KEY_ACK;		// Wait for 0x06
	}	

	// wait in the loop until receive the ACK character, or timeout
  while( p_rx(&char_received, 1) && (char_received != char_to_expect) ) {}; 
	return ( char_received == char_to_expect ) ? 0 : -1 ; 
}

char WaitPCReq(byte req_type)
{
	byte char_received;
	byte char_to_expect;

	char_to_expect = KEY_ACK; 
	// wait in the loop until receive the ACK character, or timeout
  while( p_rx(&char_received, 1) && (char_received != char_to_expect) ) {}; 
	return ( char_received == char_to_expect ) ? 0 : -1 ; 
}

char SendFill()
{
	byte bytes, byte_cnt;
	char wait_result;
	
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
		  		p_tx(TOD_cell, MODE2_3_CELL_SIZE);
			}else
			{
				p_tx(&data_cell[0], byte_cnt);
			}
			bytes -= byte_cnt;
		}
		records--;
		
		// After sending a record check for the next request
		wait_result = p_ack( records ? REQ_NEXT : REQ_LAST );
    // If all records were sent - ignore timeout
	  if(records == 0)
		{
			wait_result = ST_OK;
		}
    if(wait_result) 
			break;
	}	
	
	// If regular Type 2 3 fill - release the bus
	if( p_ack == WaitDS102Req)
	{
    	ReleaseMode23Bus();
	}
	return (p_ack == WaitMBITRReq) ? ST_DONE : wait_result;	// When send to MBITR - return with DONE flag
}


char SendStoredFill(byte stored_slot)
{
	CheckFillType(stored_slot);
	// If first fill request was not answered - just return with timeout
	// We will be called again after switches are checked
	if( p_ack(REQ_FIRST) < 0 ) return -1;
	return SendFill();
}

char WaitReqSendFill()
{
	// If first fill request was not answered - just return with timeout
	// We will be called again after switches are checked
	if( p_ack(REQ_FIRST) < 0 ) return -1;
	return SendFill();
}
