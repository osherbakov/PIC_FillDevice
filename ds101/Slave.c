#include <stdlib.h>
#include <string.h>
#include "config.h"
#include <CRC16.h>
#include <HDLC.h>
#include <DS101.h>
#include "spi_eeprom.h"
#include "fill.h"
#include "delay.h"


static unsigned short long base_address;        // EEPROM block start address
static unsigned short long saved_base_address;  // EEPROM block start address (saved)
static unsigned char block_counter;             // Counter for blocks sent

static char NewAddress;
static char Connected;


int frame_len;
int frame_FDU;

static char status;

#define RX_WAIT   (9)    // 9 Seconds

static unsigned int Timeout;
static char	retry_flag;
static void ResetTimeout(void)
{
  INTCONbits.GIE = 0; 
	Timeout = seconds_counter + RX_WAIT;
  INTCONbits.GIE = 1;
	retry_flag = 0;
}

static char IsTimeoutExpired(void)
{
  char  ret;
  INTCONbits.GIE = 0; 
	ret = (seconds_counter > Timeout) ? TRUE : FALSE;
  INTCONbits.GIE = 1;
  return ret;
}


void SlaveStart(char slot)
{
	base_address = get_eeprom_address(slot & 0x0F);
	saved_base_address = base_address;
	base_address += 2;	// Reserve memory for type and size

	// Reset all to defaults 
	NR = 0;
	NS = 0;
	PF = 1;
	frame_len = 0;
	status = ST_OK;
	block_counter = 0;
	CurrentAddress = SLAVE_ADDRESS;
	NewAddress = SLAVE_ADDRESS; 
  CurrentNumber = SLAVE_NUMBER;
	Connected = FALSE;
	ResetTimeout();
	retry_flag = 1; // No retries until first data sent
}



char GetSlaveStatus()
{
	return status;
}	

char IsSlaveValidAddressAndCommand(unsigned char Address, unsigned char Command)
{
	if((Address == BROADCAST) || (Address == CurrentAddress))
	{
    // If no connection was established - return the DM (Disconnect Mode)
    // Status message on every request
    if(!Connected && ( IsIFrame(Command) || IsSFrame(Command) ) )
		{
        TxUFrame(DM);  // Send DM
			  return FALSE;
		}
		if(Address != BROADCAST)
		{
		  NewAddress = Address; 
		}
		return TRUE;
	}
	return FALSE;
}


void SaveDataBlock(char *p_data, int n_chars)
{
	// First save the size of the saved block 0 - 0x100
	byte_write(base_address, n_chars & 0x00FF);
	base_address++;
	
	// Write the data block
	array_write(base_address, (unsigned char *)p_data, n_chars);
	base_address += n_chars;
	
	// update the block counter and type
	block_counter++;
	byte_write(saved_base_address, block_counter);  
	byte_write(saved_base_address + 1, MODE5);
}	

