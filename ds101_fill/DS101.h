#ifndef __DS101_H__
#define __DS101_H_

enum MASTER_STATE
{
    MS_IDLE,
	MS_CONNECT,
	MS_AXID_EXCH,
	MS_REQ_DISC1,
	MS_RECONNECT,
	MS_CHECK_RR,
	MS_KEY_FILL_NAME,
	MS_KEY_FILL_DATA,
	MS_KEY_FILL_END,
	MS_KEY_ISSUE_NAME,
	MS_KEY_ISSUE_DATA,
	MS_KEY_ISSUE_END,
	MS_REQ_TERM,
    MS_REQ_DISC2,
	MS_DISC
};

extern char master_mode;
extern char master_state;

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

//extern DebugOutput Serial;
//extern SerialPort SerPort;

extern void MasterProcessIFrame(char *buffer, int size);
extern void MasterProcessSFrame(unsigned char Cmd);
extern void MasterProcessUFrame(unsigned char Cmd);
extern void MasterProcessIdle();
extern char IsMasterValidAddressAndCommand();


extern void SlaveProcessIFrame(char *buffer, int size);
extern void SlaveProcessSFrame(unsigned char Cmd);
extern void SlaveProcessUFrame(unsigned char Cmd);
extern void SlaveProcessIdle();
extern char IsSlaveValidAddressAndCommand();

extern void TxIFrame(char *pdata, int n_chars);
extern void TxSFrame(unsigned char Cmd);
extern void TxUFrame(unsigned char Cmd);
extern void TxAXID(void);

extern char KeyStorage[256];
extern int  KeyStorageIdx;
extern int  KeyStorageSize;

extern char KeyName[256];
extern int  KeyNameSize;

extern char KeyType;

#endif

