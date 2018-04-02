#include "config.h"
#include "delay.h"
#include "HDLC.h"
#include "Fill.h"
#include "controls.h"
#include "serial.h"

#define TIMER_DTD 			(((XTAL_FREQ * 1000000L)/(4L * 16L * DTD_BAUDRATE)) - 1 )
#define TIMER_DTD_START 	( -(TIMER_DTD/2) )

#define TIMER_DTD_CTRL 		( (1<<2) | 2)     // ENA, 1:16

#define TIMER_DS101 		( ((64L * 1000000L) / ( 4L * DS101_BAUDRATE)) - 1 )

#define TIMER_DS101_CTRL 	( (1<<2) | 0)   // ENA, 1:1


void OpenRS232(char Master)
{
  	close_eusart();
	pinMode(RxPC, INPUT);
	pinMode(TxPC, OUTPUT);
	pinWrite(TxPC, 0);
  	if(Master) {
		DelayMs(500);		// Keep pins that way for the slave to detect condition
    }	
}

void CloseRS232()
{
	pinMode(RxPC, INPUT);
	pinMode(TxPC, INPUT);
}

//  Simulate UARTs for DTD RS-232 and DS-101 RS-485 communications
//  TMR6 is used to count bits in Tx and Rx
//
// Soft UART to communicate with RS232 port
// Returns:
//  -1  - if no symbol within timeout
//  >=0 - if symbol was detected 
//
// We DO NOT disable interrupts here due to slow (2400Baud) speed
//
static 	byte bitcount, data;

int RxRS232Char()
{
	int  result;

	pinMode(RxPC, INPUT);

	PR6 = TIMER_DTD;
	T6CON = TIMER_DTD_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag
      	
	result = -1;
  	
  	while( is_not_timeout() )
	{
		// Start conditiona was detected - count 1.5 cell size	
		if(pinRead(RxPC) )
		{
			TMR6 = TIMER_DTD_START;
			PIR5bits.TMR6IF = 0;	// Clear overflow flag
			for(bitcount = 0; bitcount < 8 ; bitcount++)
			{
				// Wait until timer overflows
				while(!PIR5bits.TMR6IF){} ;
				PIR5bits.TMR6IF = 0;	// Clear overflow flag
				data = (data >> 1) | (pinRead(RxPC) ? 0x00 : 0x80);
			}
 			while(!PIR5bits.TMR6IF){} ;	// Wait for stop bit
			result =  pinRead(RxPC) ? -1 : ((int)data) & 0x00FF;
			break;
		}
	}

	return result;
}


void TxRS232Char(char data)
{
	data = ~data;  			// Invert sysmbol

	pinMode(TxPC, OUTPUT);

	PR6 = TIMER_DTD;
	T6CON = TIMER_DTD_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag

	pinWrite(TxPC, 1) ;        		// Issue the start bit
 	// send 8 data bits and 3 stop bits
	for(bitcount = 0; bitcount < 12; bitcount++)
	{
		while(!PIR5bits.TMR6IF) {/* wait until timer overflow bit is set*/};
		PIR5bits.TMR6IF = 0;	// Clear timer overflow bit
		pinWrite(TxPC, data & 0x01);		// Set the output
		data >>= 1;				// We use the fact that 
						        // "0" bits are STOP bits
	}
} 



void OpenDTD(char Master)
{
  	close_eusart();
	pinMode(RxDTD, INPUT);
	pinMode(TxDTD, OUTPUT);
	pinWrite(TxDTD, 0);
  	if(Master) {
		DelayMs(500);		// Keep pins that way for the slave to detect condition
    }	
}

void CloseDTD()
{
	pinMode(RxDTD, INPUT);
	pinMode(TxDTD, INPUT);
}


