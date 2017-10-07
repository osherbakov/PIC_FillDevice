#include <stdlib.h>
#include <string.h>
#include "config.h"
#include <CRC16.h>
#include "fill.h"
#include <HDLC.h>
#include <DS101.h>
#include "delay.h"
#include "controls.h"


char master_name[14] = "DTD 2243";
char slave_name[14] = "PRC152 radio";

#pragma udata big_buffer   // Select large section
char  RxTx_buff[512];
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

void (*OpenDS101)(void);
int  (*RxDS101Data)(char *p_data);
void (*TxDS101Data)(char *p_data, int n_count);
void (*WriteCharDS101)(char ch);
int  (*ReadCharDS101)(void);


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
    TxDS101Data(p_buff, n_chars + 2);
    NS++;  NS &= 0x07;
}

void TxAXID(char mode)
{
	char AXID_buff[21];
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

// Select appropriate functions based on the mode requested.
// Ideally, it would be done with virtual functions in C++
void SetupDS101Mode(char slot, char mode )
{
	char TxMode = ( (mode == TX_RS232) || 
					(mode == TX_RS485) || 
					(mode == TX_DTD232) ) ? TRUE : FALSE;
    CurrentName = 	TxMode ? master_name : slave_name;
    IsValidAddressAndCommand = TxMode ?  IsMasterValidAddressAndCommand : IsSlaveValidAddressAndCommand;
    ProcessIdle = 	TxMode ? MasterProcessIdle : SlaveProcessIdle;
    ProcessUFrame = TxMode ? MasterProcessUFrame : SlaveProcessUFrame;
    ProcessSFrame = TxMode ? MasterProcessSFrame : SlaveProcessSFrame;
    ProcessIFrame = TxMode ? MasterProcessIFrame : SlaveProcessIFrame;
  	GetStatus =  	TxMode ? GetMasterStatus : GetSlaveStatus;
	StartProcess = 	TxMode ? MasterStart : SlaveStart;
  	OpenDS101 = ( (mode == TX_RS485) || (mode == RX_RS485)) ? OpenRS485 : 
        			((mode == TX_RS232) || (mode == RX_RS232)) ? OpenRS232 : OpenDTD;
    WriteCharDS101 = ((mode == TX_RS232) || (mode == RX_RS232)) ? TxRS232Char : TxDTDChar;
    ReadCharDS101 =  ((mode == TX_RS232) || (mode == RX_RS232)) ? RxRS232Char : RxDTDChar;

    RxDS101Data = ( (mode == TX_RS485) || (mode == RX_RS485)) ? RxRS485Data : RxRS232Data;
    TxDS101Data = ( (mode == TX_RS485) || (mode == RX_RS485)) ? TxRS485Data : TxRS232Data;

	OpenDS101();
	StartProcess(slot);
}	

char ProcessDS101(void)
{
  int  nSymb;
  char *p_data;
  unsigned char Address;
  unsigned char Command;

  while(GetStatus() == ST_OK)
  {
    ProcessIdle();

    p_data = &RxTx_buff[0];
    nSymb = RxDS101Data(p_data);
    if((nSymb > 4) && CRC16chk(p_data, nSymb))
    {
		nSymb -= 2;		// FCC/CRC was checked and consumed
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
	SetupDS101Mode(slot, RX_RS232 );
	return ProcessDS101();
}

char StoreDTD232Fill(char slot, char mode)
{
	SetupDS101Mode(slot, RX_DTD232);
	return ProcessDS101();
}

char StoreRS485Fill(char slot, char mode)
{
	SetupDS101Mode(slot, RX_RS485 );
	return ProcessDS101();
}

char SendRS232Fill(char slot)
{
	SetupDS101Mode(slot, TX_RS232);
	return ProcessDS101();
}

char SendDTD232Fill(char slot)
{
	SetupDS101Mode(slot, TX_DTD232);
	return ProcessDS101();
}

char SendRS485Fill(char slot)
{
	SetupDS101Mode(slot, TX_RS485);
	return ProcessDS101();
}

// The Type5 RS485 fill is detected when PIN_P is always higher than PIN_N
char CheckFillRS485Type5()
{
	if( !RCSTA1bits.SPEN )
  	{  
  		TRIS_Data_N	= INPUT;
  		TRIS_Data_P	= INPUT;
  		WPUB_Data_N = 1;
  		WPUB_Data_P = 1;
	
	 	return ( Data_P && !Data_N ) ? MODE5 : -1;
	}
	return -1;
}	
