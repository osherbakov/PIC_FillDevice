#include "string.h"
#include "config.h"
#include "delay.h"
#include "controls.h"
#include "rtc.h"
#include "serial.h"
#include "clock.h"

void high_isr (void);
void low_isr (void);

volatile unsigned char led_counter;
volatile unsigned char led_on_time;
volatile unsigned char led_off_time;
volatile signed int timeout_counter;
volatile unsigned int seconds_counter;
volatile RTC_date_t	rtc_date;

extern void CalculateHQDate(void);			

extern byte hq_data[];

volatile byte hq_enabled;
static unsigned char hq_active;
static unsigned char hq_bit_counter;
static unsigned char hq_byte_counter;
static unsigned char hq_current_bit; 
static unsigned char hq_current_byte; 

#define START_FRAME_SIZE	(400/8)		// 400 SYNC bits of all "1"
#define START_FRAME_DATA 	(0xFF)		// The data to be sent during SYNC phase
#define DATA_FRAME_SIZE		(112/8)		// 112 bits of actual timing data

//-------------------------------------------------
//  LED support fucntions
//------------------------------------------------
void set_led_state(char on_time, char off_time)
{
	led_on_time = on_time;
	led_off_time = off_time;
	LEDP = (led_on_time == 0) ? 0 : 1;	// Turn on/off LED
	led_counter = led_on_time;
}


// At this location 0x08 there will be only jump to the ISR
#pragma code high_vector=0x08
void interrupt_at_high_vector(void)
{
	_asm GOTO high_isr _endasm
}

// At this location 0x18 there will be only jump to the ISR
#pragma code low_vector=0x18
void low_interrupt ()
{
	_asm GOTO low_isr _endasm
}

#pragma code /* return to the default code section */
#pragma interrupt high_isr

// High priority interrupt handler for:
//  - 1 Second PPS from the RTC to generate precise start of
//      Havequick sequence  
//  - Havequick sequence generation (300us)
//  - eusart tx interrupt
void high_isr (void)
{
	byte TL, TH;
	// Is it a 1SEC Pulse interrupt from RTC (on both Pos and Neg edges)?
	if( INTCONbits.RBIF)
	{
    // Interrupt occurs when 1PPS pin from RTC has LOW->HIGH and HIGH->LOW
    // transitions
		if(!PIN_1PPS && hq_active)  //  On HIGH->LOW transition
		{
			HQ_PIN = 1;	
			TMR4 = 10;					// Preload to compensate for the delay 
			T4CONbits.TMR4ON = 1;		// Turn on the timer
			PIR5bits.TMR4IF = 0;		// Clear Interrupt
			PIE5bits.TMR4IE = 1;		// Enable TIMER4 Interrupt
			// Calculate next value
			hq_current_bit = 0x00;
			hq_current_byte = (START_FRAME_DATA << 1);
			hq_byte_counter = 1;    // First bytes sent
			hq_bit_counter = 0;     // Bit 0 was sent
			hq_active = 0;          // Don't do anything on next H->L until enabled
		}
		if(PIN_1PPS)				// On LOW -> HIGH transition - start collecting data
		{
      // Get statistics for the clock adjustment
	  	TL = TMR0L;	// Read LSB first to latch the data
	 		TH = TMR0H;
      TMR0H = 0;  // Reset the counter - MSB first
      TMR0L = 0;
      curr_lsb = ((int)TH << 8) + TL;
      UpdateClockData();
      ProcessClockData();
      
      // Adjust current time
			rtc_date.MilliSeconds_10 = 50; // At this moment we are exactly at 500 ms
    	TMR2 = 0;	          // zero out 10ms counter
      
      // Increment big timeout counter
      seconds_counter++;  // Advance the seconds counter (used for big timeouts)

			// Check for HQ status and prepare everything for the next falling edge
			if( hq_enabled )
			{
				GetRTCData();     // Get current time and data from RTC
				CalculateNextSecond();  // Calculate what time it will be on the next 1PPS
				CalculateHQDate();// Convert into the HQ date format
				TRIS_HQ_PIN = OUTPUT;
				HQ_PIN = 0;
				hq_active = 1;	  // Transition HIGH - > LOW - start outputting the data
			}
		}
		INTCONbits.RBIF = 0;
	}
	
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
					HQ_PIN = 0x00;
					T4CONbits.TMR4ON = 0;		// Turn off the timer
					PIE5bits.TMR4IE = 0;		// Disable interrupts
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
}

#pragma code
#pragma interruptlow low_isr
void low_isr ()
{
	// Is it TIMER2 interrupt? (10 ms)
	if(PIR1bits.TMR2IF)	
	{
		timeout_counter--;
		rtc_date.MilliSeconds_10++;
		// If the LED counter is counting
		if(led_counter && (--led_counter == 0))
		{
			// Toggle the LED and it's state
			led_counter = LAT_LEDP ? led_off_time : led_on_time;
			LAT_LEDP = ~LAT_LEDP;
		}
		PIR1bits.TMR2IF = 0;	// Clear interrupt
	}

	// Is it a EUSART RX interrupt ?
	// Maintain a circular buffer pointed by rx_data
	if(PIR1bits.RC1IF)
	{
		if(rx_count <= rx_count_1)
		{	
			rx_data[rx_count++] = RCREG1;
		}else
		{
			byte i;
			for( i = 0; i < rx_count_1; i++)
			{
				rx_data[i] = rx_data[i+1];
			}
			rx_data[rx_count_1] = RCREG1;
		}
		// No need to clear the Interrupt Flag
	}

	// Is it a EUSART TX interrupt ?
  // If there are bytes to send - get the symbol from the 
  //  buffer pointed by tx_data and decrement the counter
  // If that was the last symbol - disable interrupts.
	if(PIR1bits.TX1IF)
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
  if(INTCONbits.TMR0IF)
  {
    curr_msb++; 
    INTCONbits.TMR0IF = 0;  // Clear interrupt
  }

}


