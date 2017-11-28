#include "string.h"
#include "config.h"
#include "delay.h"
#include "controls.h"
#include "rtc.h"
#include "serial.h"
#include "clock.h"
#include "gps.h"

void high_isr (void);

volatile unsigned char led_counter;
volatile unsigned char led_on_time;
volatile unsigned char led_off_time;
volatile unsigned int seconds_counter;
volatile RTC_date_t	rtc_date;

volatile byte hq_enabled;	// HaveQuick data output is enabled

static byte hq_bit_counter;
static byte hq_byte_counter;
static byte hq_current_bit; 
static byte hq_current_byte; 

#define START_FRAME_SIZE	(400/8)		// 400 SYNC bits of all "1"
#define START_FRAME_DATA 	(0xFF)		// The data to be sent during SYNC phase
#define DATA_FRAME_SIZE		(112/8)		// 112 bits (1of actual timing data

//-------------------------------------------------
//  LED support fucntions
//------------------------------------------------
void set_led_state(char on_time, char off_time)
{
	led_on_time = on_time;
	led_off_time = off_time;
	LEDP = led_on_time ? 1 : 0;	// Turn on/off LED
	led_counter = (led_on_time && led_off_time) ? led_on_time : 0;
}

void set_led_on()
{
	led_counter = 0;
	LEDP = 1;				// Turn on LED
}

void set_led_off()
{
	led_counter = 0;
	LEDP = 0;				// Turn off LED
}


// At this location 0x08 there will be only jump to the ISR
#pragma code high_vector=0x08
void interrupt_at_high_vector(void)
{
	_asm GOTO high_isr _endasm
}

#pragma code /* return to the default code section */
#pragma interrupt high_isr nosave=section(".tmpdata"),TBLPTRL, TBLPTRH, TBLPTRU,TABLAT,PCLATH,PCLATU

