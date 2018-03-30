#include "config.h"
#include "crcmodel.h"
#include "delay.h"
#include "rtc.h"
#include "spi_eeprom.h"
#include "serial.h"
#include "Fill.h"
#include "controls.h"


static const byte key_ack = KEY_ACK;
/****************************************************************/
// Those are commands and responses that MBITR receives and sends
// When programmed from the PC
/****************************************************************/
// Command to request a serial number
static const byte SN_REQ[]		= {0x2F, 0x39, 0x38, 0x0D}; // "/98\n"	
static const byte SN_RESP[]		= {0x53, 0x4E, 0x20, 0x3D, 0x20, 0x00, // "SN = "
 0x31, 0x32, 0x33, 0x34, 0x35, 0x0D, 					// "12345\n"
 0x4F, 0x4B, 0x0D, 0x00 };								// "OK\n"

// The pattern of 2400 baud 0x7E flags as seen from 9600 baud view
static const byte HDLC_FLAGS1[] = {0x80, 0x78};
static const byte HDLC_FLAGS2[] = {0x78, 0x78};
static const byte HDLC_FLAGS3[] = {0x78, 0xF8};

// Command to request the radio capabilities
static byte OPT_REQ[] 		= {0x2F, 0x38, 0x34, 0x0D};	// "/84\n" ;

// Capabilities options
static const byte OPT_BASIC[]  		= "Basic ";
static const byte OPT_RETRAN[] 		= "Retransmit ";
static const byte OPT_SINC1[]  		= "Sincgar ";
static const byte OPT_SINC2[] 		= "SincFH2 ";
static const byte OPT_HQ2[] 		= "HqII ";
static const byte OPT_ANDVT[] 		= "Andvt ";
static const byte OPT_DES[] 		= "Des ";
static const byte OPT_AES[] 		= "Aes ";
static const byte OPT_LDRM[] 		= "LDRM ";
static const byte OPT_OPT_X[] 		= "Option X ";
static const byte OPT_MELP[] 		= "Andvt MELP ";
static const byte OPT_DIGITAL[] 	= "Clear Digital ";
static const byte OPT_GPS[] 		= "Enhanced GPS ";
static const byte OPT_END[] 		= {0x0D, 0x00};

static const byte OPT_ENABLED[] 	= "Radio Option Enabled";
static const byte OK_RESP[] 		= {0x4F, 0x4B, 0x0D};		// "OK\n"

// The list of all options enabled in the radio
static const byte *OPTS[] = 
{
	OPT_BASIC, OPT_RETRAN, OPT_SINC1, OPT_SINC2, OPT_HQ2, OPT_ANDVT,
	OPT_DES, OPT_AES, OPT_LDRM, OPT_OPT_X, OPT_MELP, OPT_DIGITAL, OPT_GPS
};

static unsigned char SerialBuffer[6];

static void send_options(void)
{
	byte idx;
	// Go thru the list of all options and send them out.
	//  Each option is terminated by 0x0D 0x00 sequence
	for( idx = 0; idx < NUM_ELEMS(OPTS); idx++)
	{
		tx_eusart_str(OPTS[idx]);
		tx_eusart_str(OPT_ENABLED);
		tx_eusart_buff(OPT_END);
	}
	// Terminate everything with "OK\n"
	tx_eusart_buff(OK_RESP);
   	tx_eusart_flush();
}

//
//		Functions to check for RS-232 MBITR Type 4, PC and DTD Type 5 fills
//
// Check serial port if there is a request to send DES keys
char CheckFillType4()
{
	if( !RCSTA1bits.SPEN)
	{
		// If RxPC (PIN_D) is LOW then maybe there is RS-232 connected
	  	if(pinRead(RxPC) == LOW)
	  	{
	      // Coming in first time - enable eusart and setup buffer
	  		open_eusart(BRREG_MBITR, DATA_POLARITY_RXTX);
	  		rx_eusart_async(SerialBuffer, 4, RX_TIMEOUT1_PC);
	  	}	
	}else if(rx_eusart_count() >= 4)
	{
    // Four characters are collected - check if it is 
    //  /98 - serial number request, or
    //  /84 - the capabilities request
		if( is_equal(SerialBuffer, SN_REQ, 4) )
		{
			rx_eusart_reset(); // Data consumed
  			// SN request - send a fake SN = 123456
			tx_eusart_buff(SN_RESP);
			tx_eusart_flush();
			set_timeout(RX_TIMEOUT1_PC);	// Restart timeout
		}else if(is_equal(SerialBuffer, OPT_REQ, 4))
		{
			rx_eusart_reset(); // Data consumed
			// Options request - send all options
			send_options();
			return MODE4;
		}
	}
	return ST_TIMEOUT;
}



