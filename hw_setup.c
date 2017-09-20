#include "config.h"
#include "delay.h"
#include "controls.h"
#include "rtc.h"
#include <spi.h>
#include "i2c_sw.h"
#include "serial.h"
#include "clock.h"
#include "gps.h"

extern void setup_clocks();	
extern void setup_spi();
extern void setup_start_io();

#define TMR2_FREQ  (100)  // 100 Hz timer tick - 10ms
#define CNT_10MS ((( XTAL_FREQ * 1000000L) / ( 4L * TMR2_FREQ * 10L * 16L )) - 1 )

void setup_clocks()
{
  OSCTUNE = 0;    // No PLL
	OSCCONbits.IDLEN = 1; 	// On Sleep() enter IDLE

	// Select appropriate clock source
#if XTAL_FREQ == 16MHZ
	OSCCONbits.IRCF = 0x7;	// 16MHz
#endif
#if XTAL_FREQ == 8MHZ
	OSCCONbits.IRCF = 0x6;	// 8MHz
#endif
#if XTAL_FREQ == 4MHZ
	OSCCONbits.IRCF = 0x5;	// 4MHz
#endif
#if XTAL_FREQ == 2MHZ
	OSCCONbits.IRCF = 0x4;	// 2MHz
#endif
#if XTAL_FREQ == 1MHZ
	OSCCONbits.IRCF = 0x3;	// 1MHz
#endif
	OSCCONbits.SCS = 0x02;	// Internal

	// TIMER 0 - to count OSC_CLOCK in 16-bit mode
	// Used to adjust/sync the internal clock with RTC 1PPS clock
//  	TMR0H = 0;  // Reset the counter
//  	TMR0L = 0;
//  	INTCONbits.TMR0IF =  0;   // Clear interrupt
//  	INTCONbits.TMR0IE =  1;   // Enable interrupt
//  	T0CON = ((0x1 << 7) | (0x1 << 3)); // Ena TIMER0, 16-bit mode,no prescaler
  
	// TIMER 1 - to count OSC_CLOCK in 16-bit mode
	//  It is used for 0.131072s timeout, when clock overflows
	// Used for timeouts when we disable interrupts
  	TMR1H = 0;
  	TMR1L = 0;	// Reset the timer
  	T1GCONbits.TMR1GE = 0;	// No gating
	PIR1bits.TMR1IF = 0;	// Clear Interrupt
	PIE1bits.TMR1IE = 0;	// Disable TIMER1 Interrupt
  	T1CON = (0x3 << 4) | (1<<1) | 1; // 8-prescalar, 16-bits, enable

	//  TIMER 2 - 10 ms heartbeat timer
	// Set up the timer 2 to fire every 10 ms, ON
	PR2 = CNT_10MS;
	TMR2 = 0;	
	PIR1bits.TMR2IF = 0;	// Clear Interrupt
	PIE1bits.TMR2IE = 1;	// Enable TIMER2 Interrupt
	T2CON = (9 << 3) | (1 << 2) | 2; // 1:10, EN, 1:16

	// Set up the timer 4 to fire every 300us
	// Is used for Have Quick data stream generation
	PR4 = HQII_TIMER;
	PIR5bits.TMR4IF = 0;	// Clear Interrupt
	PIE5bits.TMR4IE = 0;	// Disable TIMER4 Interrupt
	T4CON = 0x02;			    // 1:1 Post, 16 prescaler, off 

	// Set up the timer 6 to roll over the set amount with no interrupts
	// It is used as a general timeout counter, looking at PIR5bits.TMR6IF flag
	PR6 = 0xFF;
	PIR5bits.TMR6IF = 0;	// Clear Interrupt
	PIE5bits.TMR6IE = 0;	// Disable TIMER6 Interrupt
	T6CON = HQII_TIMER_CTRL;// 1:1 Post, 16 prescaler, on 

	// Initialize the seconds counter
  	seconds_counter = 0;
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
	RCONbits.IPEN = 0;		// Disable Priorities

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

  	// Apply power to PIN_A
  	TRIS_PIN_A_PWR = OUTPUT;
  	PIN_A_PWR = 1;

  	// Apply no power to PIN_F
  	TRIS_PIN_F_PWR = OUTPUT;
  	PIN_F_PWR = 0;

  	// Switch S1 - S16 settings
	TRIS_S1_8 = 0xFF;	// Inputs
	TRIS_S9_16 = 0xFF;	// Inputs
		
	// Audio Connector - Inputs
	TRIS_PIN_B = INPUT;
	TRIS_PIN_C = INPUT;
	TRIS_PIN_D = INPUT;
	TRIS_PIN_E = INPUT;
	TRIS_PIN_F = INPUT;

	// Apply weak pull ups
	WPUB_PIN_B = 1;
	WPUB_PIN_E = 1;
	

	// LED controls
	TRIS_LEDP = OUTPUT;		// Drive it
	LEDP = 0;				// LED off

	// Button and Power sensors
	INTCON2bits.RBPU = 0;	// Enable Weak Pull Ups

	// Zero button - input and no pullup
	TRIS_ZBR = INPUT;		// Input

	// Push button - input with weak pullup
	TRIS_BTN = INPUT;		// Input
	WPUB_BTN = 1;			// Turn on Weak pull-up

	// setup I2C pins both to high
	DATA_HI();
	CLOCK_HI();
	
	// Setup RTC 1PPS pin as input
	TRIS_1PPS = INPUT;		
	IOC_1PPS = 1;         	// Enable IOC
	INTCONbits.RBIE	= 1; 	// Enable 1PPS interrupt
	
	setup_clocks();			// 16 MHz
	setup_spi();
	SetupRTC();

	
	// Setup and disable USART
	// Configure the EUSART module
	RCSTA1 = 0x00;	      	// Disable RX
	TXSTA1 = 0x00;			// Disable TX
	PIE1bits.RC1IE = 0;	  	// Disable RX interrupt
	PIE1bits.TX1IE = 0;	  	// Disable TX Interrupts
	PIR1bits.RC1IF = 0;   	// Clear Interrupt flags
	PIR1bits.TX1IF = 0;
	

	// Enable all interrupts
	INTCONbits.PEIE = 1;
	INTCONbits.GIE = 1;
}

