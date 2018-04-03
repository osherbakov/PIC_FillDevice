#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <p18cxxx.h>
#define	MHZ	*1
#define XTAL_FREQ	16 MHZ

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
// #define PIN(port, pin)		port##_##pin
// #define PIN(port, pin)		port##_##pin
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


#define DATAPORT(port)		PORT##port
#define TRISPORT(port)		TRIS##port
#define ANSELPORT(port)		ANSEL##port
#define WPUPORT(port)		WPU##port

#define	DATA(port, pin)		PORT##port##bits.R##port##pin
#define	TRIS(port, pin)		TRIS##port##bits.R##port##pin
#define	ANSEL(port, pin)	ANSEL##port##bits.ANS##port##pin
#define	WPU(port, pin)		WPU##port##bits.WPU##port##pin
#define	IOC(port, pin)		IOC##port##bits.IOC##port##pin

#define portMode(port, mode) 	do{TRIS_##port = (mode == OUTPUT) ? 0x00 : 0xFF; \
								ANSEL_##port = (mode == INPUT_ANALOG) ? 0xFF : 0x00; \
								WPU_##port = (mode == INPUT_PULLUP) ? 0xFF : 0x00; \
								DATA_##port = (mode == INPUT_PULLUP) ? 0xFF : 0x00; \
								}while(0)
#define portWrite(port, value)	do{ DATA_##port = value;}while(0)
#define portRead(port) 			DATA_##port

#define pinRead(pin) 			DATA_##pin
#define pinWrite(pin, value) 	do{ DATA_##pin = value;}while(0)
#define pinMode(pin, mode) 		do{ TRIS_##pin = (mode == OUTPUT) ? 0 : 1; \
									ANSEL_##pin = (mode == INPUT_ANALOG) ? 1 : 0; \
									WPU_##pin = (mode == INPUT_PULLUP) ? 1 : 0; \
									DATA_##pin = (mode == INPUT_PULLUP) ? 1 : 0; \
								}while(0)

// Generic port settings
#define DATA_A		DATAPORT(A)
#define DATA_B		DATAPORT(B)
#define DATA_C		DATAPORT(C)
#define DATA_D		DATAPORT(D)
#define DATA_E		DATAPORT(E)

#define TRIS_A		TRISPORT(A)
#define TRIS_B		TRISPORT(B)
#define TRIS_C		TRISPORT(C)
#define TRIS_D		TRISPORT(D)
#define TRIS_E		TRISPORT(E)

#define ANSEL_A		ANSELPORT(A)
#define ANSEL_B		ANSELPORT(B)
#define ANSEL_C		ANSELPORT(C)
#define ANSEL_D		ANSELPORT(D)
#define ANSEL_E		ANSELPORT(E)

#define WPU_A		NO_WPU
#define WPU_B		WPUPORT(B)
#define WPU_C		NO_WPU
#define WPU_D		NO_WPU
#define WPU_E		NO_WPU

// Rotary switch ports - the position is indicated
// by the grounding of the pin.
#define DATA_S1_8 	DATAPORT(D)
#define DATA_S9_16 	DATAPORT(A)
// Registers that control the Data direction/Tristate for the ports
#define TRIS_S1_8 	TRISPORT(D)
#define TRIS_S9_16 	TRISPORT(A)
// Registers that control the Analog functions of the ports
#define ANSEL_S1_8 	ANSELPORT(D)
#define ANSEL_S9_16 ANSELPORT(A)
// Registers that control the Weak Pull Up functions of the ports
#define WPU_S1_8 	NO_WPU
#define WPU_S9_16 	NO_WPU

extern	byte	NO_WPU;
extern	byte	NO_ANSEL;

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
#define	ANSEL_PIN_F	NO_ANSEL

#define	WPU_PIN_B	WPU(B,1)
#define	WPU_PIN_C	NO_WPU
#define	WPU_PIN_D	NO_WPU
#define	WPU_PIN_E	WPU(B,2)
#define	WPU_PIN_F	NO_WPU


// LED control Ports and pins
#define	DATA_LEDP  	DATA(E,1)
#define	TRIS_LEDP	TRIS(E,1)
#define	ANSEL_LEDP	ANSEL(E,1)
#define	WPU_LEDP	NO_WPU



// Zeroizing switch
#define	DATA_ZBR	DATA(E,2)
#define	TRIS_ZBR	TRIS(E,2)
#define	ANSEL_ZBR	ANSEL(E,2)
#define	WPU_ZBR		NO_WPU


// Button press
#define	DATA_BTN	DATA(B,0)
#define	TRIS_BTN	TRIS(B,0)
#define	ANSEL_BTN	ANSEL(B,0)
#define	WPU_BTN		WPU(B,0)



