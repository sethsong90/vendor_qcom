/*===========================================================================
  FILE: QoS1X.cpp

  OVERVIEW: This file provides implementation of the QoS1X class.

  DEPENDENCIES: None

  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoS1X.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-03-25 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Net_QoS1X.h"
#include "ds_Net_Platform.h"
#include "ds_Net_Privileges_Def.h"
#include "AEEIPrivSet.h"

using namespace ds::Net;
using namespace ds::Error;
using namespace NetPlatform;


/*===========================================================================

                     PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/

QoS1X::QoS1X
(
  int32 flowHandle,
  IPrivSet * pPrivSet
) 
: mFlowHandle(flowHandle),
  mpPrivSet(pPrivSet),
  refCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p, flow handle 0x%x", 
    this, flowHandle, 0);
  
  ASSERT(0 != mpPrivSet);

  (void) mpPrivSet->AddRef();

} /* QoS1X() */

QoS1X::~QoS1X
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF(mpPrivSet);

} /* ~QoS1X() */


/*---------------------------------------------------------------------------
  Inherited functions from IQoS1X.
---------------------------------------------------------------------------*/
int QoS1X::GetRMAC3Info 
(
  RMAC3InfoType* rmac3Info
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  return FlowIoctlNonNullArg (mFlowHandle,
                              FLOW_IOCTL_HDR_GET_RMAC3_INFO,
                              (void *)rmac3Info);

} /* GetRMAC3Info() */

int QoS1X::GetTXStatus 
(
  boolean* txStatus
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  return FlowIoctlNonNullArg (mFlowHandle,
                              FLOW_IOCTL_707_GET_TX_STATUS,
                              (void *)txStatus);

} /* GetTXStatus() */


int QoS1X::GetInactivityTimer 
(
  int* inactivityTimer
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  return FlowIoctlNonNullArg (mFlowHandle,
                              FLOW_IOCTL_707_GET_INACTIVITY_TIMER,
                              (void *)inactivityTimer);

} /* GetInactivityTimer() */


int QoS1X::SetInactivityTimer 
(
  int inactivityTimer
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                (
                                  (AEEPRIVID *)&AEEPRIVID_PTech1x,
                                  sizeof(AEEPRIVID_PTech1x)
                                ))
  {
    LOG_MSG_ERROR( "No privilege for SetInactivityTimer operation", 0, 0, 0);
    return AEE_EPRIVLEVEL;
  }


  return FlowIoctlNonNullArg (mFlowHandle,
                              FLOW_IOCTL_707_SET_INACTIVITY_TIMER,
                              (void *)&inactivityTimer);

} /* SetInactivityTimer() */

