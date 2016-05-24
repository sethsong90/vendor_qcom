#ifndef DS_SOCK_CSOCKETFACTORY_H
#define DS_SOCK_CSOCKETFACTORY_H

/*
  ===============================================================================
  FILE: ds_Sock_CSocketFactory.idl
  
  DESCRIPTION: This file defines the class id that implements the 
  ds_Sock_ISocketFactory interface
  
  Copyright (c) 2010 Qualcomm Technologies, Inc. 
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  
  ===============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#define ds_Sock_AEECLSID_CSocketFactory 0x10751e7
#else /* C++ */
#include "AEEStdDef.h"
namespace ds
{
   namespace Sock
   {
      const ::AEECLSID AEECLSID_CSocketFactory = 0x10751e7;
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
/*
  ================================================================================
  Class DOCUMENTATION
  ================================================================================
  
  AEECLSID_DSSockSocketFactory
  
  Description:
  Allows clients to create an instance of ds_Sock_ISocketFactory interface,
  which can be used to create ds_Sock_ISocket objects.
  
  Default Interfaces:
  ds_Sock_ISocketFactory
  
  Other Supported Interfaces (directly):
  None
  
  Other Supported Interfaces (indirectly):
  ds_Sock_ISocket - Can be obtained via ISocketFactory_CreateSocket* methods.
  Other Interfaces supported from ds_Sock_ISocket (via IQI):
  ISocketExt.
  
  Privileges:
  None
  
  See Also:
  None
  ================================================================================*/

#endif //DS_SOCK_CSOCKETFACTORY_H
