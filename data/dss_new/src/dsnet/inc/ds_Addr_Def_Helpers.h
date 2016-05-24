/*============================================================================
                  Copyright (c) 2010 Qualcomm Technologies, Inc.
                             All Rights Reserved.
                    Qualcomm Technologies Confidential and Proprietary
============================================================================*/

//This file defines structures and helper routines/macros for use with Address
//Related definitions in QCM API IDLs.

#ifndef DS_ADDR_DEF_HELPERS_H
#define DS_ADDR_DEF_HELPERS_H

#include "AEEStdDef.h"
#include "ds_Addr_Def.h"

#if !defined(AEEINTERFACE_CPLUSPLUS)

typedef struct ds_SockAddrINType ds_SockAddrINType;
#include "AEEbeginpack.h"

/* Internet style IPv4 socket address in Network Byte order */
struct ds_SockAddrINType              
{
   ds_AddrFamilyType  family;         /* Address family - AF_INET */
   unsigned short     port;           /* transport layer port number */
   uint32             addr;           /* IPv4 address */
}
#include "AEEendpack.h"
;

/* Internet style IPv6 socket address in Network Byte order */
typedef struct ds_SockAddrIN6Type ds_SockAddrIN6Type;  
#include "AEEbeginpack.h"
struct ds_SockAddrIN6Type
{
   ds_AddrFamilyType   family;         /* Address family - AF_INET6 */
   unsigned short      port;           /* transport layer port number */
   uint32              flowInfo;       /* IPv6 flow information */
   ds_INAddr6Type      addr;           /* IPv6 address */
   uint32              scopeId;        /* Set of interface for a scope */
} 
#include "AEEendpack.h"
;

/* wildcard address */
#define ds_QDS_INADDR_ANY (0)

/* loopback address: 127.0.0.1 in host byte order */
#define ds_QDS_INADDR_LOOPBACK (0x7f000001)

/* wildcard IPv6 address */
static void __inline QDS_IN6ADDR_SET_ANY(ds_SockAddrIN6Type* psain6)
{
   int idx;

   psain6->family = ds_AddrFamily_QDS_AF_INET6;
   psain6->port = 0;
   psain6->flowInfo = 0;
   psain6->scopeId = 0;

   for (idx = 0; idx < 16; idx++)
   {
      psain6->addr[idx] = 0;
   }
}

/* loopback IPv6 address */
static void __inline QDS_IN6ADDR_SET_LOOPBACK(ds_SockAddrIN6Type* psain6)
{
   int idx;

   psain6->family = ds_AddrFamily_QDS_AF_INET6;
   psain6->port = 0;
   psain6->flowInfo = 0;
   psain6->scopeId = 0;

   for (idx = 0; idx < 15; idx++)
   {
      psain6->addr[idx] = 0;
   }

   psain6->addr[15] = 1;
}

static boolean __inline QDS_IN6_IS_ADDR_UNSPECIFIED(const ds_INAddr6Type* pina6)
{
   int i;

   for(i = 0 ; i < 16 ; i++){
      if(0 != (*pina6)[i]){
         return FALSE;
      }
   }

   return TRUE;
}

static boolean __inline QDS_IS_ADDR_UNSPECIFIED(const ds_SockAddrStorageType* psas)
{
   const ds_SockAddrINType* pSockAddr = (const ds_SockAddrINType*)psas;
   const ds_SockAddrIN6Type* pSockAddr6 = (const ds_SockAddrIN6Type*)psas;
   
   switch (pSockAddr->family)
   {
      case ds_AddrFamily_QDS_AF_UNSPEC:
         return TRUE;

      case ds_AddrFamily_QDS_AF_INET:
         return (pSockAddr->addr == ds_QDS_INADDR_ANY);

      case ds_AddrFamily_QDS_AF_INET6:
         return QDS_IN6_IS_ADDR_UNSPECIFIED((const ds_INAddr6Type*)&pSockAddr6->addr);

      default:
         return FALSE;
   }
}

static boolean __inline QDS_IN6_IS_ADDR_LOOPBACK(const ds_INAddr6Type* pina6)
{
   int i;

   for(i = 0 ; i < 15 ; i++){
      if(0 != (*pina6)[i]){
         return FALSE;
      }
   }

   return (1 == (*pina6)[15]);
}

static boolean __inline QDS_IS_ADDR_LOOPBACK(const ds_SockAddrStorageType* psas)
{
   const ds_SockAddrINType*   pSockAddr = (const ds_SockAddrINType*)psas;
   const ds_SockAddrIN6Type*  pSockAddr6 = (const ds_SockAddrIN6Type*)psas;
   
   switch (pSockAddr->family)
   {
      case ds_AddrFamily_QDS_AF_INET:
         return (pSockAddr->addr == ds_QDS_INADDR_LOOPBACK);

      case ds_AddrFamily_QDS_AF_INET6:
         return QDS_IN6_IS_ADDR_LOOPBACK((const ds_INAddr6Type*)&pSockAddr6->addr);

      default:
         return FALSE;
   }
}

static boolean __inline QDS_IN6_IS_ADDR_V4MAPPED(const ds_INAddr6Type* pina6)
{
   int i;

   for(i = 0 ; i < 10 ; i++){
      if(0 != (*pina6)[i]){
         return FALSE;
      }
   }

   return ((0xff == (*pina6)[10]) && (0xff == (*pina6)[11]));
}

