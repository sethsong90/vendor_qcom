/*===========================================================================
  FILE: QoS.cpp

  OVERVIEW: This file provides implementation of the QoS class.

  DEPENDENCIES: None

  Copyright (c) 2007 - 2011 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoS.cpp#1 $
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
#include "ds_Net_QoS.h"
#include "ds_Net_Platform.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_EventManager.h"
#include "ds_Net_QoSFlowSpec.h"
#include "ds_Net_IQoS1x.h"
#include "ds_Net_QoS1X.h"
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
QoS::QoS
(
  int32 ifaceHandle,
  int32 flowHandle,
  IPrivSet * pPrivSet,
  NetworkModeType networkMode
) : Handle (flowHandle),
    mIfaceHandle (ifaceHandle),
    mpPhysLink (NULL),
    mpQoS1x(0),
    mQoSJson(flowHandle),
    mpPrivSet(pPrivSet),
    mNetworkMode(networkMode)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p, if handle 0x%x, flow handle 0x%x", 
    this, ifaceHandle, flowHandle);

  ASSERT(0 != mpPrivSet);

  (void) mpPrivSet->AddRef();

} /* QoS() */

void QoS::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF (mpPrivSet);

  DS_UTILS_RELEASEIF (mpPhysLink);

  DS_UTILS_RELEASEIF (mpQoS1x);
  
  Handle::Destructor();

} /* QoS::Destructor() */

QoS::~QoS
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  mpPhysLink = NULL;
  mpQoS1x = NULL;
  mpPrivSet = NULL;

} /* ~QoS() */

int QoS::GetTXPhysLink 
(
  ::IPhysLink** txPhysLinkObj
)
{
  int32                       result;
  int32                       physLinkHandle;
  int32                       flowHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validation
  -------------------------------------------------------------------------*/
  if (NULL == txPhysLinkObj)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;  
  }

  /*-------------------------------------------------------------------------
    A QoS object can at most be associated with only one flow object.
    If the phys link is already set, return the same.
  -------------------------------------------------------------------------*/
  if (NULL != mpPhysLink)
  {
    (void) mpPhysLink->AddRef();
    *txPhysLinkObj = static_cast <IPhysLink *> (mpPhysLink);
    return AEE_SUCCESS;
  }

  /*-------------------------------------------------------------------------
    Get the flow handle for the QoS object.
  -------------------------------------------------------------------------*/
  flowHandle = GetHandle();
  /*-------------------------------------------------------------------------
    Get the phys link associated with the flow handle.
  -------------------------------------------------------------------------*/
  result = NetPlatform::PSGetPhysLinkFromFlow (flowHandle, &physLinkHandle);
  if (AEE_SUCCESS != result || 0 == physLinkHandle)
  {
    /*-----------------------------------------------------------------------
      TODO:Review this
      Get the phys link from iface instead.
      This condition can happen if the iface is not yet brought up
      and hence the default flow is not bound to phys link.
    -----------------------------------------------------------------------*/
    result = NetPlatform::PSGetPhysLinkFromIface (mIfaceHandle, 
                                                  &physLinkHandle);
    if (AEE_SUCCESS != result || 0 == physLinkHandle)
    {
      goto bail;
    }
  }

  /*-------------------------------------------------------------------------
    Create a new phys link object and return the same.
  -------------------------------------------------------------------------*/
  mpPhysLink = new PhysLink (physLinkHandle, mpPrivSet, mNetworkMode);
  if (NULL == mpPhysLink)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  
  *txPhysLinkObj = static_cast <IPhysLink *> (mpPhysLink);
  (void) mpPhysLink->AddRef();
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  return result;

}/* GetTXPhysLink() */

int QoS::GetRXPhysLink 
(
  ::IPhysLink** rxPhysLinkObj
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Current implementation treats RX and TX as same phys links.
  -------------------------------------------------------------------------*/
  return GetTXPhysLink(rxPhysLinkObj);

}/* GetRXPhysLink() */

int QoS::GetModifyResult 
(
  QoSResultType* resultCode
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  return FlowIoctlNonNullArg (GetHandle(),
                              FLOW_IOCTL_GET_MODIFY_RESULT,
                              (void *) (resultCode));

}/* GetModifyResult() */


