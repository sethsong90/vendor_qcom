
/*===========================================================================
  FILE: ds_Net_NatSession.cpp

  OVERVIEW: This file provides implementation of the NatSession class.

  DEPENDENCIES: None

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NatSession.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-20 bq  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "ds_Errors_Def.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Utils_Conversion.h"
#include "ds_Utils_CreateInstance.h"
#include "ds_Net_Utils.h"
#include "ds_Net_NatSession.h"
#include "ds_Net_Platform.h"
#include "ds_Net_Conversion.h"
#include "AEECCritSect.h"
#include "ps_system_heap.h"

extern "C"
{
  #include "ps_nat.h"
}

using namespace ds::Error;
using namespace ds::Net;
using namespace ds::Net::Conversion;
using namespace NetPlatform;

/*---------------------------------------------------------------------------
  CONSTRUCTOR/DESTRUCTOR
---------------------------------------------------------------------------*/
NatSession::NatSession
(
  int32 ifaceHandle 
) 
: mIfaceHandle (ifaceHandle),
  refCnt(1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating NatSession obj 0x%p, if handle 0x%x", 
                 this, ifaceHandle, 0);

} /* NatSession::NatSession() */

NatSession::~NatSession
(
  void
)
throw()
{
#ifdef FEATURE_DATA_PS_NAT_IFACE
  GetStaticNatEntryType natEntryArg;
  VPNPassThroughType    vpnArg;
  L2TPVPNPassThroughType    l2tpvpnArg;
  PPTPVPNPassThroughType    pptpvpnArg;
  SetDynamicNatEntryTimeoutType timeoutArg;
  nat_config_s  natConfig;
  int numEntries;
  int32 result;
  int index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleteing NatSession 0x%p if handle 0x%x",
                  this, mIfaceHandle, 0);

  memset (&natEntryArg, 0, sizeof (GetStaticNatEntryType));
  result = IfaceIoctlNonNullArg (mIfaceHandle,
                                 IFACE_IOCTL_GET_STATIC_NAT_ENTRY,
                                 (void *) &natEntryArg);

  /*-------------------------------------------------------------------------
    Delete static NAT entries
  -------------------------------------------------------------------------*/
  if (AEE_SUCCESS == result && natEntryArg.total_entries > 0)
  {
    numEntries = natEntryArg.total_entries;
    /*-------------------------------------------------------------------------
      Allocate memory for those static NAT entries and call the IOCTL again. 
    -------------------------------------------------------------------------*/
    memset (&natEntryArg, 0, sizeof(GetStaticNatEntryType));
    natEntryArg.num_entries = numEntries;
    natEntryArg.entries_arr = 
      (ps_iface_ioctl_static_nat_entry_type *)
      ps_system_heap_mem_alloc 
      (
        numEntries * sizeof (ps_iface_ioctl_static_nat_entry_type)
      );
  
    result = IfaceIoctlNonNullArg (mIfaceHandle,
                                   IFACE_IOCTL_GET_STATIC_NAT_ENTRY,
                                   (void *) &natEntryArg);
    if (AEE_SUCCESS == result)
    {
      for (index = 0; index < numEntries; index++)
      {
          (void) IfaceIoctlNonNullArg (mIfaceHandle,
                                       IFACE_IOCTL_DELETE_STATIC_NAT_ENTRY,
                                       (void *)&(natEntryArg.entries_arr[index]));
      }
    }
  }

  /*-------------------------------------------------------------------------
    Delete the DMZ entry
  -------------------------------------------------------------------------*/
  ps_nat_config_get(&natConfig);
  if (natConfig.is_dmz_enabled) 
  {
    LOG_MSG_INFO1 ("Deleteing NatSession: DMZ entry", 0, 0, 0);
    (void) IfaceIoctl (mIfaceHandle,
                       IFACE_IOCTL_DELETE_DMZ,
                       NULL);
  }
  
  /*-------------------------------------------------------------------------
    Set VPN Pass through to default
  -------------------------------------------------------------------------*/
  memset (&vpnArg, 0, sizeof(VPNPassThroughType));
  vpnArg.is_vpn_passthrough = TRUE;

  (void) IfaceIoctlNonNullArg (mIfaceHandle,
                               IFACE_IOCTL_SET_NAT_IPSEC_VPN_PASS_THROUGH,
                               (void *)&vpnArg);

  /*-------------------------------------------------------------------------
    Set L2TP VPN Pass through to default
  -------------------------------------------------------------------------*/
  memset (&l2tpvpnArg, 0, sizeof(L2TPVPNPassThroughType));
  l2tpvpnArg.is_l2tp_vpn_passthrough = FALSE;

  (void) IfaceIoctlNonNullArg (mIfaceHandle,
                               IFACE_IOCTL_SET_NAT_L2TP_VPN_PASS_THROUGH,
                               (void *)&l2tpvpnArg);

  /*-------------------------------------------------------------------------
    Set PPTP VPN Pass through to default
  -------------------------------------------------------------------------*/
  memset (&pptpvpnArg, 0, sizeof(PPTPVPNPassThroughType));
  pptpvpnArg.is_pptp_vpn_passthrough = FALSE;

  (void) IfaceIoctlNonNullArg (mIfaceHandle,
                               IFACE_IOCTL_SET_NAT_PPTP_VPN_PASS_THROUGH,
                               (void *)&pptpvpnArg);

  /*-------------------------------------------------------------------------
    Set dynamic NAT entry timeout to default
  -------------------------------------------------------------------------*/
  memset(&timeoutArg, 0, sizeof(SetDynamicNatEntryTimeoutType));
  timeoutArg.timeout = NAT_ENTRY_TIMEOUT;

  (void) IfaceIoctlNonNullArg (mIfaceHandle,
                               IFACE_IOCTL_SET_DYNAMIC_NAT_ENTRY_TIMEOUT,
                               (void *)&timeoutArg);

  /*-------------------------------------------------------------------------
    Set iface handle to 0
  -------------------------------------------------------------------------*/
  mIfaceHandle = 0;

  if (NULL != natEntryArg.entries_arr)
  {
    PS_SYSTEM_HEAP_MEM_FREE(natEntryArg.entries_arr);
  }