//
// Soft UART to communicate with another DTD
// Returns:
//  -1  - if no symbol within timeout
//  >=0 - if symbol was detected 
//
// We DO NOT disable interrupts here due to slow (2400Baud) speed
//
int RxDTDChar()
{
	int		result;

	pinMode(RxDTD, INPUT);

	PR6 = TIMER_DTD;
	T6CON = TIMER_DTD_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag
      	
	result = -1;
  	while( is_not_timeout() )
	{
		// Start conditiona was detected - count 1.5 cell size	
		if(pinRead(RxDTD) )
		{
			TMR6 = TIMER_DTD_START;
			PIR5bits.TMR6IF = 0;	// Clear overflow flag
			for(bitcount = 0; bitcount < 8 ; bitcount++)
			{
				// Wait until timer overflows
				while(!PIR5bits.TMR6IF){} ;
				PIR5bits.TMR6IF = 0;	// Clear overflow flag
				data = (data >> 1) | (pinRead(RxDTD) ? 0x00 : 0x80);
			}
			while(!PIR5bits.TMR6IF){} ; // Wait for stop bit
			result = pinRead(RxDTD) ? -1 : ((int)data) & 0x00FF;
			break;
		}
	}
	return result;
}


void TxDTDChar(char data)
{
	data = ~data;  			// Invert sysmbol
	
	pinMode(TxDTD, OUTPUT);

	PR6 = TIMER_DTD;
	T6CON = TIMER_DTD_CTRL;
	TMR6 = 0;
	PIR5bits.TMR6IF = 0;	// Clear overflow flag

	pinWrite(TxDTD, 1);        		// Issue the start bit
   	// send 8 data bits and 3 stop bits
	for(bitcount = 0; bitcount < 12; bitcount++)
	{
		while(!PIR5bits.TMR6IF) {	/* wait until timer overflow bit is set*/};
		PIR5bits.TMR6IF = 0;		// Clear timer overflow bit
		pinWrite(TxDTD, data & 0x01);	// Set the output
		data >>= 1;					// We use the fact that 
						        	// "0" bits are STOP bits
	}
} 

void OpenRS485(char Master)
{
  	OSCTUNEbits.PLLEN = 1;    	// *4 PLL (64MHZ)
	DelayMs(4 * 10);			// Wait for PLL to become stable

	if(Master) {
		pinMode(Data_N, OUTPUT);
		pinMode(Data_P, OUTPUT);
		pinWrite(Data_N, LOW);		
		pinWrite(Data_P, HIGH);		
		DelayMs(1000);			// Keep pins that way for the slave to detect condition
		DelayMs(1000);			// Keep pins that way for the slave to detect condition
	}else {
		pinMode(Data_N, INPUT_PULLUP);
		pinMode(Data_P, INPUT_PULLUP);
	}		
}

void CloseRS485()
{
	pinMode(Data_N, INPUT_PULLUP);
	pinMode(Data_P, INPUT_PULLUP);

  	OSCTUNEbits.PLLEN = 0;    	// No PLL (16MHZ)
	DelayMs(10);				// Wait for the clock to become stable
}

// DS-101 64000bps Differential Manchester/Bi-phase coding
typedef enum {
	INIT = 0,
	GET_EDGE,
	GET_FLAG_EDGE,
	GET_SAMPLE_EDGE,
	GET_DATA_EDGE,
	SEND_START_FLAG,
	SEND_DATA,
	SEND_END_FLAG,
	DONE,
	TIMEOUT
} HDLC_STATES;



#define TIMER_CTRL			(TIMER_DS101_CTRL)
#define PERIOD_CNTR   		(TIMER_DS101)
#define HALF_PERIOD_CNTR   	(PERIOD_CNTR/2)
#define QTR_PERIOD_CNTR   	(PERIOD_CNTR/4)
#define EIGTH_PERIOD_CNTR   (PERIOD_CNTR/8)
#define SAMPLE_CNTR 		(HALF_PERIOD_CNTR + EIGTH_PERIOD_CNTR)	// PERIOD_CNTR * 0.625 ( 0.5 + 0.125)

#define 	Timer_Period	(PR6)
#define 	Timer_Counter	(TMR6)
#define 	Timer_Ctrl		(T6CON)
#define 	TimerFlag		(PIR5bits.TMR6IF)

#define 	PIN_IN			Data_P
#define 	DATA_PIN_IN		DATA_Data_P
#define 	TRIS_PIN_IN		TRIS_Data_P
#define 	ANSEL_PIN_IN	ANSEL_Data_P
#define 	WPU_PIN_IN		WPU_Data_P

