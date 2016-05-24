#ifndef DS_NET_INETWORKFACTORY_H
#define DS_NET_INETWORKFACTORY_H

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
#include "ds_Net_DownReasons_Def.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
#include "ds_Net_IPolicy.h"
#include "ds_Net_ITechUMTS.h"

/**
  * INetwork mode that allows Factory API to know which mode to use in INetwork creation
  */
typedef int ds_Net_NetworkModeType;

/** @memberof ds_Net_NetworkMode
  * 
  * Active INetwork. This mode should be used unless special needs are required.
  * Data Network is brought up upon instantiation of this mode of INetwork.
  */
#define ds_Net_NetworkMode_QDS_ACTIVE 0x10762b4

/** @memberof ds_Net_NetworkMode
  * 
  * Monitored mode INetwork. This mode is intended for listening to network 
  * events without bringing up the Data Network. 
  * Some API of INetwork interface are unsupported in MONITORED MODE when
  * the underlying network interface is down.
  */
#define ds_Net_NetworkMode_QDS_MONITORED 0x10762b5
#define ds_Net_AEEIID_INetworkFactory 0x106d851
#define INHERIT_ds_Net_INetworkFactory(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*CreateNetwork)(iname* _pif, ds_Net_NetworkModeType networkMode, ds_Net_IPolicy* policy, ds_Net_INetwork** newNetwork); \
   AEEResult (*CreatePolicy)(iname* _pif, ds_Net_IPolicy** newPolicy); \
   AEEResult (*CreateTechUMTS)(iname* _pif, ds_Net_ITechUMTS** newTechUMTS)
AEEINTERFACE_DEFINE(ds_Net_INetworkFactory);

/** @memberof ds_Net_INetworkFactory
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkFactory_AddRef(ds_Net_INetworkFactory* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactory)->AddRef(_pif);
}

/** @memberof ds_Net_INetworkFactory
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkFactory_Release(ds_Net_INetworkFactory* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactory)->Release(_pif);
}

/** @memberof ds_Net_INetworkFactory
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INetworkFactory_QueryInterface(ds_Net_INetworkFactory* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactory)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INetworkFactory
  *     
  * This function creates an instance of INetwork. INetwork creation is
  * supported only via INetworkFactory. In this API the Application
  * specifies the network policy to be used for the creation of the
  * network object.
  * 
  * @param _pif Pointer to interface
  * @param networkMode Specification of which INetwork type to create.
  * @param policy Network policy. If NULL default policy shall be used.
  * @param newNetwork Output The newly created INetwork instance.
  * @retval AEE_SUCCESS INetwork created successfully. In the case of
  *                     ACTIVE Network Mode, the application should
  *                     register to ds_Net_NetworkEvent_STATE_CHANGED
  *                     events on the new Network object and wait for
  *                     the Network state to become OPEN.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkFactory_CreateNetwork(ds_Net_INetworkFactory* _pif, ds_Net_NetworkModeType networkMode, ds_Net_IPolicy* policy, ds_Net_INetwork** newNetwork)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactory)->CreateNetwork(_pif, networkMode, policy, newNetwork);
}

/** @memberof ds_Net_INetworkFactory
  *     
  * This function creates an instance of Policy. Policy creation is supported only
  * via INetworkFactory. 
  * @param _pif Pointer to interface
  * @param newPolicy Output The newly created Policy instance.
  * @retval AEE_SUCCESS Policy created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkFactory_CreatePolicy(ds_Net_INetworkFactory* _pif, ds_Net_IPolicy** newPolicy)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactory)->CreatePolicy(_pif, newPolicy);
}

/** @memberof ds_Net_INetworkFactory
  *     
  * This function creates an instance of ITechUMTS. ITechUMTS creation is supported only
  * via INetworkFactory. 
  * @param _pif Pointer to interface
  * @param newTechUMTS Output The newly created ITechUMTS instance.
  * @retval AEE_SUCCESS ITechUMTS created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkFactory_CreateTechUMTS(ds_Net_INetworkFactory* _pif, ds_Net_ITechUMTS** newTechUMTS)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkFactory)->CreateTechUMTS(_pif, newTechUMTS);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_DownReasons_Def.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IPhysLink.h"
#include "ds_Net_INetwork.h"
#include "ds_Net_IPolicy.h"
#include "ds_Net_ITechUMTS.h"
namespace ds
{
   namespace Net
   {
      
      /**
        * INetwork mode that allows Factory API to know which mode to use in INetwork creation
        */
      typedef int NetworkModeType;
      namespace NetworkMode
      {
         
         /**
           * Active INetwork. This mode should be used unless special needs are required.
           * Data Network is brought up upon instantiation of this mode of INetwork.
           */
         const ::ds::Net::NetworkModeType QDS_ACTIVE = 0x10762b4;
         
         /**
           * Monitored mode INetwork. This mode is intended for listening to network 
           * events without bringing up the Data Network. 
           * Some API of INetwork interface are unsupported in MONITORED MODE when
           * the underlying network interface is down.
           */
         const ::ds::Net::NetworkModeType QDS_MONITORED = 0x10762b5;
      };
      const ::AEEIID AEEIID_INetworkFactory = 0x106d851;
      struct INetworkFactory : public ::IQI
      {
         
         /**    
           * This function creates an instance of INetwork. INetwork creation is
           * supported only via INetworkFactory. In this API the Application
           * specifies the network policy to be used for the creation of the
           * network object.
           * 
           * @param networkMode Specification of which INetwork type to create.
           * @param policy Network policy. If NULL default policy shall be used.
           * @param newNetwork Output The newly created INetwork instance.
           * @retval AEE_SUCCESS INetwork created successfully. In the case of
           *                     ACTIVE Network Mode, the application should
           *                     register to ds::Net::NetworkEvent::STATE_CHANGED
           *                     events on the new Network object and wait for
           *                     the Network state to become OPEN.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateNetwork(::ds::Net::NetworkModeType networkMode, ::ds::Net::IPolicy* policy, ::ds::Net::INetwork** newNetwork) = 0;
         
         /**    
           * This function creates an instance of Policy. Policy creation is supported only
           * via INetworkFactory. 
           * @param newPolicy Output The newly created Policy instance.
           * @retval AEE_SUCCESS Policy created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreatePolicy(::ds::Net::IPolicy** newPolicy) = 0;
         
         /**    
           * This function creates an instance of ITechUMTS. ITechUMTS creation is supported only
           * via INetworkFactory. 
           * @param newTechUMTS Output The newly created ITechUMTS instance.
           * @retval AEE_SUCCESS ITechUMTS created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateTechUMTS(::ds::Net::ITechUMTS** newTechUMTS) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INETWORKFACTORY_H
