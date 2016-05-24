#ifndef __DSS_MEMORY_MANAGEMENT_H
#define __DSS_MEMORY_MANAGEMENT_H

/*===================================================

FILE:  DSS_MemoryManagement.h

SERVICES:
A utility class to provide generic use of ps_scratchpad.h

=====================================================

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_MemoryManagement.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#ifdef FEATURE_DATA_PS
#include "ps_mem.h"
#include "ds_Utils_DebugMsg.h"
#include "DSS_Globals.h"
#include "DSS_EventHandlerNetApp.h"
#include "ps_system_heap.h"


// The size is of DSSEventHandlerNetApp because there is a member added
// this is the reason we can not use DSSEventHandler to calc the size.
// DSSEventHandlerQoS, DSSEventHandlerPrivIpv6Addr, DSSEventHandlerMCast,DSSEventHandlerMCastMBMSCtrl
// also add only one pointer so the handlers that inherit from them will be the same
// size as those that inherit from DSSEventHandlerNetApp
#define EVENT_HANDLER_SIZE                  (( sizeof( DSSEventHandlerNetApp) + 3) & ~3)

/**********************************************/
/**********************************************/
/* PS_MEM */
/**********************************************/
/**********************************************/

#define PS_MEM_NEW(T)  new T
#define PS_MEM_DELETE(T) \
   if (NULL != T) {      \
     delete T;           \
     T = NULL;           \
   }

#define PS_MEM_RELEASE(T)   \
   if (NULL != T) {         \
      (void)(T)->Release(); \
      T = NULL;             \
   }

#define PS_MEM_RELEASE_WEAK(T)   \
   if (NULL != T) {              \
   (void)(T)->ReleaseWeak();     \
   T = NULL;                     \
   }


// This function should be invoked during phone power-up
// currently invoked from psi_powerup
void DSSmem_pool_init();

#endif /* FEATURE_DATA_PS */


#endif // __DSS_MEMORY_MANAGEMENT_H
