#include "config.h"
#include "crcmodel.h"
#include "delay.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"


static void AcquireBus();
static void ReleaseBus();

static char WaitReq(byte req_type);

static char GetEquipmentType(void);
static char WaitSerialReq(byte);

static void StartHandshake(void);
static void EndHandshake(void);

//--------------------------------------------------------------
// Delays for the appropriate timings in millisecs
//--------------------------------------------------------------
#define tM 		10	  // D LOW -> F LOW 	(-5us - 100ms)
#define tA  	50	  // F LOW -> D HIGH	(45us - 55us)
#define tE  	350   // REQ -> Fill		(0 - 2.3 sec)

#define tZ  	150   // End -> New Fill	
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
#define tC  (5L * 60L * 1000L)  // 5 minutes - > until REQ (300us - 5min )

static byte  PreviousState;
static byte  NewState;	

// Sends byte data on PIN_B with clocking on PIN_E
void SendQuery(byte Data)
{
  byte i;
  // Set up pins mode and levels
  pinMode(PIN_B, OUTPUT);    // make a pin active
  pinMode(PIN_E, OUTPUT);    // make a pin active
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
void SendByte(byte Data)
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
void SendCell(byte *p_cell, byte count)
{
  byte i;
  pinMode(PIN_D, OUTPUT);    // make a pin active
  pinMode(PIN_E, OUTPUT);    // make a pin active
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, HIGH);
  delay(tE);     // Delay before sending first bit
  for (i = 0; i < count; i++)
  {
    SendByte(*p_cell++);
  }
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, HIGH);
}

// Receive the equipment data that is sent on PIN_B with clocking on PIN_E
//   The total number of clocks is 40 (41), but only the last bit matters
char GetEquipmentType()
{
  byte i;
  
  i = 0;
  pinMode(PIN_B, INPUT);    // make a pin input
  pinMode(PIN_E, INPUT);    // make a pin input
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

char WaitReq(byte req_type)
{
  char   Result = 0;

  pinMode(PIN_B, INPUT);    // make a pin input
  pinMode(PIN_C, INPUT);    // make a pin input
  PreviousState = HIGH;

  if( req_type == REQ_FIRST)
  {
	  set_timeout(tD);	// Return every 500 ms to check for keys
  }else
  {
	  set_timeout(tF);
  }
  while( is_not_timeout() )  
  {
    NewState = digitalRead(PIN_C);
    if(PreviousState != NewState)  
    {
      PreviousState = NewState;
      if( (NewState == HIGH) || (req_type == REQ_LAST)) 
      {
        return Result;
 	  } 
    }

	// Do not check for pin B in MODE1 fill
    if( (fill_type != MODE1) && (digitalRead(PIN_B) == LOW) ) 
    {
      Result = 1;  // Bad CRC
    }
  }
  
  // For Type 1 there may be no ACK/REQ byte - treat timeout as OK
  return ((fill_type == MODE1) && (req_type == REQ_LAST)) ? 0 : -1;
}


void StartHandshake()
{
  delay(tZ);

  pinMode(PIN_D, OUTPUT);    // make a pin output
  pinMode(PIN_F, OUTPUT);    // make a pin output
  // Drop PIN_D first
  digitalWrite(PIN_D, LOW);
  // Drop PIN_F after delay
  digitalWrite(PIN_F, LOW);
  delay(tA);    // Pin D pulse width
  // Bring PIN_D up again
  digitalWrite(PIN_D, HIGH);
}

void EndHandshake()
{
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, HIGH);
}

void AcquireBus()
{
  pinMode(PIN_B, INPUT);
  pinMode(PIN_C, INPUT);
  pinMode(PIN_D, OUTPUT);
  pinMode(PIN_E, OUTPUT);
  pinMode(PIN_F, OUTPUT);
  digitalWrite(PIN_D, HIGH);
  digitalWrite(PIN_E, HIGH);
  digitalWrite(PIN_F, HIGH);
}


void ReleaseBus()
{
  delayMicroseconds(tL);
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

	ms = rtc_date.MilliSeconds;
	while(ms >= 10)
	{
		TOD_cell[11] += 0x10;
		ms -= 10; 
	}
}

