#include "config.h"
#include "delay.h"
#include "i2c_sw.h"

// We assume that 
// 	DATA_LAT = 0;
//	SCLK_LAT = 0;
// if not - set it up somewhere globally

/********************************************************************
*     Function Name:    void SWStartI2C(void)                       *
*     Return Value:     void                                        *
*     Parameters:       void                                        *
*     Description:      Send I2C bus start condition.               *
********************************************************************/
void SWStartI2C( void )
{
  I2C_DATA_HI();                        // release data pin to float high
  DelayI2C();                   // user may need to modify based on Fosc
  I2C_CLOCK_HI();                       // release clock pin to float high
  DelayI2C();                   // user may need to modify based on Fosc  
  I2C_DATA_LOW();                       // set pin to output to drive low
  DelayI2C();                   // user may need to modify based on Fosc
}
/********************************************************************
*     Function Name:    void SWStopI2C(void)                        *
*     Return Value:     void                                        *
*     Parameters:       void                                        *
*     Description:      Send I2C bus stop condition.                *
********************************************************************/
void SWStopI2C( void )
{
  I2C_CLOCK_LOW();                      // set clock pin to output to drive low
  I2C_DATA_LOW();                       // set data pin output to drive low
  DelayI2C();                   // user may need to modify based on Fosc
  I2C_CLOCK_HI();                       // release clock pin to float high
  DelayI2C();                   // user may need to modify based on Fosc
  I2C_DATA_HI();                        // release data pin to float high
  Delay1TCY();                    // user may need to modify based on Fosc
  Delay1TCY();
}

/********************************************************************
*     Function Name:    void SWRestartI2C(void)                     *
*     Return Value:     void                                        *
*     Parameters:       void                                        *
*     Description:      Send I2C bus restart condition.             *
********************************************************************/
void SWRestartI2C( void )
{
  I2C_CLOCK_LOW();                      // set clock pin to output to drive low
  I2C_DATA_HI();                        // release data pin to float high
  DelayI2C();                   // user may need to modify based on Fosc
  I2C_CLOCK_HI();                       // release clock pin to float high
  DelayI2C();                   // user may need to modify based on Fosc
  I2C_DATA_LOW();                       // set data pin output to drive low
  DelayI2C();                   // user may need to modify based on Fosc
}

/********************************************************************
*     Function Name:    signed char SWAckI2C(void)                  *
*     Return Value:     error condition status                      *
*     Parameters:       void                                        *
*     Description:      This function generates a bus acknowledge   *
*                       sequence.                                   *
********************************************************************/
signed char SWAckI2C(char ack)
{
  I2C_CLOCK_LOW();                  // set clock pin to output to drive low
  if ( ack ) {                	// initiate  ACK
    I2C_DATA_HI();                  // release data line to float high 
  }else {                       // else initiate ACK condition
    I2C_DATA_LOW();                 // make data pin output to drive low
  }
  DelayI2C();                   // user may need to modify based on Fosc
  I2C_CLOCK_HI();                  	// release clock line to float high 
  DelayI2C();                   // user may need to modify based on Fosc
  ack = I2C_DATA_PIN();	  
  if ( ack ) {                 	// error if ack = 1, slave did not ack
    return ( STAT_NO_ACK  );  	// return with acknowledge error
  }else {
    return ( STAT_OK );         // return with no error
  }
}

/**********************************************************************
*     Function Name:    signed char SWWriteI2C(unsigned char data_out)*
*     Return Value:     error condition if bus error occurred         *
*     Parameters:       Single data byte for I2C bus.                 *
*     Description:      This routine writes a single byte to the      *
*                       I2C bus.                                      *
**********************************************************************/
signed char SWWriteI2C( unsigned char data_out )
{
  unsigned char I2C_BUFFER;    // temp buffer for R/W operations
  unsigned char BIT_COUNTER;   // temp buffer for bit counting
  
  I2C_BUFFER = data_out;
  BIT_COUNTER = 8;                // initialize bit counter
  
  I2C_CLOCK_HI();						// Release the SCL line                         
  do
  {
    if ( !I2C_SCLK_PIN() )              // test if clock is low
    {                          		// if it is then ..
	    DelayI2C();				    // user may need to adjust timeout period
	    if ( !I2C_SCLK_PIN() )          // if clock is still low after wait 
	      return ( STAT_CLK_ERR );  // return with clock error
    }else {
      	I2C_CLOCK_LOW();                // set clock pin output to drive low
      	if ( I2C_BUFFER & 0x80 )   	// if MSB set, transmit out logic 1
      	{
       		I2C_DATA_HI();              // release data line to float high 
      	}else                      	// transmit out logic 0
      	{
        	I2C_DATA_LOW();             // set data pin output to drive low 
      	}
     	DelayI2C();              	// user may need to modify based on Fosc
     	I2C_CLOCK_HI();             // release clock line to float high 
     	DelayI2C();              	// user may need to modify based on Fosc

     	I2C_BUFFER <<= 1;
     	BIT_COUNTER --;              // reduce bit counter by 1
    }
  }while ( BIT_COUNTER );        // stay in transmit loop until byte sent 

  return ( STAT_OK );             // return with no error
}



