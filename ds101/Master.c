#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "fill.h"
#include "delay.h"
#include <CRC16.h>
#include <HDLC.h>
#include <DS101.h>
#include "spi_eeprom.h"

#pragma udata big_buffer   // Select large section
char Key_buff[512];
#pragma udata

enum MASTER_STATE
{
  MS_IDLE = 0,
	MS_SEND_SNRM,
	MS_AXID_EXCH,
	MS_REQ_DISC,
	MS_RECONNECT,
	MS_CHECK_RR,
	MS_SEND_DATA,
	MS_WAIT_RR,
	MS_DISC,
	MS_DONE,
	MS_TIMEOUT,
	MS_ERROR
};


#define TX_WAIT   (2)    // 3 Seconds

static char	retry_flag;
static unsigned int Timeout;
static void ResetTimeout(void)
{
  INTCONbits.GIE = 0; 
	Timeout = seconds_counter + TX_WAIT;
  INTCONbits.GIE = 1;
	retry_flag = 0;
}

static char IsTimeoutExpired(void)
{
  char  ret;
  INTCONbits.GIE = 0; 
	ret = (seconds_counter > Timeout) ? 1 : 0;
  INTCONbits.GIE = 1;
  return ret;
}


static char master_state;

static unsigned long short base_address;
static unsigned char block_counter;   // Counter for blocks sent

void MasterStart(char slot)
{
	base_address = get_eeprom_address(slot & 0x0F);
	block_counter = byte_read(base_address++); 
	base_address++;		// Skip the Fill type byte
	// Reset all to defaults 
	NR = 0;
	NS = 0;
	PF = 1;
	frame_len = 0;
	master_state = MS_IDLE;	
	CurrentAddress = BROADCAST;
  CurrentNumber = MASTER_NUMBER;
	ResetTimeout();
}	

char GetMasterStatus()
{
	return 
		(master_state == MS_TIMEOUT) ? ST_TIMEOUT :
			(master_state == MS_DONE) ? ST_DONE : 
				(master_state == MS_ERROR) ? ST_ERR : ST_OK; 
}	

// Get the next block from EEPROM and send it out
void TxDataBlock(void)
{
	unsigned int  byte_count;
	
	// Read the first byte of the block - it has the size
	byte_count = ((unsigned int )byte_read(base_address++)) & 0x00FF;
	//Adjust the count - 0 means 0x100
	if(byte_count == 0) byte_count = 0x0100;
	
	// Read the block of data
	array_read(base_address, (unsigned char *)&Key_buff[0], byte_count);
	base_address += byte_count;
  TxIFrame(&Key_buff[0], byte_count);
}	

void MasterProcessIdle()
{
	switch(master_state)
	{
		case MS_IDLE:
			// Reset all to defaults on connect 
			NR = 0;
			NS = 0;
			PF = 1;
			frame_len = 0;

			TxUFrame(SNRM);		// Request connection
    	ResetTimeout();
			master_state = MS_SEND_SNRM;
			break;

		case MS_SEND_SNRM:
			if(IsTimeoutExpired())
			{
				master_state = MS_TIMEOUT;
			}
			break;
    
    case MS_DONE:
    case MS_ERROR:
    case MS_TIMEOUT:
      // Stay in this state
      break;
      
		default:
			if(IsTimeoutExpired())
			{
				if(retry_flag == 0)
				{
					TxRetry();
        	ResetTimeout();
					retry_flag = 1;
				}else
				{
					master_state = MS_ERROR;
				}	
			}
			break;
	}
}

char IsMasterValidAddressAndCommand(unsigned char Address, unsigned char Command)
{
  // Valid commands for the Master mode are either broadcast (0xFF)
  // or with the explicit address specified.
  // Additionally, the UA means that SNRM was accepted
	if((Address == BROADCAST) || (Address == CurrentAddress))
	{
		return TRUE;
	}else if (IsUFrame(Command) && ( (UMASK & Command) == UA) )
	{
		// If the response is UA - assign that address
		CurrentAddress = Address;
		return TRUE;
	}
	return FALSE;
}


void MasterProcessIFrame(char *p_data, int n_chars)
{
    if(frame_len == 0)	// No data left in the frame
    {
        frame_FDU = (((int)p_data[0]) << 8) + (((int)p_data[1]) & 0x00FF);
        frame_len = (((int)p_data[2]) << 8) + (((int)p_data[3]) & 0x00FF);
        p_data += 4;   n_chars -= 4;		// 4 chars were processed
    }
    frame_len -= n_chars;    

    switch(frame_FDU)
    {
	   // Should be only in the MS_AXID_EXCH state
      case 0x0050:    // Received AXID
      case 0x0060:    // Received AXID
				master_state = MS_REQ_DISC;
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



void MasterProcessSFrame(unsigned char Cmd)
{
	if(Cmd == RR)
	{
		switch(master_state)
		{
		   case MS_REQ_DISC:
				TxUFrame(DISC);
				master_state = MS_RECONNECT;
				break;

			case MS_CHECK_RR:
				TxSFrame(RR);  // Send RR
				master_state = MS_SEND_DATA;
				break;

		  case MS_SEND_DATA:
		   	if(block_counter)
		   	{
						TxDataBlock();  // Send Data block from EEPROM
					  block_counter--;
				}else
				{
					// Done - request disconnect
					TxUFrame(DISC);
					master_state = MS_DISC;
				}	
				break;

			case MS_DISC:
				// Reset all to defaults on disconnect 
				frame_len = 0;
				NR = 0;
				NS = 0;

				master_state = MS_DONE;
				break;
		}
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

void MasterProcessUFrame(unsigned char Cmd)
{
	if(Cmd == UA)            // UA
	{
		switch(master_state)
		{
			case MS_SEND_SNRM:
				TxAXID(1);	// On connect ask for AXID
				master_state = MS_AXID_EXCH;
				break;

			case MS_RECONNECT:
				// Reset all to defaults on connect 
				frame_len = 0;
				NR = 0;
				NS = 0;
				PF = 1;

				TxUFrame(SNRM);  // Send SNRM
				master_state = MS_CHECK_RR;
				break;

			case MS_CHECK_RR:
				TxSFrame(RR);  // Send RR
				master_state = MS_SEND_DATA;
				break;

		   case MS_SEND_DATA:
		   		if(block_counter)
		   		{
					TxDataBlock();  // Send Data block from EEPROM
					block_counter--;
				}else
				{
					// Done - request disconnect
					TxUFrame(DISC);
					master_state = MS_DISC;
				}	
				break;

			case MS_DISC:
				// Reset all to defaults on disconnect 
				frame_len = 0;
				NR = 0;
				NS = 0;

				master_state = MS_DONE;
				break;
		}
	}else if(Cmd == DISC)      // DISC
	{
	  // Reset all to defaults on disconnect 
    frame_len = 0;
	  NR = 0;
	  NS = 0;

	  master_state = MS_DONE;
	}
	ResetTimeout();
}
