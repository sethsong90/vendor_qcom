/*===========================================================================
  FILE: QoSManager.cpp

  OVERVIEW: This file provides implementation of the QoSManager class.

  DEPENDENCIES: None

  Copyright (c) 2008 - 2011 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_QoSManager.cpp#2 $
  $DateTime: 2011/06/29 08:15:52 $$Author: smudired $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-12-22 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Net_QoSManager.h"
#include "ds_Net_Platform.h"
#include "ds_Net_QoSFlowSpec.h"
#include "ds_Net_IPFilterSpec.h"
#include "ds_Net_QoSSecondary.h"
#include "ds_Net_QoSDefault.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_EventManager.h"
#include "ds_Net_Conversion.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECSignalBus.h"
#include "ds_Net_Privileges_Def.h"
#include "AEEIPrivSet.h"
#include "ds_Net_QoSSecondariesOutput.h"
#include "ds_Net_QoSSecondariesInput.h"
#include "ps_system_heap.h"

using namespace ds::Error;
using namespace ds::Net;
using namespace ds::Net::Conversion;
using namespace NetPlatform;

QoSManager::QoSManager
(
  int32 ifaceHandle,
  IPrivSet      * pPrivSet,
  NetworkModeType networkMode

)
: Handle (ifaceHandle),
  mQoSManagerJason(ifaceHandle, pPrivSet, networkMode),
  mpPrivSet(pPrivSet),
  mNetworkMode(networkMode),
  refCnt (1),
  weakRefCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p, if handle 0x%x, mode %d",
                 this, ifaceHandle, networkMode);

  ASSERT(0 != mpPrivSet);

  (void) mpPrivSet->AddRef();

  (void) DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                 0, (void **) &mpSigBusProfilesChanged);

  mpQoSDefault = NULL;

  Handle::Init(EventManager::qosObjList);

}

void QoSManager::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);

  DS_UTILS_RELEASEIF (mpPrivSet);
  DS_UTILS_RELEASEIF (mpQoSDefault);
  DS_UTILS_RELEASEIF (mpSigBusProfilesChanged);

  //Should be last statement. Call destructor for the base class.
  Handle::Destructor();

} /* QoSManager::Destructor() */

QoSManager::~QoSManager()
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  mpQoSDefault = NULL;
  mpPrivSet = NULL;

} /* QoSManager::~QoSManager() */


/*---------------------------------------------------------------------------
  Inherited functions from IQoSManager.
---------------------------------------------------------------------------*/
ds::ErrorType QoSManager::GetQosDefault
(
  ::IQoSDefault**  ppIDSNetQoSDefault
)
{
  int32                 result;
  int32                 ifaceHandle;
  int32                 flowHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  if (NULL == ppIDSNetQoSDefault)
  {
    result = QDS_EFAULT;
    goto bail;
  }

  /*-------------------------------------------------------------------------
  check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                (
                                  (AEEPRIVID *)&AEEPRIVID_PNetSystem,
                                  sizeof(AEEPRIVID_PNetSystem)
                                ))
  {
    LOG_MSG_ERROR( "No privilege for GetQosDefault operation", 0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  if (NULL == mpQoSDefault)
  {
    /*-----------------------------------------------------------------------
      Get the default flow handle for the iface
    -----------------------------------------------------------------------*/
    ifaceHandle = GetHandle();
    result = NetPlatform::PSGetDefaultFlow(ifaceHandle, &flowHandle);
    if (AEE_SUCCESS != result)
    {
      LOG_MSG_ERROR( "GetQosDefault(): Couldn't get default flow "
                     "on handle 0x%x, err 0x%x", ifaceHandle, result, 0);
      goto bail;
    }

    mpQoSDefault =
      new QoSDefault(ifaceHandle, flowHandle, mpPrivSet, mNetworkMode);
    if (NULL == mpQoSDefault)
    {
      result = AEE_ENOMEMORY;
      goto bail;
    }
  }

  (void) mpQoSDefault->AddRef();
  *ppIDSNetQoSDefault = static_cast<IQoSDefault *> (mpQoSDefault);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR("Err 0x%x", result, 0, 0);
  return result;

} /* GetQosDefault() */


