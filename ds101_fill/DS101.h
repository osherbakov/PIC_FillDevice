#ifndef __DS101_H__
#define __DS101_H_

extern unsigned char CurrentAddress;
extern int			 CurrentNumber;
extern char			*CurrentName;
extern unsigned char CurrentCommand;
extern unsigned char ReceivedAddress;
extern unsigned char NewAddress;

extern char Disconnected;

extern int frame_len;
extern int frame_FDU;

extern unsigned char NR;      // Received Number
extern unsigned char NS;      // Send Number
extern unsigned char PF;      // Poll/Final Flag

extern void MasterProcessIFrame(char *buffer, int size);
extern void MasterProcessSFrame(unsigned char Cmd);
extern void MasterProcessUFrame(unsigned char Cmd);
extern void MasterProcessIdle(void);
extern char IsMasterValidAddressAndCommand(void);


extern void SlaveProcessIFrame(char *buffer, int size);
extern void SlaveProcessSFrame(unsigned char Cmd);
extern void SlaveProcessUFrame(unsigned char Cmd);
extern void SlaveProcessIdle(void);
extern char IsSlaveValidAddressAndCommand(void);

extern void TxIFrame(char *pdata, int n_chars);
extern void TxSFrame(unsigned char Cmd);
extern void TxUFrame(unsigned char Cmd);
extern void TxAXID(char mode);

enum DS_MODE{
	RX_RS232 = 0,
	RX_RS485 = 1,
	TX_RS232 = 2,
	TX_RS485 = 3
};	

extern char ProcessDS101(char mode);

#endif

