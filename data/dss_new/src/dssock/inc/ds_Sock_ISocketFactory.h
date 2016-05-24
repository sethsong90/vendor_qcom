#ifndef DS_SOCK_ISOCKETFACTORY_H
#define DS_SOCK_ISOCKETFACTORY_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Sock_Def.h"
#include "AEEISignal.h"
#include "AEEIPort1.h"
#include "ds_Sock_ISocket.h"
#include "ds_Net_IPolicy.h"
#include "ds_Net_DownReasons_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
#define ds_Sock_AEEIID_ISocketFactory 0x106d84a
#define INHERIT_ds_Sock_ISocketFactory(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*CreateSocket)(iname* _pif, ds_AddrFamilyType family, ds_Sock_SocketType sockType, ds_Sock_ProtocolType protocol, ds_Net_IPolicy* policy, boolean autoBringUp, ds_Sock_ISocket** newSocket); \
   AEEResult (*CreateSocketByNetwork)(iname* _pif, ds_AddrFamilyType family, ds_Sock_SocketType sockType, ds_Sock_ProtocolType protocol, ds_Net_INetwork* network, ds_Sock_ISocket** newSocket)
AEEINTERFACE_DEFINE(ds_Sock_ISocketFactory);

/** @memberof ds_Sock_ISocketFactory
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Sock_ISocketFactory_AddRef(ds_Sock_ISocketFactory* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketFactory)->AddRef(_pif);
}

/** @memberof ds_Sock_ISocketFactory
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Sock_ISocketFactory_Release(ds_Sock_ISocketFactory* _pif)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketFactory)->Release(_pif);
}

/** @memberof ds_Sock_ISocketFactory
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Sock_ISocketFactory_QueryInterface(ds_Sock_ISocketFactory* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketFactory)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Sock_ISocketFactory
  * 
  * This function creates an instance of ISocket. ISocket creation is
  * supported only via ISocketFactory.
  * @param _pif Pointer to interface
  * @param family Address Family of the socket (ds_AF_*). Only
  *               QDS_AF_INET and QDS_AF_INET6 are supported.    
  * @param sockType Socket Type (ds_Sock_SOCKTYPE_*).
  * @param protocol Protocol Type (ds_Sock_PROTOCOL_*).
  * @param policy Network policy. If NULL, default network policy shall
  *               be used for the relevant socket operations when those
  *               are applied on the socket.
  * @param autoBringUp TRUE: The application wants to create a socket
  *                          that has an automatic behavior in respect
  *                          to Network interface relationship i.e. a
  *                          socket that takes care to automatically
  *                          bring up the Network interface when
  *                          applicable socket operations are applied
  *                          on it. The application should not make 
  *                          assumptions in regard to which Network
  *                          interface the socket is operating on,
  *                          beyond the requirement the application
  *                          posed by specification of the Network 
  *                          policy provided to this API.
  *                    FALSE: The Application is responsible to have
  *                           Network interface(s) brought up before 
  *                           applying applicable operations on the 
  *                           socket. The socket may operate on
  *                           different network interfaces.
  * @param newSocket Output The newly created ISocket instance.
  * @retval AEE_SUCCESS ISocket created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocketFactory_CreateSocket(ds_Sock_ISocketFactory* _pif, ds_AddrFamilyType family, ds_Sock_SocketType sockType, ds_Sock_ProtocolType protocol, ds_Net_IPolicy* policy, boolean autoBringUp, ds_Sock_ISocket** newSocket)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketFactory)->CreateSocket(_pif, family, sockType, protocol, policy, autoBringUp, newSocket);
}

/** @memberof ds_Sock_ISocketFactory
  * 
  * This function creates an instance of ISocket. ISocket creation is
  * supported only via ISocketFactory. In this API the Application
  * specifies an ACTIVE mode Network object as a reference for the
  * socket creation. It is guaranteed that the produced socket shall be
  * associated to the same underlying network interface as the one used
  * for the INetwork object. This way it is guaranteed that any
  * configurations applied on the INetwork object shall be applicable
  * for the created socket as long as those configurations do not
  * change.
  * If the underlying network interface associated with the provided
  * network object goes DOWN (either due to release of the network
  * object by the application or because of another reason) socket
  * operations applied on the socket shall fail.
  * @param _pif Pointer to interface
  * @param family Address Family of the socket (ds_AF_*). Only
  *               QDS_AF_INET and QDS_AF_INET6 are supported. Must be
  *               compatible to the Network object.   
  * @param sockType Socket Type (ds_Sock_SOCKTYPE_*).
  * @param protocol Protocol Type (ds_Sock__PROTOCOL_*).
  * @param network Network object to be used as a reference for the
  *                creation of the socket.
  * @param newSocket Output The newly created ISocket instance.
  * @retval AEE_SUCCESS ISocket created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Sock_ISocketFactory_CreateSocketByNetwork(ds_Sock_ISocketFactory* _pif, ds_AddrFamilyType family, ds_Sock_SocketType sockType, ds_Sock_ProtocolType protocol, ds_Net_INetwork* network, ds_Sock_ISocket** newSocket)
{
   return AEEGETPVTBL(_pif, ds_Sock_ISocketFactory)->CreateSocketByNetwork(_pif, family, sockType, protocol, network, newSocket);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Sock_Def.h"
#include "AEEISignal.h"
#include "AEEIPort1.h"
#include "ds_Sock_ISocket.h"
#include "ds_Net_IPolicy.h"
#include "ds_Net_DownReasons_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
namespace ds
{
   namespace Sock
   {
      const ::AEEIID AEEIID_ISocketFactory = 0x106d84a;
      struct ISocketFactory : public ::IQI
      {
         
         /**
           * This function creates an instance of ISocket. ISocket creation is
           * supported only via ISocketFactory.
           * @param family Address Family of the socket (ds::AF_*). Only
           *               QDS_AF_INET and QDS_AF_INET6 are supported.    
           * @param sockType Socket Type (ds::Sock::SOCKTYPE_*).
           * @param protocol Protocol Type (ds::Sock::PROTOCOL_*).
           * @param policy Network policy. If NULL, default network policy shall
           *               be used for the relevant socket operations when those
           *               are applied on the socket.
           * @param autoBringUp TRUE: The application wants to create a socket
           *                          that has an automatic behavior in respect
           *                          to Network interface relationship i.e. a
           *                          socket that takes care to automatically
           *                          bring up the Network interface when
           *                          applicable socket operations are applied
           *                          on it. The application should not make 
           *                          assumptions in regard to which Network
           *                          interface the socket is operating on,
           *                          beyond the requirement the application
           *                          posed by specification of the Network 
           *                          policy provided to this API.
           *                    FALSE: The Application is responsible to have
           *                           Network interface(s) brought up before 
           *                           applying applicable operations on the 
           *                           socket. The socket may operate on
           *                           different network interfaces.
           * @param newSocket Output The newly created ISocket instance.
           * @retval AEE_SUCCESS ISocket created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateSocket(::ds::AddrFamilyType family, ::ds::Sock::SocketType sockType, ::ds::Sock::ProtocolType protocol, ::ds::Net::IPolicy* policy, boolean autoBringUp, ::ds::Sock::ISocket** newSocket) = 0;
         
         /**
           * This function creates an instance of ISocket. ISocket creation is
           * supported only via ISocketFactory. In this API the Application
           * specifies an ACTIVE mode Network object as a reference for the
           * socket creation. It is guaranteed that the produced socket shall be
           * associated to the same underlying network interface as the one used
           * for the INetwork object. This way it is guaranteed that any
           * configurations applied on the INetwork object shall be applicable
           * for the created socket as long as those configurations do not
           * change.
           * If the underlying network interface associated with the provided
           * network object goes DOWN (either due to release of the network
           * object by the application or because of another reason) socket
           * operations applied on the socket shall fail.
           * @param family Address Family of the socket (ds::AF_*). Only
           *               QDS_AF_INET and QDS_AF_INET6 are supported. Must be
           *               compatible to the Network object.   
           * @param sockType Socket Type (ds::Sock::SOCKTYPE_*).
           * @param protocol Protocol Type (ds::Sock::_PROTOCOL_*).
           * @param network Network object to be used as a reference for the
           *                creation of the socket.
           * @param newSocket Output The newly created ISocket instance.
           * @retval AEE_SUCCESS ISocket created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateSocketByNetwork(::ds::AddrFamilyType family, ::ds::Sock::SocketType sockType, ::ds::Sock::ProtocolType protocol, ::ds::Net::INetwork* network, ::ds::Sock::ISocket** newSocket) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_SOCK_ISOCKETFACTORY_H