// Check if there is the request from the PC to send DS-101/RS-232 keys
char CheckFillRS232Type5()
{
  	// Coming in first time - enable eusart and setup buffer
	if( !RCSTA1bits.SPEN )
	{
		// If RxPC (PIN_D) is LOW, then maybe there is RS-232 connected
	  	if(pinRead(RxPC) == LOW)
	  	{
	      // Coming in first time - enable eusart and setup buffer
	  		open_eusart(BRREG_PC, DATA_POLARITY_RXTX);
	  		rx_eusart_async(SerialBuffer, 4, RX_TIMEOUT1_PC);
	  	}	
	}else if( (rx_eusart_count() >= 2) && 	// The pattern of 2400 baud 0x7E flags as seen from 9600 baud view
			  (is_equal(SerialBuffer, HDLC_FLAGS1, 2) ||
			 	  is_equal(SerialBuffer, HDLC_FLAGS2, 2) ||
			  	  	is_equal(SerialBuffer, HDLC_FLAGS3, 2) ) )
	{
		 close_eusart();
		 return MODE5;
	}
	return ST_TIMEOUT;
}	

// Check if there is the request from the DTD to send DS-101/RS-232 keys
char CheckFillDTD232Type5()
{
	if( !RCSTA1bits.SPEN )
	{
	  	if(pinRead(RxDTD) == LOW)
	  	{
	  		 close_eusart();
	  		 return MODE5;
	  	}	
  	} 	
	return ST_TIMEOUT;
}	

// The Type5 RS485 fill is detected when PIN_P is different from PIN_N
char CheckFillRS485Type5()
{
	pinMode(Data_N, INPUT_PULLUP);
	pinMode(Data_P, INPUT_PULLUP);
	DelayMs(10);

	return ( pinRead(Data_P) != pinRead(Data_N) ) ? MODE5 : ST_TIMEOUT;
}	


// ACK the received key from PC
char SendPCAck(byte ack_type)
{
	tx_eusart_async(&key_ack, 1);			// ACK the previous packet
	tx_eusart_flush();
}

static byte GetPCFill(unsigned short long base_address)
{
	byte records, byte_cnt, record_size;
	unsigned short long saved_base_address;
	records = 0;
	record_size = 0;

	saved_base_address = base_address++;

	SendPCAck(REQ_FIRST);	// REQ the first packet
  	while(1)
	{
		byte_cnt = rx_eusart(&data_cell[0], FILL_MAX_SIZE, RX_TIMEOUT1_PC);
		// We can get byte_cnt
		//  = 0  - no data received --> finish everything
		//  == FILL_MAX_SIZE --> record and continue
		//  0 < byte_cnt < FILL_MAX_SIZE --> record and issue new request
		record_size += byte_cnt;
		if(record_size == 0)
		{
			break;	// No data provided - exit
		}
		if(byte_cnt)
		{
			array_write(base_address, &data_cell[0], byte_cnt);
			base_address += byte_cnt;
		}
		if(byte_cnt < FILL_MAX_SIZE)
		{
			byte_write(saved_base_address, record_size);
			records++; 
			record_size = 0;
			saved_base_address = base_address++;
			SendPCAck(REQ_NEXT);	// ACK the previous and REQ the next packet
		}
	}
	return records;
}


// Receive and store the fill data into the specified slot
char StorePCFill(byte stored_slot, byte required_fill)
{
	char result = ST_ERR;
	byte records;
	unsigned short long base_address;
	unsigned short long saved_base_address;
	base_address = get_eeprom_address(stored_slot & 0x0F);

	saved_base_address = base_address;	
	base_address += 2;	// Skip the fill_type and records field
						// .. to be filled at the end
	// All data are stored in 1K bytes (8K bits) slots
	// The first byte of the each slot has the number of the records (0 - 255)
	// The first byte of the record has the number of bytes that should be sent out
	// so each record has no more than 256 bytes
	// Empty slot has first byte as 0x00 or 0xFF
	
	records = GetPCFill(base_address);
	if( records > 0)
	{
  		// All records were received - put final info into EEPROM
	  	// Mark the slot as valid slot containig data
		byte_write(saved_base_address, records);
		byte_write(saved_base_address + 1, required_fill);
		result = ST_DONE;
	}
  	return result;
}
