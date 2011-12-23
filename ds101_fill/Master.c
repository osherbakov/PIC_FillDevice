#include <stdlib.h>
#include <string.h>
#include <CRC16.h>
#include <HDLC.h>
#include <DS101.h>

#pragma udata big_buffer   // Select large section
char Key_buff[512];
#pragma udata

char master_mode = 0;
char master_state = MS_IDLE;

void TxKeyName(void)
{
	int	 n_chars;

	// FDU - 2 bytes
	Key_buff[0] = 0x01;  // 0x0190 - Key fill name
	Key_buff[1] = (char) 0x90;

	// FDU length - 2 bytes
    Key_buff[2] = (KeyNameSize >> 8) & 0x00FF;	    
	Key_buff[3] = KeyNameSize & 0x00FF;	    

	n_chars = 4 + KeyNameSize;

	memcpy((void *)&Key_buff[4],(void *) KeyName, KeyNameSize);
    TxIFrame(&Key_buff[0], n_chars);
}

void TxKeyData(void)
{
	int	 n_chars;

	// FDU - 2 bytes
	Key_buff[0] = 0x01;  // 0x01B0 - Key fill data
	Key_buff[1] = (char) 0xB0;

	// FDU length - 2 bytes
    Key_buff[2] = (KeyStorageSize >> 8) & 0x00FF;	    
	Key_buff[3] = KeyStorageSize & 0x00FF;	    

	n_chars = 4 + KeyStorageSize;

	memcpy((void *)&Key_buff[4],(void *) KeyStorage, KeyStorageSize);
    TxIFrame(&Key_buff[0], n_chars);
}

void TxKeyEnd(void)
{
	int	 n_chars;
	int FDU_size = 0;

	// FDU - 2 bytes
	Key_buff[0] = 0x01;  // 0x0198 - Key fill done
	Key_buff[1] = (char) 0x98;

	// FDU length - 2 bytes
    Key_buff[2] = (FDU_size >> 8) & 0x00FF;	    
	Key_buff[3] = FDU_size & 0x00FF;	    

	n_chars = 4 + FDU_size;
    TxIFrame(&Key_buff[0], n_chars);
}

void TxTerm(void)
{
	int	 n_chars;
	int FDU_size = 0;

	// FDU - 2 bytes
	Key_buff[0] = 0x00;  // 0x0000 - Terminate remote
	Key_buff[1] = 0x00;

	// FDU length - 2 bytes
    Key_buff[2] = (FDU_size >> 8) & 0x00FF;
	Key_buff[3] = FDU_size & 0x00FF;	    

	n_chars = 4 + FDU_size;
    TxIFrame(&Key_buff[0], n_chars);
}

//#define TX_WAIT   (5000L * 10000L)    // 5 Second
//unsigned long long Timeout;

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
//			Timeout = millis() + TX_WAIT;
			master_state = MS_CONNECT;
			break;

		case MS_CONNECT:
//			if(Timeout < millis())
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
		master_state = MS_REQ_DISC1;
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
		   case MS_REQ_DISC1:
				TxUFrame(DISC);
				master_state = MS_RECONNECT;
				break;

			case MS_CHECK_RR:
				TxSFrame(RR);  // Send RR
				master_state = MS_KEY_FILL_NAME;
				break;

		   case MS_KEY_FILL_NAME:
				TxKeyName();  // Send Key Name
				master_state = MS_KEY_FILL_DATA;
				break;

			case MS_KEY_FILL_DATA:
				TxKeyData();  // Send Key Data
				master_state = MS_KEY_FILL_END;
				break;

			case MS_KEY_FILL_END:
				TxKeyEnd();  // Send Key End
				master_state = MS_REQ_TERM;
				break;

			case MS_REQ_TERM:
				TxTerm();  // Terminate remote 
				master_state = MS_REQ_DISC2;
				break;

			case MS_REQ_DISC2:
				TxUFrame(DISC);
				master_state = MS_DISC;
				break;

			case MS_DISC:
				// Reset all to defaults on disconnect 
			    Disconnected = TRUE;
				frame_len = 0;
				NR = 0;
				NS = 0;

				master_mode = 0;
				master_state = MS_IDLE;
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
				frame_len = 0;
				NR = 0;
				NS = 0;
				PF = 1;

				TxUFrame(SNRM);  // Send SNRM
				master_state = MS_CHECK_RR;
				break;

			case MS_CHECK_RR:
				TxSFrame(RR);  // Send RR
				master_state = MS_KEY_FILL_NAME;
				break;

			case MS_KEY_FILL_NAME:
				TxKeyName();  // Send Key Name
				master_state = MS_KEY_FILL_DATA;
				break;

			case MS_KEY_FILL_DATA:
				TxKeyData();  // Send Key Data
				master_state = MS_KEY_FILL_END;
				break;

			case MS_KEY_FILL_END:
				TxKeyEnd();  // Send Key End
				master_state = MS_REQ_TERM;
				break;

			case MS_REQ_TERM:
				TxTerm();  // Terminate remote 
				master_state = MS_REQ_DISC2;
				break;

			case MS_REQ_DISC2:
				TxUFrame(DISC);
				master_state = MS_DISC;
				break;

			case MS_DISC:
				// Reset all to defaults on disconnect 
			    Disconnected =TRUE;
				frame_len = 0;
				NR = 0;
				NS = 0;

				master_mode = 0;
				master_state = MS_IDLE;
				break;
		}
	}else if(Cmd == DISC)      // DISC
	{
	  // Reset all to defaults on disconnect 
	  Disconnected = TRUE;
      frame_len = 0;
	  NR = 0;
	  NS = 0;

	  master_mode = 0;
	  master_state = MS_IDLE;
	}
}
