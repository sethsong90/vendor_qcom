#ifndef HDRADDR_H
#define HDRADDR_H

/*===========================================================================

                        H D R   A D D R E S S 

DESCRIPTION
  This contains the definitions & declaration related to 128 bit HDR 
  addressing scheme.

Copyright (c) 2000 by Qualcomm Technologies, Inc.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

$PVCSPath: L:/src/asw/MSM4500/vcs/hdraddr.h_v   1.6   19 Mar 2001 21:12:58   dandrus  $
$Header: //depot/asic/sandbox/users/hmurari/qcmapi_porting/stubs/inc/hdraddr.h#1 $ $DateTime: 2009/06/27 18:01:11 $ $Author: hmurari $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
03/23/09   wsh     CMI: featurized CMI under FEATURE_CMI
02/24/09   wsh     Created Module from
                   //source/qcom/qct/modem/hdr/cp/main/latest/inc/hdraddr.h#1

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

/* <EJECT> */ 
/*===========================================================================

              DEFINITIONS AND CONSTANTS FOR ERROR CODES

===========================================================================*/

/* Length of Address in bits */
#define HDRADDR_LENGTH_IN_BITS 128

/* Length of Address in bytes */
#define HDRADDR_LENGTH_IN_BYTES 16

/* Type definition for 128 bit address. This includes ATI, Sector ID & 
   Subnet ID */
typedef unsigned char hdraddr_type[HDRADDR_LENGTH_IN_BYTES];

#ifndef FEATURE_CMI
#include "hdraddr_v.h"
#endif
#endif /* HDRADDR_H */
