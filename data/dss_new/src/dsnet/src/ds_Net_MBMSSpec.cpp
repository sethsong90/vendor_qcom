/*===========================================================================
  FILE: MBMSSpec.cpp

  OVERVIEW: This file provides implementation of the MBMSSpec class.

  DEPENDENCIES: None

  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_MBMSSpec.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-08-10 hm  Created module.

===========================================================================*/

/*===========================================================================

                            INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_CSSupport.h"
#include "AEEStdErr.h"
#include "ds_Net_MBMSSpec.h"
#include "ds_Utils_DebugMsg.h"

using namespace ds::Net;
using namespace ds::Error;

/*===========================================================================
                             CONSTRUCTOR/DESTRUCTOR
===========================================================================*/

MBMSSpec::MBMSSpec
(
  void 
):mTmgi (0), 
  mSessionStartTime (0),
  mSessionEndTime (0),
  mPriority (0),
  mServiceType (0),
  mServiceMethodType (0),
  mSelectedService (0),
  mServiceSecurity (0),
  refCnt (1)
{
  LOG_MSG_INFO2("Creating MBMSSpec", 0, 0, 0);
}

MBMSSpec::~MBMSSpec
(
  void 
)
throw()
{
  LOG_MSG_INFO2("Destroying MBMSSpec", 0, 0, 0);
  if (0 != refCnt)
  {
    ASSERT (0);
  }
}

/*===========================================================================
                             PUBLIC METHODS
===========================================================================*/
int MBMSSpec::GetTMGI (uint64* tmgi)
{
  if (NULL == tmgi)
  {
    return QDS_EFAULT;
  }

  *tmgi = mTmgi;
  return AEE_SUCCESS;
}

int MBMSSpec::SetTMGI (uint64 tmgi)
{
  mTmgi = tmgi;
  return AEE_SUCCESS;
}

int MBMSSpec::GetSessionStartTime (uint64* sessionStartTime)
{
  if (NULL == sessionStartTime)
  {
    return QDS_EFAULT;
  }

  *sessionStartTime = mSessionStartTime;
  return AEE_SUCCESS;
}

int MBMSSpec::SetSessionStartTime (uint64 sessionStartTime)
{
  mSessionStartTime = sessionStartTime;
  return AEE_SUCCESS;
}

int MBMSSpec::GetSessionEndTime (uint64* sessionEndTime)
{
  if (NULL == sessionEndTime)
  {
    return QDS_EFAULT;
  }

  *sessionEndTime = mSessionEndTime;  
  return AEE_SUCCESS;
}

int MBMSSpec::SetSessionEndTime (uint64 sessionEndTime)
{
  mSessionEndTime = sessionEndTime;
  return AEE_SUCCESS;
}

int MBMSSpec::GetPriority (unsigned short int* priority)
{
  if (NULL == priority)
  {
    return QDS_EFAULT;
  }

  *priority = mPriority;
  return AEE_SUCCESS;
}

int MBMSSpec::SetPriority (unsigned short int priority)
{
  mPriority = priority;
  return AEE_SUCCESS;
}

int MBMSSpec::GetService (MBMSServiceType* service)
{
  if (NULL == service)
  {
    return QDS_EFAULT;
  }

  *service = mServiceType;
  return AEE_SUCCESS;
}

int MBMSSpec::SetService (MBMSServiceType service)
{
  mServiceType = service;
  return AEE_SUCCESS;
}

int MBMSSpec::GetServiceMethod (MBMSServiceMethodType* serviceMethod)
{
  if (NULL == serviceMethod)
  {
    return QDS_EFAULT;
  }

  *serviceMethod = mServiceMethodType;
  return AEE_SUCCESS;
}

int MBMSSpec::SetServiceMethod (MBMSServiceMethodType serviceMethod)
{
  mServiceMethodType = serviceMethod;
  return AEE_SUCCESS;
}

int MBMSSpec::GetSelectedService (boolean* selectedService)
{
  if (NULL == selectedService)
  {
    return QDS_EFAULT;
  }

  *selectedService = mSelectedService;
  return AEE_SUCCESS;
}

int MBMSSpec::SetSelectedService (boolean selectedService)
{
  mSelectedService = selectedService;
  return AEE_SUCCESS;
}

int MBMSSpec::GetServiceSecurity (boolean* serviceSecurity)
{
  if (NULL == serviceSecurity)
  {
    return QDS_EFAULT;
  }

  *serviceSecurity = mServiceSecurity;
  return AEE_SUCCESS;
}

int MBMSSpec::SetServiceSecurity (boolean serviceSecurity)
{
  mServiceSecurity = serviceSecurity;
  return AEE_SUCCESS;
}

ds::ErrorType MBMSSpec::QueryInterface 
(
  AEEIID iid, 
  void **ppo
)
{
  if (NULL == ppo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  switch (iid)
  {
    case AEEIID_IMBMSSpecPriv:
      *ppo = static_cast <IMBMSSpecPriv *> (this);
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