// High priority interrupt handler for:
//  - 1 Second PPS from the RTC to generate precise start of Havequick sequence  
//  - Havequick sequence generation (300us)
//  - eusart tx interrupt
//
// TIMER4 is used for Havequick clock
// TIMER2 generates 10ms ticks
// TIMER0 for fine tuning of the PIC internal processor clock
void high_isr (void)
{
	byte TL, TH;
	//--------------------------------------------------------------------------
	// Is this a 1SEC Pulse interrupt from RTC? (on both Pos and Neg edges)
	if(INTCONbits.RBIF)
	{
    	// Interrupts on 1 PPS pin from RTC LOW->HIGH and HIGH->LOW transitions
    	// The HIGH->LOW transition indicates the start of the second
		if(PIN_1PPS == 0)
		{	
			// Interrupt occured on HIGH->LOW transition
	  		if(hq_enabled)  //  On HIGH->LOW transition - 0 ms
	  		{
	  			HQ_PIN = 1;	
	  			TMR4 = 10;					// Preload to compensate for the delay 
	  			PR4 = HQII_TIMER;			// Load TIMER4
	  			T4CONbits.TMR4ON = 1;		// Turn on the TIMER4
	  			PIR5bits.TMR4IF = 0;		// Clear TIMER4 Interrupt
	  			PIE5bits.TMR4IE = 1;		// Enable TIMER4 Interrupt
	  			// Calculate next value
	  			hq_current_bit = 0;
	  			hq_current_byte = (START_FRAME_DATA << 1);
	  			hq_byte_counter = 1;    	// First byte sent
	  			hq_bit_counter = 0;     	// Bit 0 was sent
			}
      		// Increment big timeout counter
      		seconds_counter++;  // Advance the seconds counter (used for big timeouts)
			ms_10 = 0;
		}else{
  			// On LOW -> HIGH transition - 500ms - start collecting data
			// Check for HQ status and prepare everything for the next falling edge
			if( hq_enabled )
			{
				GetRTCData();     		// Get current time and data from RTC
				CalculateNextSecond();  // Calculate what time it will be on the next 1PPS
				CalculateHQDate();		// Convert into the HQ date format
				TRIS_HQ_PIN = OUTPUT;
				HQ_PIN = 0;
			}
	        // Get statistics for the clock adjustment
       // Turn it off for a while
//	      	TL = TMR0L;	// Read LSB first to latch the data
//	     	TH = TMR0H;
//	        TMR0H = 0;  // Reset the counter - MSB first
//	        TMR0L = 0;
//	        curr_lsb = ((unsigned int)TH << 8) | (unsigned int)TL;
//	        UpdateClockData();
//	        ProcessClockData();			
		}
		INTCONbits.RBIF = 0;
	}
	
	//--------------------------------------------------------------------------
	// Is it TIMER4 interrupt? (300 us)
	if(PIR5bits.TMR4IF)
	{
		HQ_PIN = hq_current_bit;

		// Now prepare the next bit

		// HaveQuick uses Manchester encoding - 
		//  "1" is 300us HIGH - 300us LOW
		//  "0" is 300us LOW - 300us HIGH
		// So on the first 300us set the bit, on the second - invert it
		if(hq_bit_counter & 0x01)
		{
			hq_current_bit = ~hq_current_bit;
		}else
		{
			if(hq_bit_counter >= (16 - 2) )
			{
				// Check for the Start pattern
				if(hq_byte_counter < START_FRAME_SIZE)
				{
					hq_current_byte = START_FRAME_DATA;
				}else if(hq_byte_counter < (START_FRAME_SIZE + DATA_FRAME_SIZE))
				{
					hq_current_byte = hq_data[hq_byte_counter - START_FRAME_SIZE];
				}else 
				{
					HQ_PIN = 0;
					T4CONbits.TMR4ON = 0;		// Turn off the Timer4
					PIE5bits.TMR4IE = 0;		// Disable Timer4 interrupts
				}
				hq_byte_counter++;
			}
			hq_current_bit = (hq_current_byte & 0x80) ? 1 : 0;
			hq_current_byte <<= 1; 
		}
		hq_bit_counter++;
		hq_bit_counter &= 0x0F;
		PIR5bits.TMR4IF = 0;	// Clear the interrupt
	}

	//--------------------------------------------------------------------------
	// Is it TIMER2 interrupt? (10 ms)
	if(PIR1bits.TMR2IF)	
	{
		ms_10++;
		if(!timeout_flag) {
			timeout_counter -= 10;
			if(timeout_counter <= 0) timeout_flag = 1;
		}

		// If the LED counter is counting
		if(led_counter && (--led_counter == 0))
		{
			// Toggle the LED and it's state
			led_counter = LAT_LEDP ? led_off_time : led_on_time;
			LAT_LEDP = ~LAT_LEDP;
		}
		PIR1bits.TMR2IF = 0;	// Clear overflow flag and interrupt
	}

	//--------------------------------------------------------------------------
	// Is it a EUSART RX interrupt ?
	// Maintain a circular buffer pointed by rx_data
	if(PIE1bits.RC1IE && PIR1bits.RC1IF)
	{
		if(rx_idx > rx_idx_max)
		{
			for( rx_idx = 0; rx_idx < rx_idx_max; rx_idx++)
			{
				rx_data[rx_idx] = rx_data[rx_idx+1];
			}
		}
		rx_data[rx_idx++] = RCREG1;
		// overruns? clear it
		if(RCSTA1 & 0x06)
		{
			RCSTA1bits.CREN = 0;
			RCSTA1bits.CREN = 1;
		}
		// No need to clear the Interrupt Flag
	}

	//--------------------------------------------------------------------------
	// Is it a EUSART TX interrupt ?
  	// If there are bytes to send - get the symbol from the 
  	//  buffer pointed by tx_data and decrement the counter
  	// If that was the last symbol - disable interrupts.
	if(PIE1bits.TX1IE && PIR1bits.TX1IF)
	{
		if( tx_count )	// not the last byte
		{
			TXREG1 = *tx_data++;
			tx_count--;
			if( tx_count == 0 )	// Last byte was sent
			{
				PIE1bits.TX1IE = 0;	// Disable Interrupts
			}
		}
		// No need to clear the Interrupt Flag
	}
  
  	// Check for the clock correction timer0
//  	if(INTCONbits.TMR0IF)
//  	{
//    	curr_msb++; 
//    	INTCONbits.TMR0IF = 0;  // Clear interrupt
//  	}

}


