#ifndef DS_NET_INATSESSIONPRIV_H
#define DS_NET_INATSESSIONPRIV_H
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#define ds_Net_AEEIID_INatSessionPriv 0x10a9c0a

/** @interface ds_Net_INatSessionPriv
  * 
  * ds Net Nat session priv interface.
  */
#define INHERIT_ds_Net_INatSessionPriv(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*Enable)(iname* _pif); \
   AEEResult (*Disable)(iname* _pif); \
   AEEResult (*EnableRoamingAutoconnect)(iname* _pif); \
   AEEResult (*DisableRoamingAutoconnect)(iname* _pif)
AEEINTERFACE_DEFINE(ds_Net_INatSessionPriv);

/** @memberof ds_Net_INatSessionPriv
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INatSessionPriv_AddRef(ds_Net_INatSessionPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSessionPriv)->AddRef(_pif);
}

/** @memberof ds_Net_INatSessionPriv
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INatSessionPriv_Release(ds_Net_INatSessionPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSessionPriv)->Release(_pif);
}

/** @memberof ds_Net_INatSessionPriv
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INatSessionPriv_QueryInterface(ds_Net_INatSessionPriv* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSessionPriv)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INatSessionPriv
  * 
  * This function issues a request for enabling NAT interface.
  * @param _pif Pointer to interface
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INatSessionPriv_Enable(ds_Net_INatSessionPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSessionPriv)->Enable(_pif);
}

/** @memberof ds_Net_INatSessionPriv
  * 
  * This function issues a request for disabling NAT interface.
  * @param _pif Pointer to interface
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other ds designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_INatSessionPriv_Disable(ds_Net_INatSessionPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSessionPriv)->Disable(_pif);
}

/** @memberof ds_Net_INatSessionPriv
          * 
          * This function issues a request for enabling roaming 
          * autoconnect on NAT interface. 
          * @param _pif Pointer to interface
          * @retval ds_SUCCESS Request received successfully.
          * @retval Other ds designated error codes might be returned.
          * @see ds_Errors_Def.idl.
          */
static __inline AEEResult ds_Net_INatSessionPriv_EnableRoamingAutoconnect(ds_Net_INatSessionPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSessionPriv)->EnableRoamingAutoconnect(_pif);
}

/** @memberof ds_Net_INatSessionPriv
         * 
         * This function issues a request for disabling roaming 
         * autoconnect on NAT interface. 
         * @param _pif Pointer to interface
         * @retval ds_SUCCESS Request received successfully.
         * @retval Other ds designated error codes might be returned.
         * @see ds_Errors_Def.idl.
         */
static __inline AEEResult ds_Net_INatSessionPriv_DisableRoamingAutoconnect(ds_Net_INatSessionPriv* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSessionPriv)->DisableRoamingAutoconnect(_pif);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_INatSessionPriv = 0x10a9c0a;
      
      /** @interface INatSessionPriv
        * 
        * ds Net Nat session priv interface.
        */
      struct INatSessionPriv : public ::IQI
      {
         
         /**
           * This function issues a request for enabling NAT interface.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Enable() = 0;
         
         /**
           * This function issues a request for disabling NAT interface.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other ds designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL Disable() = 0;

         /**
          * This function issues a request for enabling roaming 
          * autoconnect on NAT interface. 
          * @retval ds::SUCCESS Request received successfully.
          * @retval Other ds designated error codes might be returned.
          * @see ds_Errors_Def.idl.
          */
         virtual ::AEEResult AEEINTERFACE_CDECL EnableRoamingAutoconnect() = 0;
         
         /**
         * This function issues a request for disabling roaming 
         * autoconnect on NAT interface. 
         * @retval ds::SUCCESS Request received successfully.
         * @retval Other ds designated error codes might be returned.
         * @see ds_Errors_Def.idl.
         */
         virtual ::AEEResult AEEINTERFACE_CDECL DisableRoamingAutoconnect() = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INATSESSIONPRIV_H
