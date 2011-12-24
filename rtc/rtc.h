#ifndef	__RTC_H_
#define __RTC_H_

typedef struct BCD_date
{
	unsigned char   Valid;
	unsigned char	JulianDayH;
	unsigned char	JulianDayL;
	unsigned char	WeekDay;
	unsigned char	Century;
	unsigned char	Year;
	unsigned char	Month;
	unsigned char	Day;
	unsigned char	Hours;
	unsigned char	Minutes;
	unsigned char	Seconds;
	unsigned char	MilliSeconds_10;	// In 10 ms chunks
}RTC_date_t;

// Timer and RTC functions
extern volatile RTC_date_t	rtc_date;

extern void SetupRTC(void);
extern void GetRTCData(void);
extern void SetRTCData(void);
extern void SetRTCDataPart1(void);
extern void SetRTCDataPart2(void);
extern void SetNextSecond(void);

#endif		// __RTC_H_
