#include "string.h"
#include "config.h"
#include "delay.h"
#include "controls.h"
#include "rtc.h"
#include "serial.h"

void high_isr (void);
void low_isr (void);

volatile unsigned char led_counter;
volatile unsigned char led_on_time;
volatile unsigned char led_off_time;
volatile signed int timeout_counter;
RTC_date_t	rtc_date;

extern void CalculateHQDate(void);			

extern byte hq_data[];

byte hq_enabled;
static unsigned char hq_active;
static unsigned char hq_bit_counter;
static unsigned char hq_byte_counter;
static unsigned char hq_current_bit; 
static unsigned char hq_current_byte; 

byte switch_pos;
byte prev_switch_pos;

byte power_pos;
byte prev_power_pos;

volatile byte *rx_orig_data;
volatile byte rx_orig_count_1;

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
void high_isr (void)
{
	// Is it a 1SEC Pulse interrupt (on both Pos and Neg edges)?
	if( INTCONbits.RBIF)
	{
		if(hq_active)
		{
			HQ_PIN = 1;	
			TMR4 = 10;					// Preload to compensate for the delay 
			T4CONbits.TMR4ON = 1;		// Turn on the timer
			PIR5bits.TMR4IF = 0;		// Clear Interrupt
			PIE5bits.TMR4IE = 1;		// Enable TIMER4 Interrupt
			// Calculate next value
			hq_current_bit = 0x00;
			hq_current_byte = (START_FRAME_DATA << 1);
			hq_byte_counter = 1;
			hq_bit_counter = 0;
			hq_active = 0;
		}
		// Only send HQ data when there is no chance of clock rollover
		if(PIN_1PPS)				// Transition LOW -> HIGH  - start collecting data
		{
			if( hq_enabled)
			{
				GetRTCData();
				SetNextSecond();
				CalculateHQDate();
				TRIS_HQ_PIN = OUTPUT;
				HQ_PIN = 0;
				hq_active = rtc_date.Valid;	// Transition HIGH - > LOW - start outputting the data
			}
		}else
		{
			rtc_date.MilliSeconds = 0;
		}
		INTCONbits.RBIF = 0;
	}
	
	// Is it TIMER4 interrupt? (300 ms)
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

	// Is it a EUSART TX interrupt ?
	if(PIR1bits.TX1IF)
	{
		if( tx_count )	// not the last byte
		{
			TXREG1 = *tx_data++;
			tx_count--;
			if( !tx_count )	// Last byte
			{
				PIE1bits.TX1IE = 0;	// Disable Interrupts
			}
		}
		// No need to clear the Interrupt Flag
	}
}

#pragma code
#pragma interruptlow low_isr
void low_isr ()
{
	// Is it TIMER2 interrupt? (10 ms)
	if(PIR1bits.TMR2IF)	
	{
		PIR1bits.TMR2IF = 0;	// Clear interrupt
		timeout_counter--;
		rtc_date.MilliSeconds++;
		// If the LED counter is counting
		if(led_counter && (--led_counter == 0))
		{
			// Toggle the LED and it's state
			led_counter = LAT_LEDP ? led_off_time : led_on_time;
			LAT_LEDP = ~LAT_LEDP;
		}
	}

	// Is it a EUSART RX interrupt ?
	if(PIR1bits.RC1IF)
	{
		if(rx_count)
		{	
			*rx_data = RCREG1;
			rx_count--;
			if(rx_count) rx_data++;
		}else
		{
			memmove((void *)rx_orig_data, (void *)(rx_orig_data + 1), rx_orig_count_1);
			*rx_data = RCREG1;
		}
		// No need to clear the Interrupt Flag
	}

}