void setup_sleep_io()
{
	INTCONbits.GIE = 0;		// Disable interrupts
	INTCONbits.PEIE = 0;

	// Use all ports as Analog In
	TRISA = 0xFF;
	TRISB = 0xFF;
	TRISC = 0xFF;
	TRISD = 0xFF;
	TRISE = 0xFF;
	ANSELA = 0xFF;
	ANSELB = 0xFF;
	ANSELC = 0xFF;
	ANSELD = 0xFF;
	ANSELE = 0xFF;

	// Disable LED
	ANSEL_LEDP = 0;
	TRIS_LEDP = 0;
	LEDP = 0;				// LED off

  	// Apply power to PIN_A
  	TRIS_PIN_A_PWR = OUTPUT;
  	PIN_A_PWR = 1;

  	// Apply no power to PIN_F
  	ANSEL_PIN_F_PWR = 0;
  	TRIS_PIN_F_PWR = OUTPUT;
  	PIN_F_PWR = 0;


	INTCON2bits.RBPU = 1;	// Disable Weak Pull Ups
	WPUB = 0x00;			// Turn off Weak pull-up

	// Turn off SPI
	ANSEL_SPI_CS = 0;	  	// Drive the CS pin
	ANSEL_SPI_SCK = 0;		// Drive the CLOCK pin		
	ANSEL_SPI_SDO = 0;		// Drive SDOut pin
	
	TRIS_SPI_CS = OUTPUT;	// Drive the CS pin
	TRIS_SPI_SCK = OUTPUT;	// Drive the CLOCK pin		
	TRIS_SPI_SDO = OUTPUT;	// Drive SDOut pin
	SPI_CS = 1;				// Keep CS HIGH
	SPI_SCK = 1;			// Keep CLOCK HIGH
	SPI_SDO = 1;			// Keep SDOut HIGH

	OSCCONbits.IDLEN = 0; 	// On Sleep() enter Sleep
}

