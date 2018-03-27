#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <p18cxxx.h>
#define	MHZ	*1
#define XTAL_FREQ	16MHZ

#ifndef MAX
#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)  ((a) < (b) ? (a) : (b))
#endif

#define _BV(bit)  (1 << (bit))
#define NUM_ELEMS(a) (sizeof((a))/sizeof((a)[0]))

#ifndef byte
typedef unsigned char byte;
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (!FALSE)
#endif

typedef enum
{
	NEG = 0,
	POS = 1
} EDGE;

typedef enum
{
	LOW = 0,
	HIGH = 1
} LEVEL;


typedef enum
{
	OUTPUT = 0,
	INPUT = 1,
	INPUT_PULLUP = 2,
	INPUT_PULLDOWN = 3,
	INPUT_ANALOG = 4
} PIN_MODE;


// Pin definitions


// Key selection switch
/* 
#define	S1	RD0	
#define	S2	RD1
#define	S3	RD2
#define	S4	RD3
#define	S5	RD4
#define	S6	RD5
#define	S7	RD6
#define	S8	RD7
#define	S9	RA0
#define	S10	RA1
#define	S11	RA2
#define	S12	RA3
#define	S13	RA4
#define	S14	RA5
#define	S15 RA6
#define	S16	RA7
*/

// Rotary switch ports - the position is indicated
// by the grounding of the pin.
#define S1_8 		PORTD
#define S9_16 		PORTA
// Registers that control the Data direction/Tristate for the ports
#define TRIS_S1_8 	TRISD
#define TRIS_S9_16 	TRISA


// Software I2C bit-banging
#define  I2C_DATA_LOW()   do{PORTBbits.RB4 = 0; TRISBbits.TRISB4 = 0;}while(0) 	// define macro for data pin output
#define  I2C_DATA_HI()    do{TRISBbits.TRISB4 = 1;}while(0)                   	// define macro for data pin input
#define  I2C_DATA_PIN()   PORTBbits.RB4     									// define macro for data pin reading

#define  I2C_CLOCK_LOW()  do{PORTBbits.RB3 = 0; TRISBbits.TRISB3 = 0;}while(0)  // define macro for clock pin output
#define  I2C_CLOCK_HI()   do{TRISBbits.TRISB3 = 1;}while(0)                     // define macro for clock pin input
#define  I2C_SCLK_PIN()   PORTBbits.RB3			      							// define macro for clock pin reading

// RTC 1 PULSE_PER_SEC pin
// Will generate Interrupt On Change (IOC)
#define  DATA_PIN_1PPS	PORTBbits.RB5		
#define  TRIS_1PPS		TRISBbits.RB5		
#define  ANSEL_1PPS		ANSELBbits.ANSB5		
#define  IOC_1PPS		IOCBbits.IOCB5


// External SPI memory
#define	DATA_SPI_CS		PORTCbits.RC2
#define	TRIS_SPI_CS		TRISCbits.RC2
#define	ANSEL_SPI_CS	ANSELCbits.ANSC2

#define	DATA_SPI_SCK	PORTCbits.RC3
#define	TRIS_SPI_SCK	TRISCbits.RC3
#define	ANSEL_SPI_SCK	ANSELCbits.ANSC3

#define	DATA_SPI_SDI	PORTCbits.RC4
#define	TRIS_SPI_SDI	TRISCbits.RC4
#define	ANSEL_SPI_SDI	ANSELCbits.ANSC4

#define	DATA_SPI_SDO	PORTCbits.RC5
#define	TRIS_SPI_SDO	TRISCbits.RC5
#define	ANSEL_SPI_SDO	ANSELCbits.ANSC5


// NMEA Serial GPS Data
#define	GPS_DATA		(PIN_D)

// NMEA 1PPS GPS pulse
#define	GPS_1PPS		(PIN_E)

// Have quick pin
#define	HQ_PIN			(PIN_B)



// To communicate with PC we use EUSART
// To communicate with MBITR we do Soft UART

//  Here are assignments of the pins for MBITR
// PIN_C - Input, PIN_D - Output
#define	 TxMBITR		(PIN_D)
#define	 RxMBITR		(PIN_C)

// To communicate with PC the following pins are used:
//  PIN_D - input, PIN_C - output
#define	 TxPC			(PIN_C)
#define	 RxPC			(PIN_D)

// To communicate with another DTD the following pins are used:
//  PIN_C - input, PIN_D - output
#define	 TxDTD			(PIN_D)
#define	 RxDTD			(PIN_C)


// To communicate with DS-101 (RS-485) @ 64Kbd the following pins are used:
//  Data+  PIN_B,  Data-  PIN_E
#define	 Data_P			(PIN_B)
#define	 Data_N			(PIN_E)
#define	 WAKEUP			(PIN_C)

#endif	// __CONFIG_H__
