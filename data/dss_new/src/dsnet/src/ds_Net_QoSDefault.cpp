/*===========================================================================
  FILE: QoSDefault.cpp

  OVERVIEW: This file provides implementation of the QoSDefault class.

  DEPENDENCIES: None

  Copyright (c) 2007 - 2011 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoSDefault.cpp#1 $
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
#include "ds_Net_QoSDefault.h"
#include "ds_Net_Platform.h"
#include "ds_Net_Conversion.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECSignalBus.h"
#include "ds_Net_EventManager.h"
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

QoSDefault::QoSDefault
(
  int32 ifaceHandle,
  int32 flowHandle,
  IPrivSet * pPrivSet,
  NetworkModeType networkMode
)
: QoS (ifaceHandle, flowHandle, pPrivSet, networkMode),
  mQoSDefaultJson(flowHandle, pPrivSet),
  refCnt (1),
  weakRefCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p, if handle 0x%x, flow handle 0x%x",
    this, ifaceHandle, flowHandle);

  // TODO those can fail - constructor is not a good place for this init...
  (void) DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                 0, (void **) &mpSigBusInfoCodeUpdated);
  (void) DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                 0, (void **) &mpSigBusModifyResult);
  (void) DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                 0, (void **) &mpSigBusDefaultModify);

  Handle::Init(EventManager::qosObjList);

} /* QoSDefault::QoSDefault() */

void QoSDefault::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF (mpSigBusInfoCodeUpdated);
  DS_UTILS_RELEASEIF (mpSigBusModifyResult);
  DS_UTILS_RELEASEIF (mpSigBusDefaultModify);

  //Should be last statement. Call destructor for the base class.
  QoS::Destructor();

} /* QoSDefault::Destructor() */

QoSDefault::~QoSDefault
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  //NO-OP: only used for freeing memory.

} /* QoSDefault::~QoSDefault() */


/*-------------------------------------------------------------------------
   Handle interface forwarders:
-------------------------------------------------------------------------*/
ds::ErrorType QoSDefault::OnStateChange 
(
  ::ISignal*            signalObj, 
  ds::Net::EventType    eventID,
  IQI**                 regObj
)
{
  return Handle::OnStateChange(signalObj, eventID, regObj);
}

