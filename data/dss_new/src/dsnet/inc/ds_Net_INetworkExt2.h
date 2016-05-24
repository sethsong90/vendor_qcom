#ifndef DS_NET_INETWORKEXT2_H
#define DS_NET_INETWORKEXT2_H

/*============================================================================
  Copyright (c) 2008-2011 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_DownReasons_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_IIPFilterPriv.h"
#include "ds_Errors_Def.h"
#include "ds_Net_IFirewallRule.h"
#include "ds_Net_IFirewallManager.h"
#include "ds_Net_INatSession.h"
struct ds_Net_FMCTunnelParamsType {
   int streamId;
   ds_SockAddrStorageType tunnelEndPointAddr;
   unsigned short addrLen;
   boolean IsNatPresent;
   AEEINTERFACE_PADMEMBERS(__pad, 1)
};
typedef struct ds_Net_FMCTunnelParamsType ds_Net_FMCTunnelParamsType;
#define ds_Net_AEEIID_INetworkExt2 0x10b37b1
#define INHERIT_ds_Net_INetworkExt2(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*CreateNetFirewallManager)(iname* _pif, ds_Net_IFirewallManager** newFirewallManagerObj); \
   AEEResult (*CreateNetNatSession)(iname* _pif, ds_Net_INatSession** newNatSessionObj); \
   AEEResult (*SetFMCTunnelParams)(iname* _pif, const ds_Net_FMCTunnelParamsType* tunnelParams); \
   AEEResult (*ResetFMCTunnelParams)(iname* _pif)
AEEINTERFACE_DEFINE(ds_Net_INetworkExt2);

/** @memberof ds_Net_INetworkExt2
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkExt2_AddRef(ds_Net_INetworkExt2* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt2)->AddRef(_pif);
}

/** @memberof ds_Net_INetworkExt2
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkExt2_Release(ds_Net_INetworkExt2* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt2)->Release(_pif);
}

/** @memberof ds_Net_INetworkExt2
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INetworkExt2_QueryInterface(ds_Net_INetworkExt2* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt2)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INetworkExt2
  * 
  * This function creates an instance of IFirewallManager. IFirewallManager creation
  * is supported only via INetwork.    
  * @param _pif Pointer to interface
  * @param newFirewallManagerObj Output The newly created IFirewallManager instance.
  * @retval DS_SUCCESS IFirewallManager created successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkExt2_CreateNetFirewallManager(ds_Net_INetworkExt2* _pif, ds_Net_IFirewallManager** newFirewallManagerObj)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt2)->CreateNetFirewallManager(_pif, newFirewallManagerObj);
}

/** @memberof ds_Net_INetworkExt2
  * 
  * This function creates an instance of INatSession. INatSession creation
  * is supported only via INetwork.    
  * @param _pif Pointer to interface
  * @param newNatSessionObj Output The newly created INatSession instance.
  * @retval DS_SUCCESS INatSession created successfully.
  * @retval Other DS designated error codes might be returned.
  * @see DS_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkExt2_CreateNetNatSession(ds_Net_INetworkExt2* _pif, ds_Net_INatSession** newNatSessionObj)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt2)->CreateNetNatSession(_pif, newNatSessionObj);
}

/** @memberof ds_Net_INetworkExt2
  * 
  * This function sets the FMC tunnel parameters for this network.
  * @param _pif Pointer to interface
  * @param tunnelParams Structure representing FMC tunnel parameters
  * @see FMCTunnelParamsType.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkExt2_SetFMCTunnelParams(ds_Net_INetworkExt2* _pif, const ds_Net_FMCTunnelParamsType* tunnelParams)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt2)->SetFMCTunnelParams(_pif, tunnelParams);
}

/** @memberof ds_Net_INetworkExt2
  * 
  * This function resets the FMC tunnel parameters for this network.
  * @param _pif Pointer to interface
  * @param none 
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkExt2_ResetFMCTunnelParams(ds_Net_INetworkExt2* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkExt2)->ResetFMCTunnelParams(_pif);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_DownReasons_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_IIPFilterPriv.h"
#include "ds_Errors_Def.h"
#include "ds_Net_IFirewallRule.h"
#include "ds_Net_IFirewallManager.h"
#include "ds_Net_INatSession.h"
namespace ds
{
   namespace Net
   {
      struct FMCTunnelParamsType {
         int streamId;
         ::ds::SockAddrStorageType tunnelEndPointAddr;
         unsigned short addrLen;
         boolean IsNatPresent;
         AEEINTERFACE_PADMEMBERS(__pad, 1)
      };
      const ::AEEIID AEEIID_INetworkExt2 = 0x10b37b1;
      struct INetworkExt2 : public ::IQI
      {
         
         /**
           * This function creates an instance of IFirewallManager. IFirewallManager creation
           * is supported only via INetwork.    
           * @param newFirewallManagerObj Output The newly created IFirewallManager instance.
           * @retval DS::SUCCESS IFirewallManager created successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateNetFirewallManager(::ds::Net::IFirewallManager** newFirewallManagerObj) = 0;
         
         /**
           * This function creates an instance of INatSession. INatSession creation
           * is supported only via INetwork.    
           * @param newNatSessionObj Output The newly created INatSession instance.
           * @retval DS::SUCCESS INatSession created successfully.
           * @retval Other DS designated error codes might be returned.
           * @see DS_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateNetNatSession(::ds::Net::INatSession** newNatSessionObj) = 0;
         
         /**
           * This function sets the FMC tunnel parameters for this network.
           * @param tunnelParams Structure representing FMC tunnel parameters
           * @see FMCTunnelParamsType.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetFMCTunnelParams(const ::ds::Net::FMCTunnelParamsType* tunnelParams) = 0;
         
         /**
           * This function resets the FMC tunnel parameters for this network.
           * @param none 
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL ResetFMCTunnelParams() = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INETWORKEXT2_H
