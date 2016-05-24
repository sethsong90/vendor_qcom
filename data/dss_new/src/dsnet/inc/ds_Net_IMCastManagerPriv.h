#ifndef DS_NET_IMCASTMANAGERPRIV_H
#define DS_NET_IMCASTMANAGERPRIV_H

/*============================================================================
  Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "ds_Net_MCast_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IMCastSessionPriv.h"
#include "ds_Net_IMBMSSpecPriv.h"
#define ds_Net_AEEIID_IMCastManagerPriv 0x109b2e5
struct ds_Net_IMCastManagerPriv__SeqSockAddrStorageType__seq_octet_ds {
   ds_SockAddrStorageType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net_IMCastManagerPriv__SeqSockAddrStorageType__seq_octet_ds ds_Net_IMCastManagerPriv__SeqSockAddrStorageType__seq_octet_ds;
typedef ds_Net_IMCastManagerPriv__SeqSockAddrStorageType__seq_octet_ds ds_Net_IMCastManagerPriv_SeqSockAddrStorageType;
struct ds_Net_IMCastManagerPriv__SeqIQI__seq_IQI {
   struct IQI** data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net_IMCastManagerPriv__SeqIQI__seq_IQI ds_Net_IMCastManagerPriv__SeqIQI__seq_IQI;
typedef ds_Net_IMCastManagerPriv__SeqIQI__seq_IQI ds_Net_IMCastManagerPriv_SeqIQI;
struct ds_Net_IMCastManagerPriv__SeqIMCastSessionPrivType__seq_IMCastSessionPriv_Net_ds {
   struct ds_Net_IMCastSessionPriv** data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net_IMCastManagerPriv__SeqIMCastSessionPrivType__seq_IMCastSessionPriv_Net_ds ds_Net_IMCastManagerPriv__SeqIMCastSessionPrivType__seq_IMCastSessionPriv_Net_ds;
typedef ds_Net_IMCastManagerPriv__SeqIMCastSessionPrivType__seq_IMCastSessionPriv_Net_ds ds_Net_IMCastManagerPriv_SeqIMCastSessionPrivType;
struct ds_Net_IMCastManagerPriv__SeqMCastJoinFlagsType__seq_long {
   ds_Net_MCastJoinFlagsType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net_IMCastManagerPriv__SeqMCastJoinFlagsType__seq_long ds_Net_IMCastManagerPriv__SeqMCastJoinFlagsType__seq_long;
typedef ds_Net_IMCastManagerPriv__SeqMCastJoinFlagsType__seq_long ds_Net_IMCastManagerPriv_SeqMCastJoinFlagsType;

/** @interface ds_Net_IMCastManagerPriv
  * 
  * ds Net MCast Manager Private interface.
  * This interface supports both BCMCS 1.0 and BCMCS 2.0 MCast semantics.
  */
#define INHERIT_ds_Net_IMCastManagerPriv(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*Join)(iname* _pif, const ds_SockAddrStorageType addr, IQI* mcastSpecInfo, ds_Net_IMCastSessionPriv** session); \
   AEEResult (*JoinBundle)(iname* _pif, const ds_SockAddrStorageType* addrSeq, int addrSeqLen, struct IQI** mcastSpecInfoSeq, int mcastSpecInfoSeqLen, const ds_Net_MCastJoinFlagsType* mcastJoinFlagsSeq, int mcastJoinFlagsSeqLen, struct ds_Net_IMCastSessionPriv** sessions, int sessionsLen, int* sessionsLenReq); \
   AEEResult (*RegisterBundle)(iname* _pif, struct ds_Net_IMCastSessionPriv** sessions, int sessionsLen); \
   AEEResult (*LeaveBundle)(iname* _pif, struct ds_Net_IMCastSessionPriv** sessions, int sessionsLen); \
   AEEResult (*GetTechObject)(iname* _pif, AEEIID techObj_iid, void** techObj); \
   AEEResult (*CreateMBMSSpecPriv)(iname* _pif, ds_Net_IMBMSSpecPriv** newMBMSSpecPriv)
