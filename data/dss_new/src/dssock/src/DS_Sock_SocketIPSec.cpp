/*===========================================================================
  FILE: DS_Sock_SocketIPSec.cpp

  OVERVIEW: This file provides implementation of the SocketIPSec class.

  DEPENDENCIES: None

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_SocketIPSec.cpp#4 $
  $DateTime: 2010/06/08 12:31:42 $$Author: smudired $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2009-10-05 ss  KW warning fixes, in FltrIPSecClient()
  2009-03-16 msr Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"
#include "target.h"

#ifdef FEATURE_DATA_PS
extern "C"
{
#include "secips.h"
}

#include "DS_Sock_SocketIPSec.h"
#include "DS_Net_Platform.h"


using namespace DS::Sock;
using namespace DS::Error;
using namespace DS::Utils;


/*===========================================================================

                         PUBLIC MEMBER FUNCTIONS

===========================================================================*/
bool SocketIPSec::IsHandleInIfaceList
(
  int32                   ifaceHandle,
  ps_rt_meta_info_type *  rtMetaInfoPtr
)
{
  (void)ifaceHandle;
  (void)rtMetaInfoPtr;
#ifdef FEATURE_SEC_IPSEC
  ps_ipsec_info_type *  ipsecInfoPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ipsecInfoPtr = &( PS_RT_META_GET_IPSEC_INFO( rtMetaInfoPtr));

  /*-------------------------------------------------------------------------
    Since data doesn't match IPSec filters all the time, check further only
    if iface_cnt != 0.

    Return true if
      1. Handle matches iface at index 0 OR
      2. iface_cnt is 2 and handle matched iface at index 1

    Theoretically iface_cnt can be any number, but in current configuration,
    it can't be more than 2
  -------------------------------------------------------------------------*/
  if (0 == ipsecInfoPtr->iface_cnt)
  {
    return true;
  }

  ASSERT( 2 >= ipsecInfoPtr->iface_cnt);

  if (ifaceHandle == (int32) ipsecInfoPtr->iface_list[0] ||
      ( 2 == ipsecInfoPtr->iface_cnt &&
        ifaceHandle == (int32) ipsecInfoPtr->iface_list[1]))
  {
    return true;
  }

  return false;
#else
  return false;
#endif /* FEATURE_SEC_IPSEC */

} /* SocketIPSec::IsHandleInIfaceList() */


bool SocketIPSec::IsIfaceListInReqState
(
  ps_rt_meta_info_type *  rtMetaInfoPtr,
  uint32                  reqIfaceState
)
{
  (void)rtMetaInfoPtr;
  (void)reqIfaceState;
#ifdef FEATURE_SEC_IPSEC
  ps_ipsec_info_type     *  ipsecInfoPtr;
  int32                     ipsecIfaceHandle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ipsecInfoPtr = &( PS_RT_META_GET_IPSEC_INFO( rtMetaInfoPtr));

  /*-------------------------------------------------------------------------
    Since data doesn't match IPSec filters all the time, check further only
    if iface_cnt != 0.

    Return true if
      1. Handle matches iface at index 0 OR
      2. iface_cnt is 2 and handle matched iface at index 1

    Theoretically iface_cnt can be any number, but in current configuration,
    it can't be more than 2
  -------------------------------------------------------------------------*/
  if (0 == ipsecInfoPtr->iface_cnt)
  {
    return true;
  }

  ASSERT( 2 >= ipsecInfoPtr->iface_cnt);

  ipsecIfaceHandle = reinterpret_cast <int32> ( ipsecInfoPtr->iface_list[0]);
  if (0 != ( reqIfaceState & NetPlatform::GetIfaceState( ipsecIfaceHandle)))
  {
    if ( 2 == ipsecInfoPtr->iface_cnt)
    {
      ipsecIfaceHandle =
        reinterpret_cast <int32> ( ipsecInfoPtr->iface_list[1]);
      if (0 != ( reqIfaceState & NetPlatform::GetIfaceState( ipsecIfaceHandle)))
      {
        return true;
      }
    }
    else
    {
      return true;
    }
  }

  return false;
#else
  return true;
#endif /* FEATURE_SEC_IPSEC */

} /* SocketIPSec::IsIfaceListInReqState() */


void SocketIPSec::ClearIfaceList
(
  ps_rt_meta_info_type *  rtMetaInfoPtr
)
{
  (void)rtMetaInfoPtr;
#ifdef FEATURE_SEC_IPSEC
  ps_ipsec_info_type *  ipsecInfoPtr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 != rtMetaInfoPtr)
  {
    ipsecInfoPtr = &( PS_RT_META_GET_IPSEC_INFO( rtMetaInfoPtr));

    memset( ipsecInfoPtr, 0, sizeof( ps_ipsec_info_type));
  }
#endif /* FEATURE_SEC_IPSEC */

  return;
} /* SocketIPSec::ClearIfaceList() */


