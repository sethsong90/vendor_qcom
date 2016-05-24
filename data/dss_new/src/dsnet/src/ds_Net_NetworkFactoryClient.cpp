/*===========================================================================
  FILE: ds_Net_NetworkFactoryClient.cpp

  OVERVIEW: This file provides implementation of the NetworkFactoryClient class.

  DEPENDENCIES: None

                    Copyright (c) 2010 Qualcomm Technologies, Inc.
                    All Rights Reserved.
                    Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NetworkFactoryClient.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-06-13 vm Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "AEEIPrivSet.h"
#include "ds_Net_NetworkFactoryClient.h"
#include "ds_Net_NetworkFactory.h"
#include "ds_Net_CNetworkFactory.h"
#include "ds_Net_CNetworkFactoryPriv.h"
#include "ds_Net_CNetworkFactoryService.h"
#include "ds_Net_CNetworkFactoryPrivService.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ps_mem.h"

using namespace ds::Net;
using namespace ds::Error;
using namespace ds::Utils;

/*===========================================================================

                         PUBLIC DATA DECLARATIONS

===========================================================================*/


/*===========================================================================

                         PUBLIC MEMBER FUNCTIONS

===========================================================================*/

ds::ErrorType CDECL NetworkFactoryClient::CreateNetwork
(
  NetworkModeType     networkMode,
  IPolicy*            pIPolicy,
  INetwork**          ppINetwork
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  ds::ErrorType retval = AEE_SUCCESS;
  LOG_MSG_FUNCTION_ENTRY( "NetworkFactoryClient 0x%x", this, 0, 0);

  /*-------------------------------------------------------------------------
    Delegate to NetworkFactory with client's privileges
  -------------------------------------------------------------------------*/
  if ( 0 == networkFactoryPtr)
  {
    LOG_MSG_ERROR( "NULL Network Factory", 0, 0, 0);

    retval = AEE_EFAILED;

    goto bail;
  }

  retval = networkFactoryPtr->CreateNetwork(networkMode,
                                            pIPolicy,
                                            ppINetwork,
                                            privSetPtr);

  /* fall through */

bail:

  LOG_MSG_FUNCTION_EXIT( "Success, NetworkFactoryClient 0x%x", this, 0, 0);

  return retval;
}/* CreateNetwork() */

ds::ErrorType CDECL NetworkFactoryClient::CreateNetworkPriv
(
  IPolicyPriv*        pIPolicyPriv,
  INetworkPriv**      ppINetworkPriv
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  ds::ErrorType retval = AEE_SUCCESS;
  LOG_MSG_FUNCTION_ENTRY( "NetworkFactoryClient 0x%x", this, 0, 0);

  /*-------------------------------------------------------------------------
  Delegate to NetworkFactory with client's privileges
  -------------------------------------------------------------------------*/
  if ( 0 == networkFactoryPtr)
  {
    LOG_MSG_ERROR( "NULL Network Factory", 0, 0, 0);

    retval = AEE_EFAILED;

    goto bail;
  }

  retval = networkFactoryPtr->CreateNetworkPriv(pIPolicyPriv,
                                                ppINetworkPriv,
                                                privSetPtr);

  /* fall through */

bail:

  LOG_MSG_FUNCTION_EXIT( "Success, NetworkFactoryClient 0x%x", this, 0, 0);

  return retval;
}/* CreateNetworkPriv() */

void * NetworkFactoryClient::CreateInstance
(
  AEECLSID    clsID,
  IPrivSet *  _privSetPtr
)
{
  void* retVal = 0;
  INetworkFactory     *piNetworkFactory = 0;
  INetworkFactoryPriv *piNetworkFactoryPriv = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (0 == _privSetPtr)
  {
    LOG_MSG_ERROR( "NULL privset", 0, 0, 0);
    goto bail;
  }

  switch (clsID)
  {
    /*-------------------------------------------------------------------------
      Note: this code can be called both
            from DS
              - AEECLSID_CNetworkFactory
              - AEECLSID_CNetworkFactoryPriv
            or from CS
              - AEECLSID_CNetworkFactoryService
              - AEECLSID_CNetworkFactoryPrivService
    -------------------------------------------------------------------------*/
    case AEECLSID_CNetworkFactory:
    case AEECLSID_CNetworkFactoryService:
      piNetworkFactory = (INetworkFactory*)(new NetworkFactoryClient(_privSetPtr,
        NetworkFactory::Instance()));
      if (0 == piNetworkFactory)
      {
        LOG_MSG_ERROR( "Failed to create NetworkFactoryClient", 0, 0, 0);
        goto bail;
      }

      (void)piNetworkFactory->AddRef();

      retVal = piNetworkFactory;

      break;
    case AEECLSID_CNetworkFactoryPriv:
    case AEECLSID_CNetworkFactoryPrivService:
      piNetworkFactoryPriv = (INetworkFactoryPriv*)(new NetworkFactoryClient(_privSetPtr,
                                      NetworkFactory::Instance()));
      if (0 == piNetworkFactoryPriv)
      {
        LOG_MSG_ERROR( "Failed to create NetworkFactoryClient", 0, 0, 0);
        goto bail;
      }

      (void)piNetworkFactoryPriv->AddRef();

      retVal = piNetworkFactoryPriv;
      break;
    default:
      LOG_MSG_ERROR( "Class ID %0x is not supported", clsID, 0, 0);
      goto bail;
  }

/* fall through */

bail:

  return retVal;
} /* CreateInstance() */

