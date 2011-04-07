/******************************************************************************/
/*                             Start of crcmodel.h                            */
/******************************************************************************/
/* The following #ifndef encloses this entire */
/* header file, rendering it indempotent.     */
#ifndef CM_DONE
#define CM_DONE
/******************************************************************************/

/* The following definitions are extracted from my style header file which    */
/* would be cumbersome to distribute with this package. The DONE_STYLE is the */
/* idempotence symbol used in my style header file.                           */

#ifndef DONE_STYLE

#ifndef bool
typedef char  bool;
#endif

#ifndef byte
typedef unsigned char  byte;
#endif

typedef unsigned char * p_ubyte_;

#ifndef TRUE
#define FALSE 0
#define TRUE  1
#endif

/* Change to the second definition if you don't have prototypes. */
#define P_(A) A
/* #define P_(A) () */

/* Uncomment this definition if you don't have void. */
/* typedef int void; */

#endif

/***************************************************************************/
/* Functions That Implement The Model */
/* ---------------------------------- */
/* The following functions animate the cm_t abstraction. */

extern byte cm;

/* Initializes the argument CRC model instance.          */
#define cm_ini() (cm)= 0

/* Processes a single message byte [0,255]. */
void cm_nxt P_((byte ch));

/* Processes a block of message bytes. */
void cm_blk P_((p_ubyte_ blk_adr, byte blk_len));

/* Append the calculated crc as the last byte of the block. */
void cm_append(p_ubyte_ blk_adr, byte blk_len);

#endif

/******************************************************************************/
/*                             End of crcmodel.h                              */
/******************************************************************************/
