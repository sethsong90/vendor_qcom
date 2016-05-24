#ifndef DS_NET_INATSESSION_H
#define DS_NET_INATSESSION_H
#include "AEEInterface.h"
#if !defined(AEEINTERFACE_CPLUSPLUS)
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Net_Def.h"
struct ds_Net_IPNatStaticEntryType {
   uint8 protocol;
   ds_Net_INAddrType private_ip_addr;
   ds_Net_INPortType private_port;
   ds_Net_INPortType global_port;
};
typedef struct ds_Net_IPNatStaticEntryType ds_Net_IPNatStaticEntryType;
struct ds_Net_DMZEntryType {
   ds_Net_INAddrType dmz_ip_addr;
};
typedef struct ds_Net_DMZEntryType ds_Net_DMZEntryType;
#define ds_Net_AEEIID_INatSession 0x109e2f7
struct ds_Net_INatSession__SeqNatStaticEntryType__seq_IPNatStaticEntryType_Net_ds {
   ds_Net_IPNatStaticEntryType* data;
   int dataLen;
   int dataLenReq;
};
typedef struct ds_Net_INatSession__SeqNatStaticEntryType__seq_IPNatStaticEntryType_Net_ds ds_Net_INatSession__SeqNatStaticEntryType__seq_IPNatStaticEntryType_Net_ds;
typedef ds_Net_INatSession__SeqNatStaticEntryType__seq_IPNatStaticEntryType_Net_ds ds_Net_INatSession_SeqNatStaticEntryType;

/** @interface ds_Net_INatSession
  * 
  * DS network NAT Session Interface.
  */
#define INHERIT_ds_Net_INatSession(iname) \
   INHERIT_IQI(iname); \
   AEEResult (*AddStaticNatEntry)(iname* _pif, const ds_Net_IPNatStaticEntryType* entry); \
   AEEResult (*DeleteStaticNatEntry)(iname* _pif, const ds_Net_IPNatStaticEntryType* entry); \
   AEEResult (*GetStaticNatEntry)(iname* _pif, ds_Net_IPNatStaticEntryType* entries, int entriesLen, int* entriesLenReq); \
   AEEResult (*SetDynamicNatEntryTimeout)(iname* _pif, unsigned short timeout); \
   AEEResult (*GetDynamicNatEntryTimeout)(iname* _pif, unsigned short* timeout); \
   AEEResult (*SetIpSecVpnPassThrough)(iname* _pif, boolean isVpnPassThrough); \
   AEEResult (*GetIpSecVpnPassThrough)(iname* _pif, boolean* isVpnPassThrough); \
   AEEResult (*SetL2TPVpnPassThrough)(iname* _pif, boolean isVpnPassThrough); \
   AEEResult (*GetL2TPVpnPassThrough)(iname* _pif, boolean* isVpnPassThrough); \
   AEEResult (*SetPPTPVpnPassThrough)(iname* _pif, boolean isVpnPassThrough); \
   AEEResult (*GetPPTPVpnPassThrough)(iname* _pif, boolean* isVpnPassThrough); \
   AEEResult (*AddDMZ)(iname* _pif, const ds_Net_DMZEntryType* entry); \
   AEEResult (*GetDMZ)(iname* _pif, ds_Net_DMZEntryType* entry); \
   AEEResult (*DeleteDMZ)(iname* _pif)
AEEINTERFACE_DEFINE(ds_Net_INatSession);

/** @memberof ds_Net_INatSession
  * 
  * Increment the object's reference count.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INatSession_AddRef(ds_Net_INatSession* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->AddRef(_pif);
}

/** @memberof ds_Net_INatSession
  * 
  * Decrement the object's reference count.  If zero, free the object.
  *
  * @param _pif Pointer to interface
  **/
static __inline uint32 ds_Net_INatSession_Release(ds_Net_INatSession* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->Release(_pif);
}

/** @memberof ds_Net_INatSession
  * 
  * Detect if an object implements the requested interface.
  *
  * @param _pif Pointer to interface
  * @param iqi_iid Interface ID of requested interface
  * @param iqi Requested interface.
  */
static __inline int ds_Net_INatSession_QueryInterface(ds_Net_INatSession* _pif, AEEIID iqi_iid, void** iqi)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->QueryInterface(_pif, iqi_iid, iqi);
}

