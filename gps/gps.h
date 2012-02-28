#ifndef	__GPS_H_
#define __GPS_H_

#define NMEA_BAUDRATE 4800
#define BRREG_GPS ((XTAL_FREQ * 1000000L)/(64L * NMEA_BAUDRATE) - 1)

#define GPS_DETECT_TIMEOUT_MS	(8000)  // 8sec to detect

#define HQ_BIT_TIME_US		(600)  // 600us for one bit.
#define HQII_TIMER ((( (XTAL_FREQ/4) * (HQ_BIT_TIME_US/2)) / 16) - 1 )
#define HQII_TIMER_CTRL ( (1<<2) | 2)	// 16:1 pre, on

extern byte is_equal(byte *p1, byte *p2, byte n);

// GPS and HQII functions
extern char ReceiveGPSTime(void);
extern char ReceiveHQTime(void);
extern void CalculateHQDate(void);
			
extern byte hq_data[];

extern volatile byte hq_enabled; 
extern volatile byte hq_active; 

#endif		// __GPS_H_
