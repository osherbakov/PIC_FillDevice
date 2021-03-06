#include <stdlib.h>
#include <string.h>
#include "config.h"
#include <CRC16.h>
#include <HDLC.h>
#include <DS101.h>
#include <delay.h>
#include <Fill.h>


#define TX_DELAY_MS	(20)

typedef enum
{
    ST_IDLE,
    ST_DATA,
    ST_ESCAPE
}RX_STATE;

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

  	set_timeout(500);
    hdlc_state = ST_IDLE;
	n_chars = 0;

    while(1)
    {
      symbol = ReadCharDS101();
      if(symbol < 0)
      { 
        return -1;   
      }
  	  set_timeout(100);
       
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
            return  (n_chars >= 4) ?  n_chars : 0;
          }else if(ch == ESCAPE){
            hdlc_state = ST_ESCAPE;
          }else{
            *p_data++ = ch;
			n_chars++;
          }
          break;
        case ST_ESCAPE:
          *p_data++ = ch;
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
			WriteCharDS101(ch);
    	}else {
    		WriteCharDS101(ch);
		}

  	}
	WriteCharDS101(FLAG);
}
