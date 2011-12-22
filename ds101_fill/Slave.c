#include <stdlib.h>
#include <string.h>
#include <CRC16.h>
#include <HDLC.h>
#include <DS101.h>

char IsSlaveValidAddressAndCommand()
{
	if((ReceivedAddress == 0xFF) || (ReceivedAddress == CurrentAddress))
	{
        // If no connection was established - return the DM (Disconnect Mode)
        // Status message on every request
        if(Disconnected )
        {
			if( IsIFrame(CurrentCommand) || IsSFrame(CurrentCommand) )
			{
              TxUFrame(DM);  // Send DM
			  return FALSE;
			}
        }
		return TRUE;
	}
	return FALSE;
}

void SlaveProcessIFrame(char *p_data, int n_chars)
{
    switch(frame_FDU)
    {
      case 0x0050:    // Get AXID
		// Send back requested AXID
		TxAXID();		
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
		// Save the Key
		memcpy((void *)KeyStorage + KeyStorageIdx, (void *)p_data, n_chars);
		KeyStorageIdx += n_chars;
		// Send response
        TxSFrame(RR);
      break;

	  case 0x0298:    // Key issue done
		// Save Key Size and init index
		KeyStorageSize = KeyStorageIdx;
		KeyStorageIdx = 0;
        TxSFrame(RR);
      break;

      case 0x01B0:    // Key fill
		// Save the Key
		memcpy((void *)KeyStorage + KeyStorageIdx, (void *)p_data, n_chars);
		KeyStorageIdx += n_chars;
		// Send response
        TxSFrame(RR);
      break;

      case 0x0198:    // Key fill done
		// Save Key Size and init index
		KeyStorageSize = KeyStorageIdx;
		KeyStorageIdx = 0;
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
		// Save the key name
		memcpy((void *)KeyName, (void *)p_data, n_chars);
		KeyNameSize = n_chars;
        TxSFrame(RR);
      break;

      case 0x0290:    // Key issue name 
		// Save the key name
		memcpy((void *)KeyName, (void *)p_data, n_chars);
		KeyNameSize = n_chars;

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
}

void SlaveProcessUFrame(unsigned char Cmd)
{
	if(Cmd == SNRM)            // SNRM
	{
	  // Reset all to defaults on connect 
	  Disconnected = FALSE;
	  frame_len = 0;
	  NR = 0;
	  NS = 0;

	  TxUFrame(UA);  // Send UA
	}else if(Cmd == DISC)      // DISC
	{
	  // Reset all to defaults on disconnect 
	  Disconnected = TRUE;
	  frame_len = 0;
	  NR = 0;
	  NS = 0;

	  TxUFrame(UA);
	  // After the disconnect  - change the address
	  CurrentAddress = NewAddress;
	}else if(Cmd == UI)      // UI
	{
	}else if(Cmd == UP)      // UP
	{
	  TxSFrame(RR);
	}else if(Cmd == RSET)      // RSET
	{
	  NR = 0;
	  TxUFrame(UA);
	}
}	


void SlaveProcessIdle()
{
}

