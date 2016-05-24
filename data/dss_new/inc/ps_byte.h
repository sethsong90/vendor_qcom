#ifndef PS_BYTE_H
#define PS_BYTE_H

/*===========================================================================
               Miscellaneous machine independent utilities

 Copyright 1991 Phil Karn, KA9Q

 Copyright (c) 1995-2009 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $PVCSPath: L:/src/asw/MM_DATA/vcs/dsbyte.h_v   1.0   08 Aug 2002 11:19:58   akhare  $
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_byte.h#1 $ $DateTime: 2011/01/10 09:44:56 $ $Author: maldinge $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
12/31/08    pp     Common Modem Interface: Public/Private API split.
10/24/08    pp     Fixed compile warnings observed in OFFTARGET build.
07/17/06    aw     Fixed a bug in get64 function.
08/15/05    msr    Added hput64()
01/27/05    ssh    This is the first incarnation of ps_byte.h, previously
                   known as dsbyte.h. Moved dsbyte.h from
                   //depot/asic/msmshared/mmdata/ds/ to
                   //depot/asic/msmshared/mmdata/ps/, renamed it as ps_byte.h
                   and edited it to include inlined versions of all get*()
                   and put*() functions to potentially improve performance.
                   The function definitions previously resided in
                   //depot/asic/msmshared/mmdata/ds/dsbyte.c, which was
                   deleted.
09/29/04    msr/ks Added hget16,hget32(),hput32(),hput16()
09/09/04    msr    Added put8, get8.
09/06/03    ifk    Added put64, get64().
11/12/01    dwp    Add "|| FEATURE_DATA_PS" to whole module.
05/24/00    hcg    Added TARGET_OS_SOLARIS to compile with Solaris DSPE.
04/21/00    mvl    Fixed a #define so compiles properly under COMET
01/09/99    jjw    Changed to generic Browser interface
10/27/98    ldg    For T_ARM included C version of TCP checksum routine.
06/16/98    na     Converted the routine that calculates the TCP checksum
                   into 186 assembly.
06/25/97    jgr    Added ifdef FEATURE_DS over whole file
07/22/95    jjw    Created Initial Version
===========================================================================*/
#include "comdef.h"
#include "amssassert.h"

/*lint -save -e1534 */

/*===========================================================================
FUNCTION get8

DESCRIPTION
  This function return the byte value from the 1 byte value

DEPENDENCIES
  None

RETURN VALUE
  A byte representing the 1 byte pointed at by the passed parameter

SIDE EFFECTS
  None
===========================================================================*/
INLINE byte get8
(
  byte *cp                /* pointer to byte string to get the 8 bits from */
)
{
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );
  return (*cp);

} /* get8() */


/*===========================================================================
FUNCTION get16

DESCRIPTION
  This function return the word value from the 2 byte value

DEPENDENCIES
  None

RETURN VALUE
  A word representing the 2 bytes pointed at by the passed parameter

SIDE EFFECTS
  None
===========================================================================*/
INLINE word get16
(
  byte *cp               /* pointer to byte string to get the 16 bits from */
)
{
  register word x;       /*  */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );

  x = *cp++;
  x <<= 8;
  x |= *cp;

  return( x);

} /* get16 */


/*===========================================================================
FUNCTION get32

DESCRIPTION
  This function
  Machine-independent, alignment insensitive network-to-host long conversion

DEPENDENCIES
  None

RETURN VALUE
  32 bit value representing the 4 network bytes in the passed char string.

SIDE EFFECTS
  None
===========================================================================*/
INLINE dword get32
(
  byte *cp               /*  */
)
{
  dword rval;            /*  */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );

  rval = *cp++;
  rval <<= 8;
  rval |= *cp++;
  rval <<= 8;
  rval |= *cp++;
  rval <<= 8;
  rval |= *cp;

  return( rval);

} /* get32 */


/*===========================================================================
FUNCTION hget16

DESCRIPTION
  This function extracts 16 bits in host order.

DEPENDENCIES
  None

RETURN VALUE
  16 bit value in host order representing the 2 network bytes in the passed
  char string.

SIDE EFFECTS
  None
===========================================================================*/
INLINE word hget16
(
  byte *cp               /*  */
)
{
  word rval;            /*  */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );
  cp += 2;

  rval = *(--cp);
  rval <<= 8;
  rval |= *(--cp);

  return( rval);

} /* hget16() */


/*===========================================================================
FUNCTION hget32

DESCRIPTION
  This function extracts 32 bits in host order.

DEPENDENCIES
  None

RETURN VALUE
  32 bit value in host order representing the 4 network bytes in the passed
  char string.

SIDE EFFECTS
  None
===========================================================================*/
INLINE dword hget32
(
  byte *cp               /*  */
)
{
  dword rval;            /*  */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );
  cp += 4;

  rval = *(--cp);
  rval <<= 8;
  rval |= *(--cp);
  rval <<= 8;
  rval |= *(--cp);
  rval <<= 8;
  rval |= *(--cp);

  return( rval);

} /* hget32() */


/*===========================================================================
FUNCTION hget64

DESCRIPTION
  This function extracts 64 bits in host order.

DEPENDENCIES
  None

RETURN VALUE
  64 bit value in host order representing the 8 network bytes in the passed
  char string.

SIDE EFFECTS
  None
===========================================================================*/
INLINE uint64 hget64
(
  byte *cp               /*  */
)
{
  uint64 rval;            /*  */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );
  cp += 8;

  rval = *(--cp);
  rval <<= 8;
  rval |= *(--cp);
  rval <<= 8;
  rval |= *(--cp);
  rval <<= 8;
  rval |= *(--cp);
  rval <<= 8;
  rval |= *(--cp);
  rval <<= 8;
  rval |= *(--cp);
  rval <<= 8;
  rval |= *(--cp);
  rval <<= 8;
  rval |= *(--cp);

  return( rval);

} /* hget64() */



