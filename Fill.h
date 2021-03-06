#ifndef __FILL_H__
#define __FILL_H__

#include "config.h"

// QUERY bytes
typedef enum 
{
	NONE = 0x00,	// No fill - empty slot
	MODE1 = 0x01,	// DS-102   Type 1 fill
	MODE2 = 0x02,	// SINCGARS Type 2 fill
	MODE3 = 0x03,	// SINCGARS Type 3 fill
	MODE4 = 0x04,	// Serial PC/MBITR fill
	MODE5 = 0x05	// DS-101 RS232/DTD/RS485 fill
} FILL_TYPE;

typedef enum
{
	REQ_FIRST,		// Requesting First Cell
	REQ_NEXT,		// Requesting Next Cell
	REQ_LAST		// Requesting Last Cell
} REQ_TYPE;

typedef enum
{
	ST_TIMEOUT = -1,// Timed out
	ST_OK = 0,		// Success
	ST_ERR = 1,		// Error
	ST_DONE = 2		// Operation completed
} ST_STATUS;	

//--------------------------------------------------------------
//
#define delayMicroseconds(a) DelayUs(a)
#define delay(a) DelayMs(a)

//--------------------------------------------------------------
#define digitalRead(pin) (pin)
#define digitalWrite(pin, value) pin = value
#define pinMode(pin, mode) TRIS_##pin = mode

#define FILL_MAX_SIZE (250)
#define MODE2_3_CELL_SIZE (16)

#define TYPE23_RETRIES (4)

#define	 TOD_CELL_NUMBER	(14)	// Time-of-Day Cell number (if present)
#define	 NUM_TYPE3_CELLS	(22)	// Total maximum number of Type 3 fill cells

#define TOD_TAG_0 (0x00)
#define TOD_TAG_1 (0x02)

#define KEY_ACK  (0x06)	// Key ack
#define KEY_NAK  (0x15)	// Key nack
#define KEY_EOL  (0x0D)	// Key End of Line

#define  RX_TIMEOUT1_PC	 	(3000)  // 3 seconds - timeout until 1st symbol
#define  RX_TIMEOUT2_PC	 	(100)   // 100ms - timeout after that
#define  RX_TIMEOUT1_MBITR 	(3000)  // 3 seconds - timeout until 1st symbol
#define  RX_TIMEOUT2_MBITR 	(100)   // 100ms - timeout after that
#define  RX_TIMEOUT1_RS	  	(3000)  // 3 seconds - timeout until 1st symbol
#define  RX_TIMEOUT2_RS	  	(100)  	// 100ms - timeout after that
#define  INF_TIMEOUT		(30*1000) // The biggest timeout we can get


// The one big buffer used by every application
extern unsigned char  RxTx_buff[FILL_MAX_SIZE];


// The proper delays to generate 8kHz
// Correction factor for the timing
#define _bitPeriod (1000000L / 2000)
#define tT (_bitPeriod/2) 

extern byte	 data_cell[];
extern byte	 TOD_cell[];

extern byte  ASCIIToHex(byte Symbol);
extern byte is_equal(byte *p1, const byte *p2, byte n);

// Functions to deal with TOD (Time-of-Day) data
extern void  FillTODData(void);
extern void  ExtractTODData(void);
extern char  IsValidYear(void);

extern void GetPCKey(byte slot);
extern void SetPCKey(byte slot);

extern void GetCurrentDayTime(void);
extern void SetCurrentDayTime(void);

// Clear a selected slot
extern char ClearFill(byte stored_slot);

// Get the fill info from the slot
extern char CheckFillType(byte stored_slot);

// To receive fill - set up pins properly
extern void SetType123PinsRx(void);

// To receive fill - check for different fill types
extern char CheckFillType1(void);
extern char CheckFillType23(void);
extern char CheckFillType1Connected(void);
extern char CheckFillType23Connected(void);

extern char CheckFillType4(void);

extern char CheckFillRS232Type5(void);
extern char CheckFillDTD232Type5(void);
extern char CheckFillRS485Type5(void);
extern char CheckType123Equipment(byte fill_type);

// Actual functions to get and store fills
extern char StoreDS102Fill(byte stored_slot, byte fill_type);
extern char StorePCFill(byte stored_slot, byte fill_type);

// Functions to wait for the request and send fill
extern char WaitReqSendDS102Fill(byte stored_slot, byte fill_type);
extern char WaitReqSendMBITRFill(byte stored_slot);
extern char WaitReqSendTODFill(void);

// Funcrtion to send fill immediately
extern char SendDS102Fill(byte stored_slot);


#endif	// __FILL_H__
