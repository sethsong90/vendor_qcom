/*===========================================================================
  @file ds_Net_QoSJson.cpp

  This file provides implementation of the QoSJson class using Json strings.

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoSJson.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-08-02 en  Created module.

===========================================================================*/
/*===========================================================================

  INCLUDE FILES FOR MODULE

===========================================================================*/
#include "AEEStdErr.h"
#include "AEEStd.h"
#include "ds_Net_QoSJson.h"
#include "ds_Net_Conversion.h"
#include "ds_Net_Platform.h"
#include "ds_Net_QoSJson2PS.h"
#include "AEEIPrivSet.h"
#include "ds_Net_Privileges_Def.h"

using namespace ds::Net;
using namespace ds::Error;
using namespace ds::Net::Conversion;
using namespace NetPlatform;

/*===========================================================================

PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/

QoSJson::QoSJson
(
  int32     ifaceHandle
)
 : mHandle(ifaceHandle),
   refCnt (1)
{
  LOG_MSG_INFO1 ("Creating object 0x%p, if handle 0x%x", this, ifaceHandle, 0);

} /* QoSManagerJson() */


void QoSJson::Destructor
(
  void 
)
 throw()
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

} /* QoSManagerJson::Destructor() */

QoSJson::~QoSJson()
throw()
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

} /* QoSManagerJson::~QoSManagerJson() */

ds::ErrorType QoSJson::GetGrantedFlowSpec 
(
   NetPlatform::FlowIoctlEnumType  ioctlKind,
   char* rxFlow, 
   int rxFlowLen, 
   int* rxFlowLenReq, 
   char* txFlow, 
   int txFlowLen, 
   int* txFlowLenReq
)
{
  int32                                     result;
  NetPlatform::QoSGetGrantedFlowSpecType    grantedFlowSpec;
  QoSJSon2PS                                localQoSJson2PS;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validation
  -------------------------------------------------------------------------*/
  if ((NULL == rxFlow && rxFlowLen >0) ||
      (NULL == txFlow && txFlowLen >0) ||
      NULL == rxFlowLenReq ||
      NULL == txFlowLenReq 
     )
  {
    return QDS_EFAULT;
  }

  if (NetPlatform::FLOW_IOCTL_QOS_GET_GRANTED_FLOW_SPEC2 != ioctlKind &&
    NetPlatform::FLOW_IOCTL_PRIMARY_QOS_GET_GRANTED_FLOW_SPEC != ioctlKind)
  {
    result = QDS_EFAULT;
    ASSERT (0);
    goto bail;
  }

  if (0 == mHandle)
  {
    result = QDS_EINVAL;
    goto bail;
  }

  memset (&grantedFlowSpec, 0, sizeof(grantedFlowSpec));

  /*-------------------------------------------------------------------------
  Perform flow IOCTL to get granted flow spec.
  -------------------------------------------------------------------------*/
  result = FlowIoctl (mHandle,
                      ioctlKind,
                      (void *)&grantedFlowSpec);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Convert to out args
  -------------------------------------------------------------------------*/
  result = localQoSJson2PS.GenerateJSonFromQoSFlow(&grantedFlowSpec.rx_ip_flow, rxFlow, rxFlowLen, rxFlowLenReq);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = localQoSJson2PS.GenerateJSonFromQoSFlow(&grantedFlowSpec.tx_ip_flow, txFlow, txFlowLen, txFlowLenReq);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  return result;
} /* GetGrantedFlowSpec() */