// PIN_A_PWR is used to control the Pin A coonection
#define	DATA_PIN_A_PWR	DATA(C,1)
#define	TRIS_PIN_A_PWR	TRIS(C,1)
#define	ANSEL_PIN_A_PWR	NO_ANSEL
#define	WPU_PIN_A_PWR	NO_WPU

// PIN_F_PWR is used to control the Pin F power
#define	DATA_PIN_F_PWR	DATA(E,0)
#define	TRIS_PIN_F_PWR	TRIS(E,0)
#define	ANSEL_PIN_F_PWR	ANSEL(E,0)
#define	WPU_PIN_F_PWR	NO_WPU


#define	DATA_S16	DATA(A,7)
#define	TRIS_S16	TRIS(A,7)
#define	ANSEL_S16	NO_ANSEL
#define	WPU_S16		NO_WPU



// Software I2C bit-banging
#define  DATA_I2C_SDA	  DATA(B,4)
#define  TRIS_I2C_SDA	  TRIS(B,4)
#define  ANSEL_I2C_SDA	  ANSEL(B,4)
#define  WPU_I2C_SDA	  WPU(B,4)

#define  DATA_I2C_SCL	  DATA(B,3)
#define  TRIS_I2C_SCL	  TRIS(B,3)
#define  ANSEL_I2C_SCL	  ANSEL(B,3)
#define  WPU_I2C_SCL	  WPU(B,3)

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
#define  WPU_RTC_1PPS		WPU(B,5)
#define  IOC_RTC_1PPS		IOC(B,5)


// External SPI memory
#define	DATA_SPI_CS		DATA(C,2)
#define	TRIS_SPI_CS		TRIS(C,2)
#define	ANSEL_SPI_CS	ANSEL(C,2)
#define	WPU_SPI_CS		NO_WPU

#define	DATA_SPI_SCK	DATA(C,3)
#define	TRIS_SPI_SCK	TRIS(C,3)
#define	ANSEL_SPI_SCK	ANSEL(C,3)
#define	WPU_SPI_SCK		NO_WPU

#define	DATA_SPI_SDI	DATA(C,4)
#define	TRIS_SPI_SDI	TRIS(C,4)
#define	ANSEL_SPI_SDI	ANSEL(C,4)
#define	WPU_SPI_SDI		NO_WPU

#define	DATA_SPI_SDO	DATA(C,5)
#define	TRIS_SPI_SDO	TRIS(C,5)
#define	ANSEL_SPI_SDO	ANSEL(C,5)
#define	WPU_SPI_SDO		NO_WPU


// NMEA Serial GPS Data
#define	GPS_DATA		(PIN_D)
#define	DATA_GPS_DATA	(DATA_PIN_D)

// NMEA 1PPS GPS pulse
#define	GPS_1PPS		(PIN_E)
#define DATA_GPS_1PPS	(DATA_PIN_E)
#define TRIS_GPS_1PPS	(TRIS_PIN_E)
#define ANSEL_GPS_1PPS	(ANSEL_PIN_E)
#define WPU_GPS_1PPS	(WPU_PIN_E)

// Have quick pin
#define	HQ_DATA			(PIN_B)
#define	DATA_HQ_DATA	(DATA_PIN_B)
#define	TRIS_HQ_DATA	(TRIS_PIN_B)
#define	ANSEL_HQ_DATA	(ANSEL_PIN_B)
#define	WPU_HQ_DATA		(WPU_PIN_B)



// To communicate with PC we use EUSART
// To communicate with MBITR we do Soft UART

//  Here are assignments of the pins for MBITR
// PIN_C - Input, PIN_D - Output
#define	 TxMBITR		(PIN_D)
#define	 DATA_TxMBITR	(DATA_PIN_D)
#define	 TRIS_TxMBITR	(TRIS_PIN_D)
#define	 ANSEL_TxMBITR	(ANSEL_PIN_D)
#define	 WPU_TxMBITR	(WPU_PIN_D)

#define	 RxMBITR		(PIN_C)
#define	 DATA_RxMBITR	(DATA_PIN_C)
#define	 TRIS_RxMBITR	(TRIS_PIN_C)
#define	 ANSEL_RxMBITR	(ANSEL_PIN_C)
#define	 WPU_RxMBITR	(WPU_PIN_C)

// To communicate with PC the following pins are used:
//  PIN_D - input, PIN_C - output
#define	 TxPC			(PIN_C)
#define	 DATA_TxPC		(DATA_PIN_C)
#define	 TRIS_TxPC		(TRIS_PIN_C)
#define	 ANSEL_TxPC		(ANSEL_PIN_C)
#define	 WPU_TxPC		(WPU_PIN_C)

