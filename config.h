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
	INPUT_ANALOG = 4,
	IRQ_ON_CHANGE = 8
} PIN_MODE;


// Pin definitions
#define PIN(port, pin)		port##_##pin
#define	DATA(port, pin)		PORT##port##bits.R##port##pin
#define	TRIS(port, pin)		TRIS##port##bits.R##port##pin
#define	ANSEL(port, pin)	ANSEL##port##bits.ANS##port##pin
#define	WPU(port, pin)		WPU##port##bits.WPU##port##pin

#define PIN_B	PIN(B,1)
#define PIN_C	PIN(C,6)
#define PIN_D	PIN(C,7)
#define PIN_E	PIN(B,2)
#define PIN_F	PIN(C,0)
#define	LEDP  	PIN(E,1)
#define	ZBR		PIN(E,2)
#define	BTN		PIN(B,0)
#define	PIN_A_PWR	PIN(C,1)
#define	PIN_F_PWR	PIN(E,0)
#define	S16		PIN(A,7)

/*
enum {
	PIN_B,
	PIN_C,
	PIN_D,
	PIN_E,
	PIN_F,
	LEDP,
	ZBR,
	BTN,
	PIN_A_PWR,
	PIN_F_PWR,
	S16
} PINS;	
*/

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


#define PIN(port, pin)		port##_##pin
#define PORT(port)			PORT##port
#define	DATA(port, pin)		PORT##port##bits.R##port##pin
#define	TRIS(port, pin)		TRIS##port##bits.R##port##pin
#define	ANSEL(port, pin)	ANSEL##port##bits.ANS##port##pin
#define	WPU(port, pin)		WPU##port##bits.WPU##port##pin
#define	IOC(port, pin)		IOC##port##bits.IOC##port##pin

#define pinRead(pin) 			DATA_##pin
#define pinWrite(pin, value) 	do{DATA_##pin = value;}while(0)


// Rotary switch ports - the position is indicated
// by the grounding of the pin.
#define S1_8 		PORT(D)
#define S9_16 		PORT(A)
// Registers that control the Data direction/Tristate for the ports
//#define TRIS_S1_8 	TRISD
//#define TRIS_S9_16 	TRISA


// Audio/FILL connector
#define DATA_PIN_B DATA(B,1)
#define DATA_PIN_C DATA(C,6)
#define DATA_PIN_D DATA(C,7)
#define DATA_PIN_E DATA(B,2)
#define DATA_PIN_F DATA(C,0)

#define	TRIS_PIN_B	TRIS(B,1)
#define	TRIS_PIN_C	TRIS(C,6)
#define	TRIS_PIN_D	TRIS(C,7)
#define	TRIS_PIN_E	TRIS(B,2)
#define	TRIS_PIN_F	TRIS(C,0)

#define	ANSEL_PIN_B	ANSEL(B,1)
#define	ANSEL_PIN_C	ANSEL(C,6)
#define	ANSEL_PIN_D	ANSEL(C,7)
#define	ANSEL_PIN_E	ANSEL(B,2)

#define	WPU_PIN_B	WPU(B,1)
#define	WPU_PIN_E	WPU(B,2)


// LED control Ports and pins
#define	DATA_LEDP  	DATA(E,1)
#define	TRIS_LEDP	TRIS(E,1)
#define	ANSEL_LEDP	ANSEL(E,1)



// Zeroizing switch
#define	DATA_ZBR	DATA(E,2)
#define	TRIS_ZBR	TRIS(E,2)
#define	ANSEL_ZBR	ANSEL(E,2)


// Button press
#define	DATA_BTN	DATA(B,0)
#define	TRIS_BTN	TRIS(B,0)
#define	ANSEL_BTN	ANSEL(B,0)
#define	WPUB_BTN	WPU(B,0)



// PIN_A_PWR is used to control the Pin A coonection
#define	DATA_PIN_A_PWR	DATA(C,1)
#define	TRIS_PIN_A_PWR	TRIS(C,1)
#define	ANSEL_PIN_A_PWR	ANSEL(C,1)

// PIN_F_PWR is used to control the Pin F power
#define	DATA_PIN_F_PWR	DATA(E,0)
#define	TRIS_PIN_F_PWR	TRIS(E,0)
#define	ANSEL_PIN_F_PWR	ANSEL(E,0)