#else /* FEATURE_DATA_PS_NAT_IFACE  */  
  LOG_MSG_ERROR("Op not supported", 0, 0, 0);
#endif /* FEATURE_DATA_PS_NAT_IFACE */
} /* NatSession::~NatSession() */

ds::ErrorType NatSession::SetDynamicNatEntryTimeout
(
  unsigned short int  timeout
)
{
  SetDynamicNatEntryTimeoutType arg;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Set dynamic NAT entry timeout %d, obj 0x%p, handle 0x%x",
                 timeout, this, mIfaceHandle);

  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  arg.timeout = timeout;

  return IfaceIoctlNonNullArg (mIfaceHandle,
                               IFACE_IOCTL_SET_DYNAMIC_NAT_ENTRY_TIMEOUT,
                               (void *)&arg);

} /* NatSession::SetDynamicNatEntryTimeout() */

ds::ErrorType NatSession::GetDynamicNatEntryTimeout
(
  unsigned short int *    pTimeout
)
{
  GetDynamicNatEntryTimeoutType     arg;
  int32                             result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Get dynamic NAT entry time, obj 0x%p, handle 0x%x",
                          this, mIfaceHandle, 0);

  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  if (NULL == pTimeout)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  memset (&arg, 0, sizeof(GetDynamicNatEntryTimeoutType));
  result = IfaceIoctlNonNullArg (mIfaceHandle,
                                 IFACE_IOCTL_GET_DYNAMIC_NAT_ENTRY_TIMEOUT,
                                 (void *)&arg);

  *pTimeout = arg.timeout;
  LOG_MSG_FUNCTION_EXIT ("Exit, return val 0x%x, timeout %d",
                          result, *pTimeout, 0);
  return result;

} /* NatSession::GetDynamicNatEntryTimeout() */

ds::ErrorType NatSession::SetIpSecVpnPassThrough
(
  boolean isVpnPassThrough
)
{
  VPNPassThroughType                arg;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Set NAT VPN PassThrough %d, obj 0x%p, handle 0x%x",
                 isVpnPassThrough, this, mIfaceHandle);

  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  memset (&arg, 0, sizeof(VPNPassThroughType));
  arg.is_vpn_passthrough = isVpnPassThrough;

  return IfaceIoctlNonNullArg (mIfaceHandle,
                               IFACE_IOCTL_SET_NAT_IPSEC_VPN_PASS_THROUGH,
                               (void *)&arg);

} /* NatSession::SetIpSecVpnPassThrough() */

