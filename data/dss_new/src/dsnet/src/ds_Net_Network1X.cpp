/*===========================================================================
  FILE: Network1X.cpp

  OVERVIEW: This file provides implementation of the Network1X class.

  DEPENDENCIES: None

  Copyright (c) 2007-2011 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_Network1X.cpp#2 $
  $DateTime: 2011/06/30 17:39:13 $$Author: brijeshd $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-03-10 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Net_Network1X.h"
#include "ds_Net_Platform.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_EventManager.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECSignalBus.h"
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
Network1X::Network1X
(
  Network* pParent
)
: Handle (pParent->GetHandle()),
  mpSigBusHDRRev0RateInertia (0),
  mpSigBusHDRSlottedMode (0),
  mpSigBusHDRChangeSlottedMode (0),
  slotModeOpResult (0),
  rev0InertiaOpSuccess (0),
  rev0InertiaFailResult (0),
  refCnt (1),
  weakRefCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p, iface handle 0x%x",
    this, GetHandle(), 0);


  /* Store the parent, perform AddRefWeak() */
  ASSERT (NULL != pParent);
  mpParent = pParent;
  (void) mpParent->AddRefWeak();


} /* Network1X::Network1X() */

ds::ErrorType Network1X::Init
(
  void
)
{
  int32  result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Init object 0x%p", this, 0, 0);

  result = DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                   0, (void **) &mpSigBusHDRRev0RateInertia);
  result = DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                   0, (void **) &mpSigBusHDRSlottedMode);
  result = DS_Utils_CreateInstance(0, AEECLSID_CSignalBus,
                                   0, (void **) &mpSigBusHDRChangeSlottedMode);	

  Handle::Init(EventManager::network1XObjList);

  return result;

} /* Network1X::Init() */

void Network1X::Destructor
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);
  (void) mpParent->ReleaseWeak();
  mpParent = NULL;

  DS_UTILS_RELEASEIF (mpSigBusHDRRev0RateInertia);
  DS_UTILS_RELEASEIF (mpSigBusHDRSlottedMode);
  DS_UTILS_RELEASEIF (mpSigBusHDRChangeSlottedMode);

  //Should be last statement. Call destructor for the base class.
  Handle::Destructor();

} /* Network1X::Destructor() */


Network1X::~Network1X
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  mpParent = NULL;

} /* Network1X::~Network1X() */


/*---------------------------------------------------------------------------
  Functions inherited from INetwork1X
---------------------------------------------------------------------------*/
int Network1X::EnableHDRRev0RateInertia
(
  boolean enable
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);
  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_ENABLE_HDR_REV0_RATE_INERTIA,
                               (void *)&enable);

} /* EnableHDRRev0RateInteria() */

int Network1X::GetHDRRev0RateInertiaResult
(
  boolean* rev0InertiaOpResult
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);
  if (NULL == rev0InertiaOpResult)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *rev0InertiaOpResult = rev0InertiaOpSuccess;
  return AEE_SUCCESS;

} /* GetHDRRev0RateInteriaResult() */

int Network1X::GetHDRSlottedModeResult
(
  Network1xPrivResultCodeType* resultCode
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);
  if (NULL == resultCode)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *resultCode = slotModeOpResult;
  return AEE_SUCCESS;

}/* GetHDRSlottedModeResult() */


int Network1X::GetHDRSlottedModeCycleIndex
(
  uint32* sci
)
{
  int res;
  Network1xPrivHDRSlottedModeArg ioctlArg;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);

  if (NULL == sci)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  ioctlArg.getSlottedMode = TRUE;
  res = IfaceIoctlNonNullArg (GetHandle(),
                              IFACE_IOCTL_707_ENABLE_HDR_SLOTTED_MODE,
                              (void *)&ioctlArg);
  *sci = ioctlArg.slottedModeOption;
  return res;

}/* GetHDRSlottedModeCycleIndex() */


