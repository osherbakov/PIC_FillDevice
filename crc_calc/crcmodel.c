/******************************************************************************/
/*                             Start of crcmodel.c                            */
/******************************************************************************/
/*                                                                            */
/* Author : Ross Williams (ross@guest.adelaide.edu.au.).                      */
/* Date   : 3 June 1993.                                                      */
/* Status : Public domain.                                                    */
/*                                                                            */
/* Description : This is the implementation (.c) file for the reference       */
/* implementation of the Rocksoft^tm Model CRC Algorithm. For more            */
/* information on the Rocksoft^tm Model CRC Algorithm, see the document       */
/* titled "A Painless Guide to CRC Error Detection Algorithms" by Ross        */
/* Williams (ross@guest.adelaide.edu.au.). This document is likely to be in   */
/* "ftp.adelaide.edu.au/pub/rocksoft".                                        */
/*                                                                            */
/* Note: Rocksoft is a trademark of Rocksoft Pty Ltd, Adelaide, Australia.    */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/* Implementation Notes                                                       */
/* --------------------                                                       */
/* To avoid inconsistencies, the specification of each function is not echoed */
/* here. See the header file for a description of these functions.            */
/* This package is light on checking because I want to keep it short and      */
/* simple and portable (i.e. it would be too messy to distribute my entire    */
/* C culture (e.g. assertions package) with this package.                     */
/*                                                                            */
/******************************************************************************/

#include "crcmodel.h"

#define CRC_POLY  (0x85)

byte cm;
/******************************************************************************/

/* The following definitions make the code more readable. */

#define BITMASK(X) (1 << (X))

/******************************************************************************/

static byte reflect (byte v)
/* Returns the value v with the bottom b [0,32] bits reflected. */
/* Example: reflect(0x3e23L,3) == 0x3e26                        */
{
 byte t = 0;
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

/******************************************************************************/

void cm_nxt (byte  ch)
{
 byte   i;

 ch = reflect(ch);
 cm ^= ch;
 for (i=0; i<8; i++)
 {
    if (cm & BITMASK(7))
       cm = (cm << 1) ^ CRC_POLY;
    else
       cm <<= 1;
 }
}

/******************************************************************************/

void cm_blk (p_ubyte_ blk_adr, byte blk_len)
{
	while (blk_len--) cm_nxt(*blk_adr++);
}

/******************************************************************************/

void cm_append(p_ubyte_ blk_adr, byte blk_len)
{
	byte crc;
	cm_ini();
	cm_blk(blk_adr, blk_len - 1);
	blk_adr[blk_len - 1] = 0xFF ^ reflect(cm);
}
/******************************************************************************/
/*                             End of crcmodel.c                              */
/******************************************************************************/
