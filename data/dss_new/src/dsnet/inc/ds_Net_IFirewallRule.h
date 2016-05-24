#ifndef DS_NET_IFIREWALLRULE_H
#define DS_NET_IFIREWALLRULE_H
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Errors_Def.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_IIPFilterPriv.h"
#define ds_Net_AEEIID_IFirewallRule 0x109e2f5
#define INHERIT_ds_Net_IFirewallRule(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*GetFirewallRule)(iname* _pif, ds_Net_IIPFilterPriv** filterSpec)
AEEINTERFACE_DEFINE(ds_Net_IFirewallRule);

/** @memberof ds_Net_IFirewallRule
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IFirewallRule_AddRef(ds_Net_IFirewallRule* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IFirewallRule)->AddRef(_pif);
}

/** @memberof ds_Net_IFirewallRule
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IFirewallRule_Release(ds_Net_IFirewallRule* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IFirewallRule)->Release(_pif);
}

/** @memberof ds_Net_IFirewallRule
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IFirewallRule_QueryInterface(ds_Net_IFirewallRule* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IFirewallRule)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IFirewallRule
  * 
  * This function retrieves one firewall rule
  * @param _pif Pointer to interface
  * @param filterSpec Output Retrieved firewall specs
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IFirewallRule_GetFirewallRule(ds_Net_IFirewallRule* _pif, ds_Net_IIPFilterPriv** filterSpec)
{
   return AEEGETPVTBL(_pif, ds_Net_IFirewallRule)->GetFirewallRule(_pif, filterSpec);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Errors_Def.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_IIPFilterPriv.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IFirewallRule = 0x109e2f5;
      struct IFirewallRule : public ::IQI
      {
         
         /**
           * This function retrieves one firewall rule
           * @param filterSpec Output Retrieved firewall specs
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetFirewallRule(::ds::Net::IIPFilterPriv** filterSpec) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IFIREWALLRULE_H