static		HDLC_STATES			st;
static		int 				byte_count;
static		unsigned char		bit_count;
static		unsigned char		stuff_count;
static		unsigned char		rcvd_byte;

static		unsigned char		timeout_cntr;
static		unsigned char		flag_detected;
	
static		unsigned char		rcvd_Sample;		// The current Sample at 0.75T
static		unsigned char		prev_Sample;		// The Sample at the 0.0T

static		byte 				prevIRQ;

#define		TIMEOUT_CNT			(15)

int RxRS485Data(char *pData)
{
	// Take care of physical pins
	pinMode(Data_P, INPUT);
	pinMode(Data_N, INPUT);
	
	st = INIT;
	Timer_Ctrl 		= TIMER_CTRL;
      	
    DISABLE_IRQ(prevIRQ);

	// Set the big 16-bit timeout counter
  	T1GCONbits.TMR1GE = 0;	// No gating
  	TMR1H = 0;
  	TMR1L = 0;				// Reset the timer
	PIR1bits.TMR1IF = 0;	// Clear Flag

	// Start the timer to sample at 0.625T
	Timer_Period	= SAMPLE_CNTR;	

	while(!PIR1bits.TMR1IF) {

		switch(st) {
		  case INIT:
			bit_count 		= 0;
			byte_count		= 0;
			rcvd_byte 		= 0;
			stuff_count 	= 0;
			flag_detected 	= 0;
			timeout_cntr	= 0;

			prev_Sample = rcvd_Sample = pinRead(PIN_IN);
	
			st = GET_EDGE;
		 	break;

		// Initial state - we are looking for any edge within big timeout
		case GET_EDGE:	
			if(rcvd_Sample)
				{while(pinRead(PIN_IN) && !PIR1bits.TMR1IF){}} 
			else
				{while(!pinRead(PIN_IN) && !PIR1bits.TMR1IF) {}}
			Timer_Counter 	= 0;
			TimerFlag 		= 0;			// Clear overflow flag

			// Save the value at the edge
			prev_Sample = pinRead(PIN_IN);

			if(PIR1bits.TMR1IF) {
				if(timeout_cntr++ >= TIMEOUT_CNT) {
					st = TIMEOUT;
				}else {				// Extend the BIG Timeout
				  	TMR1H = 0;
				  	TMR1L = 0;				// Reset the timer
					PIR1bits.TMR1IF = 0;	// Clear Flag
				}	
			}else {
			  	TMR1H = 0;
			  	TMR1L = 0;				// Reset the timer
				PIR1bits.TMR1IF = 0;	// Clear Flag
				st = GET_FLAG_EDGE;
			}
			
			while(!TimerFlag) {}
			rcvd_Sample	= pinRead(PIN_IN);
			break;
		
		// State to catch first HDLC Flag (0x7E) to achieve frame sync
		// We exit it when we detect First HDLC flag
		case GET_FLAG_EDGE:	
			if(rcvd_Sample){while(pinRead(PIN_IN)){}} else{while(!pinRead(PIN_IN)) {}}
			Timer_Counter 	= 0;
			TimerFlag 		= 0;			// Clear overflow flag
				
			rcvd_byte >>= 1; 
			if(rcvd_Sample == prev_Sample) {
				rcvd_byte |= 0x80;
			}

			// Save the value at the edge
			prev_Sample = pinRead(PIN_IN);

			if(rcvd_byte == FLAG) { 
				rcvd_byte = 0;
				bit_count = 0;
				stuff_count = 0;

				st = GET_SAMPLE_EDGE;
			}

			while(!TimerFlag) {}
			rcvd_Sample	= pinRead(PIN_IN);
			break;

		// State to handle HDLC Flags to achieve frame sync	
		// We exit it when we detect First non-HDLC flag symbol
		//  Here we also check for bit stuffing (if after 5 "1" comes "0" - discard it!
		case GET_SAMPLE_EDGE:
			if(rcvd_Sample){while(pinRead(PIN_IN)){}} else{while(!pinRead(PIN_IN)) {}}
			Timer_Counter 	= 0;
			TimerFlag 		= 0;			// Clear overflow flag
				
			// Beginning of bit stuffing detection and handling
			//  Every "0" after five consecutive "1" should be ignored 
			if(rcvd_Sample == prev_Sample){	// "1" bit is received
				rcvd_byte = (rcvd_byte >> 1) | 0x80;
				bit_count++;
				stuff_count++;				// Increment counter of consecutive "1"
				if(stuff_count > 5) {
					flag_detected = 1;	// More than 5 "1"s in a row - flag
				}	
			}else {							// "0" bit is received
				if(stuff_count != 5) {
					rcvd_byte >>= 1;
					bit_count++;
				}
				stuff_count = 0;
			}
			// End of bit stuffing handling

			// Save the value at the edge
			prev_Sample = pinRead(PIN_IN);

			// If the assembled byte is not FLAG - save it
			if(bit_count >= 8) {
				bit_count = 0;
				if(flag_detected && (rcvd_byte == FLAG)) {
				}else {
					*pData++ = rcvd_byte;
					byte_count++;
					st = GET_DATA_EDGE;
				}
				flag_detected = 0;
			}

			while(!TimerFlag) {}
			rcvd_Sample = pinRead(PIN_IN);
			break;

		// State to handle HDLC Data	
		// We exit it when we detect first HDLC flag symbol (or any symbol with 6 and more "1"s
		// We also handle bit stuffing here
		case GET_DATA_EDGE:
			if(rcvd_Sample){while(pinRead(PIN_IN)){}} else{while(!pinRead(PIN_IN)) {}}
			Timer_Counter 	= 0;
			TimerFlag 		= 0;	// Clear overflow flag

			// Beginning of bit stuffing detection and handling
			//  Every "0" after five consecutive "1" should be ignored 
			if(rcvd_Sample == prev_Sample){	// "1" bit is received
				rcvd_byte = (rcvd_byte >> 1) | 0x80;
				bit_count++;
				stuff_count++;				// Increment counter of consecutive "1"
				if(stuff_count > 5) {
					flag_detected = 1;	// More than 5 "1"s in a row - flag
				}	
			}else {							// "0" bit is received
				if(stuff_count != 5) {
					rcvd_byte >>= 1;
					bit_count++;
				}
				stuff_count = 0;
			}
			// End of bit stuffing handling

			// Save the value at the edge
			prev_Sample = pinRead(PIN_IN);

			// We got 6 "1" in a row - report as done
			if(flag_detected) {
				st = DONE;
			}else {
				if(bit_count >= 8) {
					bit_count = 0;				
					*pData++ = rcvd_byte;
					byte_count++;
				}
			}

			while(!TimerFlag) {}
			rcvd_Sample = pinRead(PIN_IN);
			break;
		
		case TIMEOUT:
    		ENABLE_IRQ(prevIRQ);
			return -1;
			break;

		case DONE:
    		ENABLE_IRQ(prevIRQ);
			return byte_count;
			break;
			
		default:
			break;
		}			
	}	
    ENABLE_IRQ(prevIRQ);
	return -1;
}	