/** @memberof ds_Net_INatSession
  * 
  * This function add static NAT entry.
  * @param _pif Pointer to interface
  * @param entry Specify the static NAT entry.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_AddStaticNatEntry(ds_Net_INatSession* _pif, const ds_Net_IPNatStaticEntryType* entry)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->AddStaticNatEntry(_pif, entry);
}

/** @memberof ds_Net_INatSession
  * 
  * This function delete static NAT entry.
  * @param _pif Pointer to interface
  * @param entry Specify the static NAT entry.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_DeleteStaticNatEntry(ds_Net_INatSession* _pif, const ds_Net_IPNatStaticEntryType* entry)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->DeleteStaticNatEntry(_pif, entry);
}

/** @memberof ds_Net_INatSession
  * 
  * This function get static NAT entries.
  * @param _pif Pointer to interface
  * @param entries Output the static NAT entries.
  * @param entriesLen Length of sequence
  * @param entriesLenReq Required length of sequence
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_GetStaticNatEntry(ds_Net_INatSession* _pif, ds_Net_IPNatStaticEntryType* entries, int entriesLen, int* entriesLenReq)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->GetStaticNatEntry(_pif, entries, entriesLen, entriesLenReq);
}

/** @memberof ds_Net_INatSession
  * 
  * This function set dynamic NAT entry timeout.
  * @param _pif Pointer to interface
  * @param timeout Specify the dynamic NAT entry timeout in sec.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_SetDynamicNatEntryTimeout(ds_Net_INatSession* _pif, unsigned short timeout)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->SetDynamicNatEntryTimeout(_pif, timeout);
}

/** @memberof ds_Net_INatSession
  * 
  * This function get dynamic NAT entry timeout.
  * @param _pif Pointer to interface
  * @param timeout Output the dynamic NAT entry timeout in sec.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_GetDynamicNatEntryTimeout(ds_Net_INatSession* _pif, unsigned short* timeout)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->GetDynamicNatEntryTimeout(_pif, timeout);
}

/** @memberof ds_Net_INatSession
  * 
  * This function set IPSEC VPN passthrough.
  * @param _pif Pointer to interface
  * @param isVpnPassThrough Specify whether VPN passthrough is enabled or not.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_SetIpSecVpnPassThrough(ds_Net_INatSession* _pif, boolean isVpnPassThrough)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->SetIpSecVpnPassThrough(_pif, isVpnPassThrough);
}

/** @memberof ds_Net_INatSession
  * 
  * This function get IPSEC VPN passthrough.
  * @param _pif Pointer to interface
  * @param isVpnPassThrough Specify whether VPN passthrough is enabled or not.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_GetIpSecVpnPassThrough(ds_Net_INatSession* _pif, boolean* isVpnPassThrough)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->GetIpSecVpnPassThrough(_pif, isVpnPassThrough);
}

/** @memberof ds_Net_INatSession
  * 
  * This function sets L2TP VPN passthrough.
  * @param _pif Pointer to interface
  * @param isVpnPassThrough Specify whether L2TP VPN passthrough is enabled or not.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_SetL2TPVpnPassThrough(ds_Net_INatSession* _pif, boolean isVpnPassThrough)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->SetL2TPVpnPassThrough(_pif, isVpnPassThrough);
}

/** @memberof ds_Net_INatSession
  * 
  * This function gets L2TP VPN passthrough.
  * @param _pif Pointer to interface
  * @param isVpnPassThrough Specify whether L2TP VPN passthrough is enabled or not.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_GetL2TPVpnPassThrough(ds_Net_INatSession* _pif, boolean* isVpnPassThrough)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->GetL2TPVpnPassThrough(_pif, isVpnPassThrough);
}

/** @memberof ds_Net_INatSession
  * 
  * This function sets PPTP VPN passthrough.
  * @param _pif Pointer to interface
  * @param isVpnPassThrough Specify whether PPTP VPN passthrough is enabled or not.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_SetPPTPVpnPassThrough(ds_Net_INatSession* _pif, boolean isVpnPassThrough)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->SetPPTPVpnPassThrough(_pif, isVpnPassThrough);
}

/** @memberof ds_Net_INatSession
  * 
  * This function gets PPTP VPN passthrough.
  * @param _pif Pointer to interface
  * @param isVpnPassThrough Specify whether PPTP VPN passthrough is enabled or not.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_GetPPTPVpnPassThrough(ds_Net_INatSession* _pif, boolean* isVpnPassThrough)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->GetPPTPVpnPassThrough(_pif, isVpnPassThrough);
}

/** @memberof ds_Net_INatSession
  * 
  * This function enables DMZ and creates a DMZ entry.
  * @param _pif Pointer to interface
  * @param entry Specify the DMZ entry.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_AddDMZ(ds_Net_INatSession* _pif, const ds_Net_DMZEntryType* entry)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->AddDMZ(_pif, entry);
}

/** @memberof ds_Net_INatSession
  * 
  * This function gets a DMZ entry.
  * @param _pif Pointer to interface
  * @param entry Specify the DMZ entry.
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_GetDMZ(ds_Net_INatSession* _pif, ds_Net_DMZEntryType* entry)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->GetDMZ(_pif, entry);
}

/** @memberof ds_Net_INatSession
  * 
  * This function is used to delete DMZ
  * @param _pif Pointer to interface
  * @retval DS_SUCCESS Request received successfully.
  * @retval Other DS designated error codes might be returned.
  * @see ds_Errors.idl.
  */
