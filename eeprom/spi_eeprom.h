#ifndef __SPI_EEPROM_H__
#define __SPI_EEPROM_H__

// SPI functions

unsigned short long get_eeprom_address(unsigned char slot);
unsigned char get_eeprom_id(void);
unsigned char status_read(void); 
void status_write(unsigned char data); 

void byte_write(unsigned short long  address, 
                 unsigned char data); 
void array_write(unsigned short long address, 
                 unsigned char *wrptr,
                 unsigned int count); 

unsigned char byte_read(unsigned short long  address); 
void array_read(unsigned short long  address, 
                 unsigned char *rdptr, 
                 unsigned int count); 

 #endif  	// __SPI_EEPROM_H__