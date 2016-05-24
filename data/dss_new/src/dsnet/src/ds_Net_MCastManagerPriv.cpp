/*===========================================================================
  FILE: MCastManagerPriv.cpp

  OVERVIEW: This file provides implementation of the MCastManagerPriv class.

  DEPENDENCIES: None

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_MCastManagerPriv.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-07-29 vm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "AEEStdErr.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Net_Utils.h"
#include "ds_Net_MCastManagerPriv.h"
#include "ds_Net_MCastSessionPriv.h"
#include "ds_Net_Platform.h"
#include "ds_Net_Conversion.h"
#include "AEECCritSect.h"
#include "ds_Net_MBMSSpec.h"
#include "ds_Net_MCastMBMSCtrl.h"
#include "ds_Net_Privileges_Def.h"
#include "AEEIPrivSet.h"
#include "ps_system_heap.h"

using namespace ds::Error;
using namespace ds::Net;
using namespace ds::Net::Conversion;
using namespace NetPlatform;


MCastManagerPriv::MCastManagerPriv
(
  int32      ifaceHandle,
  IPrivSet * pPrivSet
)
: mIfaceHandle (ifaceHandle),
  mpPrivSet(pPrivSet),
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

MCastManagerPriv::~MCastManagerPriv()
throw()
{
  LOG_MSG_INFO1 ("Deleteing 0x%p if handle 0x%x", this, mIfaceHandle, 0);
  
  mIfaceHandle = 0;

  DS_UTILS_RELEASEIF(mpPrivSet);

  /*lint -save -e1550, -e1551 */
  DS_UTILS_RELEASEIF(mpICritSect);
  /*lint -restore */
}


ds::ErrorType MCastManagerPriv::Join 
(
  const ::ds::SockAddrStorageType addr, 
  IQI*            info,
  IMCastSessionPriv** session
)
{
  int32              result;
  MCastSessionPriv   *mcastSessionPtr = NULL;
  MCastJoinType      mcastJoinInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == session || NULL == addr)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    info parameter is for the future use
  -------------------------------------------------------------------------*/
  (void) info;

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
    Create a MCastSessionPriv object for the session 
    after the IOCTL is issued.
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
  mcastSessionPtr = new MCastSessionPriv (mIfaceHandle,
                                          mcastJoinInfo.handle,
                                          MCastJoinFlags::REG_SETUP_ALLOWED);
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

