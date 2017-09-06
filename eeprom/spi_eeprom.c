#include "config.h"
#include "delay.h"
#include "controls.h"
#include "spi_eeprom.h"
#include <spi.h>

void busy_polling(void);

unsigned char EEPROM_ADDRESS_BITS = 17;
unsigned char  EEPROM_ADDRESS_BYTES = 3;
unsigned short long EEPROM_PAGE_SIZE = 256;
unsigned short long EEPROM_PAGE_MASK = 0x0000FF;
unsigned char KEY_MAX_SIZE_PWR	= 13; //(EEPROM_ADDRESS_BITS - 4);	// Max_size = 2^(N)


unsigned short long get_eeprom_address(unsigned char slot)
{
	return ((unsigned short long)(slot & 0x0F)) << KEY_MAX_SIZE_PWR;
}


void prepare_write(unsigned short long  address)
{
	  unsigned char cnt, bit_shift;
	  busy_polling(); 
	  SPI_CS = 0;               //assert chip select 
	  putcSPI(SPI_WREN);       	//send write enable command 
	  SPI_CS = 1;               //negate chip select 
	  SPI_CS = 0;               //assert chip select 
	  putcSPI(SPI_WRITE);       //send write command 
	  
	  for ( cnt = 0; cnt < EEPROM_ADDRESS_BYTES; cnt++)
	  {
		  bit_shift = (EEPROM_ADDRESS_BYTES - 1 - cnt) * 8;
		  putcSPI(address >> bit_shift);  	// send starting from 
		  																// the high byte of address 
	  }
} 

void prepare_read(unsigned short long  address)
{
	  unsigned char cnt, bit_shift;
	  busy_polling(); 
	  SPI_CS = 0;               //assert chip select 
	  putcSPI(SPI_READ);       //send write command 
	  
	  for ( cnt = 0; cnt < EEPROM_ADDRESS_BYTES; cnt++)
	  {
		  bit_shift = (EEPROM_ADDRESS_BYTES - 1 - cnt) * 8;
		  putcSPI(address >> bit_shift);  	// send starting from 
											// the high byte of address 
	  }
} 

void array_write(unsigned short long  address, 
                 unsigned char *wrptr,
                 unsigned int count) 
{ 
  unsigned int  byte_count;
  while (count)
  {
	  prepare_write(address);
	  byte_count = MIN(count, EEPROM_PAGE_SIZE - (address & EEPROM_PAGE_MASK));
	  address += byte_count;
	  count -= byte_count;
	  while(byte_count--)
	  {
      putcSPI(*wrptr++);  	//send data byte 
	  }
	  SPI_CS = 1;             //negate chip select 
  }
} 

void array_read (unsigned short long address, 
                    unsigned char *rdptr, 
                    unsigned int count) 
{ 
  prepare_read(address); 		//send address 
  while(count--)
  {
     *rdptr++ = getcSPI();			// read data byte and place it in array
  }
  SPI_CS = 1; 
} 

void byte_write (unsigned short long address, 
                    unsigned char data) 
{ 
  prepare_write(address);
  putcSPI(data);           	//send data byte 
  SPI_CS = 1;               //negate chip select 
} 

unsigned char byte_read (unsigned short long address) 
{ 
  unsigned char var;
  prepare_read(address);	 //send bytes of address 
  var = getcSPI();         //read single byte 
  SPI_CS = 1; 
  return (var); 
} 

unsigned char status_read () 
{ 
  unsigned char var;
  SPI_CS = 0;               //assert chip select 
  putcSPI(SPI_RDSR); 		//send read status command 
  var = getcSPI();         	//read data byte 
  SPI_CS = 1;               //negate chip select 
  return (var); 
} 

void status_write (unsigned char data) 
{ 
  SPI_CS = 0;               //assert chip select 
  putcSPI(SPI_WREN);       	//send write enable command 
  SPI_CS = 1;               //negate chip select 
  SPI_CS = 0; 
  putcSPI(SPI_WRSR); 		//write status command 
  putcSPI(data);         	//status byte to write 
  SPI_CS = 1;               //negate chip select 
} 

void busy_polling (void) 
{ 
  while (status_read() & 0x01) {}
} 

unsigned char get_eeprom_id()
{
	unsigned char chip_id;
  	SPI_CS = 0;               //assert chip select 
  	putcSPI(0xAB /*SPI_RDID*/ );       	//send Release from PD and get ID command 
  	putcSPI(0x00);       			//send dummy address 
  	putcSPI(0x00);
	putcSPI(0x00);
	chip_id = getcSPI();		
   	SPI_CS = 1;
	return chip_id;
}