#ifndef MEMORY_H
#define MEMORY_H
/*==========================================================================

                         Standard Memory Header

DESCRIPTION
  The ARM compiler does not include a memory.h header, so we provide one
  here and point to the standard ARM include file which does have
  memory prototypes.

-----------------------------------------------------------------------------
Copyright (c) 1997-1999, 2002-2005 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
===========================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

$Header: //linux/pkgs/proprietary/oncrpc/main/source/dsm/memory.h#3 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
09/13/05   ck      Copied over from 6100 branch
07/12/02   jct     <string.h> has ANSI defines for mem* functions
01/25/01   day     Merged from MSM5105_COMMON.00.00.05.
                     Modified to pull in string.h for NT builds.
11/03/98   jct     Added the else clause to include <memory.h> for non-ARM builds.
07/10/98   jct     Revised for coding standard, removed unused code
01/01/98   bwj     Created

===========================================================================*/

#include <string.h>

#endif /* MEMORY_H */
