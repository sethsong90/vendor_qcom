/*===========================================================================
  FILE: Policy.cpp

  OVERVIEW: This file provides implementation of the Policy class.

  DEPENDENCIES: None

  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_Policy.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-03-31 ss  Added appType get/set functions.
  2008-03-10 hm  Created module.

===========================================================================*/

/*---------------------------------------------------------------------------
  Include Files
---------------------------------------------------------------------------*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Net_Handle.h"
#include "ds_Net_Policy.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Platform.h"
#include "AEECCritSect.h"
#include "AEEstd.h"

using namespace ds::Net;
using namespace ds::Error;

/*---------------------------------------------------------------------------
  CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/
Policy::Policy
(
  void
)
: refCnt (1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating object 0x%p", this, 0, 0);

  //TODO: What should we use for default value? GRP_ANY_DEFAULT is defined only for the group in IDL.
  //  ifaceName     = ds::Net::Type::DEFAULT_NET_TYPE;

  ipFamily    = ds::AddrFamily::QDS_AF_INET;
  isRouteable = FALSE;
  policyFlag  = PolicyFlag::QDS_ANY;
  ifaceName   = 0;
  ifaceGroup  = IfaceGroup::GRP_ANY_DEFAULT;
  ifaceId     = 0;
  umtsProfileNum = 0;
  cdmaProfileNum = 0;
  umtsAPNName = NULL;
  umtsAPNNameLen = 0;

  if (AEE_SUCCESS != DS_Utils_CreateInstance (NULL,
                                              AEECLSID_CCritSect,
                                              NULL,
                                              (void **) &mpICritSect))
  {
    LOG_MSG_FATAL_ERROR ("Cannot create crit sect", 0, 0, 0);
    ASSERT (0);
  }

}

Policy::~Policy
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting object 0x%p", this, 0, 0);


  if (NULL != umtsAPNName)
  {
    PS_SYSTEM_HEAP_MEM_FREE(umtsAPNName);
  }

  /*lint -save -e1550, -e1551 */
  DS_UTILS_RELEASEIF(mpICritSect);
  /*lint -restore */

}

/*---------------------------------------------------------------------------
  Public function definitions.
---------------------------------------------------------------------------*/
int Policy::GetPolicyFlag
(
  PolicyFlagType* argPolicyFlag
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO2 ("GetPolicyFlag called on object 0x%p", this, 0, 0);

  if (NULL == argPolicyFlag)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *argPolicyFlag = policyFlag;

  LOG_MSG_FUNCTION_EXIT ("Policy flag %d", (int)policyFlag, 0, 0);
  return AEE_SUCCESS;
}

int Policy::SetPolicyFlag
(
  PolicyFlagType argPolicyFlag
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Obj 0x%p, flag %d", this, argPolicyFlag, 0);

  if (PolicyFlag::QDS_UP_PREFERRED != argPolicyFlag &&
      PolicyFlag::QDS_UP_ONLY       != argPolicyFlag &&
      PolicyFlag::QDS_ANY           != argPolicyFlag)
  {
    LOG_MSG_ERROR ("Invalid policy flag %d", (int)argPolicyFlag, 0, 0);
    return QDS_EFAULT;
  }

  mpICritSect->Enter();
  policyFlag = argPolicyFlag;
  mpICritSect->Leave();

  return AEE_SUCCESS;
}

int Policy::GetAddressFamily
(
  ::ds::AddrFamilyType* argAddressFamily
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == argAddressFamily)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *argAddressFamily = ipFamily;

  LOG_MSG_FUNCTION_EXIT ("Addr family %d", (int)ipFamily, 0, 0);
  return AEE_SUCCESS;
}

int Policy::SetAddressFamily
(
  ::ds::AddrFamilyType argAddressFamily
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY("Obj 0x%p, family %d", this, argAddressFamily, 0);

  if (ds::AddrFamily::QDS_AF_INET   != argAddressFamily &&
      ds::AddrFamily::QDS_AF_INET6  != argAddressFamily &&
      ds::AddrFamily::QDS_AF_UNSPEC != argAddressFamily)
  {
    LOG_MSG_ERROR ("Invalid argument to set addr family %d",
      (int)argAddressFamily, 0, 0);
    return QDS_EFAULT;
  }

  mpICritSect->Enter();
  ipFamily = argAddressFamily;
  mpICritSect->Leave();

  return AEE_SUCCESS;
}

