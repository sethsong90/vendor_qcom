#ifndef DSM_H
#define DSM_H
/*===========================================================================

                                  D S M . H

DESCRIPTION
  This file contains types and declarations associated with the DMSS Data
  Service Memory pool and services.

-----------------------------------------------------------------------------
Copyright (c) 2006 Qualcomm Technologies, Inc.
All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
-----------------------------------------------------------------------------
===========================================================================*/

#define DSM_MAJOR_VER_NO 3
#define DSM_MINOR_VER_NO 0

/*===========================================================================
                            EDIT HISTORY FOR FILE
                                      
  $Header: //linux/pkgs/proprietary/oncrpc/main/source/dsm/dsm.h#3 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
1/1/06      pjb    Created new dsm.h.  
===========================================================================*/

#include "comdef.h"
#include "customer.h"
#include "queue.h"

#include "dsm_item.h"
#include "dsm_init.h"
#include "dsm_kind.h"

#include "dsm_queue.h"

#ifdef DSM_TRACER
  #include "dsm_tracer.h"
#endif


#endif /* DSM_H */
