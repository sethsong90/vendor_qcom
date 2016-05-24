#ifndef DS_ADDR_DEF_H
#define DS_ADDR_DEF_H

/*============================================================================
  Copyright (c) 2009-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
typedef unsigned short ds_AddrFamilyType;
#define ds_AddrFamily_QDS_AF_UNSPEC 0
#define ds_AddrFamily_QDS_AF_INET 2
#define ds_AddrFamily_QDS_AF_INET6 3

/**
  * SockAddrStorageType is to be used for both IPv4 and IPv6 addresses.
  * Its content (including the family field) shall be in Network Byte Order.
  * Size of sockAddrStorage is derived from length of IPv6 Socket Address.
  * Usage:
  * - Allocate SockAddrStorageType instance
  * - Allocate a pointer to either AddrINType or AddrIN6Type (defined in separate, language specific headers)
  * - Assign the address of SockAddrStorageType instance to the AddrINType/AddrIN6Type pointer
  * - Fill in the address fields (in Network Byte order) using the AddrINType/AddrIN6Type pointer
  * - Provide the address of SockAddrStorageType instance to the desired Socket API 
  */
typedef unsigned char ds_SockAddrStorageType[28];
typedef unsigned char ds_INAddr6Type[16];
struct ds_IPAddrType {
   ds_AddrFamilyType family;
   ds_INAddr6Type addr;
};
typedef struct ds_IPAddrType ds_IPAddrType;
#else /* C++ */
#include "AEEStdDef.h"
namespace ds
{
   typedef unsigned short AddrFamilyType;
   namespace AddrFamily
   {
      const ::ds::AddrFamilyType QDS_AF_UNSPEC = 0;
      const ::ds::AddrFamilyType QDS_AF_INET = 2;
      const ::ds::AddrFamilyType QDS_AF_INET6 = 3;
   };
   
   /**
     * SockAddrStorageType is to be used for both IPv4 and IPv6 addresses.
     * Its content (including the family field) shall be in Network Byte Order.
     * Size of sockAddrStorage is derived from length of IPv6 Socket Address.
     * Usage:
     * - Allocate SockAddrStorageType instance
     * - Allocate a pointer to either AddrINType or AddrIN6Type (defined in separate, language specific headers)
     * - Assign the address of SockAddrStorageType instance to the AddrINType/AddrIN6Type pointer
     * - Fill in the address fields (in Network Byte order) using the AddrINType/AddrIN6Type pointer
     * - Provide the address of SockAddrStorageType instance to the desired Socket API 
     */
   typedef unsigned char SockAddrStorageType[28];
   typedef unsigned char INAddr6Type[16];
   struct IPAddrType {
      AddrFamilyType family;
      INAddr6Type addr;
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_ADDR_DEF_H