int Policy::GetRouteable
(
  boolean* argRouteable
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == argRouteable)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *argRouteable = isRouteable;

  LOG_MSG_FUNCTION_EXIT ("Routeable mode %d", (int)isRouteable, 0, 0);
  return AEE_SUCCESS;
}
int Policy::SetRouteable
(
  boolean argRouteable
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY("Obj 0x%p, routeable %d", this, argRouteable, 0);

  mpICritSect->Enter();
  isRouteable = argRouteable;
  mpICritSect->Leave();

  return AEE_SUCCESS;
}

int Policy::GetIfaceId
(
  IfaceIdType*  argIfaceId
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == argIfaceId)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *argIfaceId = ifaceId;


  LOG_MSG_FUNCTION_EXIT ("Iface ID 0x%x", (int)ifaceId, 0, 0);

  return AEE_SUCCESS;
}

int Policy::SetIfaceId
(
  IfaceIdType  argIfaceId
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_FUNCTION_ENTRY("Obj 0x%p, iface id 0x%x", this, argIfaceId, 0);

  //TODO: Validate iface id?

  mpICritSect->Enter();
  ifaceId = argIfaceId;
  mpICritSect->Leave();

  return AEE_SUCCESS;
}


int Policy::GetIfaceName
(
  ds::Net::IfaceNameType* argIfaceName
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == argIfaceName)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  mpICritSect->Enter();
  *argIfaceName = ifaceName;
  mpICritSect->Leave();

  LOG_MSG_FUNCTION_EXIT("Obj 0x%p, if name 0x%x", this, ifaceName, 0);

  return AEE_SUCCESS;

} /* GetIfaceName() */

int Policy::SetIfaceName
(
  ds::Net::IfaceNameType argIfaceName
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_FUNCTION_ENTRY("Obj 0x%p, if name 0x%x", this, argIfaceName, 0);

  mpICritSect->Enter();
  ifaceName = argIfaceName;
  mpICritSect->Leave();
  return AEE_SUCCESS;

} /* SetIfaceName() */


int Policy::GetIfaceGroup
(
  IfaceGroupType* argIfaceGroup
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == argIfaceGroup)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  mpICritSect->Enter();
  *argIfaceGroup = ifaceGroup;
  mpICritSect->Leave();

  LOG_MSG_FUNCTION_EXIT ("Obj 0x%p, group 0x%x", this, ifaceGroup, 0);
  return AEE_SUCCESS;

} /* GetIfaceGroup() */


int Policy::SetIfaceGroup
(
  IfaceGroupType argIfaceGroup
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_FUNCTION_ENTRY("Obj 0x%p, iface grp 0x%x", this, argIfaceGroup, 0);

  mpICritSect->Enter();
  ifaceGroup = argIfaceGroup;
  mpICritSect->Leave();
  return AEE_SUCCESS;

} /* SetIfaceGroup() */

ds::ErrorType Policy::GetCDMAProfileNumber
(
  int* argCDMAProfileNumber
)
{
  if (NULL == argCDMAProfileNumber)
  {
    return QDS_EFAULT;
  }

  *argCDMAProfileNumber = cdmaProfileNum;

  LOG_MSG_FUNCTION_EXIT("Obj 0x%p, prof %d", this, cdmaProfileNum, 0);

  return AEE_SUCCESS;

} /* SetCDMAProfileNumber() */

ds::ErrorType Policy::SetCDMAProfileNumber
(
  int argCDMAProfileNumber
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_FUNCTION_ENTRY("Obj 0x%p, CDMA prof num %d",
    this, argCDMAProfileNumber, 0);

  cdmaProfileNum = argCDMAProfileNumber;
  return AEE_SUCCESS;

} /* SetCDMAProfileNumber() */

