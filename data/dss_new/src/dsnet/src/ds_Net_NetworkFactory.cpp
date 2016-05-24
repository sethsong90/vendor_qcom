/*===========================================================================
  FILE: NetworkFactory.cpp

  OVERVIEW: This file provides implementation of the NetworkFactory class.

  DEPENDENCIES: None

  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/


/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NetworkFactory.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-05-13 ts  Removed CreateDefaultNetworkPriv. CreateNetworkPriv can
                 be used with NULL policy to achive the same functionality.
  2008-03-10 hm  Created module.

===========================================================================*/

/*===========================================================================

                        INCLUDE FILES FOR THE MODULE

===========================================================================*/
#include "comdef.h"
#include "AEEStdErr.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Net_INetworkFactoryPriv.h"
#include "ds_Net_NetworkFactory.h"
#include "ds_Net_Network.h"
#include "ds_Net_NetworkActive.h"
#include "ds_Net_NetworkMonitored.h"
#include "ds_Net_NatSession.h"
#include "ds_Net_NatManager.h"
#include "ds_Net_CNetworkFactory.h"
#include "ds_Net_CNetworkFactoryPriv.h"
#include "ds_Net_CNetworkFactoryService.h"
#include "ds_Net_CNetworkFactoryPrivService.h"
#include "ds_Net_TechUMTSFactory.h"
#include "ds_Net_Privileges_Def.h"
#include "AEEIPrivSet.h"
#include "AEECCritSect.h"

using namespace ds::Net;
using namespace ds::Error;

/*---------------------------------------------------------------------------
  Static Data Member Definitions
---------------------------------------------------------------------------*/
NetworkFactory* NetworkFactory::instance = 0;


/*===========================================================================

                        PRIVATE METHODS DEFINITION

===========================================================================*/

NetworkFactory::NetworkFactory
(
  void
)
throw()
: refCnt (0)
{
  LOG_MSG_INFO1 ("Creating NetworkFactory 0x%p", this, 0, 0);

  if (AEE_SUCCESS != DS_Utils_CreateInstance (NULL,
                                              AEECLSID_CCritSect,
                                              NULL,
                                             (void **) &mpICritSect))
  {
    LOG_MSG_FATAL_ERROR ("Cannot create crit sect", 0, 0, 0);
    ASSERT (0);
  }

} /* NetworkFactory() */

NetworkFactory::~NetworkFactory
(
  void
)
throw()
{

  LOG_MSG_INFO1 ("Deleting NetworkFactory 0x%p", this, 0, 0);

  /*lint -save -e1550, -e1551 */
  DS_UTILS_RELEASEIF(mpICritSect);
  /*lint -restore */

} /* ~NetworkFactory() */

/*===========================================================================

                        PUBLIC METHODS DEFINITION

===========================================================================*/

NetworkFactory * NetworkFactory::Instance
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO1 ("Instance() called",0, 0, 0);
  /*-------------------------------------------------------------------------
    Allocate a NetworkFactory object if it is not already allocated.

    Since factory pattern is used, a new object is not allocated each time
    CreateInstance() is called
  -------------------------------------------------------------------------*/
  if (0 == instance)
  {
    instance = new NetworkFactory();
    if (0 == instance)
    {
      LOG_MSG_ERROR( "No mem for NetworkFactory", 0, 0, 0);
      ASSERT(0);
    }
  }
  return instance;
} /* NetworkFactory::Instance() */