char CheckEquipment()
{
  char Equipment = 0;
  if((fill_type == MODE1))
  {
	  pinMode(PIN_B, OUTPUT);
      pinMode(PIN_C, INPUT);
	  pinMode(PIN_D, OUTPUT);
	  pinMode(PIN_E, OUTPUT);
	  pinMode(PIN_F, OUTPUT);
	  digitalWrite(PIN_B, LOW);
	  digitalWrite(PIN_D, LOW);
	  digitalWrite(PIN_E, LOW);
	  digitalWrite(PIN_F, HIGH);
	  Equipment = MODE1;
  }else if(fill_type == MODE4)
  {
	  // Connect ground on PIN B
  	  TRIS_PIN_GND = INPUT;
	  ON_GND = 1;
	  if( WaitSerialReq(REQ_FIRST) == 0 )  
		  Equipment = MODE4;
  }else
  {
	  AcquireBus();
	  StartHandshake();
  	  SendQuery(MODE3);
	  Equipment = GetEquipmentType();
  	  EndHandshake();
	  if(Equipment < 0)	// Timeout occured
	  {
		  ReleaseBus();
	  }
  }
  return Equipment;
}

// Send only TOD fill info for Type 3 SINCGARS
char SendTODFill()
{
  char pos;	
  // On timeout return and check the switches
  if( WaitReq(REQ_FIRST)  < 0 ) return -1;   
  
  for(pos = 1 ; pos <= NUM_TYPE3_CELLS; pos++)
  {
	  if(pos == TOD_CELL_NUMBER)	// Cell 13 is the TOD cell position
	  {
		  FillTODData();
		  cm_append(TOD_cell, MODE2_3_CELL_SIZE);
		  SendCell(TOD_cell, MODE2_3_CELL_SIZE);
	  }else
	  {
		  SendCell(nofill_cell, MODE2_3_CELL_SIZE);
	  }
	  WaitReq(pos == NUM_TYPE3_CELLS ? REQ_LAST : REQ_NEXT);
  }
  ReleaseBus();
  return 0;
}

byte CHECK_MBITR[4] = {0x2F, 0x39, 0x38, 0x0D };	// "/98<cr>"
byte send_SN;

unsigned int base_address;
byte  fill_type;
byte records;

byte CheckFillType(byte stored_slot)
{
	base_address = ((unsigned int)(stored_slot & 0x0F)) << KEY_MAX_SIZE_PWR;
   	records = byte_read(base_address++); if(records == 0xFF) records = 0x00;
	fill_type = byte_read(base_address++);
	if(stored_slot >> 4)	// Send fill to the PC
	{
		send_SN = 0;
		p_rx = rx_eusart;
		p_tx = tx_eusart;
		p_ack = WaitSerialReq;
	}else					// Send Fill to the MBITR
	{
		if(fill_type == MODE4) // DES fill mode
		{
			send_SN = 1;
			p_rx = rx_mbitr;
			p_tx = tx_mbitr;
			p_ack = WaitSerialReq;
		}else					// Regular MODE 1, 2, 3 fills
		{
			send_SN = 0;
			p_tx = SendCell;
			p_ack = WaitReq;
		}
	}
	return records ? fill_type : 0;
}

char WaitSerialReq(byte req_type)
{
	byte char_received;
	byte char_to_expect;

	char_to_expect = KEY_ACK; 
	// This is the DES key load - send serial number request
	if( (req_type == REQ_FIRST) && send_SN )
	{
		p_tx(&CHECK_MBITR[0], 4);
		char_to_expect = KEY_EOL ; 
	}

	// wait in the loop until receive the ACK character, or timeout
   	while( p_rx(&char_received, 1) && (char_received != char_to_expect) ) {}; 
	return ( char_received == char_to_expect ) ? 0 : -1 ; 
}

char SendFill(byte records)
{
	byte bytes, byte_cnt;

	// If first fill request was not answered - just return with timeout
	if( p_ack(REQ_FIRST) < 0 ) return -1;

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
		if( p_ack( records ? REQ_NEXT : REQ_LAST ) ) return 1;	// Error - no Ack
	}	
	if( (fill_type == MODE1) || (fill_type == MODE2) || (fill_type == MODE3))
	{
    	ReleaseBus();
	}
  	return send_SN ? 2 : 0;	// When send to MBITR - return with DONE flag
}


char SendStoredFill(byte stored_slot)
{
  // All data are stored in 2K bytes (16K bits) slots
  // The first byte of the each slot has the number of the records (0 - 255)
  // The first byte of the record has the number of bytes that should be sent out
  // so each record has no more than 255 bytes as well
  // Empty slot has first byte as 0x00
	CheckFillType(stored_slot);
	return SendFill(records);
}
