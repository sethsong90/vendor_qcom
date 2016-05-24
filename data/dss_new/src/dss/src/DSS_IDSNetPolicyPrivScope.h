#ifndef __DSS_IDSNETPOLICYPRIVSCOPE_H__
#define __DSS_IDSNETPOLICYPRIVSCOPE_H__

/*===================================================

FILE:  DSS_IDSNetPolicyPrivScope.h

SERVICES:
   A utility class to facilitate IDSNetPolicyPriv fetching
   and releasing.

=====================================================

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_IDSNetPolicyPrivScope.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/


#include "DSS_Common.h"
#include "DSS_Globals.h"
#include "ds_Errors_Def.h"

#include "ds_Net_IPolicy.h"
#include "ds_Net_IPolicyPriv.h"
#include "ds_Net_INetworkFactoryPriv.h"

using namespace ds::Error;

// This class provides an abstraction over IDSNetPolicyPriv pointer.
// Define and initialize (Init) an instance of this class at the top of the scope.
// If init is successful, the IDSNetPolicyPriv pointer is valid and can be used
// until the end of the scope where the IDSNetPolicyPriv object's reference counting
// decremented.
class DSSIDSNetPolicyPrivScope {
public:
   DSSIDSNetPolicyPrivScope();
   ~DSSIDSNetPolicyPrivScope();
   AEEResult Init();
   AEEResult Init(ds::Net::IPolicy* pIDSNetPolicy);
   ds::Net::IPolicyPriv* Fetch();

private:
   ds::Net::IPolicyPriv* mpIDSNetPolicyPriv;   
};

inline DSSIDSNetPolicyPrivScope::DSSIDSNetPolicyPrivScope() : mpIDSNetPolicyPriv(NULL)
{
}

inline AEEResult DSSIDSNetPolicyPrivScope::Init()
{
   AEEResult res;
   // Get the NetworkFactory from DSSGlobals
   ds::Net::INetworkFactoryPriv *pNetworkFactoryPriv;
   res = DSSGlobals::Instance()->GetNetworkFactoryPriv(&pNetworkFactoryPriv);

   if(AEE_SUCCESS != res){
      return res;
   }

   // Create new Policy object.
   res = pNetworkFactoryPriv->CreatePolicyPriv(&mpIDSNetPolicyPriv);

   DSSCommon::ReleaseIf((IQI**)&pNetworkFactoryPriv);

   return res;
}

inline AEEResult DSSIDSNetPolicyPrivScope::Init(ds::Net::IPolicy* pIDSNetPolicy)
{
   if (NULL == pIDSNetPolicy) {
      return QDS_EFAULT;
   }

   AEEResult res = pIDSNetPolicy->QueryInterface(ds::Net::AEEIID_IPolicyPriv, (void**)&mpIDSNetPolicyPriv);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Failed to fetch IDSNetPolicyPriv object: %d", res, 0, 0);
      return res;
   }

   return AEE_SUCCESS;
}

inline ds::Net::IPolicyPriv* DSSIDSNetPolicyPrivScope::Fetch()
{
   return mpIDSNetPolicyPriv;
}

/*lint -e{1551} */
inline DSSIDSNetPolicyPrivScope::~DSSIDSNetPolicyPrivScope()
{
   DSSCommon::ReleaseIf((IQI**)&mpIDSNetPolicyPriv);
}
/*lint –restore */

#endif // __DSS_IDSNETPOLICYPRIVSCOPE_H__
