#ifndef DS_NET_IFIREWALLMANAGER_H
#define DS_NET_IFIREWALLMANAGER_H
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_IIPFilterPriv.h"
#include "ds_Errors_Def.h"
#include "ds_Net_IFirewallRule.h"
#define ds_Net_AEEIID_IFirewallManager 0x109e2f6
struct ds_Net_IFirewallManager__IFirewallListType__seq_IIPFilterPriv_Net_ds {
   struct ds_Net_IIPFilterPriv** data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net_IFirewallManager__IFirewallListType__seq_IIPFilterPriv_Net_ds ds_Net_IFirewallManager__IFirewallListType__seq_IIPFilterPriv_Net_ds;
typedef ds_Net_IFirewallManager__IFirewallListType__seq_IIPFilterPriv_Net_ds ds_Net_IFirewallManager_IFirewallListType;
struct ds_Net_IFirewallManager__SeqFirewallRulesType__seq_IFirewallRule_Net_ds {
   struct ds_Net_IFirewallRule** data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net_IFirewallManager__SeqFirewallRulesType__seq_IFirewallRule_Net_ds ds_Net_IFirewallManager__SeqFirewallRulesType__seq_IFirewallRule_Net_ds;
typedef ds_Net_IFirewallManager__SeqFirewallRulesType__seq_IFirewallRule_Net_ds ds_Net_IFirewallManager_SeqFirewallRulesType;

/** @interface ds_Net_IFirewallManager
  * 
  * DS network Firewall manager interface.
  * This interface manages the firewall rules defined on INetwork object.
  * Application creates INetwork object and call IQI with IIPFirewallMgr
  * This is privileged - internal interface
  */
#define INHERIT_ds_Net_IFirewallManager(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*AddFirewallRule)(iname* _pif, ds_Net_IIPFilterPriv* filter, ds_Net_IFirewallRule** firewallRule); \
   AEEResult (*EnableFirewall)(iname* _pif, boolean isAllowed); \
   AEEResult (*DisableFirewall)(iname* _pif); \
   AEEResult (*GetFirewallTable)(iname* _pif, struct ds_Net_IFirewallRule** firewallRules, int firewallRulesLen, int* firewallRulesLenReq)
AEEINTERFACE_DEFINE(ds_Net_IFirewallManager);

/** @memberof ds_Net_IFirewallManager
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IFirewallManager_AddRef(ds_Net_IFirewallManager* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IFirewallManager)->AddRef(_pif);
}

/** @memberof ds_Net_IFirewallManager
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_IFirewallManager_Release(ds_Net_IFirewallManager* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IFirewallManager)->Release(_pif);
}

/** @memberof ds_Net_IFirewallManager
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_IFirewallManager_QueryInterface(ds_Net_IFirewallManager* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_IFirewallManager)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_IFirewallManager
  * 
  * This function adds firewall rules on specific DS Network object.
  * @param _pif Pointer to interface
  * @param filter Firewall rule to be defined on the DS Network object.
  * @param firewallRule Output Returned Firewall rule interface.
  * @retval ds_SUCCESS Registration succeeded.
  * @retval ds_EBADOPTS Bad options.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_IFirewallManager_AddFirewallRule(ds_Net_IFirewallManager* _pif, ds_Net_IIPFilterPriv* filter, ds_Net_IFirewallRule** firewallRule)
{
   return AEEGETPVTBL(_pif, ds_Net_IFirewallManager)->AddFirewallRule(_pif, filter, firewallRule);
}

/** @memberof ds_Net_IFirewallManager
  * 
  * This function used to enable firewall on an interface
  * @param _pif Pointer to interface
  * @param isallowed Specify if data is to be passed/dropped.
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors_Def.idl.
  */
static __inline AEEResult ds_Net_IFirewallManager_EnableFirewall(ds_Net_IFirewallManager* _pif, boolean isAllowed)
{
   return AEEGETPVTBL(_pif, ds_Net_IFirewallManager)->EnableFirewall(_pif, isAllowed);
}

/** @memberof ds_Net_IFirewallManager
  * 
  * This function used to disable firewall on an interface
  * @param _pif Pointer to interface
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_IFirewallManager_DisableFirewall(ds_Net_IFirewallManager* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_IFirewallManager)->DisableFirewall(_pif);
}

/** @memberof ds_Net_IFirewallManager
  * 
  * This function retrieves the firewall rules:
  * @param _pif Pointer to interface
  * @param firewallRules Output Retrieved firewall spec
  * @param firewallRulesLen Length of sequence
  * @param firewallRulesLenReq Required length of sequence
  * @retval ds_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_IFirewallManager_GetFirewallTable(ds_Net_IFirewallManager* _pif, struct ds_Net_IFirewallRule** firewallRules, int firewallRulesLen, int* firewallRulesLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_IFirewallManager)->GetFirewallTable(_pif, firewallRules, firewallRulesLen, firewallRulesLenReq);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Addr_Def.h"
#include "ds_Net_Def.h"
#include "ds_Net_IIPFilterPriv.h"
#include "ds_Errors_Def.h"
#include "ds_Net_IFirewallRule.h"
namespace ds
{
   namespace Net
   {
      const ::AEEIID AEEIID_IFirewallManager = 0x109e2f6;
      
      /** @interface IFirewallManager
        * 
        * DS network Firewall manager interface.
        * This interface manages the firewall rules defined on INetwork object.
        * Application creates INetwork object and call IQI with IIPFirewallMgr
        * This is privileged - internal interface
        */
      struct IFirewallManager : public ::IQI
      {
         struct _IFirewallListType__seq_IIPFilterPriv_Net_ds {
            ::ds::Net::IIPFilterPriv** data;
            int dataLen;
            int dataLenReq;
         };
         typedef _IFirewallListType__seq_IIPFilterPriv_Net_ds IFirewallListType;
         struct _SeqFirewallRulesType__seq_IFirewallRule_Net_ds {
            ::ds::Net::IFirewallRule** data;
            int dataLen;
            int dataLenReq;
         };
         typedef _SeqFirewallRulesType__seq_IFirewallRule_Net_ds SeqFirewallRulesType;
         
         /**
           * This function adds firewall rules on specific DS Network object.
           * @param filter Firewall rule to be defined on the DS Network object.
           * @param firewallRule Output Returned Firewall rule interface.
           * @retval ds::SUCCESS Registration succeeded.
           * @retval ds::EBADOPTS Bad options.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL AddFirewallRule(::ds::Net::IIPFilterPriv* filter, ::ds::Net::IFirewallRule** firewallRule) = 0;
         
         /**
           * This function used to enable firewall on an interface
           * @param isallowed Specify if data is to be passed/dropped.
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors_Def.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL EnableFirewall(boolean isAllowed) = 0;
         
         /**
           * This function used to disable firewall on an interface
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL DisableFirewall() = 0;
         
         /**
           * This function retrieves the firewall rules:
           * @param firewallRules Output Retrieved firewall spec
           * @retval ds::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetFirewallTable(::ds::Net::IFirewallRule** firewallRules, int firewallRulesLen, int* firewallRulesLenReq) = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_IFIREWALLMANAGER_H
