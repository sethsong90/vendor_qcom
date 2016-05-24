#ifndef DS_NET_IMCASTMANAGEREXT_H
#define DS_NET_IMCASTMANAGEREXT_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
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
#include "ds_Net_IMCastSession.h"
#include "ds_Net_IMCastSessionsInput.h"
#include "ds_Net_IMCastSessionsOutput.h"
#define ds_Net_AEEIID_IMCastManagerExt 0x10a380d
struct ds_Net_IMCastManagerExt__SeqSockAddrStorageType__seq_octet_ds {
   ds_SockAddrStorageType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net_IMCastManagerExt__SeqSockAddrStorageType__seq_octet_ds ds_Net_IMCastManagerExt__SeqSockAddrStorageType__seq_octet_ds;
typedef ds_Net_IMCastManagerExt__SeqSockAddrStorageType__seq_octet_ds ds_Net_IMCastManagerExt_SeqSockAddrStorageType;
struct ds_Net_IMCastManagerExt__SeqMCastJoinFlagsType__seq_long {
   ds_Net_MCastJoinFlagsType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net_IMCastManagerExt__SeqMCastJoinFlagsType__seq_long ds_Net_IMCastManagerExt__SeqMCastJoinFlagsType__seq_long;
typedef ds_Net_IMCastManagerExt__SeqMCastJoinFlagsType__seq_long ds_Net_IMCastManagerExt_SeqMCastJoinFlagsType;

/** @interface ds_Net_IMCastManagerExt
  * 
  * ds Net MCast Manager Extended interface.
  * This interface supports MultiCast management operations according to
  * BCMCS 2.0 specifications
  */
#define INHERIT_ds_Net_IMCastManagerExt(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*JoinBundle)(iname* _pif, const ds_SockAddrStorageType* addrSeq, int addrSeqLen, const char* mcastSpecs, ds_Net_IMCastSessionsOutput** sessions, const ds_Net_MCastJoinFlagsType* mcastJoinFlagsSeq, int mcastJoinFlagsSeqLen, char* errSpec, int errSpecLen, int* errSpecLenReq); \
   AEEResult (*RegisterBundle)(iname* _pif, ds_Net_IMCastSessionsInput* sessions); \
   AEEResult (*LeaveBundle)(iname* _pif, ds_Net_IMCastSessionsInput* sessions); \
   AEEResult (*CreateMCastSessionsInput)(iname* _pif, ds_Net_IMCastSessionsInput** newMCastSessionsInput); \
   AEEResult (*GetTechObject)(iname* _pif, AEEIID techObj_iid, void** techObj)
AEEINTERFACE_DEFINE(ds_Net_IMCastManagerExt);

/** @memberof ds_Net_IMCastManagerExt
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastManagerExt_AddRef(ds_Net_IMCastManagerExt* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerExt)->AddRef(_pif);
}

/** @memberof ds_Net_IMCastManagerExt
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IMCastManagerExt_Release(ds_Net_IMCastManagerExt* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerExt)->Release(_pif);
}

/** @memberof ds_Net_IMCastManagerExt
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IMCastManagerExt_QueryInterface(ds_Net_IMCastManagerExt* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerExt)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IMCastManagerExt
  *     
  * This function issues a request for joining multiple multicast
  * groups.
  * In order to receive the MCast data, the application must
  * register to the MCast groups. If the Join Flags in parameter
  * mcastJoinFlagsSeq specifies that registration setup is allowed, 
  * this API shall take care to carry out the registration part of the
  * MCast session setup. Otherwise, the application has to use the
  * RegisterBundle API after applying JoinBundle to complete the setup.
  *
  * Using the REG_SETUP_NOT_ALLOWED flag allows applications to commit
  * the "Join" part of the MCast setup at an early stage (e.g. by
  * Joining all available MCast groups) and complete the MCast session
  * setup later only once the User requests service, thus minimizing
  * the setup time required from the moment the User requests the
  * service.
  * 
  * @param _pif Pointer to interface
  * @param addrSeqLen Length of sequence
  * @param mcastJoinFlagsSeqLen Length of sequence
  * @param errSpecLen Length of sequence
  * @param errSpecLenReq Required length of sequence
  * @See RegisterBundle.
  * @param addrSeq Multicast group addresses.
  * @param mcastSpecs A JSON string specifying extra Multicast join
  *                   information. The extra Multicast join information
  *                   is supported only for certain MCast technologies.
  *                   When using this API to join other MCast
  *                   technologies, mcastSpecs can be empty.          
  *                   The application should add the Mcast Specs info
  *                   to the JSON string in the same order of the 
  *                   addresses in addrSeq.          
  *                   Currently extra Join information is not applicable
  *                   for supported technologies hence no JSON schema
  *                   is currently specified for this information.
  * @param sessions Output IMCastSessionsOutput containing IMCastSession 
  *                        objects opened for handling the requests.
  *                        The order of sessions shall correspond to the
  *                        order of addresses in addrSeq.
  *                        @see IMCastSessionsOutput, IMcastSession
  * @param mcastJoinFlagsSeq flags for the Join operation. The order of
  *                          of flags specifications must correspond to
  *                          the order of MCast addresses in addrSeq.
  *                          Different requests in the sequence may
  *                          have different flags.
  *                          @See MCastJoinFlagsType
  * @param errSpec Output specification of errors in mcastSpecs.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.          
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerExt_JoinBundle(ds_Net_IMCastManagerExt* _pif, const ds_SockAddrStorageType* addrSeq, int addrSeqLen, const char* mcastSpecs, ds_Net_IMCastSessionsOutput** sessions, const ds_Net_MCastJoinFlagsType* mcastJoinFlagsSeq, int mcastJoinFlagsSeqLen, char* errSpec, int errSpecLen, int* errSpecLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerExt)->JoinBundle(_pif, addrSeq, addrSeqLen, mcastSpecs, sessions, mcastJoinFlagsSeq, mcastJoinFlagsSeqLen, errSpec, errSpecLen, errSpecLenReq);
}

/** @memberof ds_Net_IMCastManagerExt
  *     
  * This function issues a request for registering multiple multicast
  * groups.
  * @param _pif Pointer to interface
  * @param sessions A MCastSessions group with which all
  *                 IMcastSessions.
  *                 objects to register are associated.
  *                 @see IMCastSessionsInput, IMcastSession.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerExt_RegisterBundle(ds_Net_IMCastManagerExt* _pif, ds_Net_IMCastSessionsInput* sessions)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerExt)->RegisterBundle(_pif, sessions);
}

/** @memberof ds_Net_IMCastManagerExt
  * 
  * This function issues a request for leaving multiple multicast
  * groups.
  * @param _pif Pointer to interface
  * @param sessions A MCastSessions group with which all
  IMcastSessions objects to leave are associated.
  *                 @see IMCastSessionsInput, IMcastSession.
  * @retval AEE_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerExt_LeaveBundle(ds_Net_IMCastManagerExt* _pif, ds_Net_IMCastSessionsInput* sessions)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerExt)->LeaveBundle(_pif, sessions);
}

/** @memberof ds_Net_IMCastManagerExt
  *     
  * This function creates an instance of IMCastSessionsInput.
  * IMCastSessionsInput creation is supported only via IMCastManagerExt.
  * @param _pif Pointer to interface
  * @param new newMCastSessionsInput Output The newly created 
  *                                         IMCastSessionsInput
  *                                         instance.
  * @retval AEE_SUCCESS IMCastSessionsInput created successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerExt_CreateMCastSessionsInput(ds_Net_IMCastManagerExt* _pif, ds_Net_IMCastSessionsInput** newMCastSessionsInput)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerExt)->CreateMCastSessionsInput(_pif, newMCastSessionsInput);
}

/** @memberof ds_Net_IMCastManagerExt
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
  * @retval AEE_SUCCESS Interface retrieved successfully.
  * @retval AEE_ECLASSNOTSUPPORT Specified interface unsupported.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IMCastManagerExt_GetTechObject(ds_Net_IMCastManagerExt* _pif, AEEIID techObj_iid, void** techObj)
{
   return AEEGETPVTBL(_pif, ds_Net_IMCastManagerExt)->GetTechObject(_pif, techObj_iid, techObj);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Addr_Def.h"
#include "ds_Net_MCast_Def.h"
#include "AEEIQI.h"
#include "AEEISignal.h"
#include "ds_Net_Def.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IMCastSession.h"
#include "ds_Net_IMCastSessionsInput.h"
#include "ds_Net_IMCastSessionsOutput.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IMCastManagerExt = 0x10a380d;
      
      /** @interface IMCastManagerExt
        * 
        * ds Net MCast Manager Extended interface.
        * This interface supports MultiCast management operations according to
        * BCMCS 2.0 specifications
        */
      struct IMCastManagerExt : public ::IQI
      {
         struct _SeqSockAddrStorageType__seq_octet_ds {
            ::ds::SockAddrStorageType* data;
            int dataLen;
            int dataLenReq;
         };
         typedef _SeqSockAddrStorageType__seq_octet_ds SeqSockAddrStorageType;
         struct _SeqMCastJoinFlagsType__seq_long {
            ::ds::Net::MCastJoinFlagsType* data;
            int dataLen;
            int dataLenReq;
         };
         typedef _SeqMCastJoinFlagsType__seq_long SeqMCastJoinFlagsType;
         
