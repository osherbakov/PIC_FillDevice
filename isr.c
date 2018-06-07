#include "string.h"
#include "config.h"
#include "delay.h"
#include "controls.h"
#include "rtc.h"
#include "serial.h"
#include "gps.h"

void high_isr (void);

volatile unsigned int seconds_counter;
volatile RTC_date_t	rtc_date;

volatile byte hq_enabled;	// HaveQuick data output is enabled

static byte hq_bit_counter;
static byte hq_byte_counter;
static byte hq_current_bit; 
static byte hq_current_byte; 
static byte ch;

#define START_FRAME_SIZE	(400/8)		// 400 SYNC bits of all "1"
#define START_FRAME_DATA 	(0xFF)		// The data to be sent during SYNC phase
#define DATA_FRAME_SIZE		(112/8)		// 112 bits (1of actual timing data


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
	//--------------------------------------------------------------------------
	// Is this a 1SEC Pulse interrupt from RTC? (on both Pos and Neg edges)
	if(IS_1PPS())
	{
    	// Interrupts on 1 PPS pin from RTC LOW->HIGH and HIGH->LOW transitions
    	// The HIGH->LOW transition indicates the start of the second
		if(pinRead(RTC_1PPS) == LOW)
		{	
			// Interrupt occured on HIGH->LOW transition
	  		if(hq_enabled)  //  On HIGH->LOW transition - 0 ms
	  		{
	  			pinWrite(HQ_DATA, 1);
	  			timerSetup(HQII_TIMER_CTRL, HQII_TIMER);	// Will generate IRQ every 300usec
	  			timerSet(10);				// Preload to compensate for the delay 
	  			timerEnableIRQ();
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
				pinMode(HQ_DATA, OUTPUT);
				pinWrite(HQ_DATA, 0);
			}
		}
		CLEAR_1PPS();
	}
	
	//--------------------------------------------------------------------------
	// Is it TIMER interrupt? (300 us)
	if(hq_enabled && timerIsIRQEnabled() && timerFlag())
	{
		pinWrite(HQ_DATA, hq_current_bit);

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
					pinWrite(HQ_DATA, 0);
					disable_tx_hqii();
					timerDisableIRQ();
				}
				hq_byte_counter++;
			}
			hq_current_bit = (hq_current_byte & 0x80) ? 1 : 0;
			hq_current_byte <<= 1; 
		}
		hq_bit_counter++;
		hq_bit_counter &= 0x0F;
		timerClearFlag();			// Clear the interrupt
	}

	//--------------------------------------------------------------------------
	// Is it 10ms timer interrupt?
	if(timer10msFlag())	
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
			led_counter = LED_current_bit ? led_off_time : led_on_time;
			LED_current_bit = ~LED_current_bit;
			pinWrite(LEDP, LED_current_bit);
		}
		timer10msClearFlag();	// Clear overflow flag and interrupt
	}

	//--------------------------------------------------------------------------
	// Is it a EUSART RX interrupt ?
	if(uartIsIRQRx() && uartIsRx())
	{
		// Maintain a circular buffer pointed by rx_data
		if(rx_idx_in > rx_idx_max)
		{
			// Copy all data to free space for the new character
			for( rx_idx_in = 0; rx_idx_in < rx_idx_max; rx_idx_in++)
			{
				rx_data[rx_idx_in] = rx_data[rx_idx_in+1];
			}
			if(rx_idx_out > 0) rx_idx_out--;	// Adjust OUT index
		}
		rx_data[rx_idx_in++] = uartRx();	// Save the last received symbol
		// overruns? clear it
		if(uartIsError())
		{
			uartModeRx(0);
			uartModeRx(1);
		}
		// No need to clear the Interrupt Flag
	}

	//--------------------------------------------------------------------------
	// Is it a EUSART TX interrupt ?
  	// If there are bytes to send - get the symbol from the 
  	//  buffer pointed by tx_data and decrement the counter
  	// If that was the last symbol - disable interrupts.
	if(uartIsIRQTx() && uartIsTx())
	{
		if( tx_count )	// not the last byte
		{
			uartTx(*tx_data++);
			tx_count--;
			if( tx_count == 0 )	// Last byte was sent
			{
				uartIRQTx(0);	// Disable Interrupts
			}
		}
		// No need to clear the Interrupt Flag
	}
}