// Software I2C bit-banging
#define  DATA_I2C_SDA	  DATA(B,4)
#define  TRIS_I2C_SDA	  TRIS(B,4)

#define  DATA_I2C_SCL	  DATA(B,3)
#define  TRIS_I2C_SCL	  TRIS(B,3)

#define  I2C_DATA_LOW()   do{pinWrite(I2C_SDA,0); TRIS_I2C_SDA = 0;}while(0) 	// define macro for data pin output
#define  I2C_DATA_HI()    do{TRIS_I2C_SDA = 1;}while(0)                   	// define macro for data pin input
#define  I2C_DATA_PIN()   pinRead(I2C_SDA)     									// define macro for data pin reading

#define  I2C_CLOCK_LOW()  do{pinWrite(I2C_SCL, 0); TRIS_I2C_SCL = 0;}while(0)  // define macro for clock pin output
#define  I2C_CLOCK_HI()   do{TRIS_I2C_SCL = 1;}while(0)                     // define macro for clock pin input
#define  I2C_SCLK_PIN()   pinRead(I2C_SCL)									// define macro for clock pin reading

// RTC 1 PULSE_PER_SEC pin
// Will generate Interrupt On Change (IOC)
#define  DATA_RTC_1PPS		DATA(B,5)
#define  TRIS_RTC_1PPS		TRIS(B,5)
#define  ANSEL_RTC_1PPS		ANSEL(B,5)
#define  IOC_RTC_1PPS		IOC(B,5)


// External SPI memory
#define	DATA_SPI_CS		DATA(C,2)
#define	TRIS_SPI_CS		TRIS(C,2)
#define	ANSEL_SPI_CS	ANSEL(C,2)

#define	DATA_SPI_SCK	DATA(C,3)
#define	TRIS_SPI_SCK	TRIS(C,3)
#define	ANSEL_SPI_SCK	ANSEL(C,3)

#define	DATA_SPI_SDI	DATA(C,4)
#define	TRIS_SPI_SDI	TRIS(C,4)
#define	ANSEL_SPI_SDI	ANSEL(C,4)

#define	DATA_SPI_SDO	DATA(C,5)
#define	TRIS_SPI_SDO	TRIS(C,5)
#define	ANSEL_SPI_SDO	ANSEL(C,5)


// NMEA Serial GPS Data
#define	GPS_DATA		(PIN_D)
#define	DATA_GPS_DATA	(DATA_PIN_D)

// NMEA 1PPS GPS pulse
#define	GPS_1PPS		(PIN_E)
#define DATA_GPS_1PPS	(DATA_PIN_E)

// Have quick pin
#define	HQ_DATA			(PIN_B)
#define	DATA_HQ_DATA	(DATA_PIN_B)



// To communicate with PC we use EUSART
// To communicate with MBITR we do Soft UART

//  Here are assignments of the pins for MBITR
// PIN_C - Input, PIN_D - Output
#define	 TxMBITR		(PIN_D)
#define	 DATA_TxMBITR	(DATA_PIN_D)
#define	 RxMBITR		(PIN_C)
#define	 DATA_RxMBITR	(DATA_PIN_C)

// To communicate with PC the following pins are used:
//  PIN_D - input, PIN_C - output
#define	 TxPC			(PIN_C)
#define	 DATA_TxPC		(DATA_PIN_C)
#define	 RxPC			(PIN_D)
#define	 DATA_RxPC		(DATA_PIN_D)

// To communicate with another DTD the following pins are used:
//  PIN_C - input, PIN_D - output
#define	 TxDTD			(PIN_D)
#define	 DATA_TxDTD		(DATA_PIN_D)
#define	 RxDTD			(PIN_C)
#define	 DATA_RxDTD		(DATA_PIN_C)


// To communicate with DS-101 (RS-485) @ 64Kbd the following pins are used:
//  Data+  PIN_B,  Data-  PIN_E
#define	 Data_P			(PIN_B)
#define	 DATA_Data_P	(DATA_PIN_B)
#define	 Data_N			(PIN_E)
#define	 DATA_Data_N	(DATA_PIN_E)
#define	 WAKEUP			(PIN_C)

#endif	// __CONFIG_H__
