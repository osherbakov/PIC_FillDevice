#ifndef __HDLC_H_
#define __HDLC_H_


#define IsIFrame(cmd) ((cmd & 0x01) == 0x00)
#define IsSFrame(cmd) ((cmd & 0x03) == 0x01)
#define IsUFrame(cmd) ((cmd & 0x03) == 0x03)


#define SMASK (0x0F)
#define UMASK (0xEF)
#define PMASK (0x10)

// HDLC Flags and special symbols
#define   FLAG    (0x7E)
#define   ESCAPE  (0x1B)

#define   BROADCAST (0xFF)

#define   PF_BIT    (0x01 << 4)

// S-Frame definitions
#define  RR  	(0x0001)
#define  RNR  	(0x0005)
#define  REJ  	(0x0009)
#define  SREJ  	(0x000D)

// U-Frame definitions
#define  SNRM  	(0x0083)

#define  SIM  	(0x0007)
#define  RIM  	(0x0007)
#define  DISC  	(0x0043)
#define  RD  	(0x0043)

#define  UA  	(0x0063)
#define  DM  	(0x000F)
#define  UI  	(0x0003)
#define  UP  	(0x0023)
#define  RSET 	(0x008F)
#define  XID  	(0x00AF)

// Virtual functions that are pointing to the appropriate Phy call
extern int (*RxDS101Data)(char *p_data);
extern void (*TxDS101Data)(char *p_data, int n_count);
extern void (*WriteCharDS101)(char ch);
extern int (*ReadCharDS101)(void);


extern void OpenRS232(char Master);
extern void TxRS232Char(char ch);
extern int RxRS232Char(void);
extern void CloseRS232(void);

extern void OpenDTD(char Master);
extern void TxDTDChar(char ch);
extern int RxDTDChar(void);
extern void CloseDTD(void);

extern int RxRS232Data(char *p_data);
extern void TxRS232Data(char *p_data, int n_count);

extern void OpenRS485(char Master);
extern int RxRS485Data(unsigned char *p_data);
extern void TxRS485Data(unsigned char *p_data, int n_count);
extern void CloseRS485(void);

#endif

