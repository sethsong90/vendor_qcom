#ifndef __DSS_IDSNETWORKSCOPE_H__
#define __DSS_IDSNETWORKSCOPE_H__

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

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_IDSNetworkScope.h#1 $
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
class DSSIDSNetworkScope {
public:
   DSSIDSNetworkScope();
   ~DSSIDSNetworkScope();
   AEEResult Init(DSSNetApp* pDSSNetApp);
   ds::Net::INetwork* Fetch();
   
private:
   ds::Net::INetwork* mpIDSNetwork;   
};

inline DSSIDSNetworkScope::DSSIDSNetworkScope() : mpIDSNetwork(NULL)
{
}

inline AEEResult DSSIDSNetworkScope::Init(DSSNetApp* pDSSNetApp)
{   
   AEEResult res = pDSSNetApp->GetIDSNetworkObject(&mpIDSNetwork);
   if (AEE_SUCCESS != res) { 
      LOG_MSG_ERROR("Failed to fetch IDSNetwork object: %d", res, 0, 0);
      return res;
   }

   return AEE_SUCCESS;
}

inline ds::Net::INetwork* DSSIDSNetworkScope::Fetch()
{   
   return mpIDSNetwork;
}

/*lint -e{1551} */
inline DSSIDSNetworkScope::~DSSIDSNetworkScope()
{
   DSSCommon::ReleaseIf((IQI**)&mpIDSNetwork);
}
/*lint –restore */

#endif // __DSS_IDSNETWORKSCOPE_H__
