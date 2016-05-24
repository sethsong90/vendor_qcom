#ifndef HDRERRNO_H
#define HDRERRNO_H
/*===========================================================================

           H D R   E R R O R   N U M B E R   D E F I N I T I O N S

DESCRIPTION
  This contains the definition of the HDR return codes (error numbers).
  Functions using this definition either return an error code, or set
  the global variable 'hdrerrno' to the appropriate value.
 

Copyright (c) 2000, 2001 by Qualcomm Technologies, Inc.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

$PVCSPath: O:/src/asw/COMMON/vcs/hdrerrno.h_v   1.5   25 Sep 2001 15:32:58   aneufeld  $
$Header: //depot/asic/sandbox/users/hmurari/qcmapi_porting/stubs/inc/hdrerrno.h#1 $ $DateTime: 2009/06/27 18:27:43 $ $Author: hmurari $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
08/08/07   wsh     Copied content of errno.h into this file to avoid filename
                   conflicts of errno.h in GSM build
09/25/01   ajn     Deprecated module.  Use errno.h instead.  This file exists
                   for backwards compatibility.
05/05/00   om      Created Module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#ifdef PLATFORM_LTK
#include <stdlib.h>
#endif

#include "IxErrno.h"
  /* Delegate hdr error numbers to errno.h */

/*===========================================================================

              DEFINITIONS AND CONSTANTS FOR ERROR CODES

===========================================================================*/

#define hdrerrno_enum_type errno_enum_type
  /* And use errno_enum_type for each hdrerrno_enum_type */

#endif /* HDRERRNO_H */
