#include "config.h"
#include "delay.h"
#include "controls.h"
#include "spi_eeprom.h"
#include <spi.h>

void busy_polling(void); 

void prepare_write(unsigned int  address)
{
	  busy_polling(); 
	  SPI_CS = 0;               //assert chip select 
	  putcSPI(SPI_WREN);       	//send write enable command 
	  SPI_CS = 1;               //negate chip select 
	  SPI_CS = 0;               //assert chip select 
	  putcSPI(SPI_WRITE);       //send write command 
	  putcSPI(address >> 8);   	//send high byte of address 
  	  putcSPI(address); 	    //send low byte of address 
} 

void array_write(unsigned int  address, 
                 unsigned char *wrptr,
                 unsigned int count) 
{ 
  unsigned char  byte_count;
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
	  SPI_CS = 1;               //negate chip select 
  }
} 

void array_read (unsigned int  address, 
                    unsigned char *rdptr, 
                    unsigned int count) 
{ 
  busy_polling(); 
  SPI_CS = 0;              	//assert chip select 
  putcSPI(SPI_READ); 		//send read command 
  putcSPI(address >> 8);   	//send high byte of address 
  putcSPI(address); 		//send low byte of address 
  while(count--)
  {
     *rdptr++ = getcSPI();			// read data byte and place it in array
  }
  SPI_CS = 1; 
} 

void byte_write (unsigned int  address, 
                    unsigned char data) 
{ 
  prepare_write(address);
  putcSPI(data);           	//send data byte 
  SPI_CS = 1;               //negate chip select 
} 

unsigned char byte_read (unsigned int  address) 
{ 
  unsigned char var;
  busy_polling(); 
  SPI_CS = 0;              //assert chip select 
  putcSPI(SPI_READ);       //send read command 
  putcSPI(address >> 8);   //send high byte of address 
  putcSPI(address);	 	   //send low byte of address 
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
