#ifndef DS_NET_CNETWORKFACTORYPRIVSERVICE_H
#define DS_NET_CNETWORKFACTORYPRIVSERVICE_H

/*===========================================================================
@file ds_Net_CNetworkFactoryPrivService.h

This file defines the CLSID of NetworkFactoryPrivService.

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

#include "AEEStdDef.h"

#include "AEEInterface.h"

#if !defined(AEEINTERFACE_CPLUSPLUS)

#include "AEEStdDef.h"
#define ds_Net_AEECLSID_CNetworkFactoryPrivService 0x109d01f

#else /* C++ */

#include "AEEStdDef.h"
namespace ds {

   namespace Net {

      const ::AEECLSID AEECLSID_CNetworkFactoryPrivService = 0x109d01f;

   }
}
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */

#endif /* #ifndef DS_NET_CNETWORKFACTORYPRIVSERVICE_H */