#define		NUM_INITIAL_FLAGS	(5)
#define		NUM_FINAL_FLAGS		(1)

static		unsigned char		tx_byte;
static		unsigned char		next_bit;
static		unsigned char		bit_to_send;

void TxRS485Data(char *pData, int nBytes)
{

	st = INIT;

	// Take care of physical pins
	pinMode(Data_P, OUTPUT);
	pinMode(Data_N, OUTPUT);
	pinWrite(Data_P, 1);
	pinWrite(Data_N, 0);
	next_bit 		= 1;

	DelayMs(4 * 10);			// Little timeout

	Timer_Ctrl 		= TIMER_CTRL;

	DISABLE_IRQ(prevIRQ);
	
	while(1) {
		switch(st) {
		  case INIT:
			bit_count 	= 0;
			byte_count	= 0;
			stuff_count = 0;
			

			Timer_Period	= HALF_PERIOD_CNTR;
			Timer_Counter 	= 0;
			TimerFlag 		= 0;	// Clear overflow flag
			st 				= SEND_START_FLAG;
		 	break;

		  case SEND_START_FLAG:
		  	byte_count	 = NUM_INITIAL_FLAGS;

		  	while(byte_count > 0) {
			  	tx_byte = FLAG;
			  	bit_count = 8;
			  	while(bit_count > 0)
			  	{
					while(!TimerFlag) {}	// Start of the cell
			  		pinWrite(Data_P,next_bit);
					pinWrite(Data_N, !next_bit);
					TimerFlag = 0;

				  	next_bit = tx_byte & 0x01 ? next_bit : !next_bit;	// For "0" flip the bit, for 1" keep in in the middle of the cell
				  	tx_byte >>= 1;

					while(!TimerFlag) {}	// Middle of the cell
					pinWrite(Data_P, next_bit);
					pinWrite(Data_N, !next_bit);
					TimerFlag = 0;
					next_bit = !next_bit; 	// Always flip the bit at the beginning of the cell 
					bit_count--;
				}
				byte_count--;	
			}
			st = SEND_DATA;
		 	break;

		  case SEND_DATA:
		  	byte_count	 	= nBytes;
			stuff_count 	= 0;

		  	while(byte_count > 0) {
			  	tx_byte = *pData;
			  	bit_count = 8;
			  	while(bit_count > 0)
			  	{
					while(!TimerFlag) {}		// Wait for the next cell beginning
			  		pinWrite(Data_P, next_bit);
					pinWrite(Data_N, !next_bit);
					TimerFlag = 0;

					// Start calculating the next bit
					bit_to_send = tx_byte & 0x01;
			  		next_bit = bit_to_send ? next_bit : !next_bit;	// For "0" flip the bit, for 1" keep it the same in the middle of the cell
			  		tx_byte >>= 1;

					// Do bit stuffing calculation - after five consequtive "1"s ALWAYS insert "0"
					if(bit_to_send) {
						stuff_count++;			// Count "1"s 
					} else {
						stuff_count = 0;
					} 

					while(!TimerFlag) {}		// We are now at the middle of the cell
					pinWrite(Data_P, next_bit);
					pinWrite(Data_N, !next_bit);
					TimerFlag = 0;
					next_bit = !next_bit;	// Always flip the bit at the beginning of the cell 					

					// OK, the bit was sent, now 
					//  do the actual bit stuffing if needed - insert "0"
					if(stuff_count >= 5) {
						while(!TimerFlag) {}		// Wait for the next cell beginning
			  			pinWrite(Data_P,next_bit);
						pinWrite(Data_N,!next_bit);
						TimerFlag = 0;

						next_bit = !next_bit;		// Send "0"

						while(!TimerFlag) {}		// Wait for the middle of the cell
						pinWrite(Data_P, next_bit);
						pinWrite(Data_N, !next_bit);
						TimerFlag = 0;

						next_bit = !next_bit;	// Always flip the bit at the beginning of the cell 
						stuff_count = 0;
					}
					bit_count--;
				}	
				pData++;
				byte_count--;
			}
			st = SEND_END_FLAG;
		 	break;

		  case SEND_END_FLAG:
		  	byte_count	 = NUM_FINAL_FLAGS;

		  	while(byte_count > 0) {
			  	tx_byte = FLAG;
			  	bit_count = 8;
			  	while(bit_count > 0)
			  	{
					while(!TimerFlag) {}		// Wait for the cell start
			  		pinWrite(Data_P, next_bit);
					pinWrite(Data_N, !next_bit);
					TimerFlag = 0;

				  	next_bit = tx_byte & 0x01 ? next_bit : !next_bit;	// For "0" flip the bit, for 1" keep in in the middle of the cell
				  	tx_byte >>= 1;

					while(!TimerFlag) {}		// Wait for the middle of the cell
					pinWrite(Data_P, next_bit);
					pinWrite(Data_N, !next_bit);
					TimerFlag = 0;
					next_bit = !next_bit;	// Always flip the bit at the beginning of the cell 
					bit_count--;
				}	
				byte_count--;
			}
			// Handle the last cell
			while(!TimerFlag) {}		// Wait for the cell start
	  		pinWrite(Data_P, next_bit);
			pinWrite(Data_N, !next_bit);
			TimerFlag = 0;
			st = DONE;
		 	break;

		case DONE:
    		ENABLE_IRQ(prevIRQ);
			pinMode(Data_P, INPUT);
			pinMode(Data_N, INPUT);
			return;
			break;
		} 	
	}	
}	

