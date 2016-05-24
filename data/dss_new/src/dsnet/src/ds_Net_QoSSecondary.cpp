/*===========================================================================
  FILE: QoSSecondary.cpp

  OVERVIEW: This file provides implementation of the QoSSecondary class.

  DEPENDENCIES: None

  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoSSecondary.cpp#1 $
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
#include "ds_Net_QoSFlowSpec.h"
#include "ds_Net_QoSSecondary.h"
#include "ds_Net_Platform.h"
#include "ds_Net_Network.h"
#include "ds_Net_Conversion.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECSignalBus.h"
#include "ds_Net_EventManager.h"

using namespace ds::Net;
using namespace ds::Net::Conversion;
using namespace ds::Error;
using namespace NetPlatform;

/*===========================================================================

                     PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/
QoSSecondary::QoSSecondary
(
  int32           ifaceHandle,
  int32           flowHandle,
  IPrivSet *      pPrivSet,
  NetworkModeType networkMode
)
: QoS (ifaceHandle, flowHandle, pPrivSet, networkMode),
  mQoSSecondaryJson(ifaceHandle),
  refCnt (1),
  weakRefCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p, if handle 0x%x, flow handle 0x%x",
    this, ifaceHandle, flowHandle);

  (void) DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                 0, (void **) &mpSigBusInfoCodeUpdated);
  (void) DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                 0, (void **) &mpSigBusModifyResult);
  (void) DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                 0, (void **) &mpSigBusStateChange);

  Handle::Init(EventManager::qosObjList);

} /* QoSSecondary::QoSSecondary() */

void QoSSecondary::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF (mpSigBusInfoCodeUpdated);
  DS_UTILS_RELEASEIF (mpSigBusModifyResult);
  DS_UTILS_RELEASEIF (mpSigBusStateChange);

  //Should be last statement. Call destructor for the base class.
  QoS::Destructor();

} /* QoSSecondary::Destructor() */


QoSSecondary::~QoSSecondary
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  //NO-OP: only used for freeing memory.

} /* QoSSecondary::~QoSSecondary() */


/*---------------------------------------------------------------------------
  Inherited functions from IQoSSecondary.
---------------------------------------------------------------------------*/
int QoSSecondary::Close
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  return FlowIoctl  (GetHandle(),
                     FLOW_IOCTL_QOS_RELEASE,
                     NULL);

} /* Close() */

int QoSSecondary::Modify
(
  const char* requestedSpec,
  QoSModifyMaskType modifyMask,
  char* errSpec,
  int errSpecLen,
  int* errSpecLenReq
)
{

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  return mQoSSecondaryJson.Modify(requestedSpec, modifyMask, errSpec, errSpecLen, errSpecLenReq);
} /* Modify() */

