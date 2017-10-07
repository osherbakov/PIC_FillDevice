#include "CRC16.h"

#define BITMASK(X) (1L << (X))

#define cm_poly  (0x1021)	// this is the CCITT16 Poly
#define cm_init  (0xFFFF)
#define cm_xorot (0xFFFF)
#define cm_refin  TRUE
#define cm_refot  TRUE
#define topbit   (BITMASK(16 - 1))


unsigned int cm_reg;     /* Context: Context during execution.     */


static unsigned char reflect8 (unsigned char v)
/* Returns the value v with the bottom 8 bits reflected. */
{
 unsigned char t = 0;
 if(v & 0x01) t |= 0x80;
 if(v & 0x02) t |= 0x40;
 if(v & 0x04) t |= 0x20;
 if(v & 0x08) t |= 0x10;
 if(v & 0x10) t |= 0x08;
 if(v & 0x20) t |= 0x04;
 if(v & 0x40) t |= 0x02;
 if(v & 0x80) t |= 0x01;
 return t;
}

static unsigned int reflect16 (unsigned int v)
/* Returns the value v with the 16 bits reflected. */
{
 unsigned int t = 0;
 if(v & 0x0001) t |= 0x8000;
 if(v & 0x0002) t |= 0x4000;
 if(v & 0x0004) t |= 0x2000;
 if(v & 0x0008) t |= 0x1000;
 if(v & 0x0010) t |= 0x0800;
 if(v & 0x0020) t |= 0x0400;
 if(v & 0x0040) t |= 0x0200;
 if(v & 0x0080) t |= 0x0100;
 if(v & 0x0100) t |= 0x0080;
 if(v & 0x0200) t |= 0x0040;
 if(v & 0x0400) t |= 0x0020;
 if(v & 0x0800) t |= 0x0010;
 if(v & 0x1000) t |= 0x0008;
 if(v & 0x2000) t |= 0x0004;
 if(v & 0x4000) t |= 0x0002;
 if(v & 0x8000) t |= 0x0001;
 return t;
}


/******************************************************************************/

void CRC16ini ()
{
	cm_reg = cm_init;
}

/******************************************************************************/

void CRC16nxt (unsigned char  ch)
{
 unsigned char   i;
 unsigned int  uch;
 
 if (cm_refin) ch = reflect8(ch);
 
 uch  = (unsigned int) ch;   

 cm_reg ^= (uch << (16 - 8));
 
 for (i = 0; i < 8; i++)
 {
  if (cm_reg & topbit)
     cm_reg = (cm_reg << 1) ^ cm_poly;
  else
     cm_reg <<= 1;
 }
}

/******************************************************************************/

void CRC16blk (unsigned char *blk_adr, int blk_len)
{
  while (blk_len--) CRC16nxt(*blk_adr++);
}

void CRC16appnd(unsigned char *p_buffer, int blk_len)
{
  unsigned int res;
  int   i;
  
  CRC16ini();

  for(i = 0; i < blk_len; i++)
  {
    CRC16nxt(p_buffer[i]);
  }

  res = CRC16crc();
  p_buffer[blk_len++] = res & 0x00FF;
  p_buffer[blk_len] = (res >> 8) & 0x00FF;
}

char CRC16chk(unsigned char *p_buffer, int blk_len)
{
    unsigned int res;
  int i;
  int	last_idx = blk_len - 2;
  
  CRC16ini();

  for(i = 0; i < last_idx; i++)
  {
    CRC16nxt(p_buffer[i]);
  }

  res = CRC16crc();
  return  (p_buffer[last_idx++] == (res & 0x00FF)) &&
			(p_buffer[last_idx] == ((res >> 8) & 0x00FF)) ? TRUE : FALSE;
}

unsigned int  CRC16crc ()
{
 if (cm_refot)
    return (cm_xorot ^ reflect16(cm_reg));
 else
    return (cm_xorot ^ cm_reg);
}
