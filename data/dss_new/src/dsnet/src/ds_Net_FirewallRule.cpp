
/*==========================================================================*/
/*!
  @file
  ds_Net_FirewallRule.cpp

  @brief
  This file provides implementation for the DS::Net::FirewallRule class.

  @see  ds_Net_Firewall.h

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_FirewallRule.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-20 dm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "AEEStdErr.h"
#include "ds_Errors_Def.h"
#include "ds_Utils_DebugMsg.h"
#include "ds_Net_FirewallRule.h"
#include "ds_Net_Platform.h"
#include "ds_Net_EventDefs.h"
#include "ds_Net_IPFilterSpec.h"
#include "ds_Net_Conversion.h"

using namespace ds::Net;
using namespace ds::Error;
using namespace ds::Net::Conversion;
using namespace NetPlatform;


FirewallRule::FirewallRule
(
  void
)
: mIfaceHandle (0),
  mFirewallHandle (0),
  refCnt(1)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Creating obj 0x%p", this, 0, 0);

} /* FirewallRule() */

FirewallRule::~FirewallRule
(
  void
)
throw()
{
  NetPlatform::DeleteFirewallRuleType    delInfo;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1 ("Deleting firewall rule obj 0x%x, firewall handle 0x%x",
    this, mFirewallHandle, 0);

  /*-----------------------------------------------------------------------
    Call the iface IOCTL to delete firewall rule.
  -----------------------------------------------------------------------*/
  memset (&delInfo, 0, sizeof (NetPlatform::DeleteFirewallRuleType));
  delInfo.handle = mFirewallHandle;

  (void) NetPlatform::IfaceIoctl
         (
           mIfaceHandle,
           NetPlatform::IFACE_IOCTL_DELETE_FIREWALL_RULE,
           static_cast <void *> (&delInfo)
         );

} /* FirewallRule::~FirewallRule() */

/*---------------------------------------------------------------------------
  Inherited functions from IFirewallRule.
---------------------------------------------------------------------------*/
ds::ErrorType FirewallRule::GetFirewallRule
(
  IIPFilterPriv** ppIFilterSpec
)
{
  int32                                     result;
  NetPlatform::GetFirewallRuleType          firewallRule;
  IPFilterSpec                             *pFilterSpec;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY  ("Get firewall rule, Obj 0x%p, handle 0x%x",
                            this, mFirewallHandle, 0);

  /*-------------------------------------------------------------------------
    Validation
  -------------------------------------------------------------------------*/
  if (NULL == ppIFilterSpec)
  {
    LOG_MSG_ERROR ("Invalid args", 0, 0, 0);
    return QDS_EFAULT;
  }

  *ppIFilterSpec = NULL;

  /*-------------------------------------------------------------------------
    Perform iface IOCTL to get firewall rule.
  -------------------------------------------------------------------------*/
  memset (&firewallRule, 0, sizeof(firewallRule));
  firewallRule.handle = mFirewallHandle;

  result = IfaceIoctl (mIfaceHandle,
                      IFACE_IOCTL_GET_FIREWALL_RULE,
                      (void *)&firewallRule);
  if (AEE_SUCCESS != result)
  {
    goto bail;
  }

  /*-------------------------------------------------------------------------
    Convert to out args
  -------------------------------------------------------------------------*/
  pFilterSpec = new IPFilterSpec();
  if (NULL == pFilterSpec)
  {
    result = AEE_ENOMEMORY;
    goto bail;
  }

  PS2DSIPFilterSpec(&firewallRule.fltr_spec, pFilterSpec);

  *ppIFilterSpec = static_cast <IIPFilterPriv*> (pFilterSpec);

  LOG_MSG_FUNCTION_EXIT ("Success", 0, 0, 0);
  return AEE_SUCCESS;

bail:
  LOG_MSG_ERROR ("Err 0x%x getting firewall rule for handle 0x%x",
                  result, mFirewallHandle, 0);
  return result;
}

boolean FirewallRule::Process
(
  void *pUserData
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  (void) pUserData;
  LOG_MSG_ERROR ("Process is unsupported", 0, 0, 0);
  ASSERT (0);
  return FALSE;

} /* FirewallRule::Process() */

void FirewallRule::SetIfaceHandle
(
  int32 ifaceHandle
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("Set Iface handle 0x%x", ifaceHandle, 0, 0);
  mIfaceHandle = ifaceHandle;

} /* FirewallRule::SetIfaceHandle() */


void FirewallRule::SetFirewallHandle
(
  uint32 firewallHandle
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2 ("Set firewall handle 0x%x", firewallHandle, 0, 0);
  mFirewallHandle = firewallHandle;

}

