#include <stdlib.h>
#include <string.h>
#include <CRC16.h>
#include <HDLC.h>
#include <DS101.h>


char master_mode = 0;
char master_state = MS_IDLE;

char master_name[14] = "PRC152 radio";
int	 master_number = 56;

char slave_name[14] = "PRC152 radio";
int	 slave_number = 56;

#pragma udata big_buffer   // Select large section
char  Rx_buff[256 + 64];
char  Tx_buff[256 + 64];
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


#pragma udata big_buffer   // Select large section
char KeyStorage[256 + 64];
char KeyName[40];
#pragma udata

int KeyStorageIdx = 0;
int KeyStorageSize = 0;
int  KeyNameSize = 0;

char Disconnected = TRUE;

int frame_len = 0;
int frame_FDU;

unsigned char NR;      // Received Number
unsigned char NS;      // Send Number
unsigned char PF;      // Poll/Final Flag

void (*ProcessIFrame)(char *pBuff, int size);
void (*ProcessSFrame)(unsigned char Cmd);
void (*ProcessUFrame)(unsigned char Cmd);
void (*ProcessIdle)(void);
char (*IsValidAddressAndCommand)(void);

void TxSFrame(unsigned char cmd)
{
  if (PF)
  {
	char *p_buff = &Tx_buff[0];
	p_buff[0] = CurrentAddress;  
    p_buff[1] = cmd | (NR << 5) | PF_BIT;
    TxData(p_buff, 2);
  }
}

void TxUFrame(unsigned char cmd)
{
  if (PF)
  {
	char *p_buff = &Tx_buff[0];
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
	char *p_buff = &Tx_buff[0];	  
	p_buff[0] = CurrentAddress;  
    p_buff[1] = (NR << 5) | (NS << 1) | PF_BIT;
    memcpy((void *)&p_buff[2], (void *)p_data, n_chars);
    TxData(p_buff, n_chars + 2);
    NS++;  NS &= 0x07;
  }
}

void TxAXID()
{
	char AXID_buff[21];
	int idx; 

	AXID_buff[0] = 0x00;  
	AXID_buff[1] = master_mode ? 0x50 : 0x60;	// 0x0050 - Request, 0x0060 - reply AXID FDU 

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


void loop()
{
    int  nSymb;
    char *p_data;
	
    CurrentAddress = master_mode ? MasterAddress : SlaveAddress;
    CurrentNumber = master_mode ? master_number : slave_number;
    CurrentName = master_mode ? master_name : slave_name;
    NewAddress = master_mode ? MasterAddress : SlaveAddress;
    IsValidAddressAndCommand = master_mode?  IsMasterValidAddressAndCommand : IsSlaveValidAddressAndCommand;
    ProcessIdle = master_mode ? MasterProcessIdle : SlaveProcessIdle;
    ProcessUFrame = master_mode ? MasterProcessUFrame : SlaveProcessUFrame;
    ProcessSFrame = master_mode ? MasterProcessSFrame : SlaveProcessSFrame ;
    ProcessIFrame = master_mode ? MasterProcessIFrame : SlaveProcessIFrame;

  while(1)
  {

    ProcessIdle();

    nSymb = RxData(&Rx_buff[0]);
    if(nSymb > 0)    
    {
        // Extract all possible info from the incoming packet
        p_data = &Rx_buff[0];
        ReceivedAddress = p_data[0];
        CurrentCommand = p_data[1];
        nSymb -= 2; p_data +=2;
        

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
                if(frame_len == 0)	// No data left in the frame
                {
                    frame_FDU = (((int)p_data[0]) << 8) + (((int)p_data[1]) & 0x00FF);
                    frame_len = (((int)p_data[2]) << 8) + (((int)p_data[3]) & 0x00FF);
                    p_data += 4;   nSymb -= 4;		// 4 chars were processed
                }
                frame_len -= nSymb;
                
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
}