ds::ErrorType QoSManager::RequestSecondary
(
  const QoSSpecType* pQoSSpec,
  ::IQoSSecondary** ppIDSNetQoSSecondary
)
{
  int32              result;
  QoSSecondary      *pSecQoS = NULL;
  QoSRequestType     qosRequestInfo;
  PSQoSSpecType      localQoSSpec;
  int32              ifaceHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == ppIDSNetQoSSecondary || NULL == pQoSSpec)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Validate QoS spec and construct QOS_REQUEST IOCTL arguments.
  -------------------------------------------------------------------------*/
  memset (&qosRequestInfo, 0, sizeof(qosRequestInfo));

  qosRequestInfo.flow_ptr = NULL;
  qosRequestInfo.subset_id = 0;
  qosRequestInfo.qos_ptr = &localQoSSpec;

  result = DS2PSQoSRequestSpec(pQoSSpec, qosRequestInfo.qos_ptr);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Perform QoS Request IOCTL on the iface handle to get the secondary flow.
  -------------------------------------------------------------------------*/
  ifaceHandle = GetHandle();

  result = IfaceIoctl (ifaceHandle,
                       IFACE_IOCTL_QOS_REQUEST,
                       static_cast <void *> (&qosRequestInfo));

  /*-------------------------------------------------------------------------
    Get the err mask set in the QoS Request info argument.
  -------------------------------------------------------------------------*/
  PS2DSQoSSpec (qosRequestInfo.qos_ptr,
                const_cast <QoSSpecType*> (pQoSSpec));

  if (AEE_SUCCESS != result && AEE_EWOULDBLOCK != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Create a SecondaryQoS object using the this flow handle.
  -------------------------------------------------------------------------*/
  pSecQoS = new QoSSecondary (ifaceHandle,
                              (int32) qosRequestInfo.flow_ptr,
                              mpPrivSet,
                              mNetworkMode);
  if (NULL == pSecQoS)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  *ppIDSNetQoSSecondary = static_cast <IQoSSecondary *> (pSecQoS);

  /*-------------------------------------------------------------------------
    Free the memory allocated in DS2PSQoSRequestSpec and for qos_ptr.
  -------------------------------------------------------------------------*/
  (void) CleanupPSQoSRequestSpec (qosRequestInfo.qos_ptr);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  (void) CleanupPSQoSRequestSpec (qosRequestInfo.qos_ptr);
  DS_UTILS_RELEASEIF (pSecQoS);
  *ppIDSNetQoSSecondary = NULL;
  return result;

} /* RequestSecondary() */

ds::ErrorType QoSManager::Request
(
   const char* specs,
   QoSRequestOpCodeType opCode,
   IQoSSecondariesOutput** qosSessions,
   char* errSpec,
   int errSpecLen,
   int* errSpecLenReq
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  return mQoSManagerJason.Request(specs, opCode, qosSessions, errSpec, errSpecLen, errSpecLenReq);
} /* Request() */