void SocketIPSec::TearDownIfaceList
(
  ps_rt_meta_info_type *  rtMetaInfoPtr
)
{
  (void)rtMetaInfoPtr;
#ifdef FEATURE_SEC_IPSEC
  ps_ipsec_info_type *  ipsecInfoPtr;
  int32                 idx;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ipsecInfoPtr = &( PS_RT_META_GET_IPSEC_INFO( rtMetaInfoPtr));

  for (idx = 0; idx < ipsecInfoPtr->iface_cnt; idx++)
  {
    (void) NetPlatform::IfaceTearDownCmd( (int32) ipsecInfoPtr->iface_list[idx],
                                          ipsecInfoPtr);
  }

  memset( ipsecInfoPtr, 0, sizeof( ps_ipsec_info_type));
#endif /* FEATURE_SEC_IPSEC */

  return;
} /* SocketIPSec::TearDownIfaceList() */


bool SocketIPSec::IsIfaceListFlowEnabled
(
  ps_rt_meta_info_type *  rtMetaInfoPtr
)
{
  (void)rtMetaInfoPtr;
#ifdef FEATURE_SEC_IPSEC
  ps_ipsec_info_type *  ipsecInfoPtr;
  int32                 idx;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ipsecInfoPtr = &( PS_RT_META_GET_IPSEC_INFO( rtMetaInfoPtr));

  for (idx = 0; idx < ipsecInfoPtr->iface_cnt; idx++)
  {
    if (NetPlatform::IsPSIfaceTxEnabled( (int32) ipsecInfoPtr->iface_list[idx]))
    {
      return false;
    }
  }
#endif /* FEATURE_SEC_IPSEC */

  return true;
} /* SocketIPSec::IsIfaceListFlowEnabled() */


void SocketIPSec::FltrIPSecClient
(
  Socket *                sockPtr,
  ps_rt_meta_info_type *  newRtMetaInfoPtr,
  ps_rt_meta_info_type *  oldRtMetaInfoPtr
)
{
  (void)sockPtr;
  (void)newRtMetaInfoPtr;
  (void)oldRtMetaInfoPtr;
#ifdef FEATURE_SEC_IPSEC
  ip_pkt_info_type *    pktInfoPtr;
  ps_ipsec_info_type *  oldIPSecInfoPtr;
  ps_ipsec_info_type    ipsecInfo;
  ps_ipsec_info_type    tmpIPsecInfo = {0};
  uint32                ipsecHandle;
  int32                 idx;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == newRtMetaInfoPtr)
  {
    LOG_MSG_ERROR( "NULL args", 0, 0, 0);
    ASSERT( 0);
    return;
  }

  pktInfoPtr = &( PS_RT_META_GET_PKT_INFO( newRtMetaInfoPtr));

  /*-------------------------------------------------------------------------
    Initialize ipsecInfo in new rt meta info to 0 and perform filtering
  -------------------------------------------------------------------------*/
  memset( &( PS_RT_META_GET_IPSEC_INFO( newRtMetaInfoPtr)),
          0,
          sizeof( ps_ipsec_info_type));

  ipsecHandle =
    NetPlatform::IPFltrExecute( 0, IP_FLTR_CLIENT_IPSEC_OUTPUT, pktInfoPtr);

  /*-------------------------------------------------------------------------
    Generate fresh ipsecInfo if filters matched and update newRtMetaInfoPtr
  -------------------------------------------------------------------------*/
  memset( &ipsecInfo, 0, sizeof( ipsecInfo));

  if (0 != ipsecHandle)
  {
    (void) secips_generate_ipsec_info
           (
             reinterpret_cast <void *> ( ipsecHandle),
             pktInfoPtr,
             &ipsecInfo,
             reinterpret_cast <uint32> ( sockPtr)
           );

    PS_RT_META_SET_IPSEC_INFO( newRtMetaInfoPtr, ipsecInfo);
  }

  /*-------------------------------------------------------------------------
    If new IPSEC filter result differs from old IPSEC filter result,
    tear down old IPSEC ifaces as they are no longer used.

    This has to happen even if new IPSEC filter result is 0 as even in this
    case old IPSEC filter result is no longer is used.
  -------------------------------------------------------------------------*/
  if (0 != oldRtMetaInfoPtr)
  {
    oldIPSecInfoPtr = &( PS_RT_META_GET_IPSEC_INFO( oldRtMetaInfoPtr));
  }
  else
  {
    oldIPSecInfoPtr = &tmpIPsecInfo;
  }

  /*lint -save -e644 */
  if (ipsecInfo.ipsec_handle != oldIPSecInfoPtr->ipsec_handle)
  {
    for (idx = 0; idx < oldIPSecInfoPtr->iface_cnt; idx++)
    {
      (void) NetPlatform::IfaceTearDownCmd
             (
               reinterpret_cast <int32> ( oldIPSecInfoPtr->iface_list[idx]),
               oldIPSecInfoPtr
             );
    }

    /*-----------------------------------------------------------------------
      If IPSEC filters are matched, bring up new IPSEC ifaces
    -----------------------------------------------------------------------*/
    if (0 != ipsecHandle)
    {
      for (idx = 0; idx < ipsecInfo.iface_cnt; idx++)
      {
        (void) NetPlatform::IfaceBringUpCmd
               (
                 reinterpret_cast <int32> ( ipsecInfo.iface_list[idx]),
                 &ipsecInfo
               );
      }
    }
  }
  /*lint -restore -e644 */
#endif /* FEATURE_SEC_IPSEC */

  return;
} /* SocketIPSec::FltrIPSecClient() */

#endif /* FEATURE_DATA_PS */
