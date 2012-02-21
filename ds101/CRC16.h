#ifndef __CRC16_H_
#define __CRC16_H_

#ifndef FALSE
	#define FALSE (0)
#endif

#ifndef TRUE
	#define TRUE (!FALSE)
#endif


extern  void CRC16ini(void);
extern  void CRC16nxt(unsigned char ch);
extern  void CRC16blk(unsigned char *blk_adr, int blk_len);
extern  void CRC16appnd(unsigned char *blk_adr, int blk_len);
extern  char CRC16chk(unsigned char *blk_adr, int blk_len);
extern  unsigned int CRC16crc(void);

#endif