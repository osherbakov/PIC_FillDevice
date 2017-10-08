#include <stdlib.h>
#include <string.h>
#include "config.h"
#include <CRC16.h>
#include <HDLC.h>
#include <DS101.h>
#include <delay.h>
#include <Fill.h>


#define TX_DELAY_MS	(50)

RX_STATE hdlc_state = ST_IDLE;

// Returns:
//   -1  - Timeout occured
//   0   - Aborted or incorrect FCS
//   >0  - Data received
int RxRS232Data(char *p_data)
{
  	int  symbol;
  	char ch;
  	int  n_chars;

  	set_timeout(RX_TIMEOUT1_RS);
    hdlc_state = ST_IDLE;

    while(1)
    {
      symbol = ReadCharDS101();
      if(symbol < 0)
      { 
        return -1;   
      }
  	  set_timeout(RX_TIMEOUT2_RS);
       
      ch = (char) symbol;   
      switch(hdlc_state)
      {
        case ST_IDLE:
          if (ch == FLAG) {
          	// Do nothing - skip FLAG chars
          }else if(ch == ESCAPE) {
             hdlc_state = ST_ESCAPE;
          }else{
            *p_data++ = ch;
			n_chars++;
            hdlc_state = ST_DATA;
          }
          break;
        case ST_DATA:
          if(ch == FLAG){
            hdlc_state = ST_IDLE;
	    	// Get at least 2 chars
            return  n_chars;
          }else if(ch == ESCAPE){
            hdlc_state = ST_ESCAPE;
          }else{
            *p_data++ = ch;
			n_chars++;
          }
          break;
        case ST_ESCAPE:
          *p_data++ = ch ^ INV_BIT;
		  n_chars++;
          hdlc_state = ST_DATA;
          break;
      }
    }
    return -1; 
}



void TxRS232Data(char *p_data, int n_chars)
{

	DelayMs(TX_DELAY_MS);

	// Send 5 flags  
  	WriteCharDS101(FLAG);
  	WriteCharDS101(FLAG);
  	WriteCharDS101(FLAG);
  	WriteCharDS101(FLAG);
  	WriteCharDS101(FLAG);

  	while(n_chars-- > 0)
  	{
    	char ch = *p_data++;
    	if( (ch == FLAG) || (ch == ESCAPE) )
    	{
      		WriteCharDS101(ESCAPE);
			WriteCharDS101(ch ^ INV_BIT);
    	}else {
    		WriteCharDS101(ch);
		}

  	}
	WriteCharDS101(FLAG);
}
