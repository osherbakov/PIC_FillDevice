#include <stdlib.h>
#include <string.h>
#include "config.h"
#include <CRC16.h>
#include <HDLC.h>
#include <DS101.h>
#include <delay.h>
#include <Fill.h>

typedef enum
{
    ST_IDLE,
    ST_DATA,
    ST_ESCAPE
}RX_STATE;

RX_STATE hdlc_state = ST_IDLE;

static  	int  symbol;
static  	char ch;
static  	int  n_chars;

// Returns:
//   -1  - Timeout occured
//   0   - Aborted 
//   >0  - Data received
int RxRS232Data(char *p_data)
{
  	set_timeout(DS101_RX_TIMEOUT_MS);
    hdlc_state = ST_IDLE;
	n_chars = 0;

    while(is_not_timeout())
    {
      symbol = ReadCharDS101();
      if(symbol < 0)
      { 
        return -1;   
      }
  	  reset_timeout();
       
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


static unsigned char i;
void TxRS232Data(char *p_data, int n_chars)
{
	DelayMs(DS101_TX_DELAY_MS);

	// Send starting HDLC flags 
	for(i = 0; i < DS101_NUM_INITIAL_FLAGS; i++)
	{ 
  		WriteCharDS101(FLAG);
 	} 		
 
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
  	// Send finishing HDLC flags
	for(i = 0; i < DS101_NUM_FINAL_FLAGS; i++)
	{ 
  		WriteCharDS101(FLAG);
 	} 		
}