/*---------------------------------------------------------------------------
  Inherited function from INetworkFactory
---------------------------------------------------------------------------*/
int NetworkFactory::CreateNetwork
(
  NetworkModeType     networkMode,
  IPolicy*            pIPolicy,
  INetwork**          ppINetwork,
  IPrivSet*           privSetPtr
)
{
  Network*            pINetwork = NULL;
  int32               result;
  int                 bIsDefaultPolicy = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("mode %d", networkMode, 0, 0);

  /* Validate arguments */
  if (NULL == ppINetwork || NULL == privSetPtr)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  // if pIPolicy is NULL, default policy shall be used
  if (NULL == pIPolicy)
  {
    bIsDefaultPolicy = TRUE;
    pIPolicy = new Policy();
    if (NULL == pIPolicy)
    {
      LOG_MSG_ERROR ("Cant create NetPolicy object", 0, 0, 0);
      return AEE_ENOMEMORY;
    }
  }

  /*-------------------------------------------------------------------------
    Check client provided privileges and Network Mode
  -------------------------------------------------------------------------*/
  if(ds::Net::NetworkMode::QDS_ACTIVE == networkMode)
  {
    if (AEE_SUCCESS != 
      privSetPtr->CheckPrivileges((AEEPRIVID *)&AEEPRIVID_PNet,
                                   sizeof(AEEPRIVID_PNet)))
    {
      LOG_MSG_ERROR( "No privilege for CreateNetwork operation", 0, 0, 0);
      return AEE_EPRIVLEVEL;
    }
  }
  else if(ds::Net::NetworkMode::QDS_MONITORED == networkMode)
  {
    if (AEE_SUCCESS != 
      privSetPtr->CheckPrivileges((AEEPRIVID *)&AEEPRIVID_PNetMonitored,
                                   sizeof(AEEPRIVID_PNetMonitored)))
    {
      LOG_MSG_ERROR( "No privilege for CreateNetwork operation", 0, 0, 0);
      return AEE_EPRIVLEVEL;
    }
  }
  else
  {
    LOG_MSG_ERROR ("Invalid network mode 0x%x", networkMode, 0, 0);
    return QDS_EINVAL;
  }

  /* Initialize the rout arg to NULL */
  *ppINetwork = NULL;

  if (ds::Net::NetworkMode::QDS_ACTIVE == networkMode)
  {
     pINetwork = new NetworkActive(reinterpret_cast <Policy *> (pIPolicy),
                privSetPtr);
  }
  else
  {
     pINetwork = new NetworkMonitored(reinterpret_cast <Policy *> (pIPolicy),
                                      privSetPtr);
  }
  if (NULL == pINetwork)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  if (bIsDefaultPolicy)
  {
    DS_UTILS_RELEASEIF(pIPolicy);
  }
  
  result = pINetwork->Init();
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  if (ds::Net::NetworkMode::QDS_ACTIVE == networkMode)
  {
    result = pINetwork->BringUpInterface();
    if (AEE_SUCCESS != result && AEE_EWOULDBLOCK != result)
    {
      goto bail;
    }
  }
  else
  {
    result = pINetwork->LookupInterface();
    if (AEE_SUCCESS != result && AEE_EWOULDBLOCK != result)
    {
      goto bail;
    }
  }

  /*-------------------------------------------------------------------------
    From API perspective, we are requested to create a network object. Since
    the network object creation is successful, return SUCCESS even if
    bring-up is blocking.
  -------------------------------------------------------------------------*/
  if (AEE_EWOULDBLOCK == result)
  {
    result = AEE_SUCCESS;
  }

  *ppINetwork = static_cast <INetwork *> (pINetwork);

  return result;

bail:
  /* Error handling */
  LOG_MSG_ERROR ("Err %d", result, 0, 0);
  DS_UTILS_RELEASEIF (pINetwork);
  
  if (bIsDefaultPolicy)
  {
    DS_UTILS_RELEASEIF(pIPolicy);
  }
  
  *ppINetwork = NULL;
  return result;

} /* CreateNetwork() */