ds::ErrorType NatSession::GetIpSecVpnPassThrough
(
  boolean *isVpnPassThrough
)
{
  VPNPassThroughType                arg;
  int32                             result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Get NAT VPN PassThrough, obj 0x%p, handle 0x%x",
                          this, mIfaceHandle, 0);

  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  if (NULL == isVpnPassThrough)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  memset (&arg, 0, sizeof(VPNPassThroughType));
  result = IfaceIoctlNonNullArg (mIfaceHandle,
                                 IFACE_IOCTL_GET_NAT_IPSEC_VPN_PASS_THROUGH,
                                 (void *) &arg);

  *isVpnPassThrough = arg.is_vpn_passthrough;

  LOG_MSG_FUNCTION_EXIT ("Exit, return val 0x%x, VPN PassThrough %d",
                          result, *isVpnPassThrough, 0);
  return result;

} /* NatSession::()GetIpSecVpnPassThrough */

ds::ErrorType NatSession::SetL2TPVpnPassThrough
(
  boolean isVpnPassThrough
)
{
  L2TPVPNPassThroughType                arg;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Set NAT L2TP VPN PassThrough %d, obj 0x%p, handle 0x%x",
                 isVpnPassThrough, this, mIfaceHandle);

  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  memset (&arg, 0, sizeof(L2TPVPNPassThroughType));
  arg.is_l2tp_vpn_passthrough = isVpnPassThrough;

  return IfaceIoctlNonNullArg (mIfaceHandle,
                               IFACE_IOCTL_SET_NAT_L2TP_VPN_PASS_THROUGH,
                               (void *)&arg);

} /* NatSession::SetL2TPVpnPassThrough() */

ds::ErrorType NatSession::GetL2TPVpnPassThrough
(
  boolean *isVpnPassThrough
)
{
  L2TPVPNPassThroughType            arg;
  int32                             result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Get NAT L2TP VPN PassThrough, obj 0x%p, handle 0x%x",
                          this, mIfaceHandle, 0);

  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  if (NULL == isVpnPassThrough)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  memset (&arg, 0, sizeof(L2TPVPNPassThroughType));
  result = IfaceIoctlNonNullArg (mIfaceHandle,
                                 IFACE_IOCTL_GET_NAT_L2TP_VPN_PASS_THROUGH,
                                 (void *) &arg);

  *isVpnPassThrough = arg.is_l2tp_vpn_passthrough;

  LOG_MSG_FUNCTION_EXIT ("Exit, return val 0x%x, L2TP VPN PassThrough %d",
                          result, *isVpnPassThrough, 0);
  return result;

} /* NatSession::()GetL2TPVpnPassThrough */

ds::ErrorType NatSession::SetPPTPVpnPassThrough
(
  boolean isVpnPassThrough
)
{
  PPTPVPNPassThroughType                arg;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Set NAT PPTP VPN PassThrough %d, obj 0x%p, handle 0x%x",
                 isVpnPassThrough, this, mIfaceHandle);

  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  memset (&arg, 0, sizeof(PPTPVPNPassThroughType));
  arg.is_pptp_vpn_passthrough = isVpnPassThrough;

  return IfaceIoctlNonNullArg (mIfaceHandle,
                               IFACE_IOCTL_SET_NAT_PPTP_VPN_PASS_THROUGH,
                               (void *)&arg);

} /* NatSession::SetPPTPVpnPassThrough() */

