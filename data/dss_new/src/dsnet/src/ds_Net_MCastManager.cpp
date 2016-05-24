/*===========================================================================
  FILE: MCastManager.cpp

  OVERVIEW: This file provides implementation of the MCastManager class.

  DEPENDENCIES: None

  Copyright (c) 2007-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_MCastManager.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-12-22 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "AEEStdErr.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Net_Utils.h"
#include "ds_Net_MCastManager.h"
#include "ds_Net_MCastManagerPriv.h"
#include "ds_Net_MCastSession.h"
#include "ds_Net_Platform.h"
#include "ds_Net_Conversion.h"
#include "AEECCritSect.h"
#include "ds_Net_MBMSSpec.h"
#include "ds_Net_MCastMBMSCtrl.h"
#include "ds_Net_Privileges_Def.h"
#include "ds_Net_MCastSessionsOutput.h"
#include "ds_Net_MCastSessionsInput.h"
#include "AEEIPrivSet.h"
#include "ps_system_heap.h"

using namespace ds::Error;
using namespace ds::Net;
using namespace ds::Net::Conversion;
using namespace NetPlatform;


MCastManager::MCastManager
(
  int32      ifaceHandle,
  IPrivSet      * pPrivSet,
  NetworkModeType networkMode
)
: mIfaceHandle (ifaceHandle),
  mpPrivSet(pPrivSet),
  mpMCastManagerPriv(0),
  mNetworkMode(networkMode),
  refCnt(1)
{
  LOG_MSG_INFO1 ("Creating 0x%p, if handle 0x%x", this, ifaceHandle, 0);

  ASSERT(0 != mpPrivSet);

  (void) mpPrivSet->AddRef();

  if (AEE_SUCCESS != DS_Utils_CreateInstance (NULL, 
                                              AEECLSID_CCritSect,
                                              NULL,
                                              (void **) &mpICritSect))
  {
    LOG_MSG_FATAL_ERROR ("Cannot create crit sect", 0, 0, 0);
    ASSERT (0);
  }
}

MCastManager::~MCastManager()
throw()
{
  LOG_MSG_INFO1 ("Deleteing 0x%p if handle 0x%x", this, mIfaceHandle, 0);
  
  mIfaceHandle = 0;

  DS_UTILS_RELEASEIF(mpMCastManagerPriv);

  DS_UTILS_RELEASEIF(mpPrivSet);

  /*lint -save -e1550, -e1551 */
  DS_UTILS_RELEASEIF(mpICritSect);
  /*lint -restore */
}


ds::ErrorType MCastManager::Join
(
  const ::ds::SockAddrStorageType addr,
  const char *                    mcastSpec,
  IMCastSession **                session,
  char *                          errSpec,
  int                             errSpecLen,
  int *                           errSpecLenReq
)
{
  int32              result;
  MCastSession      *mcastSessionPtr = NULL;
  MCastJoinType      mcastJoinInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == session || NULL == addr)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    JSON related arguments are for the future use
  -------------------------------------------------------------------------*/
  (void) mcastSpec;
  (void) errSpec;
  (void) errSpecLen;
  (void) errSpecLenReq;

  /*-------------------------------------------------------------------------
    Validate MCast join info and create IOCTL arguments to perform
    iface level IOCTL to join the multicast group.
  -------------------------------------------------------------------------*/
  memset (&mcastJoinInfo, 0, sizeof (MCastJoinType));
  result = DS2PSMCastJoinSpec (addr, &mcastJoinInfo);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Perform Mcast join IOCTL on the iface handle to join the multicast group.
    Create a MCastSession object for the session after the IOCTL is issued.
  -------------------------------------------------------------------------*/
  result = IfaceIoctl (mIfaceHandle,
                       IFACE_IOCTL_MCAST_JOIN,
                       static_cast <void *> (&mcastJoinInfo));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Use the Mcast handle obtained from the IOCTL to create a MCast
    session object.
  -----------------------------------------------------------------------*/
  mcastSessionPtr = 
    new MCastSession
        (
          mIfaceHandle,
          mcastJoinInfo.handle,
          MCastSessionCreation::MCAST_SESSION_CREATED_BY_JOIN,
          MCastJoinFlags::REG_SETUP_ALLOWED
        );
  if (NULL == mcastSessionPtr)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  *session = mcastSessionPtr;
  return AEE_SUCCESS;

