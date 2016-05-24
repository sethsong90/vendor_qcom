#ifndef DS_NET_IMCASTMANAGER_H
#define DS_NET_IMCASTMANAGER_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_MCast_Def.h"
#include "ds_Net_IMCastSession.h"
#define ds_Net_AEEIID_IMCastManager 0x106cf94

/** @interface ds_Net_IMCastManager
  * 
  * ds Net MCast Manager interface.
  * This interface supports basic MultiCast management operations for
  * various MCast technologies inclusing MFLO, DVB-H and BCMCS 1.0 
  */
#define INHERIT_ds_Net_IMCastManager(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*Join)(iname* _pif, const ds_SockAddrStorageType addr, const char* mcastSpec, ds_Net_IMCastSession** session, char* errSpec, int errSpecLen, int* errSpecLenReq); \
   AEEResult (*GetTechObject)(iname* _pif, AEEIID techObj_iid, void** techObj)
AEEINTERFACE_DEFINE(ds_Net_IMCastManager);

/** @memberof ds_Net_IMCastManager
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastManager_AddRef(ds_Net_IMCastManager* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManager)->AddRef(_pif);
}

/** @memberof ds_Net_IMCastManager
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastManager_Release(ds_Net_IMCastManager* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManager)->Release(_pif);
}

/** @memberof ds_Net_IMCastManager
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IMCastManager_QueryInterface(ds_Net_IMCastManager* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManager)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IMCastManager
  *     
  * This function issues a request for joining a multicast group and
  * registering to it.
  * @param _pif Pointer to interface
  * @param addr Multicast group address.
  * @param mcastSpec A JSON string specifying extra Multicast join
  *                  information. The extra Multicast join information
  *                  is supported only for certain MCast technologies.
  *                  When using this API to join other MCast
  *                  technologies, mcastSpec can be empty.          
  *                  Currently extra Join information is not applicable
  *                  for supported technologies hence no JSON schema
  *                  is currently specified for this information.
  * @param session Output The IMCastSession opened for handling the
  *                       request.
  * @param errSpec Output specification of errors in mcastSpec.
  * @param errSpecLen Length of sequence
  * @param errSpecLenReq Required length of sequence
  * @see IMCastSession
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManager_Join(ds_Net_IMCastManager* _pif, const ds_SockAddrStorageType addr, const char* mcastSpec, ds_Net_IMCastSession** session, char* errSpec, int errSpecLen, int* errSpecLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManager)->Join(_pif, addr, mcastSpec, session, errSpec, errSpecLen, errSpecLenReq);
}

/** @memberof ds_Net_IMCastManager
  * 
  * This function allows object extensibility.
  * For supported interfaces, objects implementing those interfaces may be 
  * fetched via this function. The supported interfaces are documented in
  * the DS_NET_NetworkFactory.bid file. GetTechObject-supported interface
  * does not imply any similar contract in regard to QueryInterface for the
  * respective interface.
  * Unlike IQI, the availability of a specific interface depends on some 
  * factors, e.g. current network type. Moreover, there is no guaranty 
  * for transitivity or symmetry. 
  * Note: 'interface' parameter will map to iid and techObj.
  * @param _pif Pointer to interface
  * @param techObj_iid Interface ID of requested interface
  * @param iid The interface that should be retrieved.
  * @param techObj On success, will contain the requested interface instance.
  * @retval ds_SUCCESS Interface retrieved successfully.
  * @retval AEE_ECLASSNOTSUPPORT Specified interface unsupported.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManager_GetTechObject(ds_Net_IMCastManager* _pif, AEEIID techObj_iid, void** techObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManager)->GetTechObject(_pif, techObj_iid, techObj);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_MCast_Def.h"
#include "ds_Net_IMCastSession.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IMCastManager = 0x106cf94;
      
      /** @interface IMCastManager
        * 
        * ds Net MCast Manager interface.
        * This interface supports basic MultiCast management operations for
        * various MCast technologies inclusing MFLO, DVB-H and BCMCS 1.0 
        */
      struct IMCastManager : public ::IQI
      {
         
         /**    
           * This function issues a request for joining a multicast group and
           * registering to it.
           * @param addr Multicast group address.
           * @param mcastSpec A JSON string specifying extra Multicast join
           *                  information. The extra Multicast join information
           *                  is supported only for certain MCast technologies.
           *                  When using this API to join other MCast
           *                  technologies, mcastSpec can be empty.          
           *                  Currently extra Join information is not applicable
           *                  for supported technologies hence no JSON schema
           *                  is currently specified for this information.
           * @param session Output The IMCastSession opened for handling the
           *                       request.
           * @param errSpec Output specification of errors in mcastSpec.
           * @see IMCastSession
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Join(const ::ds::SockAddrStorageType addr, const char* mcastSpec, ::ds::Net::IMCastSession** session, char* errSpec, int errSpecLen, int* errSpecLenReq) = 0;
         
         /**
           * This function allows object extensibility.
           * For supported interfaces, objects implementing those interfaces may be 
           * fetched via this function. The supported interfaces are documented in
           * the DS_NET_NetworkFactory.bid file. GetTechObject-supported interface
           * does not imply any similar contract in regard to QueryInterface for the
           * respective interface.
           * Unlike IQI, the availability of a specific interface depends on some 
           * factors, e.g. current network type. Moreover, there is no guaranty 
           * for transitivity or symmetry. 
           * Note: 'interface' parameter will map to iid and techObj.
           * @param iid The interface that should be retrieved.
           * @param techObj On success, will contain the requested interface instance.
           * @retval ds::SUCCESS Interface retrieved successfully.
           * @retval AEE_ECLASSNOTSUPPORT Specified interface unsupported.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetTechObject(AEEIID techObj_iid, void** techObj) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IMCASTMANAGER_H