int Network1X::EnableHDRSlottedMode
(
  const Network1xPrivHDRSlottedModeArg* argSlottedModeInfo
)
{
  int32 result;
  Network1xPrivHDRSlottedModeArg ioctlArg;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);

  if (NULL == argSlottedModeInfo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    The function argument is constant whereas iface ioctl requires a
    non const argument.
  -------------------------------------------------------------------------*/
  ioctlArg = *argSlottedModeInfo;

  /*-------------------------------------------------------------------------
    Call the iface IOCTL for EnableHDRSlottedMode
    TODO: Take int as arg instead of const int*
  -------------------------------------------------------------------------*/
  result = IfaceIoctl(GetHandle(),
             IFACE_IOCTL_707_ENABLE_HDR_SLOTTED_MODE,
             static_cast <void *> (&ioctlArg));
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err %d", result, 0, 0);
  }

  return result;

}/* EnableHDRSlottedMode() */

int Network1X::EnableHDRHPTMode
(
  boolean enable
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, enable %d", this, enable, 0);
  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_ENABLE_HDR_HPT_MODE,
                               (void *)&enable);
}/* EnableHDRHPTMode() */

int Network1X::EnableHoldDown
(
  boolean enable
)
{
  ds::ErrorType           result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, enable %d", this, enable, 0);

  result = Check1XPrivileges(AEEPRIVID_PTech1x);
  if(AEE_SUCCESS != result)
  {
    return result;
  }

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_ENABLE_HOLDDOWN,
                               (void *)&enable);
}/* EnableHoldDown() */


ds::ErrorType Network1X::QueryDoSSupport
(
  Network1xDoSFlagsType flags,
  boolean *             dosSupported
)
{
  SDBSupportQueryType ioctlArg;
  ds::ErrorType result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  MSG_MED ("QueryDoSSupport called on object 0x%p", this, 0, 0);
  
  if(0 == dosSupported)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  memset((void*)&ioctlArg, 0, sizeof(SDBSupportQueryType));

  ioctlArg.flags = flags;
  ioctlArg.can_do_sdb = FALSE;

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p", this, 0, 0);

  result = IfaceIoctlNonNullArg (GetHandle(),
                                 IFACE_IOCTL_707_SDB_SUPPORT_QUERY,
                                 (void *)&ioctlArg);
  *dosSupported = ioctlArg.can_do_sdb;
  return result;

}/* QueryDoSSupport() */

int Network1X::GetMDR
(
  int* mdr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_GET_MDR,
                               (void *)mdr);

}/* GetMDR() */

int Network1X::SetMDR
(
  int mdr
)
{
  ds::ErrorType result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, if handle 0x%x, MDR %d",
    this, GetHandle(), mdr);

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  result = Check1XPrivileges(AEEPRIVID_PTech1x);
  if (AEE_SUCCESS != result)
  {
    return result;
  }

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_SET_MDR,
                               (void *)&mdr);
}/* SetMDR() */


int Network1X::GetDormancyTimer
(
  int* dormancyTimer
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_GET_DORM_TIMER,
                               (void *)dormancyTimer);
}/* GetDormancyTimer() */

int Network1X::SetDormancyTimer
(
  int dormancyTimer
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x, dorm timer %d",
    this, GetHandle(), dormancyTimer);

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_SET_DORM_TIMER,
                               (void *)&dormancyTimer);
}/* SetDormancyTimer() */

int Network1X::GetSessionTimer
(
  Network1xPrivSessionTimerType* sessionTimer
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_GET_SESSION_TIMER,
                               (void *)sessionTimer);
}/* GetSessionTimer() */


int Network1X::SetSessionTimer
(
  const Network1xPrivSessionTimerType* sessionTimer
)
{
  int32 result;
  Network1xPrivSessionTimerType ioctlArg;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x, session timer %d",
    this, GetHandle(), sessionTimer);

  ioctlArg = *sessionTimer;

  /*-------------------------------------------------------------------------
    Call the iface IOCTL for SetSessionTimer
    Need the conversion because input is const arg.
    TODO: Take int as arg instead of const int*
  -------------------------------------------------------------------------*/
  result = IfaceIoctl(GetHandle(),
             IFACE_IOCTL_707_SET_SESSION_TIMER,
             (void *) &ioctlArg);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err %d", result, 0, 0);
  }

  return result;

}/* SetSessionTimer() */

int Network1X::GetQoSNAPriority
(
  int* qosNAPriority
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_GET_RLP_QOS_NA_PRI,
                               (void *)qosNAPriority);

}/* GetQoSNAPriority() */

