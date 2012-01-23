#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <p18cxxx.h>
#define	MHZ	*1
#define XTAL_FREQ	16MHZ

#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#define MIN(a,b)  ((a) < (b) ? (a) : (b))
#define _BV(bit)  (1 << (bit))
#define NUM_ELEMS(a) (sizeof((a))/sizeof((a)[0]))

#ifndef byte
typedef unsigned char byte;
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
	INPUT = 1
} TRISTATE_MODE;


// Pin definitions


// Audio/FILL connector
#define	PIN_B	PORTBbits.RB4
#define	PIN_C	PORTCbits.RC6
#define	PIN_D	PORTCbits.RC7
#define	PIN_E	PORTBbits.RB3
#define	PIN_F	PORTCbits.RC0

#define	TRIS_PIN_B	TRISBbits.RB4
#define	TRIS_PIN_C	TRISCbits.RC6
#define	TRIS_PIN_D	TRISCbits.RC7
#define	TRIS_PIN_E	TRISBbits.RB3
#define	TRIS_PIN_F	TRISCbits.RC0

#define	WPUB_PIN_B	WPUBbits.WPUB4
#define	WPUB_PIN_E	WPUBbits.WPUB3

// Key selection switch
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

// Rotary switch ports - the position is indicated
// by the grounding of the pin.
#define S1_8 PORTD
#define S9_16 PORTA

#define TRIS_S1_8 TRISD
#define TRIS_S9_16 TRISA

// Resistor Voltage Control
#define	VRD			PORTEbits.RE0
#define	TRIS_VRD	TRISEbits.RE0

// LED control
#define	LEDP	PORTEbits.RE1
#define	LEDN_R	PORTEbits.RE2

#define	TRIS_LEDP	TRISEbits.RE1
#define	TRIS_LEDN_R	TRISEbits.RE2
#define	LAT_LEDP	LATEbits.LATE1


// Zeroizing switch
#define	ZBR			PORTBbits.RB0
#define	TRIS_ZBR	TRISBbits.RB0
#define	WPUB_ZBR	WPUBbits.WPUB0

// OFFBR is used to control the strong GND
#define	OFFBR		PORTBbits.RB1
#define	TRIS_OFFBR	TRISBbits.RB1
#define	WPUB_OFFBR	WPUBbits.WPUB1

// Button press
#define	BTN			PORTBbits.RB2
#define	TRIS_BTN	TRISBbits.RB2
#define	WPUB_BTN	WPUBbits.WPUB2

// Software I2C bit-banging
#define  DATA_LOW()   TRISAbits.TRISA2 = 0  // define macro for data pin output
#define  DATA_HI()    TRISAbits.TRISA2 = 1  // define macro for data pin input
#define  DATA_PIN()   PORTAbits.RA2         // define macro for data pin

#define  CLOCK_LOW()  TRISAbits.TRISA3 = 0  // define macro for clock pin output
#define  CLOCK_HI()   TRISAbits.TRISA3 = 1  // define macro for clock pin input
#define  SCLK_PIN()   PORTAbits.RA3         // define macro for clock pin

// RTC 1 PULSE_PER_SEC pin
// Will generate Interrupt On Change (IOC)
#define  PIN_1PPS	PORTBbits.RB5		
#define  TRIS_1PPS	TRISBbits.RB5		
#define  IOC_1PPS	IOCBbits.IOCB5


// External SPI memory
#define	SPI_CS		PORTCbits.RC2
#define	SPI_SCK		PORTCbits.RC3
#define	SPI_SDI		PORTCbits.RC4
#define	SPI_SDO		PORTCbits.RC5
#define	TRIS_SPI_CS		TRISCbits.RC2
#define	TRIS_SPI_SCK	TRISCbits.RC3
#define	TRIS_SPI_SDI	TRISCbits.RC4
#define	TRIS_SPI_SDO	TRISCbits.RC5


// NMEA Serial GPS Data
#define	GPS_DATA		PIN_D
#define	TRIS_GPS_DATA	TRIS_PIN_D

// NMEA 1PPS GPS pulse
#define	GPS_1PPS		PIN_B
#define	TRIS_GPS_1PPS	TRIS_PIN_B

// Have quick pin
#define	HQ_PIN			PIN_E
#define	TRIS_HQ_PIN		TRIS_PIN_E

// For Have Quick operations we need ground on one of the Pins
// Pin B is selected as GND pin
// B1 port controls if the strong GND is applied
#define	ON_GND 			OFFBR
#define	TRIS_PIN_GND 	TRIS_PIN_B


// To communicate with PC we use EUSART
// To communicate with MBITR we do Soft UART

//  Here are assignments of the pins for MBITR
#define	 TxMBITR	(PIN_D)
#define	 TRIS_TxMBITR (TRIS_PIN_D)

#define	 RxMBITR	(PIN_C)
#define	 TRIS_RxMBITR (TRIS_PIN_C)

// To communicate with PC the following pins are used:
//  PIN_D - input, PIN_C - output
#define	 TxPC	(PIN_C)
#define	 TRIS_TxPC (TRIS_PIN_C)

#define	 RxPC	(PIN_D)
#define	 TRIS_RxPC (TRIS_PIN_D)

// To communicate with another DTD the following pins are used:
//  PIN_C - input, PIN_D - output
#define	 TxDTD	(PIN_D)
#define	 TRIS_TxDTD (TRIS_PIN_D)

#define	 RxDTD	(PIN_C)
#define	 TRIS_RxDTD (TRIS_PIN_C)



// To communicate with DS-101 @ 64Kbd the following pins are used:
//  Data+  PIN_B,  Data-  PIN_E
#define	 Data_P	(PIN_B)
#define	 TRIS_Data_P (TRIS_PIN_B)
#define	 WPUB_Data_P (WPUB_PIN_B)

#define	 Data_N	(PIN_E)
#define	 TRIS_Data_N (TRIS_PIN_E)
#define	 WPUB_Data_N (WPUB_PIN_E)


#endif	// __CONFIG_H__