/*---------------------------------------------------------------------------
  Inherited functions from IQoSDefault.
---------------------------------------------------------------------------*/
ds::ErrorType QoSDefault::Modify
(
   const char* requestedSpec,
   QoSModifyMaskType mask,
   char* errSpec,
   int errSpecLen,
   int* errSpecLenReq
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  return mQoSDefaultJson.Modify(requestedSpec, mask, errSpec, errSpecLen, errSpecLenReq);
}
/*---------------------------------------------------------------------------
  Inherited functions from IQoSDefaultPriv.
---------------------------------------------------------------------------*/
int QoSDefault::ModifyDefaultPriv
(
  const QoSSpecPrimaryType* requestedSpec,
  QoSModifyMaskType mask
)
{
  int32                          result;
  PrimaryQoSModifyType           modifyInfo;
  PrimaryQoSModifySpecType       modifySpec;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                (
                                  (AEEPRIVID *)&AEEPRIVID_PNetSystem,
                                  sizeof(AEEPRIVID_PNetSystem)
                                )
      ||
      AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                (
                                (AEEPRIVID *)&AEEPRIVID_PrimaryQoSSession,
                                sizeof(AEEPRIVID_PrimaryQoSSession)
                                )
     )
  {
    LOG_MSG_ERROR( "No privilege for Modify operation", 0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  /*-------------------------------------------------------------------------
    Create arguments for PRIMARY_QOS_MODIFY ioctl.
  -------------------------------------------------------------------------*/
  memset (&modifySpec, 0, sizeof (modifySpec));
  modifyInfo.primary_qos_spec_ptr = &modifySpec;

  (void) Conversion::DS2PSQoSModifySpec (requestedSpec->rxFlows,
                                          requestedSpec->rxFlowsLen,
                                          requestedSpec->txFlows,
                                          requestedSpec->txFlowsLen,
                                          mask,
                                          &modifySpec);

  /*-----------------------------------------------------------------------
    Call the flow IOCTL to modify the primary QoS flow.
  -----------------------------------------------------------------------*/
  result = FlowIoctl  (GetHandle(),
                       FLOW_IOCTL_PRIMARY_QOS_MODIFY,
                       static_cast <void *> (&modifyInfo));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  Conversion::CleanupPSQoSModifySpec (&modifySpec);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  Conversion::CleanupPSQoSModifySpec (&modifySpec);
  return result;
} /* Modify() */


/*---------------------------------------------------------------------------
  Inherited functions from IQoS.
---------------------------------------------------------------------------*/
ds::ErrorType QoSDefault::GetGrantedFlowSpec
(
  char* rxFlow,
  int rxFlowLen,
  int* rxFlowLenReq,
  char* txFlow,
  int txFlowLen,
  int* txFlowLenReq
)
{
  return QoS::GetGrantedFlowSpecInternal(
    FLOW_IOCTL_PRIMARY_QOS_GET_GRANTED_FLOW_SPEC,
    rxFlow,
    rxFlowLen,
    rxFlowLenReq,
    txFlow,
    txFlowLen,
    txFlowLenReq);
} /* GetGrantedFlowSpec() */

  ds::ErrorType QoSDefault::GetTXPhysLink 
  (
    IPhysLink** txPhysLinkObj
  )
  {
    return QoS::GetTXPhysLink(txPhysLinkObj);
  }

  ds::ErrorType QoSDefault::GetRXPhysLink 
  (
    IPhysLink** rxPhysLinkObj
  )
  {
    return QoS::GetRXPhysLink(rxPhysLinkObj);
  }

  ds::ErrorType QoSDefault::GetUpdatedInfoCode 
  (
    QoSInfoCodeType* infoCode
  )
  {
    return QoS::GetUpdatedInfoCode(infoCode);
  }

  ds::ErrorType QoSDefault::GetTXQueueLevel 
  (
    QoSTXQueueLevelType* TXQueueLevel
  )
  {
    if (NetworkMode::QDS_MONITORED == mNetworkMode)
    {
      return AEE_EUNSUPPORTED;
    }

    return QoS::GetTXQueueLevel(TXQueueLevel);
  }

  ds::ErrorType QoSDefault::GetTechObject 
  (
    AEEIID iid,
    void** ppo
  )
  {
    if (NetworkMode::QDS_MONITORED == mNetworkMode)
    {
      return AEE_EUNSUPPORTED;
    }

    return QoS::GetTechObject(iid, ppo);
  }

/*---------------------------------------------------------------------------
  Inherited functions from IQoS.
---------------------------------------------------------------------------*/
ds::ErrorType QoSDefault::GetGrantedFlowSpecPriv
(
 IQoSFlowPriv** rxFlow, 
 IQoSFlowPriv** txFlow
)
{
  return QoS::GetGrantedFlowSpecInternal(
    FLOW_IOCTL_PRIMARY_QOS_GET_GRANTED_FLOW_SPEC,
    rxFlow,
    txFlow);
}

ds::ErrorType QoSDefault::GetModifyResult
(
  QoSResultType* resultCode
)
{
  boolean ioctlArg = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  if (NULL == resultCode)
  {
    return QDS_EFAULT;
  }

  (void) FlowIoctl (GetHandle(),
                    FLOW_IOCTL_GET_PRIMARY_QOS_MODIFY_RESULT,
                    (void *) (&ioctlArg));

  if (TRUE == ioctlArg)
  {
    *resultCode = QoSModifyResult::QDS_ACCEPTED;
  }
  else
  {
    *resultCode = QoSModifyResult::QDS_REJECTED;
  }

  return AEE_SUCCESS;

}/* GetModifyResult() */


ds::ErrorType QoSDefault::GetSignalBus
(
  ds::Net::EventType  eventID,
  ISignalBus **       ppISigBus
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == ppISigBus)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    ASSERT (0);
    return QDS_EFAULT;
  }


  switch (eventID)
  {
    case  QoSEvent::QDS_EV_INFO_CODE_UPDATED:
      *ppISigBus = mpSigBusInfoCodeUpdated;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS;

    case  QoSEvent::QDS_EV_MODIFY_RESULT:
      *ppISigBus = mpSigBusModifyResult;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS;

    case QoSDefaultEvent::QDS_EV_MODIFIED:
      *ppISigBus = mpSigBusDefaultModify;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS;

    default:
      *ppISigBus = NULL;
      return QDS_EINVAL;
  }

} /* GetSignalBus() */

int QoSDefault::QueryInterface
(
  AEEIID iid,
  void **ppo
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  switch (iid)
  {
  case AEEIID_IQoSDefault:
    *ppo = static_cast <IQoSDefault *> (this);
    (void) AddRef();
    break;

  case AEEIID_IQoSDefaultPriv:
    *ppo = static_cast <IQoSDefaultPriv *> (this);
    (void) AddRef();
    break;

  case AEEIID_IQI:
    *ppo = reinterpret_cast <void *> (this);
    (void) AddRef ();
    break;

  default:
    return AEE_ECLASSNOTSUPPORT;
  }

  return AEE_SUCCESS;

}/* QueryInterface() */

