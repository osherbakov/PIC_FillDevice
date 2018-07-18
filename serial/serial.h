#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "string.h"

#define DTD_BAUDRATE  	(2400)
#define NMEA_BAUDRATE 	(4800)
#define NMEA1_BAUDRATE 	(9600)
#define NMEA2_BAUDRATE 	(19200)
#define PC_BAUDRATE  	(9600)
#define MBITR_BAUDRATE  (9600)
#define DAGR_BAUDRATE	(9600)

#define DS101_BAUDRATE	(64000)
#define DS101_PERIOD_US	(1000000L/DS101_BAUDRATE)

#define HQII_BAUDRATE		(1667)
#define HQII_PERIOD_US		(1000000L/HQII_BAUDRATE)
#define HQII_HALFPERIOD_US	(HQII_PERIOD_US/2)
#define HQII_DELAY_EDGE_US 	(HQII_HALFPERIOD_US + (HQII_HALFPERIOD_US / 2 ) - 4)
#define HQII_WAIT_EDGE_US  	(HQII_HALFPERIOD_US - 4 )


#define DATA_POLARITY_RX 	(0x20)
#define DATA_POLARITY_TX 	(0x10)
#define DATA_POLARITY_RXTX	(DATA_POLARITY_RX | DATA_POLARITY_TX)	// Appropriate Data polarity for RS-232 connected without Level Shifter.

// Use negative logic
// because we don't use any RS-232 interface chips
// and generate/sense RS-232 signals directly
#define  START_BIT		(1)
#define  STOP_BIT		(0)
#define  DATA_BYTE(a)	~(a)

#define TX_MBITR_DELAY_MS	(10) 	  

extern volatile byte *tx_data;    // Pointer to the data to be sent out
extern volatile byte tx_count;    // Running number of bytes sent
extern volatile byte *rx_data;	  // Pointer to the start of the buffer
extern volatile byte rx_idx_in;	  // Index of the next IN byte received from the Serial Port
extern volatile byte rx_idx_out;  // Index of the next OUT byte taken from the Serial Port Buffer
extern volatile byte rx_idx_max;  // Last byte index

extern void PCInterface(void);

extern void open_eusart(unsigned int baudrate, unsigned char rxtx_polarity);
extern void rx_eusart_async(unsigned char *p_rx_data, byte max_size, unsigned int timeout);
extern byte rx_eusart(unsigned char *p_data, byte ncount, unsigned int timeout);
extern byte rx_eusart_count(void);
extern void rx_eusart_reset(void);
extern int 	rx_eusart_symbol(void);
extern byte rx_eusart_line(unsigned char *p_data, byte ncount, unsigned int timeout);
extern byte rx_eusart_data(unsigned char *p_data, byte ncount, unsigned int timeout);

extern void tx_eusart_async(const unsigned char *p_data, byte ncount);
extern void tx_eusart_flush(void);

extern void close_eusart(void);

extern void open_mbitr(void);
extern byte rx_mbitr(unsigned char *p_data, byte ncount);
extern void tx_mbitr(const unsigned char *p_data, byte ncount);
extern void close_mbitr(void);

#define tx_eusart_str(a) 	tx_eusart_async((a), strlen((char *)a))
#define tx_eusart_buff(a) 	tx_eusart_async((a), NUM_ELEMS(a))

#endif	// __SERIAL_H__