NetworkFactoryClient::NetworkFactoryClient
(
  IPrivSet       * _privSetPtr,
  NetworkFactory * _networkFactory
)throw()
: refCnt(0)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_FUNCTION_ENTRY( "NetworkFactoryClient 0x%x", this, 0, 0);

  if (0 == _privSetPtr)
  {
    LOG_MSG_ERROR( "NULL privset", 0, 0, 0);
    ASSERT(0);
  }

  privSetPtr = _privSetPtr;
  (void)privSetPtr->AddRef();

  if (0 == _networkFactory)
  {
    LOG_MSG_ERROR( "NULL _networkFactory", 0, 0, 0);
    ASSERT(0);
  }

  networkFactoryPtr = _networkFactory;

  (void)networkFactoryPtr->AddRef();

  LOG_MSG_FUNCTION_EXIT("Created factory client 0x%p with factory 0x%p",
                        this, networkFactoryPtr, 0);

} /* NetworkFactoryClient() */


NetworkFactoryClient::~NetworkFactoryClient
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_FUNCTION_ENTRY( "NetworkFactoryClient 0x%x", this, 0, 0);

  if (0 != networkFactoryPtr)
  {
    (void)networkFactoryPtr->Release();
  }

  if (0 != privSetPtr)
  {
    (void)privSetPtr->Release();
  }

  LOG_MSG_FUNCTION_EXIT( "Deleted factory client 0x%p", this, 0, 0);
} /* ~NetworkFactoryClient() */


void * NetworkFactoryClient::operator new
(
  unsigned int numBytes
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return ps_mem_get_buf( PS_MEM_DS_NET_NETWORK_FACTORY_CLIENT_TYPE);

} /* operator new() */


void NetworkFactoryClient::operator delete
(
  void *  bufPtr
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == bufPtr)
  {
    LOG_MSG_ERROR( "NULL ptr", 0, 0, 0);
    ASSERT( 0);
    return;
  }

  PS_MEM_FREE(bufPtr);
  return;

} /* operator delete() */

int NetworkFactoryClient::QueryInterface
(
  AEEIID iid,
  void **ppo
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1  ("Obj 0x%p, iid 0x%x", this, iid, 0);

  if (NULL == ppo)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *ppo = NULL;

  switch (iid)
  {
  case AEEIID_INetworkFactory:
    *ppo = static_cast <INetworkFactory *> (this);
    break;

  case AEEIID_INetworkFactoryPriv:
    *ppo = static_cast <INetworkFactoryPriv *>(this);
    break;

  case AEEIID_IQI:
    *ppo = reinterpret_cast <IQI *> (this);
    break;

  default:
    return AEE_ECLASSNOTSUPPORT;
  }

  (void) AddRef();

  return AEE_SUCCESS;
}/* QueryInterface() */

ds::ErrorType CDECL NetworkFactoryClient::CreatePolicyPriv
(
  IPolicyPriv**       ppIPolicy
)
{
  if(0 == networkFactoryPtr)
  {
    return QDS_EFAULT;
  }

  return networkFactoryPtr->CreatePolicyPriv(ppIPolicy);
}/* CreatePolicyPriv() */

ds::ErrorType CDECL NetworkFactoryClient::CreateIPFilterSpec
(
  IIPFilterPriv**         ppIIPFilterSpec
)
{
  if(0 == networkFactoryPtr)
  {
    return QDS_EFAULT;
  }

  return networkFactoryPtr->CreateIPFilterSpec(ppIIPFilterSpec);

}/* CreateIPFilterSpec() */

ds::ErrorType CDECL NetworkFactoryClient::CreateQoSFlowSpec
(
  IQoSFlowPriv**          ppIQoSSpec
)
{
  if(0 == networkFactoryPtr)
  {
    return QDS_EFAULT;
  }

  return networkFactoryPtr->CreateQoSFlowSpec(ppIQoSSpec);

}/* CreateQoSFlowSpec() */

ds::ErrorType CDECL NetworkFactoryClient::CreatePolicy 
(
  IPolicy**           ppIPolicy
)
{
  if(0 == networkFactoryPtr)
  {
    return QDS_EFAULT;
  }

  return networkFactoryPtr->CreatePolicy(ppIPolicy, privSetPtr);

}/* CreatePolicy() */

ds::ErrorType CDECL NetworkFactoryClient::CreateTechUMTS
(
  ITechUMTS** newTechUMTS
)
{
  if(0 == networkFactoryPtr)
  {
    return QDS_EFAULT;
  }

  return networkFactoryPtr->CreateTechUMTS(newTechUMTS, privSetPtr);

}/* CreateTechUMTS() */

ds::ErrorType CDECL NetworkFactoryClient::CreateNatSessionPriv
(
 ds::Net::IPolicyPriv          *pIPolicy,
 ds::Net::INatSessionPriv     **ppINatSessionPriv
)
{
  if (0 == networkFactoryPtr)
  {
    return QDS_EFAULT;
  }
  return networkFactoryPtr->CreateNatSessionPriv(pIPolicy, ppINatSessionPriv);

} /* CreateNatSessionPriv() */


/*===========================================================================

                          PROTECTED MEMBER FUNCTIONS

===========================================================================*/



/*===========================================================================

                         PRIVATE MEMBER FUNCTIONS

===========================================================================*/



#endif /* FEATURE_DATA_PS */
