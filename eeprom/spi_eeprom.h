#ifndef __SPI_EEPROM_H__
#define __SPI_EEPROM_H__

// SPI functions
unsigned char status_read(void); 
void status_write(unsigned char data); 

void byte_write(unsigned int  address, 
                 unsigned char data); 
void array_write(unsigned int  address, 
                 unsigned char *wrptr,
                 unsigned int count); 

unsigned char byte_read(unsigned int  address); 
void array_read(unsigned int  address, 
                 unsigned char *rdptr, 
                 unsigned int count); 

 #endif  	// __SPI_EEPROM_H__