AEEINTERFACE_DEFINE(ds_Net_IMCastManagerPriv);

/** @memberof ds_Net_IMCastManagerPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastManagerPriv_AddRef(ds_Net_IMCastManagerPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IMCastManagerPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastManagerPriv_Release(ds_Net_IMCastManagerPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerPriv)->Release(_pif);
}

/** @memberof ds_Net_IMCastManagerPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IMCastManagerPriv_QueryInterface(ds_Net_IMCastManagerPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IMCastManagerPriv
  *     
  * This function issues a request for joining a multicast group and
  * registering to it.
  * @param _pif Pointer to interface
  * @param addr Multicast group address.
  * @param mcastSpecInfo Multicast extra join information. May be NULL
  *                      if no extra information is needed.          
  * @param session The IMCastSession opened for handling the request.
  * @see IMCastSession
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerPriv_Join(ds_Net_IMCastManagerPriv* _pif, const ds_SockAddrStorageType addr, IQI* mcastSpecInfo, ds_Net_IMCastSessionPriv** session)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerPriv)->Join(_pif, addr, mcastSpecInfo, session);
}

/** @memberof ds_Net_IMCastManagerPriv
  *     
  * This function issues a request for joining multiple multicast
  * groups.
  * In order to receive the MCast data, the application must also 
  * register to the MCast groups using the RegisterBundle API.
  * Applying JoinBundle allows applications to commit the "Join" part
  * of the MCast setup at an early stage (e.g. by Joining all available
  * MCast groups) and apply only the RegisterBundle API once the User
  * is requesting service, thus minimizing the setup time required from
  * the moment the User requests the service.          
  * @param _pif Pointer to interface
  * @param addrSeqLen Length of sequence
  * @param mcastSpecInfoSeqLen Length of sequence
  * @param mcastJoinFlagsSeqLen Length of sequence
  * @param sessionsLen Length of sequence
  * @param sessionsLenReq Required length of sequence
  * @See RegisterBundle.
  * @param addrSeq Multicast group addresses.
  * @param mcastSpecInfoSeq Multicast extra join information sequence.
  * @param mcastJoinFlagsSeq flags for the Join operation
  * @param sessions Output IMCastSessionsOutput containing IMCastSession 
  *                        objects opened for handling the requests.
  *                        The order of sessions shall correspond to the
  *                        order of addresses in addrSeq.
  *                        @see IMCastSessionsOutput, IMcastSession
  * @param errSpec Output specification of errors in mcastSpecs.
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.          
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerPriv_JoinBundle(ds_Net_IMCastManagerPriv* _pif, const ds_SockAddrStorageType* addrSeq, int addrSeqLen, struct IQI** mcastSpecInfoSeq, int mcastSpecInfoSeqLen, const ds_Net_MCastJoinFlagsType* mcastJoinFlagsSeq, int mcastJoinFlagsSeqLen, struct ds_Net_IMCastSessionPriv** sessions, int sessionsLen, int* sessionsLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerPriv)->JoinBundle(_pif, addrSeq, addrSeqLen, mcastSpecInfoSeq, mcastSpecInfoSeqLen, mcastJoinFlagsSeq, mcastJoinFlagsSeqLen, sessions, sessionsLen, sessionsLenReq);
}

/** @memberof ds_Net_IMCastManagerPriv
  *     
  * This function issues a request for registering multiple multicast
  * groups.
  * @param _pif Pointer to interface
  * @param sessions A MCastSessions group with which all
  *                 IMcastSessions.
  *                 objects to register are associated.
  * @param sessionsLen Length of sequence
  *                 @see IMCastSessionsInput, IMcastSession.
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerPriv_RegisterBundle(ds_Net_IMCastManagerPriv* _pif, struct ds_Net_IMCastSessionPriv** sessions, int sessionsLen)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerPriv)->RegisterBundle(_pif, sessions, sessionsLen);
}

/** @memberof ds_Net_IMCastManagerPriv
  * 
  * This function issues a request for leaving multiple multicast
  * groups.
  * @param _pif Pointer to interface
  * @param sessions A MCastSessions group with which all
  * @param sessionsLen Length of sequence
  IMcastSessions objects to leave are associated.
  *                 @see IMCastSessionsInput, IMcastSession.
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerPriv_LeaveBundle(ds_Net_IMCastManagerPriv* _pif, struct ds_Net_IMCastSessionPriv** sessions, int sessionsLen)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerPriv)->LeaveBundle(_pif, sessions, sessionsLen);
}

/** @memberof ds_Net_IMCastManagerPriv
  * 
  * This function allows object extensibility.
  * For supported interfaces, objects implementing those interfaces may
  * be fetched via this function. The supported interfaces are
  * @param _pif Pointer to interface
  * @param techObj_iid Interface ID of requested interface
  * documented in class documentation (@See ds_Net_CNetworkFactory.idl).
  * GetTechObject-supported interface does not imply any similar
  * contract in regard to QueryInterface for the respective interface.
  * Unlike IQI, the availability of a specific interface depends on
  * some factors, e.g. current network type. Moreover, there is no
  * guaranty for transitivity or symmetry. 
  * Note: 'interface' parameter will map to iid and techObj.
  * @param iid The interface that should be retrieved.
  * @param techObj On success, will contain the requested interface
  * instance.
  * @retval ds_SUCCESS Interface retrieved successfully.
  * @retval AEE_ECLASSNOTSUPPORT Specified interface unsupported.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerPriv_GetTechObject(ds_Net_IMCastManagerPriv* _pif, AEEIID techObj_iid, void** techObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerPriv)->GetTechObject(_pif, techObj_iid, techObj);
}

/** @memberof ds_Net_IMCastManagerPriv
  *     
  * This function creates an instance of MBMSSpec. MBMSSpec creation is
  * supported only via IMCastManagerPriv. 
  * IMBMSSpec instance can be used to specify MBMS MCast Join information
  * in IMCastManager_Join and IMCastManager_JoinBundle.
  * @param _pif Pointer to interface
  * @param newMBMSSpec Output The newly created MBMSSpec instance.
  * @retval AEE_SUCCESS MBMSSpec created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see IMCastManager_Join
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerPriv_CreateMBMSSpecPriv(ds_Net_IMCastManagerPriv* _pif, ds_Net_IMBMSSpecPriv** newMBMSSpecPriv)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerPriv)->CreateMBMSSpecPriv(_pif, newMBMSSpecPriv);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "ds_Net_MCast_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IMCastSessionPriv.h"
#include "ds_Net_IMBMSSpecPriv.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IMCastManagerPriv = 0x109b2e5;
      
      /** @interface IMCastManagerPriv
        * 
        * ds Net MCast Manager Private interface.
        * This interface supports both BCMCS 1.0 and BCMCS 2.0 MCast semantics.
        */
      struct IMCastManagerPriv : public ::IQI
      {
         struct _SeqSockAddrStorageType__seq_octet_ds {
            ::ds::SockAddrStorageType* data;
            int dataLen;
            int dataLenReq;
         };
         typedef _SeqSockAddrStorageType__seq_octet_ds SeqSockAddrStorageType;
         struct _SeqIQI__seq_IQI {
            ::IQI** data;
            int dataLen;
            int dataLenReq;
         };
         typedef _SeqIQI__seq_IQI SeqIQI;
         struct _SeqIMCastSessionPrivType__seq_IMCastSessionPriv_Net_ds {
            ::ds::Net::IMCastSessionPriv** data;
            int dataLen;
            int dataLenReq;
         };
         typedef _SeqIMCastSessionPrivType__seq_IMCastSessionPriv_Net_ds SeqIMCastSessionPrivType;
         struct _SeqMCastJoinFlagsType__seq_long {
            ::ds::Net::MCastJoinFlagsType* data;
            int dataLen;
            int dataLenReq;
         };
         typedef _SeqMCastJoinFlagsType__seq_long SeqMCastJoinFlagsType;
         
         /**    
           * This function issues a request for joining a multicast group and
           * registering to it.
           * @param addr Multicast group address.
           * @param mcastSpecInfo Multicast extra join information. May be NULL
           *                      if no extra information is needed.          
           * @param session The IMCastSession opened for handling the request.
           * @see IMCastSession
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Join(const ::ds::SockAddrStorageType addr, ::IQI* mcastSpecInfo, ::ds::Net::IMCastSessionPriv** session) = 0;
         
         /**    
           * This function issues a request for joining multiple multicast
           * groups.
           * In order to receive the MCast data, the application must also 
           * register to the MCast groups using the RegisterBundle API.
           * Applying JoinBundle allows applications to commit the "Join" part
           * of the MCast setup at an early stage (e.g. by Joining all available
           * MCast groups) and apply only the RegisterBundle API once the User
           * is requesting service, thus minimizing the setup time required from
           * the moment the User requests the service.          
           * @See RegisterBundle.
           * @param addrSeq Multicast group addresses.
           * @param mcastSpecInfoSeq Multicast extra join information sequence.
           * @param mcastJoinFlagsSeq flags for the Join operation
           * @param sessions Output IMCastSessionsOutput containing IMCastSession 
           *                        objects opened for handling the requests.
           *                        The order of sessions shall correspond to the
           *                        order of addresses in addrSeq.
           *                        @see IMCastSessionsOutput, IMcastSession
           * @param errSpec Output specification of errors in mcastSpecs.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.          
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL JoinBundle(const ::ds::SockAddrStorageType* addrSeq, int addrSeqLen, ::IQI** mcastSpecInfoSeq, int mcastSpecInfoSeqLen, const ::ds::Net::MCastJoinFlagsType* mcastJoinFlagsSeq, int mcastJoinFlagsSeqLen, ::ds::Net::IMCastSessionPriv** sessions, int sessionsLen, int* sessionsLenReq) = 0;
         
         /**    
           * This function issues a request for registering multiple multicast
           * groups.
           * @param sessions A MCastSessions group with which all
           *                 IMcastSessions.
           *                 objects to register are associated.
           *                 @see IMCastSessionsInput, IMcastSession.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL RegisterBundle(::ds::Net::IMCastSessionPriv** sessions, int sessionsLen) = 0;
         
         /**
           * This function issues a request for leaving multiple multicast
           * groups.
           * @param sessions A MCastSessions group with which all
           IMcastSessions objects to leave are associated.
           *                 @see IMCastSessionsInput, IMcastSession.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL LeaveBundle(::ds::Net::IMCastSessionPriv** sessions, int sessionsLen) = 0;
         
         /**
           * This function allows object extensibility.
           * For supported interfaces, objects implementing those interfaces may
           * be fetched via this function. The supported interfaces are
           * documented in class documentation (@See ds_Net_CNetworkFactory.idl).
           * GetTechObject-supported interface does not imply any similar
           * contract in regard to QueryInterface for the respective interface.
           * Unlike IQI, the availability of a specific interface depends on
           * some factors, e.g. current network type. Moreover, there is no
           * guaranty for transitivity or symmetry. 
           * Note: 'interface' parameter will map to iid and techObj.
           * @param iid The interface that should be retrieved.
           * @param techObj On success, will contain the requested interface
           * instance.
           * @retval ds::SUCCESS Interface retrieved successfully.
           * @retval AEE_ECLASSNOTSUPPORT Specified interface unsupported.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetTechObject(AEEIID techObj_iid, void** techObj) = 0;
         
         /**    
           * This function creates an instance of MBMSSpec. MBMSSpec creation is
           * supported only via IMCastManagerPriv. 
           * IMBMSSpec instance can be used to specify MBMS MCast Join information
           * in IMCastManager::Join and IMCastManager::JoinBundle.
           * @param newMBMSSpec Output The newly created MBMSSpec instance.
           * @retval AEE_SUCCESS MBMSSpec created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see IMCastManager::Join
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateMBMSSpecPriv(::ds::Net::IMBMSSpecPriv** newMBMSSpecPriv) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IMCASTMANAGERPRIV_H