ds::ErrorType MCastManagerPriv::JoinBundle 
(
  const ::ds::SockAddrStorageType * addrSeq,
  int                               addrSeqLen,
  IQI **                            mcastSpecInfoSeq,
  int                               mcastSpecInfoSeqLen,
  const MCastJoinFlagsType *        mcastJoinFlagsSeq,
  int                               mcastJoinFlagsSeqLen,
  IMCastSessionPriv **              sessions,
  int                               sessionsLen,
  int *                             sessionsLenReq
)
{
  int32                                          result = 0;
  MCastSessionPriv                               **mcastSessionArr = NULL;
  int32                                          index;
  MCastJoinExType                                mcastJoinExInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == addrSeq ||
      NULL == sessions ||
      0 >= addrSeqLen ||
      addrSeqLen != sessionsLen ||
      addrSeqLen != mcastJoinFlagsSeqLen)
  {
    LOG_MSG_ERROR ("Inv args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    mcastSpecInfoSeq and mcastSpecInfoSeqLen parameters are for future use
  -------------------------------------------------------------------------*/
  (void) mcastSpecInfoSeq;
  (void) mcastSpecInfoSeqLen;

  /*-------------------------------------------------------------------------
    sessionsLenReq should be populated with sessionsLen.
  -------------------------------------------------------------------------*/
  if(NULL != sessionsLenReq)
  {
    *sessionsLenReq = sessionsLen;
  }

  /*-------------------------------------------------------------------------
    Construct JOIN_EX IOCTL args.
  -------------------------------------------------------------------------*/
  memset (&mcastJoinExInfo, 0, sizeof (MCastJoinExType));
  result = DS2PSMCastJoinExSpec (addrSeq,
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
    (MCastSessionPriv **)
    ps_system_heap_mem_alloc
    (
      sizeof (MCastSessionPriv *) * mcastJoinExInfo.num_flows
    );
  if (NULL == mcastSessionArr)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }
  memset (mcastSessionArr, 
          0, 
          sizeof (MCastSessionPriv *) * mcastJoinExInfo.num_flows);

  /*-----------------------------------------------------------------------
    Issue a MCAST_JOIN_EX iface IOCTL to request for a bundle MCAST join
    IOCTLS. Create corresponding Mcast session objects (MCastSessionPriv
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
    Create MCastSessionPriv objects for all of these MCast handles.
  -----------------------------------------------------------------------*/
  for (index = 0; index < (int)mcastJoinExInfo.num_flows; index++)
  {
    mcastSessionArr[index] =
      new MCastSessionPriv(
                            mIfaceHandle,
                            mcastJoinExInfo.handle[index],
                            mcastJoinFlagsSeq[index]
                          );
    if (NULL == mcastSessionArr[index])
    {
      result = AEE_ENOMEMORY;
      goto bail;
    }
  }

  /*-----------------------------------------------------------------------
    update the sessionsLenReq argument.
  -----------------------------------------------------------------------*/
  for (index = 0;
       index < (int)mcastJoinExInfo.num_flows;
       index++)
  {
    sessions[index] = mcastSessionArr[index];
  }

  if(NULL != sessionsLenReq) {
    *sessionsLenReq = mcastJoinExInfo.num_flows;
  }

  PS_SYSTEM_HEAP_MEM_FREE (mcastSessionArr);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err %d", result, 0, 0);

  if (NULL != mcastSessionArr)
  {
    for (index = 0; 
          index < (int)mcastJoinExInfo.num_flows; 
          index++)
    {
    /*---------------------------------------------------------------------
      The destructor MCast session takes care of leaving MCast group.
    ---------------------------------------------------------------------*/
      DS_UTILS_RELEASEIF(mcastSessionArr[index]);
    }
    
    PS_SYSTEM_HEAP_MEM_FREE (mcastSessionArr);
  }
  return result;

} /* JoinBundle() */

ds::ErrorType MCastManagerPriv::LeaveBundle 
(
  IMCastSessionPriv** sessions,
  int sessionsLen
)
{
  int32               result;
  MCastLeaveExType    leaveExInfo;
  MCastSessionPriv *  mcastSessionObj;
  int                 index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == sessions || 0 >= sessionsLen)
  {
    LOG_MSG_ERROR ("Inv args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Validate sessions argument and construct MCAST_LEAVE_EX IOCTL arguments.
  -------------------------------------------------------------------------*/
  memset (&leaveExInfo, 0, sizeof (MCastLeaveExType));
  leaveExInfo.num_flows = (uint8) sessionsLen;
  for (index = 0; index < sessionsLen; index++)
  {
    mcastSessionObj = reinterpret_cast <MCastSessionPriv *> (sessions[index]);
    if (NULL == mcastSessionObj)
    {
      return QDS_EFAULT;
    }
    leaveExInfo.handle[index] = mcastSessionObj->GetMCastHandle();
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

ds::ErrorType MCastManagerPriv::RegisterBundle 
(
  IMCastSessionPriv** sessions,
  int sessionsLen
)
{
  int32                     result;
  MCastRegisterExType       regExInfo;
  int                       index;
  MCastSessionPriv *        mcastSessionObj;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("obj 0x%p", this, 0, 0);

  /*-------------------------------------------------------------------------
    Validate arguments.
  -------------------------------------------------------------------------*/
  if (NULL == sessions || 0 >= sessionsLen)
  {
    LOG_MSG_ERROR ("Inv args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Construct MCAST_REG_EX IOCTL arguments.
    Register can be called only on sessions group created from
    call to JoinBundle with all regFlags == REG_SETUP_NOT_ALLOWED
  -------------------------------------------------------------------------*/
  memset (&regExInfo, 0, sizeof (MCastRegisterExType));
  regExInfo.num_flows = (uint8) sessionsLen;
  for (index = 0; index < sessionsLen; index++)
  {
    mcastSessionObj = reinterpret_cast <MCastSessionPriv *> (sessions[index]);
    regExInfo.handle[index] = mcastSessionObj->GetMCastHandle();
    if(MCastJoinFlags::REG_SETUP_NOT_ALLOWED != mcastSessionObj->GetRegFlag())
    {
      result = AEE_EFAILED;
      goto bail;
    }
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

ds::ErrorType  MCastManagerPriv::CreateMBMSSpecPriv
(
  IMBMSSpecPriv** ppIMBMSSpec
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == ppIMBMSSpec)
  {
    LOG_MSG_INVALID_INPUT ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *ppIMBMSSpec = static_cast <IMBMSSpecPriv *> (new MBMSSpec());
  if (NULL == *ppIMBMSSpec)
  {
    LOG_MSG_ERROR ("Cant create MBMSSpec object", 0, 0, 0);
    return AEE_ENOMEMORY;
  }

  LOG_MSG_INFO1 ("Created 0x%p", *ppIMBMSSpec, 0, 0);
  return AEE_SUCCESS;
}/* CreateMBMSSpecPriv() */

ds::ErrorType MCastManagerPriv::GetTechObject
(
  AEEIID                                 iid,
  void**                                  ppo
)
{
  int32                       result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  switch (iid) 
  {
  case AEEIID_IMCastManagerPriv:
  case AEEIID_IQI:
    result = QueryInterface(iid, (void**)ppo);
    break;

  default:
    result = AEE_ECLASSNOTSUPPORT;
  }

  return result;

}/* GetTechObject() */

int MCastManagerPriv::QueryInterface
(
  AEEIID iid,
  void **ppo
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p", this, 0, 0);

  switch (iid) 
  {
  case AEEIID_IMCastManagerPriv:
    *ppo = static_cast <IMCastManagerPriv *>(this);
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
