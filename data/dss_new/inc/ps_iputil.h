#ifndef PS_IPUTIL_H
#define PS_IPUTIL_H
/*===========================================================================

                   I P   I N T E R N A L   U T I L I T Y

                           F U N C T I O N S

DESCRIPTION
  This file contains declarations for internal utility functions
  used by the IP module.

Copyright (c) 2002-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  $PVCSPath: L:/src/asw/MM_DATA/vcs/ps_iputil.h_v   1.1   31 Jan 2003 18:20:00   ubabbar  $
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_iputil.h#2 $ $DateTime: 2011/07/21 11:01:42 $ $Author: anupamad $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
07/20/11   gs      Removed inclusion of <target.h>
12/14/08   pp      Common Modem Interface: Public/Private API split.
10/13/04   vp      Removed inclusion of ps_iphdr.h and included ps_ip4_hdr.h.
                   Removed ntohip() and htonip() functions. Added declaration
                   of buffer_checksum() function.
11/11/03   rc/aku  Added parameter 'offset' to the cksum() function.
01/31/03   usb     Added an extra arg to ntohip() to support hdr copying only
07/25/02   om      Created module from ip.h

===========================================================================*/

/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "comdef.h"
#include "dsm.h"
#include "ps_pkt_info.h"



/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/
/*===========================================================================

FUNCTION     BUFFER_CKSUM()

DESCRIPTION  Compute the cksum of the passed buffer of data.

DEPENDENCIES Count should be even. Buffer has a uint16 boundary.

PARAMETERS   uint16*          - Buffer to be checksummed.
             uint16*          - Count of number of bytes in the buffer.

RETURN VALUE uint16  - Checksum.

SIDE EFFECTS None.
===========================================================================*/
uint16 buffer_cksum
(
  uint16 *buf,
  uint16 count
);

/*===========================================================================

FUNCTION CKSUM()

DESCRIPTION
  This function calculates the IP checksum.

DEPENDENCIES
  None

RETURN VALUE
  16 bit checksum

SIDE EFFECTS
  None
===========================================================================*/
uint16 cksum
(
  pseudo_header_type  *ph,
  struct dsm_item_s    *m,
  uint16               len,
  uint16               offset
);

/*===========================================================================

FUNCTION  lcsum()

DESCRIPTION
  This function computes the 1's-complement sum of the given data buffer.
  This uses a inline assembly routine for the 186 processor which reduces
  CPU utilization greatly.

  The inline assembly uses an unwound loop method to reduce iterations
  of loop.

  buf: ptr to the array of bytes that need to be checksummed.
  cnt: number of bytes that need to be checksummed.

DEPENDENCIES
  None

RETURN VALUE
  The ones complement sum of the bytes in the buffer.

SIDE EFFECTS
  None
===========================================================================*/
uint16 lcsum
(
  uint16 *buf,
  uint16 cnt
);

#endif  /* PS_IPUTIL_H */