/********************************************************************
*     Function Name:    signed char SWPutsI2C(unsigned char *wrptr) *
*     Return Value:     end of string indicator or bus error        *
*     Parameters:       address of write string storage location    *
*     Description:      This routine writes a string to the I2C bus.*
********************************************************************/
signed char SWPutsI2C( unsigned char *wrptr, unsigned char length )
{
  while ( length-- )
  {               
    if ( SWWriteI2C( *wrptr++) ) {   // write out data string to I2C receiver
      	return( STAT_CLK_ERR );      // return if there was an error in Putc()
    }else{
      	if ( SWAckI2C(NOACK) ) {          // go read bus ack status
        	return( STAT_NO_ACK );       // return with possible error condition 
      	}
    }
  }                              
  return ( STAT_OK );                  // return with no error
}


/********************************************************************
*     Function Name:    unsigned int SWReadI2C(void)                *
*     Return Value:     data byte or error condition                *
*     Parameters:       void                                        *
*     Description:      Read single byte from I2C bus.              *
********************************************************************/
unsigned int SWReadI2C()
{
  unsigned char I2C_BUFFER;    // temp buffer for R/W operations
  unsigned char BIT_COUNTER;   // temp buffer for bit counting

  BIT_COUNTER = 8;                // set bit count for byte 
  do
  {
    I2C_CLOCK_LOW();                    // set clock pin output to drive low
    I2C_DATA_HI();                      // release data line to float high
    DelayI2C();                 	// user may need to modify based on Fosc
    I2C_CLOCK_HI();                     // release clock line to float high
    Delay1TCY();                  	// user may need to modify based on Fosc
    Delay1TCY();

    if ( !I2C_SCLK_PIN() ) {            // test for clock low
	  	DelayI2C();					// user may need to adjust timeout period
	  	if ( !I2C_SCLK_PIN() )              // if clock is still low after wait 
	    	return ( STAT_CLK_ERR);     // return with clock error
    }

    I2C_BUFFER <<= 1;             // shift composed byte by 1
    if ( I2C_DATA_PIN() )               // is data line high
     	I2C_BUFFER |= 0x01;          // set bit 0 to logic 1
  } while ( --BIT_COUNTER );      // stay until 8 bits have been acquired

  return ( (unsigned int) I2C_BUFFER ); // return with data
}

/********************************************************************
*     Function Name:    signed char SWGetsI2C(unsigned char *rdptr, *
*                                             unsigned char length) *
*     Return Value:     return with error condition                 *
*     Parameters:       address of read string storage location and *
*                       length of string bytes to read              *
*     Description:      This routine reads a string from the I2C    *
*                       bus.                                        *
********************************************************************/

signed char SWGetsI2C( unsigned char *rdptr,  unsigned char length )
{
  unsigned int data;             // define auto variable

  while ( length --)              // stay in loop until byte count is zero
  {
    data = SWReadI2C();          // read and save 1 byte
    if ( data & 0xFF00 ){
      return ( STAT_CLK_ERR );    // return with error condition
    }else {
      *rdptr++ = data;           // save off byte read
    }

    I2C_CLOCK_LOW();                  // make clock pin output to drive low
    if ( !length ) {              // initiate NOT ACK
      	I2C_DATA_HI();                // release data line to float high 
    }else {                       // else initiate ACK condition
      	I2C_DATA_LOW();               // make data pin output to drive low
    }
    DelayI2C();                 	// user may need to modify based on Fosc
    I2C_CLOCK_HI();                   	// release clock line to float high 
    DelayI2C();                 	// user may need to modify based on Fosc
  }   
  return( STAT_OK);             // return with no error
}

