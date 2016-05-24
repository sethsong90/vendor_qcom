#ifndef __DSS_IDSNETWORKEXTSCOPE_H__
#define __DSS_IDSNETWORKEXTSCOPE_H__

/*===================================================

FILE:  DSS_IDSNetworkScope.h

SERVICES:
   A utility class to facilitate IDSNetwork fetching
   and releasing.

=====================================================

Copyright (c) 2008-2010 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_IDSNetworkExtScope.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "AEEStdErr.h"
#include "DSS_NetApp.h"
#include "DSS_Common.h"

#include "ds_Net_INetwork.h"

// This class provides an abstraction over IDSNetwork pointer.
// Define and initialize (Init) an instance of this class at the top of the scope.
// If init is successful, the IDSNetwork pointer is valid and can be used
// until the end of the scope where the IDSNetwork object's reference counting
// decremented.
class DSSIDSNetworkExtScope {
public:
   DSSIDSNetworkExtScope();
   ~DSSIDSNetworkExtScope();
   AEEResult Init(DSSNetApp* pDSSNetApp);
   ds::Net::INetworkExt* Fetch();
   
private:
   ds::Net::INetworkExt* mpIDSNetworkExt;   
};

inline DSSIDSNetworkExtScope::DSSIDSNetworkExtScope() : mpIDSNetworkExt(NULL)
{
}

inline AEEResult DSSIDSNetworkExtScope::Init(DSSNetApp* pDSSNetApp)
{   
   AEEResult res = pDSSNetApp->GetIDSNetworkExtObject(&mpIDSNetworkExt);
   if (AEE_SUCCESS != res) { 
      LOG_MSG_ERROR("Failed to fetch IDSNetworkExt object: %d", res, 0, 0);
      return res;
   }

   return AEE_SUCCESS;
}

inline ds::Net::INetworkExt* DSSIDSNetworkExtScope::Fetch()
{   
   return mpIDSNetworkExt;
}

/*lint -e{1551} */
inline DSSIDSNetworkExtScope::~DSSIDSNetworkExtScope()
{
   DSSCommon::ReleaseIf((IQI**)&mpIDSNetworkExt);
}
/*lint –restore */

#endif // __DSS_IDSNETWORKEXTSCOPE_H__
