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

void setup_xtal()
{
  	OSCTUNE = 0;    		// No PLL
	OSCCONbits.IDLEN = 1; 	// On Sleep() enter IDLE
	pllEnable(0);

	// Select appropriate clock source
#if XTAL_FREQ == 16
	OSCCONbits.IRCF = 0x7;	// 16MHz
#endif
#if XTAL_FREQ == 8 
	OSCCONbits.IRCF = 0x6;	// 8MHz
#endif
#if XTAL_FREQ == 4 
	OSCCONbits.IRCF = 0x5;	// 4MHz
#endif
#if XTAL_FREQ == 2 
	OSCCONbits.IRCF = 0x4;	// 2MHz
#endif
#if XTAL_FREQ == 1 
	OSCCONbits.IRCF = 0x3;	// 1MHz
#endif
	OSCCONbits.SCS = 0x00;	// Internal with MUX (can be PLL'd*4)
}	

void setup_clocks()
{
	setup_xtal();
	//  10 ms heartbeat timer
	// Set up the timer to fire every 10 ms, ON
	timer10msSetup();
	timer10msEnableIRQ();

	// TIMER - to roll over with no interrupts
	// It is used as a general timeout counter, looking at timerFlag()
	timerSetupBaudrate(HQII_BAUDRATE);
	timerDisableIRQ();

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

void setup_start_io()
{
	static byte  prev;
	DISABLE_IRQ(prev);
	DISABLE_PIRQ();

	// Initially set up all ports as Digital IO
	portMode(A, INPUT);
	portMode(B, INPUT);
	portMode(C, INPUT);
	portMode(D, INPUT);
	portMode(E, INPUT);

  	// Apply GND to PIN_A and no PWR on PIN_F
	set_pin_a_as_gnd();		//  Set GND on Pin A
  	set_pin_f_as_io();		//  Apply no power to PIN_F

	pullUps(1);		// Enable Weak Pull Ups

  	// Switch S1 - S16 settings
	portMode(S1_8, INPUT_PULLUP);
	portMode(S9_16, INPUT_PULLUP);
		
	// Audio Connector - Inputs
	pinMode(PIN_B, INPUT_PULLUP);
	pinMode(PIN_C, INPUT_PULLUP);
	pinMode(PIN_D, INPUT_PULLUP);
	pinMode(PIN_E, INPUT_PULLUP);
	pinMode(PIN_F, INPUT_PULLUP);


	// LED controls - output and LED OFF
	pinMode(LEDP, OUTPUT);
	pinWrite(LEDP, LOW);

	//
	// Button and Power sensors
	//

	// Zero button - input and no pullup
	pinMode(ZBR, INPUT);

	// Push button - input with weak pullup
	pinMode(BTN, INPUT_PULLUP);

	// setup I2C pins both to high
	pinMode(I2C_SDA, OUTPUT);
	pinMode(I2C_SCL, OUTPUT);
	I2C_DATA_HI();
	I2C_CLOCK_HI();
	
	// Setup RTC 1PPS pin as input
	pinMode(RTC_1PPS, INPUT);
	ENABLE_1PPS_IRQ();		// Enable 1PPS interrupt
	
	setup_clocks();			// 16 MHz
	setup_spi();
	SetupRTC();

	
	// Setup and disable USART
	// Configure the EUSART module
	uartEnable(0);
	uartIRQRx(0);
	uartIRQTx(0);
	uartEnableRx(0);
	uartEnableTx(0);

	// Enable all interrupts
	ENABLE_PIRQ();
	ENABLE_IRQ(1);
}

void setup_sleep_io()
{

	static byte  prev;
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

  	// Apply GND to PIN_A
  	// Apply no power to PIN_F
	set_pin_a_as_gnd();		//  Set GND on Pin A
  	set_pin_f_as_io();		//  Apply no power to PIN_F


	pullUps(0);				// Disable Weak Pull Ups

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