void SlaveProcessIFrame(char *p_data, int n_chars)
{
	char *p_data_saved = p_data;
	int  n_saved_chars = n_chars;
  if(frame_len == 0)	// No data left in the frame
  {
      frame_FDU = (((int)p_data[0]) << 8) + (((int)p_data[1]) & 0x00FF);
      frame_len = (((int)p_data[2]) << 8) + (((int)p_data[3]) & 0x00FF);
      p_data += 4;   n_chars -= 4;		// 4 chars were processed (FDU and Length)
  }
  frame_len -= n_chars;    

	//  Save all data packets except AXID FDU
	if( (frame_FDU != 0x0050) && (frame_FDU != 0x0060) )
	{
		SaveDataBlock(p_data_saved, n_saved_chars);
	}	
	
  switch(frame_FDU)
  {
    case 0x0050:    // Get AXID
	  // Send back requested AXID
	  TxAXID(0);		
    break;

//*************************************************
//			KEY ISSUE SHA
//*************************************************

  case 0x03FF:    // Issue the key with SHA
      TxSFrame(RR);
    break;
    
//*************************************************
//			CIK FILL/ISSUE
//*************************************************

    case 0x02D8:    // CIK split issue
      TxSFrame(RR);
    break;

    case 0x02D0:    // CIK split issue continue
      TxSFrame(RR);
    break;

    case 0x01D8:    // CIK split fill
      TxSFrame(RR);
    break;

    case 0x01D0:    // CIK split fill continue
      TxSFrame(RR);
    break;

//*************************************************
//			KEY FILL/ISSUE
//*************************************************

  case 0x02B0:    // Key issue
	// Send response
      TxSFrame(RR);
    break;

  case 0x0298:    // Key issue done
      TxSFrame(RR);
    break;

    case 0x01B0:    // Key fill
	// Send response
      TxSFrame(RR);
    break;

    case 0x0198:    // Key fill done
      TxSFrame(RR);
    break;

//*************************************************
//			FILE FILL/ISSUE
//*************************************************

  case 0x02B8:    // Program transfer issue
      TxSFrame(RR);
    break;

    case 0x01B8:    // Program transfer fill
      TxSFrame(RR);
    break;

//*************************************************
//			KEY FILL/ISSUE NAME
//*************************************************
  case 0x0190:    // Key fill name
      TxSFrame(RR);
    break;

    case 0x0290:    // Key issue name 
      TxSFrame(RR);
    break;

//*************************************************
//			TRKEK ISSUE
//*************************************************
    case 0x02E0:    // Transfer TrKEK Issue
	frame_len = 0;	// Force it to be in one frame
      TxSFrame(RR);
    break;

//*************************************************
//			USER FORMAT KEY
//*************************************************
    case 0x0070:    // Send User format key
      TxSFrame(RR);
    break;

//*************************************************
//			SET DTD ADDRESS
//*************************************************
    case 0x0010:    // Set DTD Address
      NewAddress = *p_data;
      TxSFrame(RR);
    break;

//*************************************************
//			TERMINATE REMOTE CONNECTION
//*************************************************
    case 0x0000:    // End of remote
      TxSFrame(RR);
    break;

    default:    
      TxSFrame(RR);
    break;
  }
  ResetTimeout();
}

void SlaveProcessSFrame(unsigned char Cmd)
{
	// Valid in-sequence frame
	if(Cmd == RR)            // RR
	{
		TxSFrame(RR);
	}else if(Cmd == RNR)      // RNR
	{
		TxSFrame(RR);
	}else if(Cmd == REJ)      // REJ
	{
	}else if(Cmd == SREJ)      // SREJ
	{
	}
	ResetTimeout();
}

void SlaveProcessUFrame(unsigned char Cmd)
{
	if(Cmd == SNRM)            // SNRM
	{
	  // Reset all to defaults on connect 
	  Connected = TRUE;
	  frame_len = 0;
	  NR = 0;
	  NS = 0;

	  TxUFrame(UA);  // Send UA
	}else if(Cmd == DISC)      // DISC
	{
	  // Reset all to defaults on disconnect
	  Connected = FALSE;
	  frame_len = 0;
	  NR = 0;
	  NS = 0;
	  TxUFrame(UA);
	  // After the disconnect  - change the address
	  CurrentAddress = NewAddress;
		if(block_counter > 0)
		{
			status = ST_DONE;
		}
	}else if(Cmd == UI)      // UI
	{
	}else if(Cmd == UP)      // UP
	{
	  TxSFrame(RR);
	}else if(Cmd == RSET)    // RSET
	{
	  NR = 0;
	  TxUFrame(UA);
	}
	ResetTimeout();
}	


void SlaveProcessIdle()
{
	if(IsTimeoutExpired())
	{
		if(retry_flag == 0)
		{
			TxRetry();
    	ResetTimeout();
			retry_flag = 1;
		}else
		{
  		status = ST_TIMEOUT;
		}	
	}	
}
