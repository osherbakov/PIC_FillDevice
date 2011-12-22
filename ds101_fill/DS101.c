#include <stdlib.h>
#include <string.h>
#include <CRC16.h>
#include <HDLC.h>
#include <DS101.h>


//#define RX_WAIT   (1000 * 10000L)    // 1 Second

char master_mode = 0;
char master_state = MS_IDLE;

char master_name[14] = "PRC152 radio";
int	 master_number = 56;

char slave_name[14] = "PRC152 radio";
int	 slave_number = 56;

char  Rx_buff[0x100];
char  Tx_buff[0x100];

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


char KeyStorage[256];
int KeyStorageIdx = 0;
int KeyStorageSize = 0;

char KeyName[256];
int  KeyNameSize = 0;

char Disconnected = TRUE;

int frame_len = 0;
int frame_FDU;

unsigned char NR;      // Received Number
unsigned char NS;      // Send Number
unsigned char PF;      // Poll/Final Flag


typedef enum
{
    ST_IDLE,
    ST_DATA,
    ST_ESCAPE
}RX_STATE;

// Returns:
//   -1  - Timeout occured
//   0   - Aborted or incorrect FCS
//   >0  - Data received
int RxData(char *p_data)
{
  char *p_Rx_buff;
  RX_STATE state;
  
  state = ST_IDLE;
  p_Rx_buff = p_data;

//  unsigned long long Timeout = millis() + RX_WAIT;
//  while(millis() <= Timeout)
  while (1)
  {
    if(SerPort.available())
    {
//	  Timeout = millis() + RX_WAIT;	// Kick timeout
      char ch = SerPort.read();
      // Serial.print(ch & 0x00FF, HEX); Serial.print(" ");
      switch(state)
      {
        case ST_IDLE:
          if(ch != FLAG)
          {
            *p_data++ = ch;
            state = ST_DATA;
          }
          break;
        case ST_DATA:
          if(ch == FLAG)
          {
            // Check the Rx_buffer for correct FCS
            int  n_chars = p_data - p_Rx_buff;
			state = ST_IDLE;

			PrintBuff(p_Rx_buff, n_chars);
			// Get at least 2 chars and FCS should match
            return (  (n_chars > 2)  && 
						CRC16chk((unsigned char *)p_Rx_buff, n_chars) ) ? 
							(n_chars - 2) : 0;
          }else if(ch == ESCAPE)
          {
            state = ST_ESCAPE;
          }else
          {
            *p_data++ = ch;
          }
          break;
        case ST_ESCAPE:
          *p_data++ = ch;
          state = ST_DATA;
          break;
      }
    }
  }
  return -1; 
}

void TxData(char *p_data, int n_chars)
{
  int  i = 0;

  for(i = 0; i < 5; i++)
  {
      SerPort.print(FLAG);
  }

  n_chars += 2;    // Space for extra 2 chars of FCS
  CRC16appnd((unsigned char *)p_data, n_chars);

//  PrintSentData(p_data, n_chars);

  while(n_chars--)
  {
    char ch = *p_data++;
    if( (ch == FLAG) || (ch == ESCAPE) )
    {
      SerPort.print(ESCAPE);
    }
    SerPort.print(ch);
  }
  SerPort.print(FLAG);
}

void TxSFrame(unsigned char cmd)
{
  if (PF)
  {
	Tx_buff[0] = CurrentAddress;  
    Tx_buff[1] = cmd | (NR << 5) | PF_BIT;
    TxData(Tx_buff, 2);
  }
}

void TxUFrame(unsigned char cmd)
{
  if (PF)
  {
	Tx_buff[0] = CurrentAddress;  
    Tx_buff[1] = cmd | PF_BIT;
    TxData(Tx_buff, 2);
  }
}

void TxIFrame(char *p_data, int n_chars)
{
  int i;
  if (PF)
  {
	Tx_buff[0] = CurrentAddress;  
    Tx_buff[1] = (NR << 5) | (NS << 1) | PF_BIT;
    for (i = 0; i < n_chars; i++)
    {
      Tx_buff[i + 2] = p_data[i];
    }
    TxData(Tx_buff, n_chars + 2);
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

void ProcessIFrame(char *pBuff, int size)
{
  if(frame_len == 0)	// No data left in the frame
  {
		frame_FDU = (((int)pBuff[0]) << 8) + (((int)pBuff[1]) & 0x00FF);
		frame_len = (((int)pBuff[2]) << 8) + (((int)pBuff[3]) & 0x00FF);
		pBuff += 4;
		size -= 4;		// 4 chars were processed
  }
  PrintData(frame_FDU, frame_len, pBuff, size);
  frame_len -= size;

  if(master_mode)
  {
	  MasterProcessIFrame(pBuff, size); 
  }else
  {
	  SlaveProcessIFrame(pBuff, size ); 
  }
}


void ProcessSFrame(char Cmd)
{
  if(master_mode)
  {
	MasterProcessSFrame(Cmd);
  }else
  {
	SlaveProcessSFrame(Cmd);
  }
}


void ProcessUFrame(char Cmd)
{
	if(master_mode)
	{
		MasterProcessUFrame(Cmd);
	}else
	{
		SlaveProcessUFrame(Cmd);
	}
}

void ProcessIdle()
{
	if(master_mode)
	{
		MasterProcessIdle();
	}else
	{
		SlaveProcessIdle();
	}
}

char IsValidAddressAndCommand()
{
	if(master_mode)
	{
		return IsMasterValidAddressAndCommand();
	}else
	{
		return IsSlaveValidAddressAndCommand();
	}
}



void loop()
{
  int  nSymb; 
	
  CurrentAddress = master_mode ? MasterAddress : SlaveAddress;
  CurrentNumber = master_mode ? master_number : slave_number;
  CurrentName = master_mode ? master_name : slave_name;
  NewAddress = master_mode ? MasterAddress : SlaveAddress;

  while(1)
  {

    ProcessIdle();

    nSymb = RxData(Rx_buff);
    if(nSymb > 0)    
    {
        // Extract all possible info from the incoming packet
        ReceivedAddress = Rx_buff[0];
        CurrentCommand = Rx_buff[1];

		// Extract the PF flag and detect the FRAME type
        PF = CurrentCommand & PMASK;      // Poll/Final flag
		PrintCommand(CurrentCommand);

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
			  ProcessIFrame(&Rx_buff[2], nSymb - 2);
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