ds::ErrorType NatSession::GetPPTPVpnPassThrough
(
  boolean *isVpnPassThrough
)
{
  PPTPVPNPassThroughType            arg;
  int32                             result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Get NAT PPTP VPN PassThrough, obj 0x%p, handle 0x%x",
                          this, mIfaceHandle, 0);

  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  if (NULL == isVpnPassThrough)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  memset (&arg, 0, sizeof(PPTPVPNPassThroughType));
  result = IfaceIoctlNonNullArg (mIfaceHandle,
                                 IFACE_IOCTL_GET_NAT_PPTP_VPN_PASS_THROUGH,
                                 (void *) &arg);

  *isVpnPassThrough = arg.is_pptp_vpn_passthrough;

  LOG_MSG_FUNCTION_EXIT ("Exit, return val 0x%x, PPTP VPN PassThrough %d",
                          result, *isVpnPassThrough, 0);
  return result;

} /* NatSession::()GetPPTPVpnPassThrough */

ds::ErrorType NatSession::AddStaticNatEntry
(
  const ds::Net::IPNatStaticEntryType*   pStaticNatEntry
)
{
  AddStaticNatEntryType arg;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Add static NAT entry, Obj 0x%p, handle 0x%x",
                          this, mIfaceHandle, 0);

  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  if (NULL == pStaticNatEntry)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /* Convert to PS_IFACE_IOCTL argument type */
  memset (&arg, 0, sizeof(AddStaticNatEntryType));
  DS2PSIPStaticNatEntry(pStaticNatEntry, &arg);

  LOG_MSG_INFO2_6 ("IP addr 0x%x, Priv port 0x%x, Global port 0x%x, Proto %d",
                    arg.private_ip_addr.ps_s_addr,
                    arg.private_port,
                    arg.global_port,
                    arg.protocol,
                    0,
                    0);

  return IfaceIoctlNonNullArg (mIfaceHandle,
                               IFACE_IOCTL_ADD_STATIC_NAT_ENTRY,
                               (void *)&arg);

} /* NatSession::AddStaticNatEntry() */

ds::ErrorType NatSession::DeleteStaticNatEntry
(
  const ds::Net::IPNatStaticEntryType* pStaticNatEntry
)
{
  DeleteStaticNatEntryType arg;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Del static NAT entry, Obj 0x%p, handle 0x%x",
                          this, mIfaceHandle, 0);

  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  if (NULL == pStaticNatEntry)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  /* Convert to PS_IFACE_IOCTL argument type */
  memset (&arg, 0, sizeof(DeleteStaticNatEntryType));
  DS2PSIPStaticNatEntry(pStaticNatEntry, &arg);

  LOG_MSG_INFO2_6 ("IP addr 0x%x, Priv port 0x%x, Global port 0x%x, Proto %d",
                    arg.private_ip_addr.ps_s_addr,
                    arg.private_port,
                    arg.global_port,
                    arg.protocol,
                    0,
                    0);

  return IfaceIoctlNonNullArg (mIfaceHandle,
                               IFACE_IOCTL_DELETE_STATIC_NAT_ENTRY,
                               (void *)&arg);

} /* NatSession::DeleteStaticNatEntry() */

ds::ErrorType NatSession::GetStaticNatEntry
(
  ds::Net::IPNatStaticEntryType*  pStaticNatEntries,
  int                             entriesLen,
  int*                            entriesLenReq
)
{
  GetStaticNatEntryType           arg;
  int32                           result;
  int                             numEntries = 0;
  int                             index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Get Static Nat entries, Obj 0x%p, handle 0x%x"
                          "Number of entries to get %d",
                          this, mIfaceHandle, entriesLen);
  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  /*-------------------------------------------------------------------------
    Figure out the total number of static NAT entries available.
  -------------------------------------------------------------------------*/
  memset (&arg, 0, sizeof (GetStaticNatEntryType));
  result = IfaceIoctlNonNullArg (mIfaceHandle,
                                 IFACE_IOCTL_GET_STATIC_NAT_ENTRY,
                                 (void *) &arg);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err 0x%x getting Static NAT entries", result, 0, 0);
    goto bail;
  }

  LOG_MSG_INFO2 ("Num static NAT entries avail %d", arg.total_entries, 0, 0);

  result =
    ds::Utils::Conversion::ProcessLenReq
    (
      entriesLen,
      entriesLenReq,
      arg.total_entries,
      &numEntries
    );

  if (AEE_SUCCESS != result || 0 == numEntries)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Allocate memory for those static NAT entries and call the IOCTL again.
  -------------------------------------------------------------------------*/
  memset (&arg, 0, sizeof(GetStaticNatEntryType));
  arg.num_entries = numEntries;
  arg.entries_arr =
    (ps_iface_ioctl_static_nat_entry_type *)
    ps_system_heap_mem_alloc
    (
      numEntries * sizeof (ps_iface_ioctl_static_nat_entry_type)
    );

  if (NULL == arg.entries_arr)
  {
    LOG_MSG_ERROR ("Cannot alloc memory", 0, 0, 0);
    result = AEE_ENOMEMORY;
    goto bail;
  }

  result = IfaceIoctlNonNullArg (mIfaceHandle,
                                 IFACE_IOCTL_GET_STATIC_NAT_ENTRY,
                                 (void *) &arg);
  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err 0x%x retrieving static NAT entries", result, 0, 0);
    goto bail;
  }

  for (index = 0; index < numEntries; index++)
  {
    PS2DSIPStaticNatEntry (&arg.entries_arr[index], &pStaticNatEntries[index]);
  }

  /* Fail-through to bail */

