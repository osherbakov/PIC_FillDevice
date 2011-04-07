#ifndef	__GPS_H_
#define __GPS_H_

#define NMEA_BAUDRATE 4800
#define BRREG_GPS ((XTAL_FREQ * 1000000L)/(64L * NMEA_BAUDRATE) - 1)

#define GPS_DETECT_TIMEOUT_MS	(8000)  // 8sec to detect

extern byte is_equal(byte *p1, byte *p2, byte n);

// Timer and RTC functions
extern char ReceiveGPSTime(void);

#endif		// __GPS_H_