int QoSSecondary::ModifySecondaryPriv
(
  const QoSSpecType* requestedSpec,
  QoSModifyMaskType modifyMask
)
{
  QoSModifyType                     modifyInfo;
  int32                             result;
  PSQoSSpecType                     localQoSSpec;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);


  if (NULL == requestedSpec)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Create IOCTL argument for QOS_MODIFY
  -------------------------------------------------------------------------*/
  memset (&modifyInfo, 0, sizeof (QoSModifyType));
  modifyInfo.subset_id = 0;

  memset (&localQoSSpec, 0, sizeof (localQoSSpec));
  modifyInfo.qos_ptr = &localQoSSpec;

  /*-------------------------------------------------------------------------
  Create Spec according to the mask
  -------------------------------------------------------------------------*/

  /* All other masks are set inside the conversion function */
  result = DS2PSQoSRequestSpec(requestedSpec, modifyInfo.qos_ptr);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    If a RX_SPEC_DELETE mask is set then we should set QOS_MASK_RX_FLOW mask
    and IPFLOW_MASK_NONE should be set in the flow.
    No need to check the min_req_flow and
    aux flow specs here as this will checked by lower layers.
  -------------------------------------------------------------------------*/

  if (QoSModifyMask::QDS_RX_SPEC_DELETE & modifyMask)
  {
    modifyInfo.qos_ptr->field_mask |= (uint32) QOS_MASK_RX_FLOW;
    if ( (uint32) IPFLOW_MASK_NONE !=
          modifyInfo.qos_ptr->rx.flow_template.req_flow.field_mask )
    {
      return QDS_EFAULT;
    }
    modifyInfo.qos_ptr->rx.flow_template.req_flow.field_mask =
      (uint32) IPFLOW_MASK_NONE;
    modifyInfo.qos_ptr->rx.flow_template.num_aux_flows = 0;
  }

  if (QoSModifyMask::QDS_TX_SPEC_DELETE & modifyMask)
  {
    modifyInfo.qos_ptr->field_mask |= (uint32) QOS_MASK_TX_FLOW;
    if ( (uint32) IPFLOW_MASK_NONE !=
         modifyInfo.qos_ptr->tx.flow_template.req_flow.field_mask )
    {
      return QDS_EFAULT;
    }
    modifyInfo.qos_ptr->tx.flow_template.num_aux_flows = 0;
  }

  /*-------------------------------------------------------------------------
    If the RX_FILTER is set then we should modify the RX filters, thus
    a QOS_MODIFY_MASK_RX_FLTR_MODIFY must be set in the spec
    that is passed to PS according to the semantics all the
    filters (new and old) should be provided.
  -------------------------------------------------------------------------*/
  if (QoSModifyMask::QDS_RX_FILTER & modifyMask)
  {
    modifyInfo.qos_ptr->field_mask |= (uint32) QOS_MODIFY_MASK_RX_FLTR_MODIFY;
  }

  /*-------------------------------------------------------------------------
    Same for the case of TX
  -------------------------------------------------------------------------*/

  if (QoSModifyMask::QDS_TX_FILTER & modifyMask)
  {
    modifyInfo.qos_ptr->field_mask |= (uint32) QOS_MODIFY_MASK_TX_FLTR_MODIFY;
  }

  /*-------------------------------------------------------------------------
    Call the flow IOCTL to modify the Qos session.
  -------------------------------------------------------------------------*/
  result = FlowIoctl  (GetHandle(),
                       FLOW_IOCTL_QOS_MODIFY,
                       static_cast <void *> (&modifyInfo));

  /*-------------------------------------------------------------------------
    Get the err mask set in the QoS Request info argument.
  -------------------------------------------------------------------------*/
  PS2DSQoSSpec (modifyInfo.qos_ptr,
                const_cast <QoSSpecType*> (requestedSpec));

  if ((AEE_SUCCESS != result) && (AEE_EWOULDBLOCK != result))
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Cleanup the dynamic memories allocated for the Qos specs.
  -------------------------------------------------------------------------*/
  (void) CleanupPSQoSRequestSpec (modifyInfo.qos_ptr);
  return AEE_SUCCESS;

bail:
  (void) CleanupPSQoSRequestSpec (modifyInfo.qos_ptr);
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  return result;

} /* ModifySecondaryPriv() */

int QoSSecondary::Resume
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  return FlowIoctl  (GetHandle(),
                     FLOW_IOCTL_QOS_RESUME,
                     NULL);

} /* Resume() */

int QoSSecondary::Suspend
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  return FlowIoctl  (GetHandle(),
                     FLOW_IOCTL_QOS_SUSPEND,
                     NULL);
} /* Suspend() */

