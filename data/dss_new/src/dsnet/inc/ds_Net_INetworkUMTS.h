#ifndef DS_NET_INETWORKUMTS_H
#define DS_NET_INETWORKUMTS_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
typedef boolean ds_Net_UMTSIMCNFlagType;
#define ds_Net_AEEIID_INetworkUMTS 0x107fc24

/** @interface ds_Net_INetworkUMTS
  * 
  * ds Net Network UMTS interface.
  * This interface provides UMTS network operations.
  */
#define INHERIT_ds_Net_INetworkUMTS(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*GetIMCNFlag)(iname* _pif, ds_Net_UMTSIMCNFlagType* IMCNFlag)
AEEINTERFACE_DEFINE(ds_Net_INetworkUMTS);

/** @memberof ds_Net_INetworkUMTS
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkUMTS_AddRef(ds_Net_INetworkUMTS* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkUMTS)->AddRef(_pif);
}

/** @memberof ds_Net_INetworkUMTS
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INetworkUMTS_Release(ds_Net_INetworkUMTS* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkUMTS)->Release(_pif);
}

/** @memberof ds_Net_INetworkUMTS
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INetworkUMTS_QueryInterface(ds_Net_INetworkUMTS* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkUMTS)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INetworkUMTS
  * 
  * This function Gets the IM CN Flag.
  * @param _pif Pointer to interface
  * @param IMCNFlag Output IM CN flag information.
  * @retval AEE_SUCCESS INetworkUMTS cloned successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INetworkUMTS_GetIMCNFlag(ds_Net_INetworkUMTS* _pif, ds_Net_UMTSIMCNFlagType* IMCNFlag)
{
   return AEEGETPVTBL(_pif, ds_Net_INetworkUMTS)->GetIMCNFlag(_pif, IMCNFlag);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
namespace ds
{
   namespace Net
   {
      typedef boolean UMTSIMCNFlagType;
      const ::AEEIID AEEIID_INetworkUMTS = 0x107fc24;
      
      /** @interface INetworkUMTS
        * 
        * ds Net Network UMTS interface.
        * This interface provides UMTS network operations.
        */
      struct INetworkUMTS : public ::IQI
      {
         
         /**
           * This function Gets the IM CN Flag.
           * @param IMCNFlag Output IM CN flag information.
           * @retval AEE_SUCCESS INetworkUMTS cloned successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetIMCNFlag(::ds::Net::UMTSIMCNFlagType* IMCNFlag) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INETWORKUMTS_H
