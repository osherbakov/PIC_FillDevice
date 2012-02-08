#ifndef __FILL_H__
#define __FILL_H__

#include "config.h"

// QUERY bytes
typedef enum 
{
	MODE0 = 0x00,	// No fill - empty slot
	MODE1 = 0x01,	// DS-102   Type 1 fill
	MODE2 = 0x02,	// SINCGARS Type 2 fill
	MODE3 = 0x03,	// SINCGARS Type 3 fill
	MODE4 = 0x04,	// Serial PC/MBITR fill
	MODE5 = 0x05,	// DS-101 RS232 fill
	MODE6 = 0x06, // DS-101 DTS232 fill
	MODE7 = 0x07  // DS-101 RS485 fill
} FILL_TYPE;

typedef enum
{
	REQ_FIRST,
	REQ_NEXT,
	REQ_LAST
} REQ_TYPE;

typedef enum
{
	ST_TIMEOUT = -1,
	ST_OK = 0,
	ST_ERR = 1,
	ST_DONE = 2
} ST_STATUS;	

//--------------------------------------------------------------
//
#define delayMicroseconds(a) DelayUs(a)
#define delay(a) DelayMs(a)

//--------------------------------------------------------------
#define digitalRead(pin) (pin)
#define digitalWrite(pin, value) pin = value
#define pinMode(pin, mode) TRIS_##pin = mode

#define FILL_MAX_SIZE (64)
#define MODE2_3_CELL_SIZE (16)

#define	 TOD_CELL_NUMBER	(14)
#define	 NUM_TYPE3_CELLS	(22)

#define TOD_TAG_0 (0x00)
#define TOD_TAG_1 (0x02)

#define KEY_ACK  (0x06)	// Key ack
#define KEY_NAK  (0x15)	// Key nack
#define KEY_EOL  (0x0D)	// Key End of Line

#define  RX_TIMEOUT1_PC	 	(5000)    // 5 seconds
#define  RX_TIMEOUT2_PC	 	(100)
#define  RX_TIMEOUT1_MBITR 	(3000)  // 3 seconds
#define  RX_TIMEOUT2_MBITR 	(100)   // 100ms


// The proper delays to generate 8kHz
// Correction factor for the timing
#define _bitPeriod (1000000L / 8000)
#define tT (_bitPeriod/2) 

extern byte	 data_cell[];
extern byte	 TOD_cell[];
extern void  FillTODData(void);
extern void  ExtractTODData(void);

extern unsigned short long base_address;
extern byte  	fill_type;
extern byte 	records;

// Clear a selected slot
extern char ClearFill(byte stored_slot);
// Get the fill info from the slot
extern char CheckFillType(byte stored_slot);

// To receive fill - check for different fill types
extern char CheckFillType23(void);
extern char CheckFillType4(void);
extern char CheckFillRS232Type5(void);
extern char CheckFillDTD232Type5(void);
extern char CheckFillRS485Type5(void);

// Actual functions to get and store fills
extern char StoreDS102Fill(byte stored_slot, byte fill_type);
extern char StorePCFill(byte stored_slot, byte fill_type);

extern char CheckType123Equipment(void);

// Functions to wait for the request and send fill
extern char WaitReqSendDS102Fill(void);
extern char WaitReqSendTODFill(void);
extern char WaitReqSendMBITRFill(void);
extern char WaitReqSendPCFill(byte slot); // Any slot can be sent (dumped)

// Funcrtion to send fill immediately
extern char SendDS102Fill(void);


#endif	// __FILL_H__
