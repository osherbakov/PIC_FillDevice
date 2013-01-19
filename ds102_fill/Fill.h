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
	MODE5 = 0x05	// DS-101 fill
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

#define KEY_MAX_SIZE_PWR	(11)	// Max_size = 2^(N)

#define FILL_MAX_SIZE (64)
#define MODE2_3_CELL_SIZE (16)

#define	 TOD_CELL_NUMBER	(14)
#define	 NUM_TYPE3_CELLS	(22)

#define TOD_TAG_0 (0x00)
#define TOD_TAG_1 (0x02)

#define KEY_ACK  (0x06)	// Key ack
#define KEY_EOL  (0x0D)	// Key End of Line

// The proper delays to generate 8kHz
// Correction factor for the timing
#define _bitPeriod (1000000L / 8000)
#define tT (_bitPeriod/2) 

extern byte	 data_cell[];
extern byte  fill_type;

// Pointers to the different Rx and Tx functions (PC, MBITR, and DS-102)
extern void (*p_tx)(byte *, byte);
extern byte (*p_rx)(byte *, byte);
extern char (*p_ack)(byte);

extern char CheckFillType23(void);
extern char CheckFillType4(void);
extern char CheckFillRS232Type5(void);
extern char CheckFillRS485Type5(void);


extern char GetStoreFill(byte stored_slot);

extern char CheckEquipment(void);

extern char ClearFill(byte stored_slot);
extern char CheckFillType(byte stored_slot);
extern char SendStoredFill(byte stored_slot);
extern char WaitReqSendFill(void);
extern char WaitReqSendTODFill(void);
extern char SendFill(void);



#endif	// __FILL_H__
