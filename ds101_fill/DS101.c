#include <stdlib.h>
#include <string.h>
#include "config.h"
#include <CRC16.h>
#include "fill.h"
#include <HDLC.h>
#include <DS101.h>
#include "delay.h"


char master_name[14] = "DTD 2243";
char slave_name[14] = "PRC152 radio";

#pragma udata big_buffer   // Select large section
char  RxTx_buff[512];
#pragma udata               // Return to normal section

unsigned char ReceivedAddress;
unsigned char CurrentAddress;
unsigned char CurrentCommand;
int			  CurrentNumber;
char		  *CurrentName;

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
void SetupDS101Mode(char mode, char slot)
{
		char TxMode = ((mode == TX_RS232) || 
										(mode == TX_RS485) ) ? TRUE : FALSE;
    CurrentName = TxMode ? master_name : slave_name;
    IsValidAddressAndCommand = TxMode ?  IsMasterValidAddressAndCommand : IsSlaveValidAddressAndCommand;
    ProcessIdle = TxMode ? MasterProcessIdle : SlaveProcessIdle;
    ProcessUFrame = TxMode ? MasterProcessUFrame : SlaveProcessUFrame;
    ProcessSFrame = TxMode ? MasterProcessSFrame : SlaveProcessSFrame;
    ProcessIFrame = TxMode ? MasterProcessIFrame : SlaveProcessIFrame;
  	GetStatus =  TxMode ? GetMasterStatus : GetSlaveStatus; 
    WriteCharDS101 = ( (mode == TX_RS485) || (mode == RX_RS485)) ? TxRS485Char : TxRS232Char;
    ReadCharDS101 = ( (mode == TX_RS485) || (mode == RX_RS485)) ? RxRS485Char : RxRS232Char;

		TRIS_PIN_GND = INPUT;		// Prepare Ground
		if((mode == TX_RS485) || (mode == RX_RS485))
	  {
			ON_GND = 0;							// Remove ground from Pin B
		}else
		{
			ON_GND = 1;							// Set ground on Pin B
		}

    if(TxMode) 
    	MasterStart(slot);
    else
    	SlaveStart(slot);
}	


char ProcessDS101(void)
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

char GetDS101Fill(char mode, char slot)
{
	SetupDS101Mode(mode, slot);
	return ProcessDS101();
}

char SendDS101Fill(char mode, char slot)
{
	SetupDS101Mode(mode, slot);
	return ProcessDS101();
}

char GetRS232Fill(char slot)
{
	SetupDS101Mode(RX_RS232, slot);
	return ProcessDS101();
}

char GetRS485Fill(char slot)
{
	SetupDS101Mode(RX_RS485, slot);
	return ProcessDS101();
}

char SendRS232Fill(char slot)
{
	SetupDS101Mode(TX_RS232, slot);
	return ProcessDS101();
}

char SendRS485Fill(char slot)
{
	SetupDS101Mode(TX_RS485, slot);
	return ProcessDS101();
}

// The Type5 RS485 fill is detected when PIN_P is always higher than PIN_N
char CheckFillRS485Type5()
{
	TRIS_PIN_GND = INPUT;	
	ON_GND = 0;							// Set ground from Pin B	

	TRIS_Data_P = INPUT;
	TRIS_Data_N = INPUT;
	
	DelayUs(100);
	
  TMR1H = 0;
	TMR1L = 0;	// Reset the timeout timer
	PIR1bits.TMR1IF = 0;	// Clear Interrupt
	while( !PIR1bits.TMR1IF )	// Until timeout
	{
		if( !Data_P || Data_N )
		{
			return -1;
		}	
	}	
	return MODE5;
}	
