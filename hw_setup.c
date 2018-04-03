#include "config.h"
#include "delay.h"
#include "controls.h"
#include "rtc.h"
#include <spi.h>
#include "i2c_sw.h"
#include "serial.h"
#include "gps.h"

extern void setup_clocks();	
extern void setup_xtal();	
extern void setup_spi();
extern void setup_start_io();

#define TMR2_FREQ  (100)  // 100 Hz timer tick - 10ms
#define CNT_10MS ((( XTAL_FREQ * 1000000L) / ( 4L * TMR2_FREQ * 10L * 16L )) - 1 )

void setup_xtal()
{
  	OSCTUNE = 0;    		// No PLL
	OSCCONbits.IDLEN = 1; 	// On Sleep() enter IDLE

	// Select appropriate clock source
#if XTAL_FREQ == 16 MHZ
	OSCCONbits.IRCF = 0x7;	// 16MHz
#endif
#if XTAL_FREQ == 8 MHZ
	OSCCONbits.IRCF = 0x6;	// 8MHz
#endif
#if XTAL_FREQ == 4 MHZ
	OSCCONbits.IRCF = 0x5;	// 4MHz
#endif
#if XTAL_FREQ == 2 MHZ
	OSCCONbits.IRCF = 0x4;	// 2MHz
#endif
#if XTAL_FREQ == 1MHZ
	OSCCONbits.IRCF = 0x3;	// 1MHz
#endif
	OSCCONbits.SCS = 0x00;	// Internal with MUX (can be PLL'd*4)
}	

void setup_clocks()
{
	setup_xtal();

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
	T4CON = 0x02;			// 1:1 Post, 16 prescaler, off 

	// Set up the timer 6 to roll over with no interrupts
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
	pinMode(SPI_CS, OUTPUT);
	pinMode(SPI_SCK, OUTPUT);
	pinMode(SPI_SDI, INPUT);
	pinMode(SPI_SDO, OUTPUT);
	pinWrite(SPI_CS, 1);				// Keep CS HIGH
	// Mode (0,0), clock is FOSC/16, sample data in the middle
	OpenSPI1(SPI_FOSC_16, MODE_00, SMPEND);
}

static byte prev;
void setup_start_io()
{
	DISABLE_IRQ(prev);
	DISABLE_PIRQ();

	// Initially set up all ports as Digital IO
	portMode(A, INPUT);
	portMode(B, INPUT);
	portMode(C, INPUT);
	portMode(D, INPUT);
	portMode(E, INPUT);

  	// Apply power to PIN_A
  	pinMode(PIN_A_PWR, OUTPUT);
  	pinWrite(PIN_A_PWR, 1);

  	// Apply no power to PIN_F
  	pinMode(PIN_F_PWR, OUTPUT);
  	pinWrite(PIN_F_PWR, 0);

  	// Switch S1 - S16 settings
	TRIS_S1_8 = 0xFF;	// Inputs
	TRIS_S9_16 = 0xFF;	// Inputs
		
	// Audio Connector - Inputs
	pinMode(PIN_B, INPUT_PULLUP);
	pinMode(PIN_C, INPUT_PULLUP);
	pinMode(PIN_D, INPUT_PULLUP);
	pinMode(PIN_E, INPUT_PULLUP);
	pinMode(PIN_F, INPUT_PULLUP);


	// LED controls
	pinMode(LEDP, OUTPUT);
	pinWrite(LEDP, LOW);

	// Button and Power sensors
	INTCON2bits.RBPU = 0;	// Enable Weak Pull Ups

	// Zero button - input and no pullup
	pinMode(ZBR, INPUT_PULLUP);

	// Push button - input with weak pullup
	pinMode(BTN, INPUT_PULLUP);

	// setup I2C pins both to high
	pinMode(I2C_SDA, OUTPUT);
	pinMode(I2C_SCL, OUTPUT);
	I2C_DATA_HI();
	I2C_CLOCK_HI();
	
	// Setup RTC 1PPS pin as input
	pinMode(RTC_1PPS, INPUT);
	// 	IOC_1PPS = 1;         	// Enable IOC
	ENABLE_1PPS_IRQ();		// Enable 1PPS interrupt
	
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
	ENABLE_PIRQ();
	ENABLE_IRQ(1);
}

void setup_sleep_io()
{
	DISABLE_PIRQ();
	DISABLE_IRQ(prev);

	// Use all ports as Analog In
	portMode(A, INPUT_ANALOG);
	portMode(B, INPUT_ANALOG);
	portMode(C, INPUT_ANALOG);
	portMode(D, INPUT_ANALOG);
	portMode(E, INPUT_ANALOG);

	// Disable LED
	pinMode(LEDP, OUTPUT);
	pinWrite(LEDP, 0);

  	// Apply power to PIN_A
  	pinMode(PIN_A_PWR, OUTPUT);
  	pinWrite(PIN_A_PWR, 1);

  	// Apply no power to PIN_F
  	pinMode(PIN_F_PWR, OUTPUT);
  	pinWrite(PIN_F_PWR, 0);


	INTCON2bits.RBPU = 1;	// Disable Weak Pull Ups
	WPUB = 0x00;			// Turn off Weak pull-up

	// Turn off SPI
	pinMode(SPI_CS, OUTPUT);
	pinMode(SPI_SCK, OUTPUT);
	pinMode(SPI_SDI, INPUT);
	pinMode(SPI_SDO, OUTPUT);
	pinWrite(SPI_CS, 1);
	pinWrite(SPI_SCK, 1);
	pinWrite(SPI_SDO, 1);

	OSCCONbits.IDLEN = 0; 	// On Sleep() enter Sleep
}