int Network1X::SetQoSNAPriority
(
  int qosNAPriority
)
{
  ds::ErrorType result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x, QoS NA pri %d",
    this, GetHandle(), qosNAPriority);

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  result = Check1XPrivileges(AEEPRIVID_PTech1x);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = IfaceIoctlNonNullArg (GetHandle(),
                                 IFACE_IOCTL_707_SET_RLP_QOS_NA_PRI,
                                 (void *)&qosNAPriority);

/* fall through */

bail:

  return result;

}/* SetQoSNAPriority() */


int Network1X::GetRLPAllCurrentNAK
(
  Network1xRLPOptionType* rlpAllCurrentNAK
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_GET_RLP_ALL_CURR_NAK,
                               (void *)rlpAllCurrentNAK);

}/* GetRLPAllCurrentNAK() */


int Network1X::SetRLPAllCurrentNAK
(
  const Network1xRLPOptionType* rlpAllCurrentNAK
)
{
  ds::ErrorType           result = AEE_SUCCESS;
  Network1xRLPOptionType ioctlArg;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if(0 == rlpAllCurrentNAK)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    result = QDS_EFAULT;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  result = Check1XPrivileges(AEEPRIVID_PTech1x);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }
  
  result = Check1XPrivileges(AEEPRIVID_RLPConfig);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    The function argument is constant whereas iface ioctl requires a
    non const argument.
  -------------------------------------------------------------------------*/
  ioctlArg = *rlpAllCurrentNAK;

  /*-------------------------------------------------------------------------
    Call the iface IOCTL for SetRLPAllCurrentNAK
    TODO: Take int as arg instead of const int *
  -------------------------------------------------------------------------*/
  result = IfaceIoctl(GetHandle(),
                      IFACE_IOCTL_707_SET_RLP_ALL_CURR_NAK,
                      static_cast <void *> (&ioctlArg));
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err %d", result, 0, 0);
  }
/* fall through */

bail:

  return result;

}/* SetRLPAllCurrentNAK() */

int Network1X::GetRLPDefCurrentNAK
(
  Network1xRLPOptionType* rlpDefCurrentNAK
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_GET_RLP_ALL_DEF_NAK,
                               (void *)rlpDefCurrentNAK);
}/* GetRLPDefCurrentNAK() */


int Network1X::SetRLPDefCurrentNAK
(
  const Network1xRLPOptionType* rlpDefCurrentNAK
)
{
  ds::ErrorType result = AEE_SUCCESS;
  Network1xRLPOptionType ioctlArg;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == rlpDefCurrentNAK)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    result = QDS_EFAULT;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  result = Check1XPrivileges(AEEPRIVID_PTech1x);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  result = Check1XPrivileges(AEEPRIVID_RLPConfig);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    The function argument is constant whereas iface ioctl requires a
    non const argument.
  -------------------------------------------------------------------------*/
  ioctlArg = *rlpDefCurrentNAK;

  /*-------------------------------------------------------------------------
    Call the iface IOCTL for SetRLPDefCurrentNAK
    TODO: Take int as arg instead of const int*
  -------------------------------------------------------------------------*/
  result = IfaceIoctl(GetHandle(),
             IFACE_IOCTL_707_SET_RLP_ALL_DEF_NAK,
             static_cast <void *> (&ioctlArg));
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err %d", result, 0, 0);
  }

/* fall through */

bail:

  return result;

}/* SetRLPDefCurrentNAK() */


int Network1X::GetRLPNegCurrentNAK
(
  Network1xRLPOptionType* rlpNegCurrentNAK
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_GET_RLP_ALL_NEG_NAK,
                               (void *)rlpNegCurrentNAK);

}/* GetRLPNegCurrentNAK() */