bail:

  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  DS_UTILS_RELEASEIF(mcastSessionPtr);
  return result;

} /* Join() */

ds::ErrorType MCastManager::LeaveBundle 
(
  IMCastSessionsInput* sessions
)
{
  int32              result;
  MCastLeaveExType   leaveExInfo;
  MCastSession *      mcastSessionObj = 0;
  MCastSessionsInput * sessionsInput = 0;
  int                index;
  int32              sessionsLen = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("obj 0x%p", this, 0, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (0 == sessions)
  {
    LOG_MSG_ERROR ("Inv args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Validate sessions argument and construct MCAST_LEAVE_EX IOCTL arguments.
  -------------------------------------------------------------------------*/
  sessionsInput = static_cast<MCastSessionsInput *>(sessions);
  if(0 == sessionsInput)
  {
    ASSERT(0);
    LOG_MSG_ERROR ("Bad type cast", 0, 0, 0);
    result = QDS_EFAULT;
    goto bail;
  }
  (void )sessionsInput->GetNumOfSessions(&sessionsLen);

  memset (&leaveExInfo, 0, sizeof (MCastLeaveExType));
  
  leaveExInfo.num_flows = (uint8) sessionsLen;
  for (index = 0; index < sessionsLen; index++)
  {
    result = sessionsInput->GetNth(index, (IMCastSession **)&mcastSessionObj);
    
    if (AEE_SUCCESS != result || 0 == mcastSessionObj)
    {
      goto bail;
    }
    
    leaveExInfo.handle[index] = mcastSessionObj->GetMCastHandle();
    
    (void) mcastSessionObj->Release();
  }

  /*-------------------------------------------------------------------------
    Call bundled leave IOCTL on underlying iface to leave the mcast group.
  -------------------------------------------------------------------------*/
  result = IfaceIoctl (mIfaceHandle,
                       IFACE_IOCTL_MCAST_LEAVE_EX,
                       static_cast <void *> (&leaveExInfo));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  return AEE_SUCCESS;

bail:

  LOG_MSG_ERROR ("Err %d", result, 0, 0);

  return result;

} /* LeaveBundle() */

ds::ErrorType MCastManager::RegisterBundle 
(
  IMCastSessionsInput* sessions
)
{
  int32                     result;
  MCastRegisterExType       regExInfo;
  int                       index;
  MCastSession*             mcastSessionObj;
  MCastSessionsInput *      sessionsInput = 0;
  int32                     sessionsLen = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("obj 0x%p", this, 0, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (0 == sessions)
  {
    LOG_MSG_ERROR ("Inv args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Construct MCAST_REG_EX IOCTL arguments.
    Register can be called only on sessions group created from
    call to JoinBundle with all regFlags == REG_SETUP_NOT_ALLOWED
  -------------------------------------------------------------------------*/
  sessionsInput = static_cast<MCastSessionsInput *>(sessions);
  if(0 == sessionsInput)
  {
    ASSERT(0);
    LOG_MSG_ERROR ("Bad type cast", 0, 0, 0);
    result = QDS_EFAULT;
    goto bail;
  }

  (void )sessionsInput->GetNumOfSessions(&sessionsLen);

  memset (&regExInfo, 0, sizeof (MCastRegisterExType));
  regExInfo.num_flows = (uint8) sessionsLen;
  for (index = 0; index < sessionsLen; index++)
  {
    result = sessionsInput->GetNth(index, (IMCastSession **)&mcastSessionObj);

    if (AEE_SUCCESS != result || 0 == mcastSessionObj)
    {
      goto bail;
    }

    regExInfo.handle[index] = mcastSessionObj->GetMCastHandle();

    if(MCastJoinFlags::REG_SETUP_NOT_ALLOWED != mcastSessionObj->GetRegFlag())
    {
      result = AEE_EFAILED;
      goto bail;
    }

    (void) mcastSessionObj->Release();
  }

  /*-------------------------------------------------------------------------
    Call bundled REG_EX IOCTL on underlying iface.
  -------------------------------------------------------------------------*/
  result = IfaceIoctl (mIfaceHandle,
                       IFACE_IOCTL_MCAST_REGISTER_EX,
                       static_cast <void *> (&regExInfo));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  return AEE_SUCCESS;

bail:
  
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  
  return result;
} /* RegisterBundle() */

ds::ErrorType CDECL MCastManager::JoinBundle
(
  const ::ds::SockAddrStorageType * addrSeq,
  int                               addrSeqLen,
  const char *                      mcastSpecs,
  IMCastSessionsOutput **           sessions,
  const MCastJoinFlagsType *        mcastJoinFlagsSeq,
  int                               mcastJoinFlagsSeqLen,
  char *                            errSpec,
  int                               errSpecLen,
  int *                             errSpecLenReq
)
{
  int32                       result = 0;
  MCastSession **             mcastSessionArr = NULL;
  int32                       index;
  MCastJoinExType             mcastJoinExInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == addrSeq ||
      NULL == sessions ||
      0 == mcastJoinFlagsSeq ||
      0 >= addrSeqLen ||
      addrSeqLen != mcastJoinFlagsSeqLen)
  {
    LOG_MSG_ERROR ("Inv args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    JSON related arguments are for the future use
  -------------------------------------------------------------------------*/
  (void) mcastSpecs;
  (void) errSpec;
  (void) errSpecLen;
  (void) errSpecLenReq;

  /*-------------------------------------------------------------------------
    Construct JOIN_EX IOCTL args.
  -------------------------------------------------------------------------*/
  memset (&mcastJoinExInfo, 0, sizeof (MCastJoinExType));
  result = DS2PSMCastJoinExSpec ( addrSeq,
                                  addrSeqLen,
                                  &mcastJoinExInfo,
                                  mcastJoinFlagsSeq);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Allocate memory for the array of MCast Session objects pointers:
  -----------------------------------------------------------------------*/
  mcastSessionArr =
    (MCastSession **)
    ps_system_heap_mem_alloc
    (
      sizeof (MCastSession *) * mcastJoinExInfo.num_flows
    );
  if (NULL == mcastSessionArr)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  memset (mcastSessionArr, 
          0, 
          sizeof (MCastSession *) * mcastJoinExInfo.num_flows);

  /*-----------------------------------------------------------------------
    Issue a MCAST_JOIN_EX iface IOCTL to request for a bundle MCAST join
    IOCTLS. Create corresponding Mcast session objects (MCastSession
    objects) associated with these MCast Handles.
  -----------------------------------------------------------------------*/
  result = IfaceIoctl (mIfaceHandle,
                       IFACE_IOCTL_MCAST_JOIN_EX,
                       static_cast <void *> (&mcastJoinExInfo));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-----------------------------------------------------------------------
    Create MCastSession objects for all of these MCast handles.
  -----------------------------------------------------------------------*/
  for (index = 0; index < (int)mcastJoinExInfo.num_flows; index++)
  {
    mcastSessionArr[index] =
      new MCastSession
      (
        mIfaceHandle,
        mcastJoinExInfo.handle[index],
        MCastSessionCreation::MCAST_SESSION_CREATED_BY_JOIN_BUNDLE,
        mcastJoinFlagsSeq[index]
      );
    if (NULL == mcastSessionArr[index])
    {
      result = AEE_ENOMEMORY;
      goto bail;
    }
  }

  /*-----------------------------------------------------------------------
    Create IMCastSessionOutput and populate it with retrieved sessions.
  -----------------------------------------------------------------------*/
  *sessions = new MCastSessionsOutput();
  if(0 == *sessions)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  for (index = 0;
    index < (int)mcastJoinExInfo.num_flows;
    index++)
  {
    result = ((MCastSessionsOutput*)(*sessions))->AddMCastSession
                                                  (
                                                    mcastSessionArr[index]
                                                  );
    if(AEE_SUCCESS != result)
    {
      DS_UTILS_RELEASEIF((*sessions));
      goto bail;
    }
  }

  result = AEE_SUCCESS;

  /* fall through */

bail:

  if (0 != mcastSessionArr)
  {
    for (index = 0; 
      index < (int)mcastJoinExInfo.num_flows; 
      index++)
    {
      /*---------------------------------------------------------------------
        MCastSessionsOutput object holds the reference to MCastSessions we
        created, so here we can release them.
      ---------------------------------------------------------------------*/
      DS_UTILS_RELEASEIF(mcastSessionArr[index]);
    }

    PS_SYSTEM_HEAP_MEM_FREE (mcastSessionArr);
  }

  return result;

}/* JoinBundle() */

ds::ErrorType MCastManager::CreateMCastSessionsInput
(
  IMCastSessionsInput ** newMCastSessionsInput
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  if (NULL == newMCastSessionsInput)
  {
    LOG_MSG_INVALID_INPUT ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }
  
  *newMCastSessionsInput = static_cast <IMCastSessionsInput *> 
                            (new MCastSessionsInput());
  if(0 == *newMCastSessionsInput)
  {
    LOG_MSG_ERROR ("Cant create MCastSessionsInput object", 0, 0, 0);
    return AEE_ENOMEMORY;
  }
  return AEE_SUCCESS;
}

ds::ErrorType MCastManager::GetTechObject
(
  AEEIID                                 iid,
  void**                                  ppo
)
{
  int32                       result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  switch (iid) 
  {
  case AEEIID_IMCastManager:
  case AEEIID_IMCastManagerMBMSPriv:
  case AEEIID_IMCastManagerBCMCS:
  case AEEIID_IMCastManagerExt:
  case AEEIID_IQI:
    result = QueryInterface(iid, (void**)ppo);
    break;

  case AEEIID_IMCastManagerPriv:
    /*----------------------------------------------------------------------
      return cached MCastManagerPriv object, create and cache if not created
    ------------------------------------------------------------------------*/
    if(0 == mpMCastManagerPriv)
    {
      mpMCastManagerPriv = new MCastManagerPriv(mIfaceHandle, mpPrivSet);
      if(0 == mpMCastManagerPriv)
      {
        LOG_MSG_ERROR ("No memory to create MCastManagerPriv", 0, 0, 0);
        return AEE_ENOMEMORY;
      }
    }
    
    *ppo = mpMCastManagerPriv;

    (void) mpMCastManagerPriv->AddRef();

    break;
  default:
    result = AEE_ECLASSNOTSUPPORT;
  }

  return result;

}/* GetTechObject() */

int MCastManager::QueryInterface
(
  AEEIID iid,
  void **ppo
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  switch (iid) 
  {
  case AEEIID_IMCastManager:
    *ppo = static_cast <IMCastManager *> (this);
    (void) AddRef();
    break;

  case AEEIID_IMCastManagerExt:
    *ppo = static_cast <IMCastManagerExt *> (this);
    (void) AddRef();
    break;

  case AEEIID_IMCastManagerMBMSPriv:
    *ppo = static_cast <IMCastManagerMBMSPriv *> (this);
    (void) AddRef();
    break;

  case AEEIID_IMCastManagerBCMCS:
    *ppo = static_cast <IMCastManagerBCMCS *> (this);
    (void) AddRef();
    break;

  case AEEIID_IQI:
    *ppo = reinterpret_cast <IQI *> (this);
    (void) AddRef();
    break;

  default:
    return AEE_ECLASSNOTSUPPORT;
  }

  return AEE_SUCCESS;

}/* QueryInterface() */

/*---------------------------------------------------------------------------
  Inherited functions from IMCastManagerMBMSPriv.
---------------------------------------------------------------------------*/
ds::ErrorType MCastManager::Activate 
(
  const ds::IPAddrType* addr, 
  int                   pdpNumber, 
  IMCastMBMSCtrlPriv**  ppMCastMBMSCtrl
)
{
  int32                result;
  MBMSContextActType   activateInfo;
  /*lint -esym(429,pMCastMBMSCtrl) */
  MCastMBMSCtrl*       pMCastMBMSCtrl = 0;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, mIfaceHandle, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
    Validation.
  -------------------------------------------------------------------------*/
  if (NULL == addr || NULL == ppMCastMBMSCtrl)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Construct the MBMS_CONTEXT_ACT IOCTL arg.
  -------------------------------------------------------------------------*/
  memset (&activateInfo, 0, sizeof(activateInfo));
  activateInfo.profile_id = pdpNumber;

  /* ds::IPAddrType is memcpy compatible with ps_ip_addr_type */
  memcpy (&activateInfo.ip_addr, addr, sizeof(ip_addr_type));

  /*-------------------------------------------------------------------------
    Perform iface ioctl to activate MBMS multicast context.
  -------------------------------------------------------------------------*/
  result = IfaceIoctl (mIfaceHandle,
    NetPlatform::IFACE_IOCTL_MBMS_MCAST_CONTEXT_ACTIVATE,
    static_cast <void *> (&activateInfo));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Construct a MCastMBMSCtrl object and return as out-arg
  -------------------------------------------------------------------------*/
  pMCastMBMSCtrl =
    new MCastMBMSCtrl(mIfaceHandle, activateInfo.handle);
  if (NULL == pMCastMBMSCtrl)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  *ppMCastMBMSCtrl = static_cast <IMCastMBMSCtrlPriv *> (pMCastMBMSCtrl);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  return result;

} /* Activate() */

/*---------------------------------------------------------------------------
  Functions inherited from IMCastManagerBCMCS
---------------------------------------------------------------------------*/
ds::ErrorType CDECL MCastManager::UpdateDB
(
  const BCMCSDBSpecType* updateParam
)
{
  ::AEEResult                       result;
  NetPlatform::PSBCMCSDbUpdateType  ioctlArg;
  memset((void*)&ioctlArg, 0, sizeof(NetPlatform::PSBCMCSDbUpdateType));
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("UpdateDB() called on 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validation.
  -------------------------------------------------------------------------*/
  if (NULL == updateParam)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != mpPrivSet->CheckPrivileges
                                (
                                  (AEEPRIVID *)&AEEPRIVID_PBCMCSUpdateDB,
                                  sizeof(AEEPRIVID_PBCMCSUpdateDB)
                                ))
  {
    LOG_MSG_ERROR( "No privilege for UpdateDB operation", 0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  if(AEE_SUCCESS != Conversion::DS2PSBCMCSSpec(updateParam, &ioctlArg))
  {
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Perform iface ioctl to update database for BCMCS.
  -------------------------------------------------------------------------*/
  result = IfaceIoctl(mIfaceHandle,
                      IFACE_IOCTL_BCMCS_DB_UPDATE,
                      (void *) &ioctlArg);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err %d in UpdateDB", result, 0, 0);
  }

  return result;

} /* UpdateDB() */

ds::ErrorType MCastManager::RegisterUsingHandoffOpt
(
  const ::ds::SockAddrStorageType* addrSeq,
  int addrSeqLen
)
{
  int32                            result;
  MCastRegisterUsingHandoffOptType mcastRegInfo;
  int                              i;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("RegisterUsingHandoffOpt called on 0x%p",
                          this, 0, 0);

  if (NetworkMode::QDS_MONITORED == mNetworkMode)
  {
    return AEE_EUNSUPPORTED;
  }

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == addrSeq || 0 > addrSeqLen)
  {
    LOG_MSG_ERROR ("NULL arg", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Validate MCast RegisterUsingHandoffOpt info and create IOCTL arguments 
    to perform iface level IOCTL to register using handoff optimization.
  -------------------------------------------------------------------------*/
  memset (&mcastRegInfo, 0, sizeof (MCastRegisterUsingHandoffOptType));
  mcastRegInfo.num_mcast_addr = (uint8) addrSeqLen;
  for (i = 0; i < addrSeqLen; i++)
  {
    result = DS2PSMCastAddrInfo (addrSeq[i], 
                                 &mcastRegInfo.mcast_addr_info[i]);
    if (AEE_SUCCESS != result)
    {
      goto bail;
    }
  }

  /*-------------------------------------------------------------------------
    Call bundled REGISTER_USING_HANDOFF IOCTL on underlying iface.
  -------------------------------------------------------------------------*/
  result = IfaceIoctl (mIfaceHandle,
                       IFACE_IOCTL_BCMCS_REGISTER_USING_HANDOFF,
                       static_cast <void *> (&mcastRegInfo));
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  return result;
} /* RegisterUsingHandoffOpt() */


