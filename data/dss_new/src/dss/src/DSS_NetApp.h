#ifndef __DSS_NETAPP_H__
#define __DSS_NETAPP_H__

/*======================================================

FILE:  DSS_NetApp.h

SERVICES:
Network Application class in Backward Compatibility Layer

GENERAL DESCRIPTION:

=====================================================

Copyright (c) 2008-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_NetApp.h#3 $
  $DateTime: 2011/08/02 03:56:27 $$Author: eazriel $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/


//===================================================================
//   Includes and Public Data Declarations
//===================================================================

//-------------------------------------------------------------------
// Include Files
//-------------------------------------------------------------------
#include "comdef.h"
#include "customer.h"
#include "dssocket.h"

#include "AEEStdErr.h"
#include "ds_Net_INetworkPriv.h"
#include "ds_Net_INetworkExt2.h"
#include "ds_Net_IQoSManager.h"
#include "ds_Net_IPolicy.h"
#include "ds_Net_IFirewallRule.h"
#include "ds_Net_INatSession.h"

#include "AEEISignal.h"
#include "AEEISignalCtl.h"

#include "AEEICritSect.h"
#include "DSS_CritScope.h"

#include "DSS_Common.h"
#include "DSS_NetQoSDefault.h"
#include "DSS_NetQoSSecondary.h"

#include "DSS_MCast.h"
#include "ds_Net_IMCastMBMSCtrlPriv.h"
#include "ds_Net_IIPv6Address.h"
#include "ds_Net_ITechUMTS.h"
#include "ds_Net_IPhysLink.h"

#include "ds_Utils_CSSupport.h"
#include "ds_Utils_SignalHandlerBase.h"

//-------------------------------------------------------------------
// Constant / Define Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Type Declarations (typedef, struct, enum, etc.)
//-------------------------------------------------------------------
typedef uint32 FlowID;

//-------------------------------------------------------------------
// Global Constant Data Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Global Data Declarations
//-------------------------------------------------------------------


//-------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------
class DSSMCast;
class DSSNetMCastMBMSCtrl;
//===================================================================
//              Macro Definitions
//===================================================================


//===================================================================
//              Class Definitions
//===================================================================

//===================================================================
//  CLASS:      DSSNetApp
//
//  DESCRIPTION:
//
//  HOW TO USE:
//
//===================================================================

enum NetHandleType
{
   NETLIB1,
   NETLIB2
};

// Forward declarations to prevent circular inclusion of DSSNetApp.
class DSSEventHandler;
class DSSNetworkStateHandler;
class DSSNetworkIPHandler;
class DSSExtendedIPConfigHandler;
class DSSRFConditionsHandler;
class DSSBearerTechHandler;
class DSSOutageHandler;
class DSSPhysLinkStateHandler;
class DSSQoSAwareUnAwareHandler;
class DSSNetQoSSecondary;
class DSSNetQoSDefault;
class DSSPrimaryQoSModifyHandler;
class DSSPrimaryQoSModifyStatusHandler;
class DSSMTPDRequestHandler;
class DSSNetMCastMBMSCtrl;
class DSSSlottedResultHandler;
class DSSSlottedSessionChangedHandler;
class DSSHDRRev0RateInteriaHandler;
class DSSQoSProfileChangedHandler;
class DSSPrivIpv6Addr;
class DSSIPv6PrefixChangedStateHandler;

class DSSNetApp: public ds::Utils::SignalHandlerBase
{
//-------------------------------------------------------------------
//  Constructors/Desctructors
//-------------------------------------------------------------------
  public:
    DSSNetApp();
    virtual ~DSSNetApp() throw() {};
    virtual void Destructor(void) throw();

//-------------------------------------------------------------------
//  Get/Set functions for protected members access
//
//-------------------------------------------------------------------
     // TODO: maybe the functions should return ::AEEResult
    inline void GetNetHandle(sint15* pNetHandle)
    {
       *pNetHandle = mNetHandle;
    }
    inline void SetNetHandle(sint15 netHandle)
    {
       mNetHandle = netHandle;
    }

    inline void GetIfaceId(dss_iface_id_type* pIfaceId)
    {
       *pIfaceId = mIfaceId;
    }
    inline void SetIfaceId(dss_iface_id_type ifaceId)
    {
       mIfaceId = ifaceId;
    }

    inline void GetEventReportIfaceId(dss_iface_id_type* pIfaceId)
    {
      *pIfaceId = mEventReportIfaceId;
    }

    AEEResult GetIDSNetworkPrivObject(ds::Net::INetworkPriv** ppIDSNetworkPriv);

    inline void SetIDSNetworkPrivObject(ds::Net::INetworkPriv* pIDSNetworkPriv)
    {
       DSSCritScope cs(*mpCritSect);

       if (NULL != mpIDSNetworkPriv) {
          // This should never happened
          // IDSNetworkPriv is allocated and set together with the allocation of the DSSNetApp.
          // It is destroyed upon DSSNetApp destruction
          LOG_MSG_ERROR("SetIDSNetworkPrivObject: mpIDSNetworkPriv is NULL",0,0,0);
          ASSERT(0);
       }

       mpIDSNetworkPriv = pIDSNetworkPriv;
       (void)mpIDSNetworkPriv->AddRef();     // This shall keep the IDSNetworkPriv object alive until released in DSSNetApp DTor.
    }

    AEEResult GetIDSNetworkObject(ds::Net::INetwork** ppIDSNetwork);

    inline void SetIDSNetworkObject(ds::Net::INetwork* pIDSNetwork)
    {
       DSSCritScope cs(*mpCritSect);

       if (NULL != mpIDSNetwork) {
          // This should never happened
          // IDSNetworkPriv is allocated and set together with the allocation of the DSSNetApp.
          // It is destroyed upon DSSNetApp destruction
          LOG_MSG_ERROR("SetIDSNetworkObject: mpIDSNetwork is NULL",0,0,0);
          ASSERT(0);
       }

       mpIDSNetwork = pIDSNetwork;
       (void)mpIDSNetwork->AddRef();     // This shall keep the IDSNetwork object alive until released in DSSNetApp DTor.
    }

    AEEResult GetIDSNetworkExtObject(ds::Net::INetworkExt** ppIDSNetworkExt);

    inline void SetIDSNetworkExtObject(ds::Net::INetworkExt* pIDSNetworkExt)
    {
       DSSCritScope cs(*mpCritSect);

       if (NULL != mpIDSNetworkExt) {
          // This should never happened
          // IDSNetworkPriv is allocated and set together with the allocation of the DSSNetApp.
          // It is destroyed upon DSSNetApp destruction
          LOG_MSG_ERROR("SetIDSNetworkExtObject: mpIDSNetworkExt is NULL",0,0,0);
          ASSERT(0);
       }

       mpIDSNetworkExt = pIDSNetworkExt;
       (void)mpIDSNetworkExt->AddRef();     // This shall keep the IDSNetworkExt object alive until released in DSSNetApp DTor.
    }

    AEEResult GetIDSNetworkExt2Object(ds::Net::INetworkExt2** ppIDSNetworkExt2);

    inline void SetIDSNetworkExt2Object(ds::Net::INetworkExt2* pIDSNetworkExt2)
    {
      DSSCritScope cs(*mpCritSect);

      if (NULL != mpIDSNetworkExt2) {
        // This should never happened
        // IDSNetworkPriv is allocated and set together with the allocation of the DSSNetApp.
        // It is destroyed upon DSSNetApp destruction
        LOG_MSG_ERROR("SetIDSNetworkExt2Object: mpIDSNetworkExt2 is NULL",0,0,0);
        ASSERT(0);
      }

      mpIDSNetworkExt2 = pIDSNetworkExt2;
      (void)mpIDSNetworkExt2->AddRef();     // This shall keep the IDSNetworkExt2 object alive until released in DSSNetApp DTor.
    }

    inline AEEResult GetNetworkCallback(dss_net_cb_fcn* pNetCb)
    {
       *pNetCb = mNetCb;
       return AEE_SUCCESS;
    }
    inline void SetNetworkCallback(dss_net_cb_fcn netCb)
    {
       mNetCb = netCb;
    }

    inline AEEResult GetNetworkUserData(void**  ppNetUserData)
    {
       *ppNetUserData = mNetUserData;
       return AEE_SUCCESS;
    }
    inline void SetNetworkUserData(void*  pNetUserData)
    {
       mNetUserData = pNetUserData;
    }

    // dss_sock_cb_fcn_type includes the user data.
    inline void GetSocketCallback(dss_sock_cb_fcn_type* pSockCb)
    {
       *pSockCb = mSockCb;
    }
    // dss_sock_cb_fcn_type includes the user data.
    inline void SetSocketCallback(dss_sock_cb_fcn_type* pSockCb)
    {
       mSockCb = *pSockCb;
    }

    AEEResult GetPolicy(ds::Net::IPolicy** ppIDSNetPolicy);

    inline void GetIpSecDisabled(boolean *pbIpSecDisabled)
    {
       *pbIpSecDisabled = mbIpsecDisabled;
    }

    inline void SetIpSecDisabled(boolean bIpSecDisabled)
    {
       mbIpsecDisabled = bIpSecDisabled;
    }

    inline AEEResult SetLegacyPolicy(dss_net_policy_info_type * from)
    {
       if (NULL == from) {
          SetIpSecDisabled(FALSE);
       } else {
         SetIpSecDisabled(from->ipsec_disabled);
       }

       return AEE_SUCCESS;
    }

    inline AEEResult GetLegacyPolicy(dss_net_policy_info_type * to)
    {
       GetIpSecDisabled(&(to->ipsec_disabled));
       return AEE_SUCCESS;
    }

    inline void GetNumOfSockets(int *pnNumOfSockets)
    {
       *pnNumOfSockets = mnNumOfSockets;
    }

    inline void SetNumOfSockets(int nNumOfSockets)
    {
       mnNumOfSockets = nNumOfSockets;
    }

    inline void GetNetworkIsUp(boolean *pbNetworkIsUp)
    {
       *pbNetworkIsUp = mbNetworkIsUp;
    }

    inline void SetNetworkIsUp(boolean bNetworkIsUp)
    {
       mbNetworkIsUp = bNetworkIsUp;
    }

    inline void GetIsPPPOpen(boolean *pbIsPPPOpen)
    {
       *pbIsPPPOpen = mbIsPPPOpen;
    }

    inline void SetIsPPPOpen(boolean pbIsPPPOpen)
    {
       mbIsPPPOpen = pbIsPPPOpen;
    }

    inline void ResetLastIfaceStateSentToApp(void)
    {
       DSSCritScope cs(*mpCritSect);
       
       mbLastIfaceStateSentToApp = IFACE_STATE_INVALID;
    }
    
    inline AEEResult GetMTPDIsRegistered(boolean* bMTPDIsReg)
    {
       if (NULL == mpMTPDRequestHandler)
       {
         *bMTPDIsReg = FALSE;
       }
       else
       {
          *bMTPDIsReg = TRUE;
       }

       return AEE_SUCCESS;
    }

    inline void SetIDSNetwork1xPrivObject
    (
       ds::Net::INetwork1xPriv* pIDSNetwork1xPriv
    )
    {
       // This is just a precaution and not really needed if application
       // is calling the relevant iface ioctl from a single thread
       DSSCritScope cs(*mpCritSect);                                   

       if (NULL == pIDSNetwork1xPriv) 
       {
          LOG_MSG_ERROR( "SetIDSNetwork1xPrivObject: Input arg"
                         "pIDSNetwork1xPriv is NULL",0, 0, 0);   
          return;  
       }

       if (NULL != mpIDSNetwork1xPriv) 
       {
          LOG_MSG_INVALID_INPUT( "SetIDSNetwork1xPrivObject: member variable"
                                 " already set to 0x%p, "
                                 "Input arg pIDSNetwork1xPriv is 0x%p", 
                                  mpIDSNetwork1xPriv, pIDSNetwork1xPriv, 0);   
          return;                    
       }

       mpIDSNetwork1xPriv = pIDSNetwork1xPriv;
       // This shall keep the IDSNetworkPriv object alive until 
       // released in DSSNetApp DTor.
       (void)mpIDSNetwork1xPriv->AddRef();     
    }
	
    AEEResult RegAutoEvents();
    AEEResult SetPolicy(ds::Net::IPolicy* pIDSNetPolicy);

    // The following member functions serve iface_ioctl ds API. They return int as iface_ioctl returns int.
    AEEResult GetIPv4Addr(dss_iface_ioctl_ipv4_addr_type* pIpv4Addr);
    AEEResult GetIPv6Addr(dss_iface_ioctl_ipv6_addr_type* pIpv6Addr);
    AEEResult GetIPv4PrimDnsAddr(dss_iface_ioctl_ipv4_prim_dns_addr_type* pIpv4Addr);
    AEEResult GetIPv6PrimDnsAddr(dss_iface_ioctl_ipv6_prim_dns_addr_type* pIpv6Addr);
    AEEResult GetIPv4SecoDnsAddr(dss_iface_ioctl_ipv4_seco_dns_addr_type* pIpv4Addr);
    AEEResult GetIPv6SecoDnsAddr(dss_iface_ioctl_ipv6_seco_dns_addr_type* pIpv6Addr);
    AEEResult GetNatPublicIPAddress(dss_iface_ioctl_nat_public_ip_addr_type* pIpAddr);
    AEEResult GetAllDnsAddrs(dss_iface_ioctl_get_all_dns_addrs_type* pDnsAddrs);
    AEEResult GetMtu(dss_iface_ioctl_mtu_type* pMtu);
    AEEResult GetIPAddress(dss_iface_ioctl_get_ip_addr_type* pIpAddr);
    // TODO: maybe it should be declared separately since I'm not sure if it serves a specific ioctl
    AEEResult GetPreviousIPAddress(dss_iface_ioctl_get_ip_addr_type* pIpAddr);
    AEEResult GetIfaceState(dss_iface_ioctl_state_type* pIfaceState);

   AEEResult RegEventCB(dss_iface_ioctl_ev_cb_type* pEvArg);
   AEEResult RegEventCB(dss_iface_ioctl_event_enum_type event, dss_iface_ioctl_event_cb event_cb,
                        void* user_data_ptr);
   AEEResult RegEventCBPrimary(dss_iface_ioctl_event_enum_type event, dss_iface_ioctl_event_cb event_cb,
                               void* user_data_ptr);
   AEEResult DeregEventCB(dss_iface_ioctl_ev_cb_type* pEvArg);
   AEEResult DeregEventCB(dss_iface_ioctl_event_enum_type event);
   AEEResult GetRFConditions(dss_iface_ioctl_rf_conditions_type* pRFConds);
   AEEResult GetBearerTech(dss_iface_ioctl_bearer_tech_type* pBearerTech);
   // TODO: maybe it should be declared separately since I'm not sure if it serves a specific ioctl
   AEEResult GetPreviousBearerTech(dss_iface_ioctl_bearer_tech_type* pBearerTech);
   AEEResult GetNetQoSManager(ds::Net::IQoSManager** ppNetQosManager);
   AEEResult SetNetQoSManager(ds::Net::IQoSManager* pNetQosManager);
   AEEResult GetNetMCastManager(ds::Net::IMCastManager** ppNetMCastManager);
   AEEResult SetNetMCastManager(ds::Net::IMCastManager* pNetMCastManager);
   AEEResult GetNetFirewallManager(ds::Net::IFirewallManager** ppNetFirewallManager);
   AEEResult SetNetFirewallManager(ds::Net::IFirewallManager* pNetFirewallManager);
   AEEResult GetNetNatSession(ds::Net::INatSession** ppNetNatSession);
   AEEResult SetNetNatSession(ds::Net::INatSession* pNetNatSession);
   AEEResult SetQoSDefault(ds::Net::IQoSDefault* pNetQoSDefault);
   AEEResult GetQoSDefault(ds::Net::IQoSDefault** ppNetQoSDefault);
   AEEResult AddNetQoSSecondary(ds::Net::IQoSSecondary* pNetQoSSecondary,
                                uint32 flowID,
                                dss_iface_ioctl_event_cb cback_fn,
                                void* user_data);
   AEEResult AddIpv6PrivAddr(ds::Net::IIPv6Address* pNetIpv6Address,
                             uint32 flowID,
                             boolean isUnique,
                             dss_iface_ioctl_event_cb cback_fn,
                             void* user_data);
   AEEResult GetDSSNetQoSSecondary(uint32 iface_id, DSSNetQoSSecondary** ppDSSNetQoSSecondary);
   AEEResult RemoveDSSNetQoSSecondary(uint32 iface_id);
   AEEResult RemoveDSSPrivIpv6Addr(ds::Net::IIPv6Address *pIDSNetIpv6Address);
   AEEResult RemoveDSSMCast(uint32 iface_id, ds::Net::EventType handlerType);
   AEEResult RemoveDSSMcastMBMSCtrl(ds::Net::IMCastMBMSCtrlPriv* pIDSNetMCastMBMS);
   AEEResult AddDSSMCast(ds::Net::IMCastSessionPriv* pMCastSession, uint32 flowID, dss_iface_ioctl_event_cb event_cb, void *user_data_ptr);
   AEEResult GetDSSMCast(uint32 iface_id, DSSMCast** ppDSSMCast);
   AEEResult GetPhysLinkObject(uint32 ifaceid,ds::Net::IPhysLink **pPhyslink);
   AEEResult RegMTPDEventCB(dss_iface_ioctl_mt_reg_cb_type* pEvArg);
   AEEResult DeRegMTPDEventCB(dss_iface_ioctl_mt_dereg_cb_type* pEvArg);
   AEEResult AddDSSMCastMBMSCtrl(ds::Net::IMCastMBMSCtrlPriv* pMCastMBMSCtrl, uint32 MCastMBMSHandle, dss_iface_ioctl_event_cb event_cb, void *user_data_ptr);
   AEEResult GetMCastMBMSCtrl(uint32 iface_id, ds::Net::IMCastMBMSCtrlPriv** ppDSSMCastMBMSCtrl);
   AEEResult AddDSSFirewallRule(ds::Net::IFirewallRule* pIFirewallRule, uint32* pFirewallRuleHandle);
   AEEResult GetDSSFirewallRule(uint32 firewallRuleHandle, ds::Net::IFirewallRule** ppIFirewallRule);
   AEEResult DeleteDSSFirewallRule(uint32 firewallRuleHandle);
   AEEResult FlushDSSFirewallRuleTable(void);
   AEEResult Init(ds::Net::INetworkPriv* pIDSNetworkPriv);
   AEEResult GetPhysLink(ds::Net::IPhysLink** pIDSPhysLink);
   AEEResult GetTechUMTS(ds::Net::ITechUMTS** pIDSTechUMTS);
   AEEResult GetNetworkStatistics(dss_iface_ioctl_get_iface_stats_type* pStats);
   AEEResult ResetNetworkStatistics(void);
   void ReleasePhysLink(void);

   // Frees up the memory used by mpDSSQoSSecondaryList mpDSSMCastList and mpDSSNetMCastMBMSList
   void FreeLists();

   // Removes primary qos modify status handler (default qos handler)
   void RemovePrimaryQoSModifyStatusHandler();


//-------------------------------------------------------------------
//  Protected members
//-------------------------------------------------------------------
  protected:

//-------------------------------------------------------------------
//  Data members
//-------------------------------------------------------------------
  protected:
     // TODO: I am not sure that we need to store such a field, since it is just the index of the array
      sint15 mNetHandle;                        // nethandle representing this network library connection.

      dss_iface_id_type   mIfaceId;             // interface id
      dss_iface_id_type   mEventReportIfaceId;  // interface id for event reporting - in case 
                                                // mIfaceId is cleared by IFACE_DOWN event

      ds::Net::INetworkPriv* mpIDSNetworkPriv;  // corresponding IDSNetworkPriv object for the nethandle.
      ds::Net::INetwork* mpIDSNetwork;
      ds::Net::INetworkExt* mpIDSNetworkExt;    // corresponding IDSNetworkExt object for the nethandle.
      ds::Net::INetworkExt2* mpIDSNetworkExt2;    // corresponding IDSNetworkExt2 object for the nethandle.

      dss_net_cb_fcn mNetCb;                    // network callback function
                                                // TODO: declare dss_net_cb_fcn in dssocket.h
                                                //       with extern "C" {} so it can be correctly
                                                //       called from DSS to the application

      void *  mNetUserData;                     // User data for network callback (relevant if this
                                                // DSSNetApp object was created when app called
                                                // dsnet_get_handle
      dss_sock_cb_fcn_type mSockCb;             // socket callback function + User Data

      ds::Net::IPolicy* mpIDSNetPolicy;
      ds::Net::IQoSManager* mpDSNetQoSManager;
      DSSNetQoSSecondary* mpDSSQoSSecondaryList;
      DSSPrivIpv6Addr* mpDSSPrivIpv6AddrList;
      DSSNetQoSDefault* mpDSSNetQoSDefault;
      DSSMCast* mpDSSMCastList;
      DSSNetMCastMBMSCtrl* mpDSSNetMCastMBMSList;
      ds::Net::IMCastManager* mpDSNetMCastManager;
      ds::Net::IFirewallManager* mpDSNetFirewallManager;
      ds::Net::INatSession*  mpDSNetNatSession;
      ds::Net::INetwork1xPriv* mpIDSNetwork1xPriv;

      //TODO: This mapping can be enhanced to use a list instead.
      #define DSS_MAX_FIREWALL_HANDLES 10
      struct DSSFirewallHandleToObjectMapping
      {
        uint32                    handle;
        ds::Net::IFirewallRule *  pIFirewallRule;
      } DSSFirewallHandleToObject[DSS_MAX_FIREWALL_HANDLES];

      DSSNetApp* mNext;                              // DSSNetApp list

      // TODO: int?
      int mnNumOfSockets;

      // Need to add methods for the secondary structures management.
      // TBD Need to add support to query the DSSNetApp about if it was created by open_netlib1 or 2.

      // These fields store the event handlers.
      // TODO: they should be zero-initialized on construction and released in the dtor
      DSSNetworkStateHandler* mpNetStateHandler;
      DSSNetworkIPHandler* mpNetIPHandler;
      DSSExtendedIPConfigHandler* mpExtendedIPConfigHandler;
      DSSRFConditionsHandler* mpRFConditionsHandler;
      DSSBearerTechHandler* mpBearerTechHandler;
      DSSOutageHandler* mpOutageHandler;
      DSSPhysLinkStateHandler* mpPhysLinkStateHandler;
      DSSQoSAwareUnAwareHandler* mpQoSAwareUnAwareHandler;
      DSSPrimaryQoSModifyHandler* mpPrimaryQoSModifyHandler;
      DSSPrimaryQoSModifyStatusHandler* mpPrimaryQoSModifyStatusHandler;
      DSSMTPDRequestHandler* mpMTPDRequestHandler;
      DSSSlottedResultHandler* mpSlottedResultHandler;
      DSSSlottedSessionChangedHandler* mpSlottedSessionChangedHandler;
      DSSQoSProfileChangedHandler* mpQoSProfileChangedHandler;
      DSSHDRRev0RateInteriaHandler* mpHDRRev0RateInteriaHandler;
      DSSIPv6PrefixChangedStateHandler* mpDSSIPv6PrefixChangedStateHandler;

      boolean  mbIpsecDisabled;  /* Is IPSEC disabled ? */


      ISignal* mpIfaceUpOrDownSignal;

      ISignalCtl* mpIfaceUpOrDownSignalCtl;

      // TODO: need to ensure everything that should be locked is locked.
      ICritSect* mpCritSect;

      AEEResult GetEventHandler(dss_iface_ioctl_event_enum_type event,
                                DSSEventHandler** ppEventHandler, bool bInit);
      template<typename HandlerType>
      AEEResult FetchHandler(HandlerType** pHandler, DSSEventHandler** ppEventHandler, bool bInit);

      static void IfaceUpOrDownSignalFcn(void *pNetAppInstance);
      static AEEResult SetIfaceIDOnIfaceUPEvent(ds::Net::INetwork* pIDSNetwork, DSSNetApp* pNetApp, sint15 appid);

      boolean mbNetworkIsUp;
      boolean mbIsPPPOpen;
      dss_iface_ioctl_state_type mbLastIfaceStateSentToApp;

   private:
      FlowID ExtractFlowIDFromIFaceID(uint32 ifaceid);
      boolean mbAutoEventsRegistered;
      IQI* mRegObj; // hold the IQI object returned by the OnStateChange function

      ds::Net::IPhysLink*     mpDSNetPhysLink;
      ds::Net::ITechUMTS*     mpDSNetTechUMTS;
};

//===================================================================

#endif // __DSS_NETAPP_H__

