#ifndef DS_NET_CNETWORKFACTORYSERVICE_H
#define DS_NET_CNETWORKFACTORYSERVICE_H

/*===========================================================================
@file ds_Net_CNetworkFactoryService.h

This file defines the CLSID of NetworkFactoryService.

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

#include "AEEStdDef.h"

#include "AEEInterface.h"

#if !defined(AEEINTERFACE_CPLUSPLUS)

#include "AEEStdDef.h"
#define ds_Net_AEECLSID_CNetworkFactoryService 0x109b3e5

#else /* C++ */

#include "AEEStdDef.h"
namespace ds {

   namespace Net {

      const ::AEECLSID AEECLSID_CNetworkFactoryService = 0x109b3e5;

   }
}
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */

#endif /* #ifndef DS_NET_CNETWORKFACTORYSERVICE_H */