#define	 RxPC			(PIN_D)
#define	 DATA_RxPC		(DATA_PIN_D)
#define	 TRIS_RxPC		(TRIS_PIN_D)
#define	 ANSEL_RxPC		(ANSEL_PIN_D)
#define	 WPU_RxPC		(WPU_PIN_D)

// To communicate with another DTD the following pins are used:
//  PIN_C - input, PIN_D - output
#define	 TxDTD			(PIN_D)
#define	 DATA_TxDTD		(DATA_PIN_D)
#define	 TRIS_TxDTD		(TRIS_PIN_D)
#define	 ANSEL_TxDTD	(ANSEL_PIN_D)
#define	 WPU_TxDTD		(WPU_PIN_D)

#define	 RxDTD			(PIN_C)
#define	 DATA_RxDTD		(DATA_PIN_C)
#define	 TRIS_RxDTD		(TRIS_PIN_C)
#define	 ANSEL_RxDTD	(ANSEL_PIN_C)
#define	 WPU_RxDTD		(WPU_PIN_C)


// To communicate with DS-101 (RS-485) @ 64Kbd the following pins are used:
//  Data+  PIN_B,  Data-  PIN_E
#define	 Data_P			(PIN_B)
#define	 DATA_Data_P	(DATA_PIN_B)
#define	 TRIS_Data_P	(TRIS_PIN_B)
#define	 ANSEL_Data_P	(ANSEL_PIN_B)
#define	 WPU_Data_P		(WPU_PIN_B)

#define	 Data_N			(PIN_E)
#define	 DATA_Data_N	(DATA_PIN_E)
#define	 TRIS_Data_N	(TRIS_PIN_E)
#define	 ANSEL_Data_N	(ANSEL_PIN_E)
#define	 WPU_Data_N		(WPU_PIN_E)

#define	 WAKEUP			(PIN_C)

#define	DISABLE_IRQ(a)	do{a = INTCONbits.GIE;	INTCONbits.GIE = 0;}while(0)
#define ENABLE_IRQ(a)	do{INTCONbits.GIE = a;}while(0)
#define DISABLE_PIRQ()	do{INTCONbits.PEIE = 0;}while(0)
#define ENABLE_PIRQ()	do{INTCONbits.PEIE = 1;}while(0)
#define IS_IRQ_ENABLED() (INTCONbits.GIE)
#define IS_IRQ_DISABLED() (!INTCONbits.GIE)

#define ENABLE_1PPS_IRQ()	do{INTCONbits.RBIE = 1;}while(0)
#define IS_1PPS()			INTCONbits.RBIF
#define CLEAR_1PPS()		do{INTCONbits.RBIF = 0;}while(0)


#define 	timer10msSetup(control, period)	do{T2CON = control; PR2 = period; TMR2 = 0; PIR1bits.TMR2IF = 0;}while(0)
#define 	timer10msReset()			do{TMR2 = 0;PIR5bits.TMR2IF = 0;}while(0)
#define 	timer10msSet(value)			do{TMR2 = value;PIR1bits.TMR2IF = 0;}while(0)
#define 	timer10msFlag()				PIR1bits.TMR2IF
#define 	timer10msClearFlag()		do{PIR1bits.TMR2IF = 0;}while(0)
#define		timer10msEnableIRQ()		do{PIE1bits.TMR2IE = 1;}while(0)
#define		timer10msDisableIRQ()		do{PIE1bits.TMR2IE = 0;}while(0)

#define 	timerSetup(control, period)	do{T6CON = control; PR6 = period; TMR6 = 0; PIR5bits.TMR6IF = 0;}while(0)
#define 	timerReset()				do{TMR6 = 0;PIR5bits.TMR6IF = 0;}while(0)
#define 	timerSet(value)				do{TMR6 = value;PIR5bits.TMR6IF = 0;}while(0)
#define 	timerFlag()					PIR5bits.TMR6IF
#define 	timerClearFlag()			do{PIR5bits.TMR6IF = 0;}while(0)
#define		timerEnableIRQ()			do{PIE5bits.TMR6IE = 1;}while(0)
#define		timerDisableIRQ()			do{PIE5bits.TMR6IE = 0;}while(0)

#define		timeoutReset()				do{TMR1H = 0; TMR1L = 0;PIR1bits.TMR1IF = 0;}while(0)
#define		timeoutFlag()				PIR1bits.TMR1IF

#endif	// __CONFIG_H__