         /**    
           * This function issues a request for joining multiple multicast
           * groups.
           * In order to receive the MCast data, the application must
           * register to the MCast groups. If the Join Flags in parameter
           * mcastJoinFlagsSeq specifies that registration setup is allowed, 
           * this API shall take care to carry out the registration part of the
           * MCast session setup. Otherwise, the application has to use the
           * RegisterBundle API after applying JoinBundle to complete the setup.
           *
           * Using the REG_SETUP_NOT_ALLOWED flag allows applications to commit
           * the "Join" part of the MCast setup at an early stage (e.g. by
           * Joining all available MCast groups) and complete the MCast session
           * setup later only once the User requests service, thus minimizing
           * the setup time required from the moment the User requests the
           * service.
           * 
           * @See RegisterBundle.
           * @param addrSeq Multicast group addresses.
           * @param mcastSpecs A JSON string specifying extra Multicast join
           *                   information. The extra Multicast join information
           *                   is supported only for certain MCast technologies.
           *                   When using this API to join other MCast
           *                   technologies, mcastSpecs can be empty.          
           *                   The application should add the Mcast Specs info
           *                   to the JSON string in the same order of the 
           *                   addresses in addrSeq.          
           *                   Currently extra Join information is not applicable
           *                   for supported technologies hence no JSON schema
           *                   is currently specified for this information.
           * @param sessions Output IMCastSessionsOutput containing IMCastSession 
           *                        objects opened for handling the requests.
           *                        The order of sessions shall correspond to the
           *                        order of addresses in addrSeq.
           *                        @see IMCastSessionsOutput, IMcastSession
           * @param mcastJoinFlagsSeq flags for the Join operation. The order of
           *                          of flags specifications must correspond to
           *                          the order of MCast addresses in addrSeq.
           *                          Different requests in the sequence may
           *                          have different flags.
           *                          @See MCastJoinFlagsType
           * @param errSpec Output specification of errors in mcastSpecs.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.          
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL JoinBundle(const ::ds::SockAddrStorageType* addrSeq, int addrSeqLen, const char* mcastSpecs, ::ds::Net::IMCastSessionsOutput** sessions, const ::ds::Net::MCastJoinFlagsType* mcastJoinFlagsSeq, int mcastJoinFlagsSeqLen, char* errSpec, int errSpecLen, int* errSpecLenReq) = 0;
         
         /**    
           * This function issues a request for registering multiple multicast
           * groups.
           * @param sessions A MCastSessions group with which all
           *                 IMcastSessions.
           *                 objects to register are associated.
           *                 @see IMCastSessionsInput, IMcastSession.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL RegisterBundle(::ds::Net::IMCastSessionsInput* sessions) = 0;
         
         /**
           * This function issues a request for leaving multiple multicast
           * groups.
           * @param sessions A MCastSessions group with which all
           IMcastSessions objects to leave are associated.
           *                 @see IMCastSessionsInput, IMcastSession.
           * @retval AEE_SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL LeaveBundle(::ds::Net::IMCastSessionsInput* sessions) = 0;
         
         /**    
           * This function creates an instance of IMCastSessionsInput.
           * IMCastSessionsInput creation is supported only via IMCastManagerExt.
           * @param new newMCastSessionsInput Output The newly created 
           *                                         IMCastSessionsInput
           *                                         instance.
           * @retval AEE_SUCCESS IMCastSessionsInput created successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL CreateMCastSessionsInput(::ds::Net::IMCastSessionsInput** newMCastSessionsInput) = 0;
         
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
           * @retval AEE_SUCCESS Interface retrieved successfully.
           * @retval AEE_ECLASSNOTSUPPORT Specified interface unsupported.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetTechObject(AEEIID techObj_iid, void** techObj) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IMCASTMANAGEREXT_H
