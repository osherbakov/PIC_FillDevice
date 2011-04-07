#ifndef	__I2C_SW_H_
#define __I2C_SW_H_

extern void SWStopI2C ( void );
extern void SWStartI2C ( void );
extern void SWRestartI2C ( void );
extern void SWStopI2C ( void );

extern signed char SWAckI2C( char level );
extern unsigned int SWReadI2C(void);
extern signed char SWWriteI2C( unsigned char data_out );
extern signed char SWGetsI2C( unsigned char *rdptr, unsigned char length );
extern signed char SWPutsI2C( unsigned char *wrptr, unsigned char length );

#define STAT_OK			(0)
#define STAT_NO_ACK		(-1)
#define STAT_CLK_ERR 	(-2)

#define I2C_READ		(0x01)
#define I2C_WRITE		(0x00)

#define ACK		(0)
#define NOACK	(1)
#define NACK	(1)
#define READ	(1)


#define DelayI2C()  Delay10TCY()

#endif 	// __I2C_SW_H_
