#include <stdlib.h>
#include <string.h>
#include "config.h"
#include <CRC16.h>
#include <HDLC.h>
#include <DS101.h>
#include <delay.h>


#define TX_DELAY_MS	(30)

RX_STATE hdlc_state = ST_IDLE;

// Returns:
//   -1  - Timeout occured
//   0   - Aborted or incorrect FCS
//   >0  - Data received
int RxData(char *p_data)
{
  	char *p_Rx_buff;
  	int  symbol;
  	char ch;
  	int  n_chars;
  
  	p_Rx_buff = p_data;
    while(1)
    {
      symbol = ReadCharDS101();
      if(symbol < 0) 
         break;
         
      ch = (char) symbol;   
      switch(hdlc_state)
      {
        case ST_IDLE:
          if(ch != FLAG)
          {
            *p_data++ = ch;
            hdlc_state = ST_DATA;
          }
          break;
        case ST_DATA:
          if(ch == FLAG)
          {
            n_chars = p_data - p_Rx_buff;
						if(n_chars)
						{
				    	// Get at least 2 chars and FCS should match
	            return  ( (n_chars > 2)  && 
								CRC16chk((unsigned char *)p_Rx_buff, n_chars) ) ? 
									(n_chars - 2) : 0;
						}
          }else if(ch == ESCAPE)
          {
            hdlc_state = ST_ESCAPE;
          }else
          {
            *p_data++ = ch;
          }
          break;
        case ST_ESCAPE:
          *p_data++ = ch;
          hdlc_state = ST_DATA;
          break;
      }
    }
    hdlc_state = ST_IDLE;
    return -1; 
}


char *p_data_retry;
int n_chars_retry;

void TxRetry()
{
	TxData(p_data_retry, n_chars_retry);
}


void TxData(char *p_data, int n_chars)
{
  unsigned int crc;

	p_data_retry = p_data;
	n_chars_retry = n_chars;
	
	DelayMs(TX_DELAY_MS);

	// Send 5 flags  
    WriteCharDS101(FLAG);
    WriteCharDS101(FLAG);
    WriteCharDS101(FLAG);
    WriteCharDS101(FLAG);
    WriteCharDS101(FLAG);

    CRC16ini();
    while(n_chars-- > 0)
    {
      char ch = *p_data++;
      if( (ch == FLAG) || (ch == ESCAPE) )
      {
        WriteCharDS101(ESCAPE);
      }
      WriteCharDS101(ch);
      CRC16nxt(ch);
    }
    // Send the CRC 16 now
    crc = CRC16crc();
    WriteCharDS101(crc & 0x00FF);
    WriteCharDS101((crc >> 8) & 0x00FF);
  	WriteCharDS101(FLAG);
}