int QoSSecondary::GetState
(
  QoSSecondaryStateInfoType* argStatusInfo
)
{
  PSFlowStateType        flowState;
  int32                  result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validation.
  -------------------------------------------------------------------------*/
  if (NULL == argStatusInfo)
  {
    return QDS_EFAULT;
  }

  if (0 == GetHandle())
  {
    argStatusInfo->infoCode  = QoSInfoCode::QDS_NOT_SPECIFIED;
    argStatusInfo->state     = QoSSecondaryState::QDS_UNAVAILABLE;
    return AEE_SUCCESS;
  }

  /*-----------------------------------------------------------------------
    Call the flow IOCTL to get the status of the Qos session.
  -----------------------------------------------------------------------*/
  result = FlowIoctl  (GetHandle(),
                       FLOW_IOCTL_QOS_GET_STATUS,
                       (void *)&flowState);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Call the flow IOCTL to get the info code of the Qos session.
  -----------------------------------------------------------------------*/
  result = FlowIoctl  (GetHandle(),
                       FLOW_IOCTL_GET_FLOW_UPDATED_INFO_CODE,
                       (void *)&(argStatusInfo->infoCode));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  argStatusInfo->state = PS2DSFlowState(flowState);

  LOG_MSG_INFO1 ("Obj 0x%p, state %d, info code %d",
    this, argStatusInfo->state, argStatusInfo->infoCode);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  return result;
} /* GetState() */


/*---------------------------------------------------------------------------
  Inherited functions from IQoS.
---------------------------------------------------------------------------*/
ds::ErrorType QoSSecondary::GetGrantedFlowSpec
(
  char* rxFlow,
  int rxFlowLen,
  int* rxFlowLenReq,
  char* txFlow,
  int txFlowLen,
  int* txFlowLenReq
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  return QoS::GetGrantedFlowSpecInternal(
    FLOW_IOCTL_QOS_GET_GRANTED_FLOW_SPEC2,
    rxFlow,
    rxFlowLen,
    rxFlowLenReq,
    txFlow,
    txFlowLen,
    txFlowLenReq);

} /* GetGrantedFlowSpec() */

/*---------------------------------------------------------------------------
  Inherited functions from IQoSPriv.
---------------------------------------------------------------------------*/
ds::ErrorType QoSSecondary::GetGrantedFlowSpecPriv
(
 ::IQoSFlowPriv** rxFlow,
 ::IQoSFlowPriv** txFlow
 )
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  return QoS::GetGrantedFlowSpecInternal(
    FLOW_IOCTL_QOS_GET_GRANTED_FLOW_SPEC2,
    rxFlow,
    txFlow);

} /* GetGrantedFlowSpecPriv() */


/*---------------------------------------------------------------------------
  Event processing.
---------------------------------------------------------------------------*/
boolean QoSSecondary::Process
(
  void* pUserData
)
{
  EventInfoType*          pEventInfo;
/*-------------------------------------------------------------------------*/

  /*-------------------------------------------------------------------------
    User data should never be NULL for event processing.
  -------------------------------------------------------------------------*/
  if (NULL == pUserData)
  {
    ASSERT (0);
    return FALSE;
  }

  pEventInfo = static_cast <EventInfoType *> (pUserData);

  LOG_MSG_INFO1 ("this handle 0x%x, ev 0x%x, ev handle 0x%x",
    GetHandle(), pEventInfo->eventName, pEventInfo->handle);

  if (GetHandle() == pEventInfo->handle)
  {
    /*-----------------------------------------------------------------------
      For FLOW_NULL event, reset the handle before notifying.
    -----------------------------------------------------------------------*/
    if (FLOW_NULL_EV == pEventInfo->psEventName)
    {
      SetHandle (0);
    }


    /*-----------------------------------------------------------------------
      Event belongs to this handle. Call Notify()
    -----------------------------------------------------------------------*/
    Notify (pEventInfo->eventName);
  }

  return TRUE;

} /* Process() */


ds::ErrorType QoSSecondary::GetSignalBus
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

    case QoSSecondaryEvent::QDS_EV_STATE_CHANGED:
      *ppISigBus = mpSigBusStateChange;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS;

    default:
      *ppISigBus = NULL;
      return QDS_EINVAL;
  }

} /* GetSignalBus() */

int QoSSecondary::QueryInterface
(
 AEEIID iid,
 void **ppo
 )
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  switch (iid)
  {
  case AEEIID_IQoSSecondary:
    *ppo = static_cast <IQoSSecondary *> (this);
    (void) AddRef();
    break;

  case AEEIID_IQoSSecondaryPriv:
    *ppo = static_cast <IQoSSecondaryPriv *> (this);
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
