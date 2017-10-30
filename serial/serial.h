#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "string.h"

#define DTD_BAUDRATE  	(2400L)
#define NMEA_BAUDRATE 	(4800L)
#define PC_BAUDRATE  	(9600L)
#define MBITR_BAUDRATE  (9600L)
#define PLGR_BAUDRATE	(19200L)
#define DS101_BAUDRATE	(64000L)

#define DATA_POLARITY_RX 	(0x20)
#define DATA_POLARITY_TX 	(0x10)
#define DATA_POLARITY  		(0x30)		// Appropriate Data polarity for RS-232 connected without Level Shifter.

// Values for the Baud Rate Control registers
#define BRREG_DTD 	( ( (XTAL_FREQ * 1000000L)/(4L * 16L * DTD_BAUDRATE)) - 1 )
#define BRREG_MBITR ( ( (XTAL_FREQ * 1000000L)/(4L * 16L * MBITR_BAUDRATE)) - 1 )
#define BRREG_PC 	( ( (XTAL_FREQ * 1000000L)/(4L * 16L * PC_BAUDRATE)) - 1 )
#define BRREG_PLGR 	( ( (XTAL_FREQ * 1000000L)/(4L * 16L * PLGR_BAUDRATE)) - 1 )
#define BRREG_GPS 	( ( (XTAL_FREQ * 1000000L)/(4L * 16L * NMEA_BAUDRATE)) - 1)

// For MBITR we implement the Software USART - use TIMER6 as the bit timer
#define TIMER_MBITR 		( ( (XTAL_FREQ * 1000000L) / ( 4L * 16L * MBITR_BAUDRATE)) - 1 )
#define TIMER_MBITR_START 	( -(TIMER_MBITR/2) )
#define TIMER_MBITR_CTRL 	( 15<<3 | 1<<2 | 2)

// Use negative logic
// because we don't use any RS-232 interface chips
// and generate/sense RS-232 signals directly
#define  START	(1)
#define  STOP	(0)

#define TX_MBITR_DELAY_MS	(10) 	  

extern volatile byte *tx_data;    // Pointer to the data to be sent out
extern volatile byte tx_count;    // Running number of bytes sent
extern volatile byte *rx_data;	  // Pointer to the start of the buffer
extern volatile byte rx_idx;	    // Number of symbols collected
extern volatile byte rx_idx_max;  // Last byte index

extern void PCInterface(void);

extern void open_eusart(unsigned char baudrate_reg, unsigned char rxtx_polarity);
extern void rx_eusart_async(unsigned char *p_rx_data, byte max_size, unsigned int timeout);
extern byte rx_eusart(unsigned char *p_data, byte ncount, unsigned int timeout);
extern byte rx_eusart_line(unsigned char *p_data, byte ncount, unsigned int timeout);
extern void tx_eusart(unsigned char *p_data, byte ncount);
extern void flush_eusart(void);
extern void close_eusart(void);

extern byte rx_mbitr(unsigned char *p_data, byte ncount);
extern void tx_mbitr(unsigned char *p_data, byte ncount);
extern void close_mbitr(void);

#define tx_eusart_str(a) 	tx_eusart((a), strlen((char *)a))
#define tx_eusart_buff(a) 	tx_eusart((a), NUM_ELEMS(a))

#endif	// __SERIAL_H__
