#include <stdlib.h>
#include <string.h>
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
    MS_IDLE,
	MS_CONNECT,
	MS_AXID_EXCH,
	MS_REQ_DISC,
	MS_RECONNECT,
	MS_CHECK_RR,
	MS_SEND_DATA,
	MS_DISC,
	MS_DONE
};


char master_state = MS_IDLE;

static unsigned int base_address;	// Address of the current EEPROM data
static unsigned char block_counter = 0;     // Counter for blocks sent

// Get the next block from EEPROM and send it out
void TxDataBlock(void)
{
	unsigned int  byte_count;
	
	// Read the first byte of the block - it has the size
	byte_count = byte_read(base_address++);
	//Adjust the count - 0 means 0x100
	if(byte_count == 0) byte_count = 0x0100;
	
	// Read the block of data
	array_read(base_address, &Key_buff[0], byte_count);
	base_address += byte_count;
    TxIFrame(&Key_buff[0], byte_count);
}	


#define TX_WAIT   (5)    // 5 Second
unsigned int Timeout;

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
			CurrentAddress = 0xFF;

			TxUFrame(SNRM);		// Request connection
			Timeout = seconds_counter + TX_WAIT;
			master_state = MS_CONNECT;
			break;

		case MS_CONNECT:
			if(Timeout < seconds_counter)
			{
				master_state = MS_IDLE;
			}
			break;
	}
}

char IsMasterValidAddressAndCommand()
{
  // Valid commands for the Master mode are either broadcast (0xFF)
  // or with the explicit address specified.
  // Additionally, the UA means that SNRM was accepted
	if((ReceivedAddress == 0xFF) || (ReceivedAddress == CurrentAddress))
	{
		return TRUE;
	}else if (IsUFrame(CurrentCommand) && ( (UMASK & CurrentCommand) == UA) )
	{
		// If the response is UA - assign that address
		CurrentAddress = ReceivedAddress;
		Disconnected = FALSE;
		return TRUE;
	}
	return FALSE;
}


void MasterProcessIFrame(char *p_data, int n_chars)
{
    switch(frame_FDU)
    {
	   // Should be only in the MS_AXID_EXCH state
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
			    Disconnected = TRUE;
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
}

void MasterProcessUFrame(unsigned char Cmd)
{
	if(Cmd == UA)            // UA
	{
		switch(master_state)
		{
			case MS_CONNECT:
				TxAXID(1);	// On connect ask for AXID
				master_state = MS_AXID_EXCH;
				break;

			case MS_RECONNECT:
				// Reset all to defaults on connect 
	  			Disconnected = TRUE;
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
			    Disconnected = TRUE;
				frame_len = 0;
				NR = 0;
				NS = 0;

				master_state = MS_DONE;
				break;
		}
	}else if(Cmd == DISC)      // DISC
	{
	  // Reset all to defaults on disconnect 
	  Disconnected = TRUE;
      frame_len = 0;
	  NR = 0;
	  NS = 0;

	  master_state = MS_DONE;
	}
}
