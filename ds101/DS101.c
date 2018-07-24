#include <stdlib.h>
#include <string.h>
#include "config.h"
#include <CRC16.h>
#include "fill.h"
#include <HDLC.h>
#include <DS101.h>
#include "delay.h"
#include "controls.h"


static const char master_name[15] 	= 	"T3COMMS       ";
static const char slave_name[15] 	= 	"KOV 21 0015415";

#pragma udata big_buffer   // Select large section
unsigned char  RxTx_buff[FILL_MAX_SIZE];
#pragma udata               // Return to normal section

unsigned char CurrentAddress;
int			  CurrentNumber;
char		  *CurrentName;

unsigned char NR;      // Received Number
unsigned char NS;      // Send Number
unsigned char PF;      // Poll/Final Flag

void (*ProcessIFrame)(char *pBuff, int size);
void (*ProcessSFrame)(unsigned char Cmd);
void (*ProcessUFrame)(unsigned char Cmd);
void (*ProcessIdle)(void);
char (*IsValidAddressAndCommand)(unsigned char  Address, unsigned char  Command);
char (*GetStatus)(void);
void (*StartProcess)(char slot);

int  (*RxDS101Data)(char *p_data);
void (*TxDS101Data)(char *p_data, int n_count);
void (*WriteCharDS101)(char ch);
int  (*ReadCharDS101)(void);

static char KeySlot;

void TxSFrame(unsigned char cmd)
{
	char *p_buff = &RxTx_buff[0];
	p_buff[0] = CurrentAddress;  
    p_buff[1] = cmd | (NR << 5) | PF_BIT;
	CRC16appnd(p_buff, 2);
    TxDS101Data(p_buff, 2 + 2);
}

void TxUFrame(unsigned char cmd)
{
	char *p_buff = &RxTx_buff[0];
	p_buff[0] = CurrentAddress;  
    p_buff[1] = cmd | PF_BIT;
	CRC16appnd(p_buff, 2);
    TxDS101Data(p_buff, 2 + 2);
}

void TxIFrame(char *p_data, int n_chars)
{
	int i;
	char *p_buff = &RxTx_buff[0];	  
  	p_buff[0] = CurrentAddress;  
    p_buff[1] = (NR << 5) | (NS << 1) | PF_BIT;
    memcpy((void *)&p_buff[2], (void *)p_data, n_chars);
	n_chars += 2;
	CRC16appnd(p_buff, n_chars);
    NS++;  NS &= 0x07;
    TxDS101Data(p_buff, n_chars + 2);
}

static char AXID_buff[21];
void TxAXID(char mode)
{
	int idx; 

	AXID_buff[0] = 0x00;  
	AXID_buff[1] = mode ? 0x50 : 0x60;	// 0x0050 - Request, 0x0060 - reply AXID FDU 

  	AXID_buff[2] = 0x00;    
	AXID_buff[3] = 0x11;	// Length = 17

  	AXID_buff[4] = 0x01;	// Frames = 0x1
    
	AXID_buff[5] = (CurrentNumber >> 8) & 0x00FF;	
	AXID_buff[6] = CurrentNumber & 0x00FF;	// Station Number
	// The station ID should be exactly 14 chars
	for(idx = 0; idx < 14; idx++)
	{
		char ch = CurrentName[idx];
		if(ch == 0) break;
		AXID_buff[7 + idx] = ch;
	}
	// Fill the rest with spaces
	for(; idx < 14; idx++)
	{
		AXID_buff[7 + idx] = ' ';
	}
    TxIFrame(&AXID_buff[0], 21);		// 7 + 14 = 21
}

typedef enum {
	SLAVE = 0,
	MASTER = 1
} DS101_MODE;