ds::ErrorType QoSManager::RequestBundle
(
  const QoSSpecType* specs,
  int specsLen,
  QoSRequestOpCodeType opCode,
  IQoSSecondariesOutput** sessions
)
{
  int32                             result;
  int32                             index;
  QoSRequestExType                  qosRequestExInfo;
  QoSSecondary**                    ppQoSSecondary = NULL;
  int32                             ifaceHandle;
  QoSSecondariesOutput*             pQoSOutput = NULL;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == specs ||
      NULL == sessions ||
      0    >= specsLen)
  {
    LOG_MSG_ERROR ("Inv args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Construct QOS_REQUEST_EX_IOCTL arguments.
  -------------------------------------------------------------------------*/
  memset (&qosRequestExInfo, 0, sizeof (QoSRequestExType));
  qosRequestExInfo.num_qos_specs = (uint8) specsLen;
  qosRequestExInfo.subset_id = 0;
  qosRequestExInfo.qos_control_flags = 0;

  if (opCode == QoSRequestOpCode::QDS_REQUEST)
  {
    qosRequestExInfo.opcode = PS_IFACE_IOCTL_QOS_REQUEST_OP;
  }
  else
  {
    qosRequestExInfo.opcode = PS_IFACE_IOCTL_QOS_CONFIGURE_OP;
  }


  /*-------------------------------------------------------------------------
  Allocate memory for flow_ptrs array. Here we are allocating memory
  for holding only the pointers to ps flows and not the entire
  flow structures.
  -------------------------------------------------------------------------*/
  qosRequestExInfo.flows_ptr =
    (ps_flow_type **)
    ps_system_heap_mem_alloc
    (
      sizeof (ps_flow_type *) * specsLen
    );
  if (NULL == qosRequestExInfo.flows_ptr)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Allocate memory for specs_ptr array.
  -------------------------------------------------------------------------*/
  qosRequestExInfo.qos_specs_ptr =
    (qos_spec_type *)
    ps_system_heap_mem_alloc
    (
      sizeof (qos_spec_type) * specsLen
    );
  if (NULL == qosRequestExInfo.qos_specs_ptr)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Fill the filter and flow specs into the bundled IOCTL request arg.
  -------------------------------------------------------------------------*/
  for (index = 0; index < specsLen; index++)
  {
    qosRequestExInfo.flows_ptr[index] = NULL;
    (void) DS2PSQoSRequestSpec (&specs[index],
      &qosRequestExInfo.qos_specs_ptr[index]);
  }

  /*-------------------------------------------------------------------------
    Allocate memory for secondary QoS objects array. Here we are allocating
    memory for only the array of pointers and not the QoSSecondary objects
    themselves.
  -------------------------------------------------------------------------*/
  ppQoSSecondary =
    (QoSSecondary**)
    ps_system_heap_mem_alloc
    (
    sizeof (QoSSecondary *) * specsLen
    );
  if (NULL == ppQoSSecondary)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  memset (ppQoSSecondary, 0, specsLen * sizeof(QoSSecondary *));

  /*-------------------------------------------------------------------------
    Issue a QOS_REQUEST_EX_IOCTL to request for a bundle of flows.
  -------------------------------------------------------------------------*/
  ifaceHandle = GetHandle();
  result = IfaceIoctl (ifaceHandle,
                       IFACE_IOCTL_QOS_REQUEST_EX,
                       static_cast <void *> (&qosRequestExInfo));

  for (index = 0; index < specsLen; index++)
  {
    /*-------------------------------------------------------------------------
      Get the err mask set in the QoS Request info argument.
    -------------------------------------------------------------------------*/
    PS2DSQoSSpec (&qosRequestExInfo.qos_specs_ptr[index],
                  const_cast <QoSSpecType*> (&specs[index]));
  }

  if (AEE_SUCCESS != result && AEE_EWOULDBLOCK != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Create corresponding secondary flow objects (QoSSecondary objects)
    associated with these flows.
  -------------------------------------------------------------------------*/
  pQoSOutput = new QoSSecondariesOutput();
  if (NULL == pQoSOutput)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  result = AEE_SUCCESS;
  for (index = 0;
       index < qosRequestExInfo.num_qos_specs && index < specsLen;
       index++)
  {
    ppQoSSecondary[index] =
      new QoSSecondary (ifaceHandle,
                        (int32) qosRequestExInfo.flows_ptr[index],
                        mpPrivSet,
                        mNetworkMode);
    if (NULL == ppQoSSecondary[index])
    {
      result = AEE_ENOMEMORY;
      goto bail;
    }
    pQoSOutput->AddQoSSecondary(ppQoSSecondary[index]);
  } /* for */

  /*-----------------------------------------------------------------------
    Populate the out parameter with the IQoSSecondary interface.
  -----------------------------------------------------------------------*/
  *sessions = static_cast<IQoSSecondariesOutput *> (pQoSOutput);
  /*-------------------------------------------------------------------------
    ***FALL-THROUGH***
  -------------------------------------------------------------------------*/
bail:
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err %d", result, 0, 0);
    if (NULL != ppQoSSecondary)
    {
      for (index = 0;
        index < qosRequestExInfo.num_qos_specs && index < specsLen;
        index++)
      {
        DS_UTILS_RELEASEIF (ppQoSSecondary[index]);
      }
    }
  }

  for (index = 0;
    index < qosRequestExInfo.num_qos_specs && index < specsLen;
    index++)
  {
    (void) CleanupPSQoSRequestSpec (&qosRequestExInfo.qos_specs_ptr[index]);
  }

  PS_SYSTEM_HEAP_MEM_FREE (qosRequestExInfo.flows_ptr);
  PS_SYSTEM_HEAP_MEM_FREE (qosRequestExInfo.qos_specs_ptr);
  PS_SYSTEM_HEAP_MEM_FREE (ppQoSSecondary);
  return result;

} /* RequestBundle() */

ds::ErrorType QoSManager::Close
(
  IQoSSecondariesInput* qosSessions
)
{
  int32                 result;
  int                   index;
  QoSReleaseExType      releaseExInfo;
  IQoSSecondary*        pIQoSSecondary = NULL;
  QoSSecondary*         pQoSSecondary = NULL;
  int32                 flowHandle;
  int                   sessionsLen;
  QoSSecondariesInput*  qoSSecondariesInput;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == qosSessions)
  {
    LOG_MSG_ERROR ("Inv args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Construct QOS_RELEASE_EX_IOCTL arguments.
  -------------------------------------------------------------------------*/
  qoSSecondariesInput = static_cast<QoSSecondariesInput *> (qosSessions);
  result = qoSSecondariesInput->GetNumElements(&sessionsLen);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  memset (&releaseExInfo, 0, sizeof(releaseExInfo));
  releaseExInfo.num_flows = (uint8) sessionsLen;
  releaseExInfo.flows_ptr =
    (ps_flow_type **)
    ps_system_heap_mem_alloc
    (
      sizeof (ps_flow_type*) * sessionsLen
    );
  if (NULL == releaseExInfo.flows_ptr)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  for (index = 0; index < sessionsLen; index++)
  {
    result = qoSSecondariesInput->GetNth(index, &pIQoSSecondary);
    if (AEE_SUCCESS != result)
    {
      goto bail;
    }

    pQoSSecondary = static_cast<QoSSecondary*>(pIQoSSecondary);
    releaseExInfo.flows_ptr[index] =
      (ps_flow_type *) pQoSSecondary->GetHandle();
  }

  /*-------------------------------------------------------------------------
    This is a bundled IOCTL operation to be performed on a set of flows.
    However, since this is a flow IOCTL, we need to specify a single flow
    handle to drive the FlowIoctl function. The flow handle in this case
    would be the first flow the bundle.
  -------------------------------------------------------------------------*/
  flowHandle = (int32) releaseExInfo.flows_ptr[0];

  /*-------------------------------------------------------------------------
    Call bundled close for the secondary QoS flows.
  -------------------------------------------------------------------------*/
  result = FlowIoctl (flowHandle,
                      FLOW_IOCTL_QOS_RELEASE_EX,
                      static_cast <void *> (&releaseExInfo));
  if (AEE_SUCCESS != result && AEE_EWOULDBLOCK != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    TODO: When do the secondary flows get freed from QoS manager list??
  -------------------------------------------------------------------------*/
  PS_SYSTEM_HEAP_MEM_FREE (releaseExInfo.flows_ptr);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  PS_SYSTEM_HEAP_MEM_FREE (releaseExInfo.flows_ptr);
  return result;

} /* Close() */


ds::ErrorType QoSManager::Resume
(
  IQoSSecondariesInput* qosSessions
)
{
  int32                 result;
  int                   index;
  QoSResumeExType       resumeExInfo;
  IQoSSecondary*        pIQoSSecondary = NULL;
  QoSSecondary*         pQoSSecondary = NULL;
  int32                 flowHandle;
  int                   sessionsLen;
  QoSSecondariesInput*  qoSSecondariesInput;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == qosSessions)
  {
    LOG_MSG_ERROR ("Inv. args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Construct QOS_RESUME_EX_IOCTL arguments.
  -------------------------------------------------------------------------*/
  qoSSecondariesInput = static_cast<QoSSecondariesInput *> (qosSessions);
  result = qoSSecondariesInput->GetNumElements(&sessionsLen);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  memset (&resumeExInfo, 0, sizeof(resumeExInfo));
  resumeExInfo.num_flows = (uint8) sessionsLen;
  resumeExInfo.flows_ptr =
    (ps_flow_type **)
    ps_system_heap_mem_alloc
    (
      sizeof (ps_flow_type*) * sessionsLen
    );
  if (NULL == resumeExInfo.flows_ptr)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  for (index = 0; index < sessionsLen; index++)
  {
    result = qoSSecondariesInput->GetNth(index, &pIQoSSecondary);
    if (AEE_SUCCESS != result)
    {
      goto bail;
    }

    pQoSSecondary = static_cast<QoSSecondary*>(pIQoSSecondary);
    resumeExInfo.flows_ptr[index] =
      (ps_flow_type *) pQoSSecondary->GetHandle();
  }

  /*-------------------------------------------------------------------------
    This is a bundled IOCTL operation to be performed on a set of flows.
    However, since this is a flow IOCTL, we need to specify a single flow
    handle to drive the FlowIoctl function. The flow handle in this case
    would be the first flow the bundle.
  -------------------------------------------------------------------------*/
  flowHandle = (int32) resumeExInfo.flows_ptr[0];

  /*-------------------------------------------------------------------------
    Call bundled resume for the secondary QoS flows.
  -------------------------------------------------------------------------*/
  result = FlowIoctl (flowHandle,
                      FLOW_IOCTL_QOS_RESUME_EX,
                      static_cast <void *> (&resumeExInfo));
  if (AEE_SUCCESS != result && AEE_EWOULDBLOCK != result)
  {
    goto bail;
  }

  PS_SYSTEM_HEAP_MEM_FREE (resumeExInfo.flows_ptr);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  PS_SYSTEM_HEAP_MEM_FREE (resumeExInfo.flows_ptr);
  return result;

} /* Resume() */


ds::ErrorType QoSManager::Suspend
(
  IQoSSecondariesInput* qosSessions
)
{
  int32                 result;
  int                   index;
  QoSSuspendExType      suspendExInfo;
  IQoSSecondary*        pIQoSSecondary = NULL;
  QoSSecondary*         pQoSSecondary = NULL;
  int32                 flowHandle;
  int                   sessionsLen;
  QoSSecondariesInput*  qoSSecondariesInput;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == qosSessions)
  {
    LOG_MSG_ERROR ("Inv args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Construct QOS_SUSPEND_EX_IOCTL arguments.
  -------------------------------------------------------------------------*/
  qoSSecondariesInput = static_cast<QoSSecondariesInput *> (qosSessions);
  result = qoSSecondariesInput->GetNumElements(&sessionsLen);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  memset (&suspendExInfo, 0, sizeof(suspendExInfo));
  suspendExInfo.num_flows = (uint8) sessionsLen;
  suspendExInfo.flows_ptr =
    (ps_flow_type **)
    ps_system_heap_mem_alloc
    (
      sizeof (ps_flow_type*) * sessionsLen
    );
  if (NULL == suspendExInfo.flows_ptr)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  for (index = 0; index < sessionsLen; index++)
  {
    result = qoSSecondariesInput->GetNth(index, &pIQoSSecondary);
    if (AEE_SUCCESS != result)
    {
      goto bail;
    }

    pQoSSecondary = static_cast<QoSSecondary*>(pIQoSSecondary);
    suspendExInfo.flows_ptr[index] =
      (ps_flow_type *) pQoSSecondary->GetHandle();
  }

  /*-------------------------------------------------------------------------
    This is a bundled IOCTL operation to be performed on a set of flows.
    However, since this is a flow IOCTL, we need to specify a single flow
    handle to drive the FlowIoctl function. The flow handle in this case
    would be the first flow the bundle.
  -------------------------------------------------------------------------*/
  flowHandle = (int32) suspendExInfo.flows_ptr[0];

  /*-------------------------------------------------------------------------
    Call bundled suspend for the secondary QoS flows.
  -------------------------------------------------------------------------*/
  result = FlowIoctl (flowHandle,
                      FLOW_IOCTL_QOS_SUSPEND_EX,
                      static_cast <void *> (&suspendExInfo));
  if (AEE_SUCCESS != result && AEE_EWOULDBLOCK != result)
  {
    goto bail;
  }

  PS_SYSTEM_HEAP_MEM_FREE (suspendExInfo.flows_ptr);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  PS_SYSTEM_HEAP_MEM_FREE (suspendExInfo.flows_ptr);
  return result;
}/* Suspend() */

ds::ErrorType QoSManager::GetSupportedProfiles
(
  QoSProfileIdType            *profiles,
  int                         profilesLen,
  int                         *profilesLenReq
)
{
  int32              result;
  int32              nNumOfItemsToCopy = 0;
  ps_iface_ioctl_get_network_supported_qos_profiles_type stTempVal;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);
  /*-------------------------------------------------------------------------
    Perform iface IOCTL for GET_NETWORK_SUPPORTED_QOS_PROFILES on the iface
    handle associated with this QoS Manager object.
  -------------------------------------------------------------------------*/
  do
  {
    result = IfaceIoctl (GetHandle(),
                         IFACE_IOCTL_GET_NETWORK_SUPPORTED_QOS_PROFILES,
                         (void*)(&stTempVal));
    if (AEE_SUCCESS != result)
    {
      LOG_MSG_ERROR ("Err %d", result, 0, 0);
      break;
    }
    if(NULL != profilesLenReq)
    {
      *profilesLenReq = stTempVal.profile_count;
    }
    if(NULL == profiles)
    {
       if(0 != profilesLen)
       {
         result = QDS_EFAULT;
         break;
       }
       break;
    }
    if(0 == profilesLen)
    {
       break;
    }
    nNumOfItemsToCopy = MIN(stTempVal.profile_count, profilesLen);
    for(int i = 0; i < nNumOfItemsToCopy; i++)
    {
       profiles[i] = (QoSProfileIdType)stTempVal.profile_value[i];
    }
  } while (0);

  /* Error handling */
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err %d", result, 0, 0);
  }
  return result;

} /* GetSupportedProfiles() */

ds::ErrorType QoSManager::CreateQoSSecondariesInput
(
  IQoSSecondariesInput** newQoSSecondariesInput
)
{
  QoSSecondariesInput*  qoSSecondariesInput;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == newQoSSecondariesInput)
  {
    LOG_MSG_ERROR ("Inv args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Construct QoSSecondariesInput object.
  -------------------------------------------------------------------------*/
  qoSSecondariesInput = new QoSSecondariesInput();

  if (NULL == qoSSecondariesInput)
  {
    return AEE_ENOMEMORY;
  }

  *newQoSSecondariesInput = static_cast<IQoSSecondariesInput *> (qoSSecondariesInput);

  return AEE_SUCCESS;

}/* CreateQoSSecondariesInput() */

boolean QoSManager::Process
(
  void* userDataPtr
)
{
  EventInfoType*          eventInfoPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == userDataPtr)
  {
    return FALSE;
  }

  eventInfoPtr = (EventInfoType *)userDataPtr;

  if (EVENT_GROUP_QOS == eventInfoPtr->eventGroup)
  {
    if (GetHandle() == eventInfoPtr->handle)
    {
      this->Notify(eventInfoPtr->eventName);
    }
  }

  return TRUE;

} /* Process() */


ds::ErrorType QoSManager::OnStateChange
(
  ISignal*                pISignal,
  ds::Net::EventType      eventName,
  IQI**                   regObj
)
{
  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    if (QoSMgrEvent::QDS_EV_PROFILES_CHANGED != eventName)
    {
      return AEE_EUNSUPPORTED;
    }
  }

  return Handle::OnStateChange(pISignal, eventName, regObj);
} /* OnStateChange() */

ds::ErrorType QoSManager::GetSignalBus
(
  ds::Net::EventType  eventID,
  ISignalBus **       ppISigBus
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* TODO: What event is supported by IQoSManager? */
  if (NULL == ppISigBus)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    ASSERT (0);
    return AEE_EBADPARM;
  }

  switch (eventID)
  {

    case  QoSMgrEvent::QDS_EV_PROFILES_CHANGED:
      *ppISigBus = mpSigBusProfilesChanged;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS;

    default:
      *ppISigBus = NULL;
      return AEE_EBADPARM;
  }

} /* GetSignalBus() */


int QoSManager::QueryInterface (AEEIID iid, void **ppo)
{
  switch (iid)
  {
    case AEEIID_IQoSManager:
      *ppo = static_cast <IQoSManager *> (this);
      (void) AddRef();
      break;

    case AEEIID_IQoSManagerPriv:
      *ppo = static_cast <IQoSManagerPriv *> (this);
      (void) AddRef();
      break;

    case AEEIID_IQI:
      *ppo = reinterpret_cast <void *> (this);
      (void) AddRef ();
      break;

    default:
      return AEE_ECLASSNOTSUPPORT;
  }

  LOG_MSG_INFO3 ("Obj 0x%p, IID 0x%x, ref cnt %d", this, iid, refCnt);
  return AEE_SUCCESS;
}
