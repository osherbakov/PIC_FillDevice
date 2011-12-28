#include <stdlib.h>
#include <string.h>
#include "config.h"
#include <CRC16.h>
#include "fill.h"
#include <HDLC.h>
#include <DS101.h>


char master_name[14] = "PRC152 radio";
int	 master_number = 56;

char slave_name[14] = "PRC152 radio";
int	 slave_number = 56;

#pragma udata big_buffer   // Select large section
char  RxTx_buff[512];
#pragma udata               // Return to normal section


unsigned char MasterNewAddress = 0x35;
unsigned char SlaveNewAddress = 0x35;

unsigned char MasterAddress = 0x35;
unsigned char SlaveAddress = 0x35;
unsigned char ReceivedAddress;


unsigned char CurrentAddress;
unsigned char CurrentCommand;
int			  CurrentNumber;
char		  *CurrentName;
unsigned char NewAddress;

char Disconnected = TRUE;

unsigned char NR;      // Received Number
unsigned char NS;      // Send Number
unsigned char PF;      // Poll/Final Flag

void (*ProcessIFrame)(char *pBuff, int size);
void (*ProcessSFrame)(unsigned char Cmd);
void (*ProcessUFrame)(unsigned char Cmd);
void (*ProcessIdle)(void);
char (*IsValidAddressAndCommand)(void);
char (*GetStatus)(void);


void TxSFrame(unsigned char cmd)
{
  if (PF)
  {
	char *p_buff = &RxTx_buff[0];
	p_buff[0] = CurrentAddress;  
    p_buff[1] = cmd | (NR << 5) | PF_BIT;
    TxData(p_buff, 2);
  }
}

void TxUFrame(unsigned char cmd)
{
  if (PF)
  {
	char *p_buff = &RxTx_buff[0];
	p_buff[0] = CurrentAddress;  
    p_buff[1] = cmd | PF_BIT;
    TxData(p_buff, 2);
  }
}

void TxIFrame(char *p_data, int n_chars)
{
  int i;
  if (PF)
  {
	char *p_buff = &RxTx_buff[0];	  
	p_buff[0] = CurrentAddress;  
    p_buff[1] = (NR << 5) | (NS << 1) | PF_BIT;
    memcpy((void *)&p_buff[2], (void *)p_data, n_chars);
    TxData(p_buff, n_chars + 2);
    NS++;  NS &= 0x07;
  }
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
		if(ch)
		{
			AXID_buff[7 + idx] = ch;
		}else
		{
			break;
		}
	}
	// Fill the rest with spaces
	for(; idx < 14; idx++)
	{
		AXID_buff[7 + idx] = ' ';
	}
 
    TxIFrame(&AXID_buff[0], 21);
}

// Select appropriate functions based on the mode requested.
// Ideally, it would be done with virtual functions in C++
void SetupDS101Mode(char mode)
{
		char TxMode = ((mode == TX_RS232) || 
										(mode == TX_RS485) || 
											(mode == TX_PC232) ) ? TRUE : FALSE;
    CurrentAddress = TxMode ? MasterAddress : SlaveAddress;
    CurrentNumber = TxMode ? master_number : slave_number;
    CurrentName = TxMode ? master_name : slave_name;
    NewAddress = TxMode ? MasterAddress : SlaveAddress;
    IsValidAddressAndCommand = TxMode ?  IsMasterValidAddressAndCommand : IsSlaveValidAddressAndCommand;
    ProcessIdle = TxMode ? MasterProcessIdle : SlaveProcessIdle;
    ProcessUFrame = TxMode ? MasterProcessUFrame : SlaveProcessUFrame;
    ProcessSFrame = TxMode ? MasterProcessSFrame : SlaveProcessSFrame;
    ProcessIFrame = TxMode ? MasterProcessIFrame : SlaveProcessIFrame;
  	GetStatus =  TxMode ? GetMasterStatus : GetSlaveStatus; 
    WriteCharDS101 = ( (mode == TX_RS485) || (mode == RX_RS485)) ? TxRS485Char :
						( (mode == TX_RS232) || (mode == RX_RS232) ) ? TxRS232Char :
								TxPC232Char ;
    ReadCharDS101 = ( (mode == TX_RS485) || (mode == RX_RS485)) ? RxRS485Char :
						( (mode == TX_RS232) || (mode == RX_RS232) ) ? RxRS232Char :
								RxPC232Char ;
		WaitFlagDS101 = ( (mode == TX_RS485) || (mode == RX_RS485)) ? RxRS485Flag :
						( (mode == TX_RS232) || (mode == RX_RS232) ) ? RxRS232Flag :
								RxPC232Flag ;

		TRIS_PIN_GND = INPUT;		// Prepare Ground
		if((mode == TX_RS485) || (mode == RX_RS485))
	  {
			ON_GND = 0;							// Remove ground from Pin B
		}else
		{
			ON_GND = 1;							// Set ground on Pin B
		}

    if(TxMode) 
    	MasterStart();
    else
    	SlaveStart();
}	


char SendDS101Fill(char mode)
{
	SetupDS101Mode(mode);
	ProcessDS101();
}

char ProcessDS101()
{
    int  nSymb;
    char *p_data;

  while(GetStatus() == ST_OK)
  {
    ProcessIdle();

    p_data = &RxTx_buff[0];
    nSymb = RxData(p_data);
    if(nSymb > 0)    
    {
        // Extract all possible info from the incoming packet
        ReceivedAddress = p_data[0];
        CurrentCommand = p_data[1];
        p_data +=2; nSymb -= 2;  // 2 chars were processed
        
				// Extract the PF flag and detect the FRAME type
        PF = CurrentCommand & PMASK;      // Poll/Final flag

		// Only accept your or broadcast data, 
        if( IsValidAddressAndCommand() )
        {
          unsigned char NRR = (CurrentCommand >> 5) & 0x07;
          unsigned char NSR = (CurrentCommand >> 1) & 0x07;

		  // Select the type of the frame to process
          if(IsIFrame(CurrentCommand))          // IFRAME
          {
            if( (NSR == NR) && (NRR == NS)) 
            {
                NR = (NR + 1) & 0x07;	// Increment received frame number
			    ProcessIFrame(p_data, nSymb);
            }else
            {
              TxSFrame(REJ);				// Reject frame
            }
          }else if(IsSFrame(CurrentCommand))    // SFRAME
          {
            if( NRR == NS ) 
            {
				ProcessSFrame(CurrentCommand & SMASK);
	        }else
            {
              TxSFrame(REJ);				// Reject frame
            }
          }else if(IsUFrame(CurrentCommand))    // UFRAME
          {
			ProcessUFrame(CurrentCommand & UMASK);
          }
        }
    }
  }
  return GetStatus();  
}

char CheckRS232(void)
{
	SetupDS101Mode(RX_RS232);
	if( WaitFlagDS101() == FLAG)
	{
		return ProcessDS101();
	}
	return -1;
}

char CheckPC232(void)
{
	SetupDS101Mode(RX_PC232);
	if( WaitFlagDS101() == FLAG)
	{
		return ProcessDS101();
	}
	return -1;
}

char CheckRS485(void)
{
	SetupDS101Mode(RX_RS485);
	if( WaitFlagDS101() == FLAG)
	{
		return ProcessDS101();
	}
	return -1;
}