static boolean __inline QDS_IS_ADDR_V4MAPPED(const ds_SockAddrStorageType* psas)
{
   const ds_SockAddrINType* pSockAddr = (const ds_SockAddrINType*)psas;
   const ds_SockAddrIN6Type* pSockAddr6 = (const ds_SockAddrIN6Type*)psas;
   
   switch (pSockAddr->family)
   {
      case ds_AddrFamily_QDS_AF_INET:
         return FALSE;

      case ds_AddrFamily_QDS_AF_INET6:
         return QDS_IN6_IS_ADDR_V4MAPPED((const ds_INAddr6Type*)&pSockAddr6->addr);

      default:
         return FALSE;
   }
}

#else /* C++ */

namespace ds
{
   /* Internet style IPv4 socket address in Network Byte order */
   typedef struct SockAddrINType SockAddrINType;            
   #include "AEEbeginpack.h"
   struct SockAddrINType
   {
      AddrFamilyType   family;         /* Address family - AF_INET */
      unsigned short   port;           /* transport layer port number */
      uint32           addr;           /* IPv4 address */
   }
   #include "AEEendpack.h"
   ;

   /* Internet style IPv6 socket address in Network Byte order */
   typedef struct SockAddrIN6Type SockAddrIN6Type;
   #include "AEEbeginpack.h"
   struct SockAddrIN6Type
   {
      AddrFamilyType   family;         /* Address family - AF_INET6 */
      unsigned short   port;           /* transport layer port number */
      uint32           flowInfo;       /* IPv6 flow information */
      INAddr6Type      addr;           /* IPv6 address */
      uint32           scopeId;        /* Set of interface for a scope */
   }
   #include "AEEendpack.h"
   ;

   /* wildcard address */
   const unsigned int QDS_INADDR_ANY = 0;

   /* loopback address: 127.0.0.1 in host byte order */
   const unsigned int QDS_INADDR_LOOPBACK = 0x7f000001;

   /* wildcard IPv6 address */
   static void __inline QDS_IN6ADDR_SET_ANY(SockAddrIN6Type* psain6)
   {
      int idx = 0;

      psain6->family = AddrFamily::QDS_AF_INET6;
      psain6->port = 0;
      psain6->flowInfo = 0;
      psain6->scopeId = 0;

      for (idx = 0; idx < 16; idx++)
      {
         psain6->addr[idx] = 0;
      }
   }

   /* loopback IPv6 address */
   static void __inline QDS_IN6ADDR_SET_LOOPBACK(SockAddrIN6Type* psain6)
   {
      int idx;
      
      psain6->family = AddrFamily::QDS_AF_INET6;
      psain6->port = 0;
      psain6->flowInfo = 0;
      psain6->scopeId = 0;

      for (idx = 0; idx < 15; idx++)
      {
         psain6->addr[idx] = 0;
      }

      psain6->addr[15] = 1;
   }


   static boolean __inline QDS_IN6_IS_ADDR_UNSPECIFIED(const INAddr6Type* pina6)
   {
      int i;

      for(i = 0 ; i < 16 ; i++){
         if(0 != (*pina6)[i]){
            return FALSE;
         }
      }

      return TRUE;
   }

   static boolean __inline QDS_IS_ADDR_UNSPECIFIED(const SockAddrStorageType* psas)
   {
      const SockAddrINType* pSockAddr = (const SockAddrINType*)psas;
      const SockAddrIN6Type* pSockAddr6 = (const SockAddrIN6Type*)psas;

      switch (pSockAddr->family)
      {
         case AddrFamily::QDS_AF_UNSPEC:
            return TRUE;

         case AddrFamily::QDS_AF_INET:
            return (pSockAddr->addr == QDS_INADDR_ANY);

         case AddrFamily::QDS_AF_INET6:
            return QDS_IN6_IS_ADDR_UNSPECIFIED((const INAddr6Type*)&pSockAddr6->addr);

         default:
            return FALSE;
      }
   }

   static boolean __inline QDS_IN6_IS_ADDR_LOOPBACK(const INAddr6Type* pina6)
   {
      int i;

      for(i = 0 ; i < 15 ; i++){
         if(0 != (*pina6)[i]){
            return FALSE;
         }
      }

      return (1 == (*pina6)[15]);
   }

   static boolean __inline QDS_IS_ADDR_LOOPBACK(const SockAddrStorageType* psas)
   {
      const SockAddrINType* pSockAddr = (const SockAddrINType*)psas;
      const SockAddrIN6Type* pSockAddr6 = (const SockAddrIN6Type*)psas;

      switch (pSockAddr->family)
      {
         case AddrFamily::QDS_AF_INET:
            return (pSockAddr->addr == QDS_INADDR_LOOPBACK);

         case AddrFamily::QDS_AF_INET6:
            return QDS_IN6_IS_ADDR_LOOPBACK((const INAddr6Type*)&pSockAddr6->addr);

         default:
            return FALSE;
      }
   }

   static boolean __inline QDS_IN6_IS_ADDR_V4MAPPED(const INAddr6Type* pina6)
   {
      int i;

      for(i = 0 ; i < 10 ; i++){
         if(0 != (*pina6)[i]){
            return FALSE;
         }
      }

      return ((0xff == (*pina6)[10]) && (0xff == (*pina6)[11]));
   }

   static boolean __inline QDS_IS_ADDR_V4MAPPED(const SockAddrStorageType* psas)
   {
      const SockAddrINType* pSockAddr = (const SockAddrINType*)psas;
      const SockAddrIN6Type* pSockAddr6 = (const SockAddrIN6Type*)psas;

      switch (pSockAddr->family)
      {
         case AddrFamily::QDS_AF_INET:
            return FALSE;

         case AddrFamily::QDS_AF_INET6:
            return QDS_IN6_IS_ADDR_V4MAPPED((const INAddr6Type*)&pSockAddr6->addr);

         default:
            return FALSE;
      }
   }

} /* namespace ds */
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif /* #ifndef DS_ADDR_DEF_HELPERS_H */