int QoS::GetUpdatedInfoCode 
(
  QoSInfoCodeType* infoCode
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  return FlowIoctlNonNullArg (GetHandle(),
                              FLOW_IOCTL_GET_FLOW_UPDATED_INFO_CODE,
                              (void *)infoCode);

}/* GetUpdatedInfoCode() */

int QoS::GetTXQueueLevel 
(
  QoSTXQueueLevelType* argTXQLevel
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  return FlowIoctlNonNullArg (GetHandle(),
                              FLOW_IOCTL_GET_TX_QUEUE_LEVEL,
                              (void *)argTXQLevel);

}/* GetTXQueueLevel() */


ds::ErrorType QoS::GetGrantedFlowSpecInternal 
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
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  return mQoSJson.GetGrantedFlowSpec(ioctlKind, rxFlow, rxFlowLen, rxFlowLenReq, txFlow, txFlowLen, txFlowLenReq);
}

int QoS::GetGrantedFlowSpecInternal 
(
  NetPlatform::FlowIoctlEnumType  ioctlKind,
  ::IQoSFlowPriv**               rxFlowObj, 
  ::IQoSFlowPriv**               txFlowObj
)
{
  int32                                     result;
  int32                                     flowHandle;
  NetPlatform::QoSGetGrantedFlowSpecType    grantedFlowSpec;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validation
  -------------------------------------------------------------------------*/
  if (NULL == txFlowObj || NULL == rxFlowObj)
  {
    return QDS_EFAULT;
  }

  *txFlowObj = NULL;
  *rxFlowObj = NULL;

  if (NetPlatform::FLOW_IOCTL_QOS_GET_GRANTED_FLOW_SPEC2 != ioctlKind &&
      NetPlatform::FLOW_IOCTL_PRIMARY_QOS_GET_GRANTED_FLOW_SPEC != ioctlKind)
  {
    result = QDS_EFAULT;
    ASSERT (0);
    goto bail;
  }

  flowHandle = GetHandle();
  if (0 == flowHandle)
  {
    result = QDS_EINVAL;
    goto bail;
  }

  memset (&grantedFlowSpec, 0, sizeof(grantedFlowSpec));

  /*-------------------------------------------------------------------------
    Perform flow IOCTL to get granted flow spec.
  -------------------------------------------------------------------------*/
  result = FlowIoctl (GetHandle(),
                      ioctlKind,
                      (void *)&grantedFlowSpec);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }


  /*-------------------------------------------------------------------------
    Convert to out args
  -------------------------------------------------------------------------*/
  *txFlowObj = static_cast <IQoSFlowPriv *> 
    (new QoSFlowSpec(&grantedFlowSpec.tx_ip_flow));
  *rxFlowObj = static_cast <IQoSFlowPriv *> 
    (new QoSFlowSpec(&grantedFlowSpec.rx_ip_flow));
  if (NULL == *txFlowObj || NULL == *rxFlowObj)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  delete *txFlowObj;
  delete *rxFlowObj;
  return result;

} /* GetGrantedFlowSpecInternal() */

ds::ErrorType QoS::GetQoS1x
(
  IQoS1x* *pQoS1x
)
{
  int32                                     result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validation
  -------------------------------------------------------------------------*/
  if(0 == pQoS1x)
  {
    result = QDS_EFAULT;
    goto bail;
  }
  /*-------------------------------------------------------------------------
    Create QoS1x object and cache it in QoS object, if not cached before
  -------------------------------------------------------------------------*/
  if(0 == mpQoS1x)
  {
    mpQoS1x = new QoS1X(GetHandle(), mpPrivSet);
    if(0 == mpQoS1x)
    {
      result = AEE_ENOMEMORY;
      goto bail;
    }
  }

  *pQoS1x = mpQoS1x;

  (void) mpQoS1x->AddRef();

  return AEE_SUCCESS;

bail:

  return result;
}/* GetQoS1x() */

ds::ErrorType QoS::GetTechObject
(
  AEEIID iid,
  void** ppo
)
{
  int32                         result;
  IQoS1x                        *pQoS1x = 0;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == ppo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  if (AEEIID_IQoS1x == iid)
  {
    /*------------------------------------------------------------------------
    No need to AddRef() here, because GetQoS1x adds reference count
    ------------------------------------------------------------------------*/
    result = GetQoS1x(&pQoS1x);
    if (AEE_SUCCESS == result)
    {
      *ppo = pQoS1x;
    }
    return result;
  }

  return AEE_ECLASSNOTSUPPORT;

} /* GetTechObject() */
