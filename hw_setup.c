#include "config.h"
#include "delay.h"
#include "controls.h"
#include "rtc.h"
#include <spi.h>
#include "i2c_sw.h"
#include "serial.h"

extern void setup_clocks();	
extern void setup_spi();
extern void setup_start_io();

#define MS_10_PER_1MHZ (10*1000L)
#define CNT_10MS (((XTAL_FREQ / 4) * (MS_10_PER_1MHZ / 10))/16 )

#define HQ_BIT_TIME_US		(600)  // 600us for one bit.
#define TIMER_300MS_PERIOD ( (XTAL_FREQ/4) * (HQ_BIT_TIME_US/2) / 16)

void setup_clocks()
{
	// Wait until HF Oscillator is stable 
	while(!OSCCONbits.HFIOFS) ;
	OSCCONbits.IDLEN = 1; 	// On Sleep() enter IDLE

	// Select appropriate clock source
#if XTAL_FREQ == 16MHZ
	OSCCONbits.IRCF = 0x7;	// 8MHz
#endif
#if XTAL_FREQ == 8MHZ
	OSCCONbits.IRCF = 0x6;	// 8MHz
#endif
#if XTAL_FREQ == 4MHZ
	OSCCONbits.IRCF = 0x5;	// 8MHz
#endif
#if XTAL_FREQ == 2MHZ
	OSCCONbits.IRCF = 0x4;	// 8MHz
#endif
#if XTAL_FREQ == 1MHZ
	OSCCONbits.IRCF = 0x3;	// 8MHz
#endif
	OSCCONbits.SCS = 0x02;	// Internal

// Set up the timer 2 to fire every 10 ms at low priority, ON
	PR2 = CNT_10MS;
	TMR2 = 0;	
	T2CON = 0x02 | (10 << 3) | (1 << 2);
	IPR1bits.TMR2IP = 0;	// Low priority
	PIR1bits.TMR2IF = 0;	// Clear Interrupt
	PIE1bits.TMR2IE = 1;	// Enable TIMER2 Interrupt

// Set up the timer 4 to fire every 300us at high priority
// for Have Quick data stream generation
	PR4 = TIMER_300MS_PERIOD;
	IPR5bits.TMR4IP = 1;	// HIGH priority
	T4CON = 0x02;			// 1:1 Post, 16 prescaler, off 
	PIR5bits.TMR4IF = 0;	// Clear Interrupt
	PIE5bits.TMR4IE = 1;	// Enable TIMER4 Interrupt

// Set up the timer 6 to roll over every 300us but with no interrupts
// for Have Quick data stream detection
	PR6 = 0xFF;
	IPR5bits.TMR6IP = 0;	// LOW priority
	T6CON = 0x06;			// 1:1 Post, 16 prescaler, on 
	PIR5bits.TMR6IF = 0;	// Clear Interrupt
	PIE5bits.TMR6IE = 0;	// Disable TIMER6 Interrupt
}

void setup_spi()
{
	TRIS_SPI_CS 	= 0;	// Drive the CS pin
	TRIS_SPI_SCK	= 0;	// Drive the CLOCK pin
	TRIS_SPI_SDI	= 1;	// SDInput is Input
	TRIS_SPI_SDO	= 0;	// Drive the SDOout

	SPI_CS = 1;				// Keep CS HIGH
	
	// Mode (0,0), clock is FOSC/16, sample data in the middle
	OpenSPI1(SPI_FOSC_16, MODE_00, SMPEND);
}

void setup_start_io()
{
	INTCONbits.GIE = 0;		// Disable interrupts
	RCONbits.IPEN = 1;		// Enable Priorities

	// Initially set up all ports as Digital IO
	PORTA = 0x00;
	PORTB = 0x00;
	PORTC = 0x00;
	PORTD = 0x00;
	PORTE = 0x00;
	ANSELA = 0x00;
	ANSELB = 0x00;
	ANSELC = 0x00;
	ANSELD = 0x00;
	ANSELE = 0x00;

	// Switch S1 - S16 settings
	TRIS_S1_8 = 0xFF;	// Inputs
	TRIS_S9_16 = 0xFF;	// Inputs
	
	// Voltage on Resistor network
	TRIS_VRD = 0;		// Drive it
	VRD = 0;			// Zero to save power
	
	// Audio Connector - Inputs
	TRIS_PIN_B = INPUT;
	TRIS_PIN_C = INPUT;
	TRIS_PIN_D = INPUT;
	TRIS_PIN_E = INPUT;
	TRIS_PIN_F = INPUT;
	

	// LED controls
	TRIS_LEDP = OUTPUT;		// Drive it
	TRIS_LEDN_R = OUTPUT;	// Drive it
	LEDP = 0;				// LED off
	LEDN_R = 0;

	// Button and Power sensors
	INTCON2bits.RBPU = 0;	// Enable Weak Pull Ups

	TRIS_ZBR = INPUT;		// Input
	WPUB_ZBR = 0;			// Turn off Weak pull-up

	TRIS_OFFBR = OUTPUT;	// Output
	ON_GND = 0;

	TRIS_BTN = INPUT;		// Input
	WPUB_BTN = 1;			// Turn on Weak pull-up

	// setup I2C pins
	DATA_HI();
	CLOCK_HI();
	
	// Setup RTC 1PPS pin as input, High priority
	TRIS_1PPS = INPUT;		
	IOC_1PPS = 1;
	INTCON2bits.RBIP = 1;
	INTCONbits.RBIE	= 1; // Enable 1PPS interrupt
	
	setup_clocks();		// 16 MHz
	setup_spi();
	SetupRTC();

	// Setup and disable USART
	// Configure the EUSART module
	IPR1bits.TX1IP = 1;		// High priority TX
	TXSTA1 = 0x00;
	RCSTA1bits.SPEN = 0;


	// Enable all interrupts
	INTCONbits.PEIE = 1;
	INTCONbits.GIE = 1;
}

//void setup_sleep_io()
//{
//	INTCONbits.GIE = 0;		// Disable interrupts
//
//	// Release I2C bus
//	SWStopI2C();
//
//	// Use all ports as Analog In
//	TRISA = 0xFF;
//	TRISB = 0xFF;
//	TRISC = 0xFF;
//	TRISD = 0xFF;
//	TRISE = 0xFF;
//	ANSELA = 0xFF;
//	ANSELB = 0xFF;
//	ANSELC = 0xFF;
//	ANSELD = 0xFF;
//	ANSELE = 0xFF;
//
//	INTCON2bits.RBPU = 1;	// Disable Weak Pull Ups
//	WPUB = 0x00;			// Turn off Weak pull-up
//
//	// Turn off SPI
//	TRIS_SPI_CS = OUTPUT;	// Drive the CS pin
//	TRIS_SPI_SCK = OUTPUT;	// Drive the CLOCK pin		
//	TRIS_SPI_SDO = OUTPUT;	// Drive SDOut pin
//	SPI_CS = 1;				// Keep CS HIGH
//	SPI_SCK = 0;			// Keep CLOCK LOW
//	SPI_SDO = 0;			// Keep SDOut LOW
//
//	OSCCONbits.IDLEN = 0; 	// On Sleep() enter Sleep
//}
//