/*===========================================================================
FUNCTION put8

DESCRIPTION
  This function writes 8 bits to the pointer and returns the updated pointer.

DEPENDENCIES
  None

RETURN VALUE
  Updated pointer

SIDE EFFECTS
  None
===========================================================================*/
INLINE byte *put8
(
  byte *cp,      /*  */
  byte  x         /*  */
)
{
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );

  *cp++ = x;
  return (cp);

} /* put8() */


/*===========================================================================
FUNCTION put16

DESCRIPTION
  This function

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
INLINE byte *put16
(
  byte *cp,      /*  */
  word x         /*  */
)
{
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );

  *cp++ = x >> 8;
  *cp++ = (x & 0xFF);

  return( cp);

} /* put16 */


/*===========================================================================
FUNCTION put32

DESCRIPTION
  This function

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
INLINE byte *put32
(
  byte *cp,             /*  */
  dword x               /*  */
)
{
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );

  *cp++ = (byte)(x >> 24);
  *cp++ = (byte)(x >> 16);
  *cp++ = (byte)(x >> 8);
  *cp++ = (byte)x;

  return( cp);

} /* put32 */


/*===========================================================================
FUNCTION hput16

DESCRIPTION
  This function

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
INLINE byte *hput16
(
  byte *cp,      /*  */
  word x         /*  */
)
{
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );

  cp += 2;
  *(--cp) = x >> 8;
  *(--cp) = (byte)x;

  return (cp + 2);

} /* hput16 */


/*===========================================================================
FUNCTION hput32

DESCRIPTION
  This function writes 32 bits to the pointer in host order and returns the
  updated pointer.

DEPENDENCIES
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
INLINE byte *hput32
(
  byte *cp,             /*  */
  dword x               /*  */
)
{
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );

  cp += 4;
  *(--cp) = (byte)(x >> 24);
  *(--cp) = (byte)(x >> 16);
  *(--cp) = (byte)(x >> 8);
  *(--cp) = (byte)x;

  return (cp + 4);

} /* hput32 */


/*===========================================================================
FUNCTION PUT64

DESCRIPTION
  This function copies a 64 bit value into a buffer

DEPENDENCIES
  Sufficient memory must be allocated to the buffer to correctly copy the
  64 bit value

RETURN VALUE
  Pointer in the buffer after the 64-bit quantity is returned

SIDE EFFECTS
  Copies the 64 bit quantity into the passed buffer
===========================================================================*/
INLINE byte *put64
(
  byte   *cp,             /* Buffer to which the 64-bit value is copied    */
  uint64  x               /* 64-bit value to be copied                     */
)
{
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );

  *cp++ = (byte)(x >> 56);
  *cp++ = (byte)(x >> 48);
  *cp++ = (byte)(x >> 40);
  *cp++ = (byte)(x >> 32);
  *cp++ = (byte)(x >> 24);
  *cp++ = (byte)(x >> 16);
  *cp++ = (byte)(x >> 8);
  *cp++ = (byte)x;

  return( cp);

} /* put64 */


/*===========================================================================
FUNCTION GET64

DESCRIPTION
  This function acquires and returns a 64 bit value from the passed buffer.

DEPENDENCIES
  The passed buffer must be at least 64-bit.

RETURN VALUE
  The 64-bit value acquried.

SIDE EFFECTS
  None
===========================================================================*/
INLINE uint64 get64
(
  byte   *cp              /* Buffer from which the 64-bit value is copied  */
)
{
  uint64 rval;            /* The 64-bit quantity to be returned            */
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );

  rval = *cp++;
  rval <<= 8;
  rval |= *cp++;
  rval <<= 8;
  rval |= *cp++;
  rval <<= 8;
  rval |= *cp++;
  rval <<= 8;
  rval |= *cp++;
  rval <<= 8;
  rval |= *cp++;
  rval <<= 8;
  rval |= *cp++;
  rval <<= 8;
  rval |= *cp;

  return( rval);

} /* get64() */



/*===========================================================================
FUNCTION HPUT64

DESCRIPTION
  This function copies a 64 bit value into a buffer in host order

DEPENDENCIES
  Sufficient memory must be allocated to the buffer to correctly copy the
  64 bit value

RETURN VALUE
  Pointer in the buffer after the 64-bit quantity is returned

SIDE EFFECTS
  Copies the 64 bit quantity into the passed buffer
===========================================================================*/
INLINE byte *hput64
(
  byte   *cp,             /* Buffer to which the 64-bit value is copied    */
  uint64  x               /* 64-bit value to be copied                     */
)
{
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

  ASSERT( NULL != cp );

  cp += 8;
  *(--cp) = (byte)(x >> 56);
  *(--cp) = (byte)(x >> 48);
  *(--cp) = (byte)(x >> 40);
  *(--cp) = (byte)(x >> 32);
  *(--cp) = (byte)(x >> 24);
  *(--cp) = (byte)(x >> 16);
  *(--cp) = (byte)(x >> 8);
  *(--cp) = (byte)x;

  return (cp + 8);

} /* hput64 */

/*lint -restore */
#endif /* PS_BYTE_H */