ds::ErrorType NetworkFactory::CreateIPFilterSpec
(
 IIPFilterPriv** ppIIPFilterSpec
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#ifndef FEATURE_DSS_LINUX
  if (NULL == ppIIPFilterSpec)
  {
    LOG_MSG_INVALID_INPUT ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *ppIIPFilterSpec = static_cast <IIPFilterPriv *>(new IPFilterSpec());
  if (NULL == *ppIIPFilterSpec)
  {
    LOG_MSG_ERROR ("Cant create IPFilterSpec object", 0, 0, 0);
    return AEE_ENOMEMORY;
  }

  LOG_MSG_INFO1 ("Created 0x%p", *ppIIPFilterSpec, 0, 0);

  return AEE_SUCCESS;
#else
  LOG_MSG_ERROR ("QOS not yet supported", 0, 0, 0);
  return AEE_EFAILED;
#endif
}

ds::ErrorType NetworkFactory::CreateQoSFlowSpec
(
 IQoSFlowPriv** ppIQoSFlowSpec
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#ifndef FEATURE_DSS_LINUX
  if (NULL == ppIQoSFlowSpec)
  {
    LOG_MSG_INVALID_INPUT ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *ppIQoSFlowSpec = static_cast <IQoSFlowPriv *> (new QoSFlowSpec());
  if (NULL == *ppIQoSFlowSpec)
  {
    LOG_MSG_ERROR ("Cant create QoSFlowSpec object", 0, 0, 0);
    return AEE_ENOMEMORY;
  }

  LOG_MSG_INFO1 ("Created 0x%p", *ppIQoSFlowSpec, 0, 0);
  return AEE_SUCCESS;
#else
  LOG_MSG_ERROR ("QOS not yet supported", 0, 0, 0);
  return AEE_EFAILED;
#endif
}

ds::ErrorType NetworkFactory::CreatePolicy
(
  IPolicy**           ppIPolicy,
  IPrivSet*           privSetPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO2 ("CreateMBMSSpec called on object 0x%p", this, 0, 0);

  if (NULL == ppIPolicy || 0 == privSetPtr)
  {
    LOG_MSG_INVALID_INPUT ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != 
        privSetPtr->CheckPrivileges((AEEPRIVID *)&AEEPRIVID_PNetPolicy,
                                     sizeof(AEEPRIVID_PNetPolicy)))
  {
    LOG_MSG_ERROR ("Cant create MBMSSpec object", 0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  *ppIPolicy = static_cast <IPolicy *> (new Policy());
  if (NULL == *ppIPolicy)
  {
    LOG_MSG_ERROR ("Cant create Policy object", 0, 0, 0);
   return AEE_ENOMEMORY;
  }

  LOG_MSG_INFO1 ("Created 0x%p", *ppIPolicy, 0, 0);
  return AEE_SUCCESS;
}

ds::ErrorType NetworkFactory::CreateTechUMTS
(
  ITechUMTS **         newTechUMTS,
  IPrivSet *           privSetPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == newTechUMTS || 0 == privSetPtr)
  {
    LOG_MSG_INVALID_INPUT ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Check client provided privileges
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS != 
    privSetPtr->CheckPrivileges((AEEPRIVID *)&AEEPRIVID_PMTPD,
                                sizeof(AEEPRIVID_PMTPD)))
  {
    LOG_MSG_ERROR( "No privilege for CreateTechUMTS operation", 0, 0, 0);
    return AEE_EPRIVLEVEL;
  }

  *newTechUMTS = static_cast <ITechUMTS *> (TechUMTSFactory::Instance());
  if (NULL == *newTechUMTS)
  {
    LOG_MSG_ERROR ("Cant create TechUMTSFactory object", 0, 0, 0);
    return AEE_ENOMEMORY;
  }

  LOG_MSG_INFO1 ("Created 0x%p", *newTechUMTS, 0, 0);
  return AEE_SUCCESS;
}

/*---------------------------------------------------------------------------
  Inherited functions from INetworkFactoryPriv
---------------------------------------------------------------------------*/
int NetworkFactory::CreateNetworkPriv
(
  IPolicyPriv*      pIPolicyPriv, /*lint -esym(429,pIPolicyPriv) */
  INetworkPriv**    ppINetworkPriv,
  IPrivSet*         privSetPtr
)
{
  Network     *pINetwork = NULL; /*lint -esym(429,pINetwork) */
  IPolicyPriv *pIDefaultPolicy = NULL;
  int32        result = AEE_SUCCESS;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Validate arguments */
  if (NULL == ppINetworkPriv || 0 == privSetPtr)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  if (NULL == pIPolicyPriv)
  {/* Create default policy if policy was not provided. */
    pIDefaultPolicy = new Policy();
    if (NULL == pIDefaultPolicy)
    {
      LOG_MSG_ERROR ("Cant create default NetPolicy object", 0, 0, 0);
      return AEE_ENOMEMORY;
    }
    pIPolicyPriv = pIDefaultPolicy;
  }

  do
  {
    /* Initialize the rout arg to NULL */
    *ppINetworkPriv = NULL;

    pINetwork = new NetworkActive(
                  reinterpret_cast <Policy *> (pIPolicyPriv),
                  privSetPtr);
    if (NULL == pINetwork)
    {
      result = AEE_ENOMEMORY;
      break;
    }


    result = pINetwork->Init();
    if (AEE_SUCCESS != result)
    {
      break;
    }

    *ppINetworkPriv = static_cast <INetworkPriv *> (pINetwork);

  } while (0);

  /* Error handling */
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err %d creating net obj", result, 0, 0);
    DS_UTILS_RELEASEIF(pINetwork);
    *ppINetworkPriv = NULL;
  }

  if (NULL != pIDefaultPolicy)
  {
    (void)pIDefaultPolicy->Release();
  }

  LOG_MSG_INFO1 ("Created 0x%p", *ppINetworkPriv, 0, 0);
  return result;
}

ds::ErrorType NetworkFactory::CreatePolicyPriv
(
  IPolicyPriv** ppIPolicyPriv
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   if (NULL == ppIPolicyPriv)
   {
     LOG_MSG_INVALID_INPUT ("NULL Args", 0, 0, 0);
     return QDS_EFAULT;
   }

   *ppIPolicyPriv = static_cast <IPolicyPriv *> (new Policy());
   if (NULL == *ppIPolicyPriv)
   {
     LOG_MSG_ERROR ("Cant create Policy object", 0, 0, 0);
     return AEE_ENOMEMORY;
   }

   LOG_MSG_INFO1 ("Created 0x%p", *ppIPolicyPriv, 0, 0);


   return AEE_SUCCESS;
}

ds::ErrorType NetworkFactory::CreateNatSessionPriv
(
  ds::Net::IPolicyPriv          *pIPolicyPriv,
  ds::Net::INatSessionPriv     **ppINatSessionPriv
)
{
  ds::Net::IPolicy              *pIPolicy;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Creating NAT session, policy %p",
                          pIPolicyPriv, 0, 0);

  if (NULL == ppINatSessionPriv)
  {
    LOG_MSG_ERROR ("Invalid arg", 0, 0, 0);
    return QDS_EFAULT;
  }

  pIPolicy = static_cast <IPolicy *> (pIPolicyPriv);
  *ppINatSessionPriv = static_cast <INatSessionPriv *> (new NatManager(pIPolicy));
  if (NULL == *ppINatSessionPriv)
  {
    LOG_MSG_ERROR ("Cant create NAT session", 0, 0, 0);
    return AEE_ENOMEMORY;
  }

  LOG_MSG_FUNCTION_EXIT ("Success", 0, 0, 0);
  return AEE_SUCCESS;

} /* NetworkFactory::CreateNatSessionPriv() */