// Select appropriate functions based on the mode requested.
// Ideally, it would be done with virtual functions in C++
void SetupDS101Mode(char slot, char mode )
{
	char TxRxMode = ( (mode == TX_RS232) || (mode == TX_RS485) || (mode == TX_DTD232) ) ? MASTER : SLAVE;
    CurrentName = 	TxRxMode ? master_name : slave_name;
    IsValidAddressAndCommand = TxRxMode ?  IsMasterValidAddressAndCommand : IsSlaveValidAddressAndCommand;
    ProcessIdle = 	TxRxMode ? MasterProcessIdle : SlaveProcessIdle;
    ProcessUFrame = TxRxMode ? MasterProcessUFrame : SlaveProcessUFrame;
    ProcessSFrame = TxRxMode ? MasterProcessSFrame : SlaveProcessSFrame;
    ProcessIFrame = TxRxMode ? MasterProcessIFrame : SlaveProcessIFrame;
  	GetStatus =  	TxRxMode ? GetMasterStatus : GetSlaveStatus;
	StartProcess = 	TxRxMode ? MasterStart : SlaveStart;
    WriteCharDS101 = ((mode == TX_RS232) || (mode == RX_RS232)) ? TxRS232Char : TxDTDChar;
    ReadCharDS101 =  ((mode == TX_RS232) || (mode == RX_RS232)) ? RxRS232Char : RxDTDChar;

    RxDS101Data = ( (mode == TX_RS485) || (mode == RX_RS485)) ? RxRS485Data : RxRS232Data;
    TxDS101Data = ( (mode == TX_RS485) || (mode == RX_RS485)) ? TxRS485Data : TxRS232Data;
    KeySlot = slot;
}	

static  unsigned char Address;
static  unsigned char Command;
  
static  int  	nSymb;
static  char 	*p_data;

char ProcessDS101(void)
{
  StartProcess(KeySlot);

  while(GetStatus() == ST_OK)
  {
    ProcessIdle();

    p_data = &RxTx_buff[0];
    nSymb = RxDS101Data(p_data);
	
    if( (nSymb >= 4) && CRC16chk(p_data, nSymb))
    {
		nSymb -= 2;		// FCC/CRC at the end was checked and consumed
        // Extract all possible info from the incoming packet
        Address = p_data[0];
        Command = p_data[1];
        p_data +=2; nSymb -= 2;  // 2 chars were processed
        
		// Extract the PF flag and detect the FRAME type
        PF = Command & PMASK;      // Poll/Final flag

		// Only accept your or broadcast data, 
        if( IsValidAddressAndCommand(Address, Command) )
        {
          unsigned char NRR = (Command >> 5) & 0x07;
          unsigned char NSR = (Command >> 1) & 0x07;

		  // Select the type of the frame to process
          if(IsIFrame(Command))          // IFRAME
          {
            if( (NSR == NR) && (NRR == NS)) {
                NR = (NR + 1) & 0x07;	// Increment received frame number
				ProcessIFrame(p_data, nSymb);
            }else{
              TxSFrame(REJ);				// Reject frame
            }
          }else if(IsSFrame(Command))    // SFRAME
          {
            if( NRR == NS ) {
				ProcessSFrame(Command & SMASK);
	       	}else{
              TxSFrame(REJ);				// Reject frame
            }
          }else if(IsUFrame(Command))    // UFRAME
          {
			ProcessUFrame(Command & UMASK);
          }
        }
    }
  }
  return GetStatus();  
}


char StoreRS232Fill(char slot, char mode)
{
	char	ret;
	SetupDS101Mode(slot, RX_RS232 );
	OpenRS232(SLAVE);
	ret = ProcessDS101();
	CloseRS232();
	return ret;
}

char StoreDTD232Fill(char slot, char mode)
{
	char	ret;
	SetupDS101Mode(slot, RX_DTD232);
	OpenDTD(SLAVE);
	ret = ProcessDS101();
	CloseDTD();
	return ret;
}

char StoreRS485Fill(char slot, char mode)
{
	char	ret;
	SetupDS101Mode(slot, RX_RS485 );
	OpenRS485(SLAVE);
	ret = ProcessDS101();
	CloseRS485();
	return ret;
}

char SendRS232Fill(char slot)
{
	char	ret;
	SetupDS101Mode(slot, TX_RS232);
	OpenRS232(MASTER);
	ret = ProcessDS101();
	CloseRS232();
	return ret;
}

char SendDTD232Fill(char slot)
{
	char	ret;
	SetupDS101Mode(slot, TX_DTD232);
	OpenDTD(MASTER);
	ret = ProcessDS101();
	CloseDTD();
	return ret;
}

char SendRS485Fill(char slot)
{
	char	ret;
	SetupDS101Mode(slot, TX_RS485);
	OpenRS485(MASTER);
	ret = ProcessDS101();
	CloseRS485();
	return ret;
}