bail:
  if (NULL != arg.entries_arr)
  {
    PS_SYSTEM_HEAP_MEM_FREE(arg.entries_arr);
  }

  LOG_MSG_FUNCTION_EXIT ("Exit, result 0x%x", result, 0, 0);
  return result;

} /* NatSession::GetStaticNatEntry() */

ds::ErrorType NatSession::AddDMZ
(
  const ds::Net::DMZEntryType*   pDMZEntry 
)
{
  DMZType arg;
  int32   result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Enable and Add DMZ entry, Obj 0x%p, handle 0x%x", 
                          this, mIfaceHandle, 0);

  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  if (NULL == pDMZEntry)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  memset (&arg, 0, sizeof(DMZType));

  /* Convert to PS_IFACE_IOCTL argument type */
  (void)DS2PSDMZEntry(pDMZEntry, &arg);

  result =  IfaceIoctlNonNullArg (mIfaceHandle,
                                  IFACE_IOCTL_ADD_DMZ,
                                  (void *)&arg);

  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err 0x%x adding DMZ entry", result, 0, 0);
    goto bail;
  }

bail:

  LOG_MSG_FUNCTION_EXIT ("Exit, result 0x%x", result, 0, 0);
  return result;


} /* NatSession::AddDMZEntry() */

ds::ErrorType NatSession::GetDMZ
(
  ds::Net::DMZEntryType*   pDMZEntry 
)
{
  DMZType arg;
  int32   result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Get DMZ entry, Obj 0x%p, handle 0x%x", 
                          this, mIfaceHandle, 0);

  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  if (NULL == pDMZEntry)
  {
    LOG_MSG_ERROR ("NULL args", 0, 0, 0);
    return QDS_EFAULT;
  }

  memset (&arg, 0, sizeof(DMZType));

  result = IfaceIoctlNonNullArg (mIfaceHandle,
                                 IFACE_IOCTL_GET_DMZ,
                                 (void *)&arg);

  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err 0x%x retrieving DMZ entry", result, 0, 0);
    goto bail;
  }

  (void)PS2DSDMZEntry (&arg, pDMZEntry);

  /* Fail-through to bail */

bail:

  LOG_MSG_FUNCTION_EXIT ("Exit, result 0x%x", result, 0, 0);
  return result;

} /* NatSession::GetDMZEntry() */

ds::ErrorType NatSession::DeleteDMZ
(
  void 
)
{
  int32 result;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY ("Delete DMZ , Obj 0x%p, handle 0x%x", 
                          this, mIfaceHandle, 0);

  if (0 == mIfaceHandle)
  {
    LOG_MSG_ERROR ("Route lookup not performed", 0, 0, 0);
    return QDS_EINVAL;
  }

  result = IfaceIoctl (mIfaceHandle,
                       IFACE_IOCTL_DELETE_DMZ,
                       NULL);

  if (AEE_SUCCESS != result)
  {
    LOG_MSG_ERROR ("Err 0x%x deleting DMZ entry", result, 0, 0);
    goto bail;
  }

bail:

  LOG_MSG_FUNCTION_EXIT ("Exit, result 0x%x", result, 0, 0);
  return result;

} /* NatSession::DeleteDMZ() */


