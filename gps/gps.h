#ifndef	__GPS_H_
#define __GPS_H_

#define GPS_DETECT_TIMEOUT_MS	(4000)  	// 4sec to detect
#define HQ_DETECT_TIMEOUT_MS 	(4000)  	// 4sec to detect
#define DAGR_DETECT_TIMEOUT_MS	(8000)  	// 8sec to detect (the MSG 253 is sent once every 6 seconds)
#define DAGR_PROCESS_TIMEOUT_MS	(2000)  	// 2sec to process Time Transfer (5101) or Status (5040) Message

// GPS and HQII functions
extern char ReceiveDAGRTime(void);
extern char ReceiveGPSTime(void);
extern char ReceiveHQTime(void);
extern void CalculateHQDate(void);
			
extern byte hq_data[];

extern volatile byte hq_enabled; 

#endif		// __GPS_H_
