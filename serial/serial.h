#ifndef __SERIAL_H__
#define __SERIAL_H__


#define MBITR_BAUDRATE  (9600)
#define PLGR_BAUDRATE	(19200)
#define DATA_POLARITY  	(0x30)

#define BRREG_CMD ( ( (XTAL_FREQ * 1000000L)/(4L * 16L * MBITR_BAUDRATE)) - 1 )
#define BRREG_PLGR ( ( (XTAL_FREQ * 1000000L)/(4L * 16L * PLGR_BAUDRATE)) - 1 )

#define TIMER_MBITR 	( ( (XTAL_FREQ * 1000000L) / ( 4L * 16L * MBITR_BAUDRATE)) - 1 )
#define TIMER_MBITR_START 	( -(TIMER_MBITR/2) )
#define TIMER_MBITR_CTRL ( 15<<3 | 1<<2 | 2)

// Use negative logic
#define  START	(1)
#define  STOP	(0)

#define  RX_TIMEOUT1_PC	 	(5000)    // 5 seconds
#define  RX_TIMEOUT2_PC	 	(100)
#define  RX_TIMEOUT1_MBITR 	(3000)  // 3 seconds
#define  RX_TIMEOUT2_MBITR 	(100)   // 100ms

#define TX_MBITR_DELAY_MS	(10) 	  

extern volatile byte *tx_data;
extern volatile byte tx_count;
extern volatile byte *rx_data;	// Pointer to the start of the buffer
extern volatile byte rx_count;	// Number of symbols collected
extern volatile byte rx_count_1; // Last byte index

extern void PCInterface(void);

extern void open_eusart_rxtx(void);
extern void open_eusart_rx(void);
extern void start_eusart_rx(unsigned char *, byte );
extern byte rx_eusart(unsigned char *p_data, byte ncount);
extern void tx_eusart(unsigned char *p_data, byte ncount);
extern void close_eusart(void);


extern void open_mbitr_rxtx(void);
extern void open_mbitr_rx(void);
extern void start_mbitr_rx(unsigned char *, byte );
extern byte rx_mbitr(unsigned char *p_data, byte ncount);
extern void tx_mbitr(unsigned char *p_data, byte ncount);
extern void close_mbitr(void);

#endif	// __SERIAL_H__
