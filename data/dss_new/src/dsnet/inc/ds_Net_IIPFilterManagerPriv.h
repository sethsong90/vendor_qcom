#ifndef DS_NET_IIPFILTERMANAGERPRIV_H
#define DS_NET_IIPFILTERMANAGERPRIV_H

/*============================================================================
  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
  ============================================================================*/
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "ds_Net_Def.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_IIPFilterPriv.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IIPFilterRegPriv.h"
#define ds_Net_AEEIID_IIPFilterManagerPriv 0x107dd61

/** @interface ds_Net_IIPFilterManagerPriv
  * 
  * ds network IP Filter manager interface.
  * This interface manages the IP filters defined on INetwork object.
  * Application creates INetwork object and call IQI with IIPFilterMgr - TODO - move this documentation to a bid file.
  * This is privileged - internal interface
  */
#define INHERIT_ds_Net_IIPFilterManagerPriv(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*RegisterFilters)(iname* _pif, int fi_result, struct ds_Net_IIPFilterPriv** filters, int filtersLen, ds_Net_IIPFilterRegPriv** filterReg)
AEEINTERFACE_DEFINE(ds_Net_IIPFilterManagerPriv);

/** @memberof ds_Net_IIPFilterManagerPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IIPFilterManagerPriv_AddRef(ds_Net_IIPFilterManagerPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterManagerPriv)->AddRef(_pif);
}

/** @memberof ds_Net_IIPFilterManagerPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IIPFilterManagerPriv_Release(ds_Net_IIPFilterManagerPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterManagerPriv)->Release(_pif);
}

/** @memberof ds_Net_IIPFilterManagerPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IIPFilterManagerPriv_QueryInterface(ds_Net_IIPFilterManagerPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterManagerPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IIPFilterManagerPriv
  * 
  * This function registers filters on specific ds Network object.
  * IMPORTANT: This interface is non-public. If and when it becomes
  *            public there may be need to change the usage of sequence
  *            to IGroup-like interface if there is a requirement to
  *            support more than 14 filters in a single API call
  *            (currently not supported in CS).
  *            In Addition, IIPFilterRegPriv shall be modified if this
  *            functionality is exposed to external apps. See comment
  *            in IIPFilterRegPriv.
  * 
  * @param _pif Pointer to interface
  * @param fi_result TODO: document this parameter.
  * @param filters List of IPFilters to be defined on the ds Network object.
  *                define IP filters on the ds Network object.
  *                           
  * @param filtersLen Length of sequence
  * @See IIPFilter.
  * @retval AEE_SUCCESS Registration succeeded.
  * @retval ds_EBADOPTS Bad options.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IIPFilterManagerPriv_RegisterFilters(ds_Net_IIPFilterManagerPriv* _pif, int fi_result, struct ds_Net_IIPFilterPriv** filters, int filtersLen, ds_Net_IIPFilterRegPriv** filterReg)
{
   return AEEGETPVTBL(_pif, ds_Net_IIPFilterManagerPriv)->RegisterFilters(_pif, fi_result, filters, filtersLen, filterReg);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "ds_Net_Def.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_IIPFilterPriv.h"
#include "AEEISignal.h"
#include "ds_Net_IEventManager.h"
#include "ds_Net_IIPFilterRegPriv.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IIPFilterManagerPriv = 0x107dd61;
      
      /** @interface IIPFilterManagerPriv
        * 
        * ds network IP Filter manager interface.
        * This interface manages the IP filters defined on INetwork object.
        * Application creates INetwork object and call IQI with IIPFilterMgr - TODO - move this documentation to a bid file.
        * This is privileged - internal interface
        */
      struct IIPFilterManagerPriv : public ::IQI
      {
         
         /**
           * This function registers filters on specific ds Network object.
           * IMPORTANT: This interface is non-public. If and when it becomes
           *            public there may be need to change the usage of sequence
           *            to IGroup-like interface if there is a requirement to
           *            support more than 14 filters in a single API call
           *            (currently not supported in CS).
           *            In Addition, IIPFilterRegPriv shall be modified if this
           *            functionality is exposed to external apps. See comment
           *            in IIPFilterRegPriv.
           * 
           * @param fi_result TODO: document this parameter.
           * @param filters List of IPFilters to be defined on the ds Network object.
           *                define IP filters on the ds Network object.
           *                           
           * @See IIPFilter.
           * @retval AEE_SUCCESS Registration succeeded.
           * @retval ds::EBADOPTS Bad options.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL RegisterFilters(int fi_result, ::ds::Net::IIPFilterPriv** filters, int filtersLen, ::ds::Net::IIPFilterRegPriv** filterReg) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IIPFILTERMANAGERPRIV_H