static __inline AEEResult ds_Net_INatSession_DeleteDMZ(ds_Net_INatSession* _pif)
{
   return AEEGETPVTBL(_pif, ds_Net_INatSession)->DeleteDMZ(_pif);
}
#else /* C++ */
#include "AEEStdDef.h"
#include "AEEIQI.h"
#include "ds_Net_Def.h"
namespace ds
{
   namespace Net
   {
      struct IPNatStaticEntryType {
         ::uint8 protocol;
         INAddrType private_ip_addr;
         INPortType private_port;
         INPortType global_port;
      };
      struct DMZEntryType {
         INAddrType dmz_ip_addr;
      };
      const ::AEEIID AEEIID_INatSession = 0x109e2f7;
      
      /** @interface INatSession
        * 
        * DS network NAT Session Interface.
        */
      struct INatSession : public ::IQI
      {
         struct _SeqNatStaticEntryType__seq_IPNatStaticEntryType_Net_ds {
            ::ds::Net::IPNatStaticEntryType* data;
            int dataLen;
            int dataLenReq;
         };
         typedef _SeqNatStaticEntryType__seq_IPNatStaticEntryType_Net_ds SeqNatStaticEntryType;
         
         /**
           * This function add static NAT entry.
           * @param entry Specify the static NAT entry.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL AddStaticNatEntry(const ::ds::Net::IPNatStaticEntryType* entry) = 0;
         
         /**
           * This function delete static NAT entry.
           * @param entry Specify the static NAT entry.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL DeleteStaticNatEntry(const ::ds::Net::IPNatStaticEntryType* entry) = 0;
         
         /**
           * This function get static NAT entries.
           * @param entries Output the static NAT entries.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetStaticNatEntry(::ds::Net::IPNatStaticEntryType* entries, int entriesLen, int* entriesLenReq) = 0;
         
         /**
           * This function set dynamic NAT entry timeout.
           * @param timeout Specify the dynamic NAT entry timeout in sec.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetDynamicNatEntryTimeout(unsigned short timeout) = 0;
         
         /**
           * This function get dynamic NAT entry timeout.
           * @param timeout Output the dynamic NAT entry timeout in sec.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetDynamicNatEntryTimeout(unsigned short* timeout) = 0;
         
         /**
           * This function set IPSEC VPN passthrough.
           * @param isVpnPassThrough Specify whether VPN passthrough is enabled or not.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetIpSecVpnPassThrough(boolean isVpnPassThrough) = 0;
         
         /**
           * This function get IPSEC VPN passthrough.
           * @param isVpnPassThrough Specify whether VPN passthrough is enabled or not.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetIpSecVpnPassThrough(boolean* isVpnPassThrough) = 0;
         
         /**
           * This function sets L2TP VPN passthrough.
           * @param isVpnPassThrough Specify whether L2TP VPN passthrough is enabled or not.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetL2TPVpnPassThrough(boolean isVpnPassThrough) = 0;
         
         /**
           * This function gets L2TP VPN passthrough.
           * @param isVpnPassThrough Specify whether L2TP VPN passthrough is enabled or not.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetL2TPVpnPassThrough(boolean* isVpnPassThrough) = 0;
         
         /**
           * This function sets PPTP VPN passthrough.
           * @param isVpnPassThrough Specify whether PPTP VPN passthrough is enabled or not.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL SetPPTPVpnPassThrough(boolean isVpnPassThrough) = 0;
         
         /**
           * This function gets PPTP VPN passthrough.
           * @param isVpnPassThrough Specify whether PPTP VPN passthrough is enabled or not.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetPPTPVpnPassThrough(boolean* isVpnPassThrough) = 0;
         
         /**
           * This function enables DMZ and creates a DMZ entry.
           * @param entry Specify the DMZ entry.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL AddDMZ(const ::ds::Net::DMZEntryType* entry) = 0;
         
         /**
           * This function gets a DMZ entry.
           * @param entry Specify the DMZ entry.
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL GetDMZ(::ds::Net::DMZEntryType* entry) = 0;
         
         /**
           * This function is used to delete DMZ
           * @retval DS::SUCCESS Request received successfully.
           * @retval Other DS designated error codes might be returned.
           * @see ds_Errors.idl.
           */
         virtual ::AEEResult AEEINTERFACE_CDECL DeleteDMZ() = 0;
      };
   };
};
#endif /* !defined(AEEINTERFACE_CPLUSPLUS) */
#endif //DS_NET_INATSESSION_H