int Network1X::QueryInterface
(
  AEEIID iid,
  void **ppo
)
throw()
{
  int result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1  ("Obj 0x%p, ref cnt %d, iid 0x%x", this, refCnt, iid);

  if (NULL == ppo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *ppo = NULL;

  switch (iid)
  {
  case AEEIID_INetwork:
    /* Get Strong reference to INetwork parent object and return */
    if (TRUE == mpParent->GetStrongRef())
    {
      /* No need to perform AddRef() again, GetStrongRef()
       * already does that for us */
      *ppo = static_cast <INetwork *> (mpParent);
      result = AEE_SUCCESS;
    }
    else
    {
      result = AEE_ECLASSNOTSUPPORT;
    }
    break;

  case AEEIID_INetwork1x:
    *ppo = static_cast <INetwork1x *>(this);
    (void) AddRef();
    result = AEE_SUCCESS;
    break;

  case AEEIID_INetwork1xPriv:
     *ppo = static_cast <INetwork1xPriv *>(this);
     (void) AddRef();
     result = AEE_SUCCESS;
     break;

  case AEEIID_IQI:
    *ppo = reinterpret_cast <IQI *> (this);
    (void) AddRef();
    result = AEE_SUCCESS;
    break;

  default:
    result = AEE_ECLASSNOTSUPPORT;
    break;
  }

  return result;
}

int Network1X::GetHDRRev0RateInertiaResultInfoCode
(
  Network1xPrivHDRRev0RateInertiaFailureCodeType* failureCode
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  if (NULL == failureCode)
  {
    MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *failureCode = rev0InertiaFailResult;
  return AEE_SUCCESS;

} /* GetHDRRev0RateInertiaResultInfoCode() */


int Network1X::GetHDR1xHandDownOption
(
  boolean* HDR1xHandDownOption
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_GET_HDR_1X_HANDDOWN_OPTION,
                               (void *)HDR1xHandDownOption);
}

int Network1X::SetHDR1xHandDownOption
(
  boolean HDR1xHandDownOption
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, handle 0x%x", this, GetHandle(), 0);

  return IfaceIoctlNonNullArg (GetHandle(),
                               IFACE_IOCTL_707_SET_HDR_1X_HANDDOWN_OPTION,
                               (void *)&HDR1xHandDownOption);
}


int Network1X::GetHysteresisTimer
(
  int* HysteresisTimer
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  //TODO
  //Dont have an IOCTL for this.
  (void) HysteresisTimer;
  return 0;
}

int Network1X::SetHysteresisTimer
(
  int HysteresisTimer
)
{
  ds::ErrorType result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  result = Check1XPrivileges(AEEPRIVID_NetHAT);
  if(AEE_SUCCESS != result)
  {
    return result;
  }
  
  /* fall through */

  //TODO
  //Dont have an IOCTL for this.
  (void) HysteresisTimer;
  return 0;
}

ds::ErrorType Network1X::GetSignalBus
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
    return AEE_EBADPARM;
  }


  switch (eventID)
  {
    case  Network1xPrivEvent::HDR_REV0_RATE_INERTIA_RESULT:
      *ppISigBus = mpSigBusHDRRev0RateInertia;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS;

    case  Network1xPrivEvent::SLOTTED_MODE_RESULT:
      *ppISigBus = mpSigBusHDRSlottedMode;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS;
	
    case  Network1xPrivEvent::SLOTTED_MODE_CHANGED:
      *ppISigBus = mpSigBusHDRChangeSlottedMode;
      (void)(*ppISigBus)->AddRef();
      return AEE_SUCCESS; 
      
    default:
      *ppISigBus = NULL;
      return AEE_EBADPARM;
  }

} /* GetSignalBus() */

ds::ErrorType Network1X::Check1XPrivileges
(
  AEEPRIVID pPriv
)
{
  IPrivSet                *privSet = 0;
  ds::ErrorType            result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  privSet = mpParent->GetPrivSet();

  if (0 == privSet)
  {
    LOG_MSG_ERROR ("NULL privSet", 0, 0, 0);
    result = QDS_EFAULT;
    goto bail;
  }

  /*-------------------------------------------------------------------------
    check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != privSet->CheckPrivileges
    (
    (AEEPRIVID *)&AEEPRIVID_PTech1x,
    sizeof(AEEPRIVID_PTech1x)
    ))


  if (AEE_SUCCESS != privSet->CheckPrivileges
                              (
                                (AEEPRIVID *)&pPriv,
                                sizeof(pPriv)
                              ))
  {
    LOG_MSG_ERROR( "No privilege for 1x operation", 0, 0, 0);
    result = AEE_EPRIVLEVEL;
    goto bail;
  }

  /* fall through */

bail:

  DS_UTILS_RELEASEIF(privSet);

  return result;

} /* Check1XPrivileges() */

