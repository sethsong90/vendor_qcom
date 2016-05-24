#ifndef __DSS_IDSNETWORKPRIVSCOPE_H__
#define __DSS_IDSNETWORKPRIVSCOPE_H__

/*===================================================

FILE:  DSS_IDSNetworkPrivScope.h

SERVICES:
   A utility class to facilitate IDSNetworkPriv fetching
   and releasing.

=====================================================

Copyright (c) 2008-2010 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_IDSNetworkPrivScope.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "AEEStdErr.h"
#include "DSS_NetApp.h"
#include "DSS_Common.h"

#include "ds_Net_INetworkPriv.h"

// This class provides an abstraction over IDSNetworkPriv pointer.
// Define and initialize (Init) an instance of this class at the top of the scope.
// If init is successful, the IDSNetworkPriv pointer is valid and can be used
// until the end of the scope where the IDSNetworkPriv object's reference counting
// decremented.
class DSSIDSNetworkPrivScope {
public:
   DSSIDSNetworkPrivScope();
   ~DSSIDSNetworkPrivScope();
   AEEResult Init(DSSNetApp* pDSSNetApp);
   ds::Net::INetworkPriv* Fetch();
   
private:
   ds::Net::INetworkPriv* mpIDSNetworkPriv;   
};

inline DSSIDSNetworkPrivScope::DSSIDSNetworkPrivScope() : mpIDSNetworkPriv(NULL)
{
}

inline AEEResult DSSIDSNetworkPrivScope::Init(DSSNetApp* pDSSNetApp)
{   
   AEEResult res = pDSSNetApp->GetIDSNetworkPrivObject(&mpIDSNetworkPriv);
   if (AEE_SUCCESS != res) { 
      LOG_MSG_ERROR("Failed to fetch IDSNetworkPriv object: %d", res, 0, 0);
      return res;
   }

   return AEE_SUCCESS;
}

inline ds::Net::INetworkPriv* DSSIDSNetworkPrivScope::Fetch()
{   
   return mpIDSNetworkPriv;
}

/*lint -e{1551} */
inline DSSIDSNetworkPrivScope::~DSSIDSNetworkPrivScope()
{
   DSSCommon::ReleaseIf((IQI**)&mpIDSNetworkPriv);
}
/*lint –restore */

#endif // __DSS_IDSNETWORKPRIVSCOPE_H__