ds::ErrorType Policy::GetUMTSProfileNumber
(
  int* argUMTSProfileNumber
)
{
  if (NULL == argUMTSProfileNumber)
  {
    return QDS_EFAULT;
  }

  *argUMTSProfileNumber = umtsProfileNum;

  LOG_MSG_FUNCTION_EXIT("Obj 0x%p, prof %d", this, umtsProfileNum, 0);

  return AEE_SUCCESS;
}

ds::ErrorType Policy::SetUMTSProfileNumber
(
  int argUMTSProfileNumber
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_FUNCTION_ENTRY("Obj 0x%p, UMTS prof num %d",
    this, argUMTSProfileNumber, 0);

  umtsProfileNum = argUMTSProfileNumber;
  return AEE_SUCCESS;
}

ds::ErrorType Policy::QueryInterface
(
  AEEIID iid,
  void **ppo
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1  ("Obj 0x%p, ref cnt %d, iid 0x%x", this, refCnt, iid);
  if (NULL == ppo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  switch (iid)
  {
    case AEEIID_IPolicy:
      *ppo = reinterpret_cast <IPolicy *> (this);
      (void) AddRef ();
      break;

    case AEEIID_IPolicyPriv:
      *ppo = static_cast <IPolicyPriv *> (this);
      (void) AddRef ();
      break;

    case AEEIID_IQI:
      *ppo = reinterpret_cast <IQI *> (this);
      (void) AddRef ();
      break;

    default:
      return AEE_ECLASSNOTSUPPORT;
  }

  return AEE_SUCCESS;
}

ds::ErrorType Policy::GetUMTSAPNName
(
  char *  pName,
  int     len,
  int*    pLenReq
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, apn name len %d, apn name 0x%p",
    this, len, pName);

  if (pLenReq != NULL)
  {
    *pLenReq = umtsAPNNameLen;
  }

  if (NULL == pName && 0 != len)
  {
    return QDS_EFAULT;
  }

  if (NULL == umtsAPNName)
  {
    pName   = NULL;
    len     = 0;
    pLenReq = 0;
    return AEE_SUCCESS;
  }

  (void) std_strlcpy (pName,
                      (const char *)umtsAPNName,
                      len);
  return AEE_SUCCESS;

} /* GetUMTSAPNName() */


ds::ErrorType Policy::SetUMTSAPNName
(
  const char *  pName
)
{
  int len = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Obj 0x%p, apn name 0x%p", this, pName, 0);

  len = std_strlen(pName);

  /*-------------------------------------------------------------------------
    Max supported domain name is 255
  -------------------------------------------------------------------------*/
  if (len > 255 || len < 0)
  {
    LOG_MSG_INVALID_INPUT("Incorrect apn len %d, obj 0x%p", len, this, 0);
    return QDS_EFAULT;
  }

  if ( 0 != len )
  {
    umtsAPNName = (char *) ps_system_heap_mem_alloc(len + 1);
    if (NULL == umtsAPNName)
    {
      return AEE_ENOMEMORY;
    }

    (void) std_strlcpy (umtsAPNName, pName, len + 1);
    umtsAPNNameLen = len;
  }

  LOG_MSG_FUNCTION_EXIT("Success Obj 0x%p, apn len %d, apn name %s",
                        this, umtsAPNNameLen, umtsAPNName);

  return AEE_SUCCESS;

} /* SetUMTSAPNName() */

ds::ErrorType Policy::GetAppType
(
  int* AppType
)
{
  LOG_MSG_FUNCTION_ENTRY("Obj 0x%p, appType %d", this, appType, 0);
  if (NULL == AppType)
  {
    return QDS_EFAULT;
  }

  *AppType = appType;

  LOG_MSG_FUNCTION_EXIT("Obj 0x%p, appType %d", this, appType, 0);

  return AEE_SUCCESS;
} /* GetAppType() */

ds::ErrorType Policy::SetAppType
(
  int AppType
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_FUNCTION_ENTRY("Obj 0x%p, appType %d", this, AppType, 0);

  appType = AppType;
  return AEE_SUCCESS;
} /* SetAppType() */