boolean Network1X::Process
(
  void* pUserData
)
{
  EventInfoType*                        pEventInfo;
  SlottedModeFailureCodeType            slotModeResult;
  Rev0RateInertiaFailureCodeType        inertiaFailReason;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    User data should never be NULL for event processing.
  -------------------------------------------------------------------------*/
  if (NULL == pUserData)
  {
    ASSERT (0);
    return FALSE;
  }

  pEventInfo = static_cast <EventInfoType *> (pUserData);

  LOG_MSG_INFO1 (" ev 0x%x, ev handle 0x%x", 
                 pEventInfo->eventName, pEventInfo->handle, 0);
  
  switch (pEventInfo->eventName)
  {
    case Network1xPrivEvent:: SLOTTED_MODE_RESULT:
    {
      slotModeResult = static_cast < SlottedModeFailureCodeType >
	                               ((uint32)(pEventInfo->psEventInfo));
      if (pEventInfo->psEventName == 
            IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SUCCESS_EV)
      {
        slotModeOpResult = 
          ds::Net::Network1xPrivSlottedMode::REQUEST_SUCCEES;
      }
      else if (pEventInfo->psEventName ==
                 IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV)
      {
        switch (slotModeResult)
        {
          case PS_HDR_SLOTTED_MODE_REQUEST_REJECTED:
            slotModeOpResult = 
              ds::Net::Network1xPrivSlottedMode::REQUEST_REJECTED;
            break;
 
          case PS_HDR_SLOTTED_MODE_REQUEST_FAILED_TX:
            slotModeOpResult = 
              ds::Net::Network1xPrivSlottedMode::REQUEST_FAILED_TX;
            break;
               
          case PS_HDR_SLOTTED_MODE_NOT_SUPPORTED:
            slotModeOpResult = 
              ds::Net::Network1xPrivSlottedMode::REQUEST_UNSUPPORTED;
            break; 

          case PS_HDR_SLOTTED_MODE_NO_NET: 
            slotModeOpResult = 
              ds::Net::Network1xPrivSlottedMode::REQUEST_NO_NET;
            break;      
  
          default:
            LOG_MSG_ERROR ("Slotted Mode Error code %d", 
                             slotModeResult, 0, 0);
            slotModeOpResult = 
              ds::Net::Network1xPrivSlottedMode::REQUEST_REJECTED;
            break;
        }
      }
    }
    break;

    case Network1xPrivEvent:: HDR_REV0_RATE_INERTIA_RESULT:
    {
      if (pEventInfo->psEventName == 
            IFACE_ENABLE_HDR_REV0_RATE_INERTIA_SUCCESS_EV)
      {
        rev0InertiaOpSuccess = TRUE;
      }
      else if (pEventInfo->psEventName == 
                IFACE_ENABLE_HDR_REV0_RATE_INERTIA_FAILURE_EV)
      {
        inertiaFailReason = static_cast < Rev0RateInertiaFailureCodeType > 
                              ((int)(pEventInfo->psEventInfo));
        
        switch (inertiaFailReason)
        {
          case PS_HDR_REV0_RATE_INERTIA_REQUEST_REJECTED:
            rev0InertiaFailResult = 
              ds::Net::Network1xPrivHDRRev0RateInertiaFailureCode::REQUEST_REJECTED;
            break;
   
          case PS_HDR_REV0_RATE_INERTIA_REQUEST_FAILED_TX:
            rev0InertiaFailResult = 
              ds::Net::Network1xPrivHDRRev0RateInertiaFailureCode::REQUEST_FAILED_TX;
            break;
  
          case PS_HDR_REV0_RATE_INERTIA_NOT_SUPPORTED:
            rev0InertiaFailResult = 
              ds::Net::Network1xPrivHDRRev0RateInertiaFailureCode::NOT_SUPPORTED;
            break;
  
          case PS_HDR_REV0_RATE_INERTIA_NO_NET:
            rev0InertiaFailResult = 
              ds::Net::Network1xPrivHDRRev0RateInertiaFailureCode::NO_NET;
            break;
          default:
              LOG_MSG_INFO1 (" ev 0x%x, ev handle 0x%x not handled!", 
                 pEventInfo->eventName, pEventInfo->handle, 0);
            break;
        }
      }
    }
    break;

    default:
     break;
  }

  Notify (pEventInfo->eventName);

  return TRUE;

} /* Process() */
