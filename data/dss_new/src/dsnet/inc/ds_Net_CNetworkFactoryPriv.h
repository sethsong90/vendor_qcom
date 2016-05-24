#ifndef DS_NET_CNETWORKFACTORYPRIV_H
#define DS_NET_CNETWORKFACTORYPRIV_H

/*============================================================================
  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#define ds_Net_AEECLSID_CNetworkFactoryPriv 0x1073e56
#else /* C++ */
#include "AEEStdDef.h"
namespace ds
{
   namespace Net
   {
      const ::AEECLSID AEECLSID_CNetworkFactoryPriv = 0x1073e56;
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_CNETWORKFACTORYPRIV_H
