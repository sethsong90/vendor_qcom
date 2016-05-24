/*===========================================================================
  FILE: DS_Sock_RoutingManager.cpp

  OVERVIEW: This file provides implementation of the RoutingManager class.

  DEPENDENCIES: None

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_RoutingManager.cpp#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-14 msr Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"
#include "target.h"

#ifdef FEATURE_DATA_PS
#include "amssassert.h"
#include <string.h>
#include "DS_Sock_RoutingManager.h"
#include "DS_Net_Platform.h"
#include "DS_Utils_DebugMsg.h"

using namespace DS::Sock;
using namespace DS::Net;
using namespace DS::Error;


/*===========================================================================

                     EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/
DS::ErrorType RoutingManager::RoutePacket
(
  DS::Sock::Socket *      sockPtr,
  bool                    isSystemSocket,
  IPolicy *               policyPtr,
  ps_rt_meta_info_type *  newRtMetaInfoPtr
)
{
  ip_pkt_info_type *  pktInfoPtr;
  int32               routingCache;
  int32               retVal;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x", sockPtr, 0, 0);

  if (0 == newRtMetaInfoPtr)
  {
    LOG_MSG_ERROR( "NULL meta info, sock 0x%x", sockPtr, 0, 0);
    ASSERT( 0);
    return DSS_EFAULT;
  }

  /*-------------------------------------------------------------------------
    Perform routing look up
  -------------------------------------------------------------------------*/
  pktInfoPtr = &( PS_RT_META_GET_PKT_INFO( newRtMetaInfoPtr));

  retVal = NetPlatform::RouteDataPathLookup( pktInfoPtr,
                                             isSystemSocket,
                                             policyPtr,
                                             FALSE,
                                             &routingCache);
  if (DSS_ERROR == retVal)
  {
    LOG_MSG_INFO1( "Rt lookup failed, sock 0x%x", sockPtr, 0, 0);
    return DSS_ENOROUTE;
  }

  /*-------------------------------------------------------------------------
    Update rt meta info with routing cache
  -------------------------------------------------------------------------*/
  LOG_MSG_INFO1( "Rt cache 0x%x, sock 0x%x", routingCache, sockPtr, 0);
  PS_RT_META_SET_ROUTING_CACHE( newRtMetaInfoPtr,
                                reinterpret_cast <void *> ( routingCache));

  LOG_MSG_FUNCTION_EXIT( "Success, sock 0x%x", sockPtr, 0, 0);
  return SUCCESS;

} /* RoutingManager::RoutePacket() */


void RoutingManager::FltrClient
(
  DS::Sock::Socket *                   sockPtr,
  ps_iface_ipfltr_client_id_enum_type  fltrClient,
  ps_rt_meta_info_type *               newRtMetaInfoPtr
)
{
  ip_pkt_info_type *  pktInfoPtr;
  int32               fltrResult;
  int32               routingCache;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Sock 0x%x fltr client %d", sockPtr, fltrClient, 0);

  if (0 == newRtMetaInfoPtr)
  {
    LOG_MSG_ERROR( "NULL args", 0, 0, 0);
    ASSERT( 0);
    return;
  }

  pktInfoPtr = &( PS_RT_META_GET_PKT_INFO( newRtMetaInfoPtr));
  routingCache =
    reinterpret_cast <int32> ( PS_RT_META_GET_ROUTING_CACHE( newRtMetaInfoPtr));

  switch (fltrClient)
  {
    case IP_FLTR_CLIENT_QOS_OUTPUT:
    {
      /*---------------------------------------------------------------------
        Filter QoS client
      ---------------------------------------------------------------------*/
      fltrResult =
        NetPlatform::IPFltrExecute( routingCache, fltrClient, pktInfoPtr);

      /*---------------------------------------------------------------------
        Update rt meta info with fltr result. Use default ps_flow if filters
        didn't match
      ---------------------------------------------------------------------*/
      if (0 == fltrResult)
      {
        (void) NetPlatform::PSGetDefaultFlow( routingCache, &fltrResult);
      }

      PS_RT_META_SET_FILTER_RESULT( newRtMetaInfoPtr, fltrClient, fltrResult);
      break;
    }

    case IP_FLTR_CLIENT_HEADER_COMP:
    {
      /*---------------------------------------------------------------------
        Filter HC client
      ---------------------------------------------------------------------*/
      fltrResult =
        NetPlatform::IPFltrExecute( routingCache, fltrClient, pktInfoPtr);

      /*---------------------------------------------------------------------
        Update rt meta info with fltr result
      ---------------------------------------------------------------------*/
      PS_RT_META_SET_FILTER_RESULT( newRtMetaInfoPtr, fltrClient, fltrResult);
      break;
    }

    case IP_FLTR_CLIENT_IPSEC_OUTPUT:
    default:
    {
      LOG_MSG_ERROR( "Unknown fltr client %d", fltrClient, 0, 0);
      ASSERT( 0);
      break;
    }
  }

  return;
} /* RoutingManager::FltrClient() */

#endif /* FEATURE_DATA_PS */
