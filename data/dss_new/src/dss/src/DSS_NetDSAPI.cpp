/*======================================================

FILE:  DSS_NetDSAPI.cpp

SERVICES:

GENERAL DESCRIPTION:
Implementation of Network related ds functions.

=====================================================

Copyright (c) 2008 - 2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_NetDSAPI.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "customer.h"
#include "dssocket.h"
#include "dssinternal.h"

#include "DSS_Common.h"

#include "DSS_NetApp.h"
#include "DSS_Globals.h"
#include "DSS_Conversion.h"
#include "DSS_IDSNetworkPrivScope.h"
#include "DSS_IDSNetworkScope.h"
#include "DSS_IDSNetPolicyPrivScope.h"
#include "DSS_IDSNetPolicyScope.h"
#include "DSS_MemoryManagement.h"

#include "ds_Net_INetworkPriv.h"
#include "ds_Addr_Def.h"

#include "ds_Net_INetworkFactoryPriv.h"
#include "ds_Net_CNetworkFactory.h"
#include "ds_Net_CNetworkFactoryPriv.h"

#include "ds_Net_IIPFilterManagerPriv.h"
#include "ps_policy_mgr.h"
#include "ds_Net_CreateInstance.h"

#if ( defined(FEATURE_DATACOMMON_PACKAGE_SINGLE_PROC) || defined(FEATURE_DATACOMMON_2H09_2_SINGLE_PROC_MDM) )
extern "C" {
#include "ps_policyi_mgr.h"
}
#endif

#define DSS_NETPOLICY_COOKIE (0x12343210L)

using namespace ds::Net;

sint15 dss_get_app_net_policy
(
  sint15 appid,                                          /* Application id */
  dss_net_policy_info_type * policy_info_ptr,     /* policy info structure */
  sint15 * dss_errno                                       /* error number */
)
{
   DSSNetApp *pNetApp = NULL;
   DSSIDSNetPolicyScope IDSNetPolicyScope;

   LOG_MSG_FUNCTION_ENTRY("dss_get_app_net_policy(): app_id: %d, policy_ptr:0x%p", appid, policy_info_ptr, 0);

   IDS_ERR_RET_ERRNO(DSSGlobals::Instance()->GetNetApp(appid, &pNetApp));
   IDS_ERR_RET_ERRNO(IDSNetPolicyScope.Init(pNetApp));
   IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSNetPolicy(IDSNetPolicyScope.Fetch(), policy_info_ptr, appid));
   IDS_ERR_RET_ERRNO(pNetApp->GetLegacyPolicy(policy_info_ptr));
   return DSS_SUCCESS;
}

void dss_init_net_policy_info
(
  dss_net_policy_info_type * policy_info_ptr       /* policy info structure */
)
{
   if(policy_info_ptr != NULL)
   {
      policy_info_ptr->iface.kind = DSS_IFACE_NAME;
      policy_info_ptr->iface.info.name = DSS_IFACE_ANY_DEFAULT;
      policy_info_ptr->policy_flag = DSS_IFACE_POLICY_ANY;
      policy_info_ptr->ipsec_disabled = FALSE;
      policy_info_ptr->is_routeable = FALSE;
      policy_info_ptr->family = DSS_AF_INET;

#if (defined (FEATURE_DS_MOBILE_IP) && defined (FEATURE_DATA_PS_MIP_CCOA))
      policy_info_ptr->sip_iface_reqd = FALSE;
#endif

      policy_info_ptr->app_identifier = PS_POLICY_MGR_APP_DONT_CARE;
      policy_info_ptr->umts.pdp_profile_num = 0;
      policy_info_ptr->umts.im_cn_flag = FALSE;
      policy_info_ptr->umts.apn.length = 0;
      policy_info_ptr->umts.apn.name = NULL;
      policy_info_ptr->cdma.data_session_profile_id = 0;
      policy_info_ptr->dss_netpolicy_private.cookie = DSS_NETPOLICY_COOKIE;
   }

} /* dss_init_net_policy_info() */

sint15 dss_netstatus
(
  sint15 app_id,                                         /* application ID */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSNetApp *pNetApp = NULL;
   DSSIDSNetworkScope IDSNetworkScope;
   NetworkStateType netState;
   boolean bNetworkIsUp;
   sint15 ret = DSS_ERROR;

   LOG_MSG_FUNCTION_ENTRY("dss_netstatus(): app_id:%d", app_id, 0, 0);

   // result from GetState() is AEE_SUCCESS. dss_netstatus() returns DSS_ERROR + relevant errno.
   // This is not actually an error.

   IDS_ERR_RET_ERRNO(DSSGlobals::Instance()->GetNetApp(app_id, &pNetApp));
   IDS_ERR_RET_ERRNO(IDSNetworkScope.Init(pNetApp));
   pNetApp->GetNetworkIsUp(&bNetworkIsUp);
   if (!bNetworkIsUp) {
      // If the network is down it is not connected to an iface, so we don't call IDSNetworkPriv->GetState since it might return a wrong state
      *dss_errno = DS_ENETNONET;
      goto bail;
   }
   IDS_ERR_RET_ERRNO(IDSNetworkScope.Fetch()->GetState(&netState));

   switch (netState) {
      case NetworkState::QDS_OPEN_IN_PROGRESS:
         *dss_errno = DS_ENETINPROGRESS;
         break;

      case NetworkState::QDS_OPEN:
         *dss_errno = DS_ENETISCONN;
         break;

      case NetworkState::QDS_CLOSE_IN_PROGRESS:
         *dss_errno = DS_ENETCLOSEINPROGRESS;
         break;

      case NetworkState::QDS_CLOSED:
      case NetworkState::QDS_LINGERING:
         *dss_errno = DS_ENETNONET;
         break;

      default:
         *dss_errno = DS_ENETDOWN; // TODO: revisit this errno
         goto bail;
   }

bail:
   return ret;
}

sint15 dss_pppclose
(
  sint15 app_id,                                         /* application id */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSNetApp *pNetApp = NULL;
   DSSIDSNetworkScope IDSNetworkScope;
   boolean bNetworkIsUp;
   sint15 ret = DSS_SUCCESS;


   LOG_MSG_FUNCTION_ENTRY("dss_pppclose(): app_id:%d", app_id, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      ret = DSS_ERROR;
      goto bail;
   }

   *dss_errno = DSS_SUCCESS;

   IDS_ERR_RET_ERRNO(DSSGlobals::Instance()->GetNetApp(app_id, &pNetApp));
   IDS_ERR_RET_ERRNO(IDSNetworkScope.Init(pNetApp));
   
   pNetApp->ResetLastIfaceStateSentToApp();
   
   pNetApp->GetNetworkIsUp(&bNetworkIsUp);
   if (!bNetworkIsUp) {
      // If the network is down it we can just return DSS_SUCCESS
      goto bail;
   }
   IDS_ERR_RET_ERRNO(IDSNetworkScope.Fetch()->Stop());


   // Synchronized pppclose: If Stop returns AEE_SUCCESS, it means that the network is already
   // down and we don't need to wait for event
   // TODO: call pIDSNetworkPriv->GetLastNetDownReason and store the result
   // TODO: this is not synchronized with the iface state signal callback!
   // it should be moved into the pNetApp and protected with its critical section!

bail:
   return ret;
}

sint15 dss_pppopen
(
   sint15 app_id,                                         /* application id */
   sint15 *dss_errno                               /* error condition value */
)
{
   DSSNetApp *pNetApp = NULL;
   DSSIDSNetworkPrivScope IDSNetworkPrivScope;
   AEEResult res;


   LOG_MSG_FUNCTION_ENTRY("dss_pppopen(): app_id:%d", app_id, 0, 0);

   res = DSSGlobals::Instance()->GetNetApp(app_id, &pNetApp);
   if ((AEE_SUCCESS != res) || (NULL == pNetApp)) {
      *dss_errno = DS_EBADAPP;
      return DSS_ERROR;
   }

   IDS_ERR_RET_ERRNO(IDSNetworkPrivScope.Init(pNetApp));

   pNetApp->SetIsPPPOpen(TRUE);
   
   pNetApp->ResetLastIfaceStateSentToApp();
   
   res = IDSNetworkPrivScope.Fetch()->BringUpInterface();
   if (AEE_SUCCESS == res || AEE_EWOULDBLOCK == res) {
      pNetApp->SetNetworkIsUp(TRUE);
      (void) pNetApp->RegAutoEvents();
   }
   IDS_ERR_RET_ERRNO(res);

   return DSS_SUCCESS;
}

sint15 dss_set_app_net_policy
(
   sint15 appid,                                          /* Application id */
   dss_net_policy_info_type * policy_info_ptr,     /* policy info structure */
   sint15 * dss_errno                                       /* error number */
)
{
   DSSNetApp *pNetApp = NULL;
   DSSIDSNetPolicyPrivScope IDSNetPolicyPrivScope;
   boolean bNetworkIsUp;

   LOG_MSG_FUNCTION_ENTRY("dss_set_app_net_policy(): app_id:%d, poicy_ptr:0x%p", appid, policy_info_ptr, 0);

   IDS_ERR_RET_ERRNO(DSSGlobals::Instance()->GetNetApp(appid, &pNetApp));

   //Verify that network was not brought up yet. Policy cannot be changed if the app
   //already brought up the network (because in such case the Network object is already
   //bound to an iface and policy change would not take effect.
   pNetApp->GetNetworkIsUp(&bNetworkIsUp);
   if (TRUE == bNetworkIsUp) {
      LOG_MSG_ERROR("Cannot set policy cause network was already brought up.",0,0,0);
      *dss_errno = DS_EINVAL;
      return DSS_ERROR;
   }

   //Verify that the call is not registered for a Mobile Terminated event.
   boolean bMTPDIsReg;
   IDS_ERR_RET_ERRNO(pNetApp->GetMTPDIsRegistered(&bMTPDIsReg));

   if(TRUE == bMTPDIsReg)
   {
      LOG_MSG_ERROR("Cannot set policy while registered for MTPD call.",0,0,0);
      *dss_errno = DS_EOPNOTSUPP;
      return DSS_ERROR;
   }

   IDS_ERR_RET_ERRNO(IDSNetPolicyPrivScope.Init());
   IDS_ERR_RET_ERRNO(DSSConversion::DS2IDSNetPolicy(policy_info_ptr, IDSNetPolicyPrivScope.Fetch()));
   IDS_ERR_RET_ERRNO(pNetApp->SetPolicy(IDSNetPolicyPrivScope.Fetch()));
   IDS_ERR_RET_ERRNO(pNetApp->SetLegacyPolicy(policy_info_ptr));

   return DSS_SUCCESS;
}

sint15
dss_last_netdownreason
(
   sint15                      appid,                /* Application id      */
   dss_net_down_reason_type  * reason,               /* network down reason */
   sint15                    * dss_errno             /* error number        */
)
{
   DSSNetApp *pNetApp = NULL;
   DSSIDSNetworkScope IDSNetworkScope;
   NetDownReasonType netdownreason;


   LOG_MSG_FUNCTION_ENTRY("dss_last_netdownreason(): app_id:%d", appid, 0, 0);

   IDS_ERR_RET_ERRNO(DSSGlobals::Instance()->GetNetApp(appid, &pNetApp));
   IDS_ERR_RET_ERRNO(IDSNetworkScope.Init(pNetApp));
   IDS_ERR_RET_ERRNO(IDSNetworkScope.Fetch()->GetLastNetDownReason(&netdownreason));
   IDS_ERR_RET_ERRNO(DSSConversion::IDS2DSNetworkDownReason(netdownreason, reason));

   return DSS_SUCCESS;
}

dss_iface_id_type dss_get_iface_id(sint15  appid)
{
   DSSNetApp *pNetApp = NULL;
   DSSIDSNetworkScope IDSNetworkScope;
   dss_iface_id_type iface_id;
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY("dss_get_iface_id(): app_id:%d", appid, 0, 0);

   iface_id = DSS_IFACE_INVALID_ID;

   res = DSSGlobals::Instance()->GetNetApp(appid, &pNetApp);
   if (AEE_SUCCESS != res) {
      goto bail;
   }
   res = IDSNetworkScope.Init(pNetApp);
   if (AEE_SUCCESS != res) {
      goto bail;
   }

   res = BuildIfaceId(IDSNetworkScope.Fetch(),appid,&iface_id);
   if (AEE_SUCCESS != res) {
      iface_id = DSS_IFACE_INVALID_ID;
      goto bail;
   }

   // Save the iface index in the DSSNetApp structure
   pNetApp->SetIfaceId(iface_id);

bail:
   return iface_id;
}

// TODO: make changes to dss_get_iface_id_by_policy (don't use IDSNetwork)
dss_iface_id_type
dss_get_iface_id_by_policy
(
   dss_net_policy_info_type  net_policy_info,        /* Network policy info */
   sint15                  * dss_errno             /* error condition value */
)
{
   INetworkFactory* pIDSNetworkFactory = NULL;
   INetwork* pIDSNetwork = NULL;
   DSSIDSNetPolicyPrivScope IDSNetPolicyPrivScope;
   dss_iface_id_type iface_id;
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY("dss_get_iface_id_by_policy(): net_policy_info:0x%p ", &net_policy_info, 0, 0);

   iface_id = DSS_IFACE_INVALID_ID;

   res = IDSNetPolicyPrivScope.Init();
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't create DSNetPolicy", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      goto bail;
   }
   // Convert from ds to IDS network policy.
   res = DSSConversion::DS2IDSNetPolicy(&net_policy_info, IDSNetPolicyPrivScope.Fetch());
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      goto bail;
   }

   res = DSSGlobals::Instance()->GetNetworkFactory(&pIDSNetworkFactory);

   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      goto bail;
   }
   res = pIDSNetworkFactory->CreateNetwork(ds::Net::NetworkMode::QDS_MONITORED, IDSNetPolicyPrivScope.Fetch(), &pIDSNetwork);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      goto bail;
   }

   res = BuildIfaceId(pIDSNetwork,-1,&iface_id);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      iface_id = DSS_IFACE_INVALID_ID;
      goto bail;
   }

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSNetwork);
   return iface_id;
}

dss_iface_id_type dss_get_iface_id_by_qos_handle
(
   dss_qos_handle_type  handle     // Handle to QOS instance
)
{
   LOG_MSG_FUNCTION_ENTRY("dss_get_iface_id_by_qos_handle(): qos_handle:%d", handle, 0, 0);

   return handle & 0xFFFFFF00;
}
// TODO Support of this API depends on QMI additions. Complete support here when support in lower layers is provided.
int32 dss_get_app_profile_id (uint32 app_type)
{
   LOG_MSG_FUNCTION_ENTRY("dss_get_app_profile_id(): app_type:%d", app_type, 0, 0);

   // Currently this API supported by single proc targets only
#ifdef FEATURE_DATACOMMON_PACKAGE_SINGLE_PROC
   return ps_policy_mgr_get_profile((int64)app_type);
#else
   LOG_MSG_ERROR("dss_get_app_profile_id is not supported",0,0,0);
   return (DSS_ERROR);
#endif
}

/*===========================================================================
FUNCTION DSS_GET_SCOPE_ID_BY_IFACE_ID()

DESCRIPTION
  This function allows to retrieve a route_scope from the iface_id.
  Currently, for applications the notion of scope id is basically same as
  iface id as we do not support sitelocal addresses. However, applications
  need not know that scopeid and ifaceid are same as the interpretation can
  change in future when sitelocal multicast is supported.

DEPENDENCIES
  None.

PARAMETERS
  uint32  - Iface id.
  sint15* - Errno.

RETURN VALUE
  On success - Scope Id
  On failure - 0

  dss_errno Values
  ----------------
  DS_EINVAL      Invalid iface id.

SIDE EFFECTS
  None
===========================================================================*/
dss_scope_id_type dss_get_scope_id_by_iface_id
(
  dss_iface_id_type   iface_id,
  sint15 *dss_errno
)
{
   dss_iface_id_type iface_id_internal = DSS_IFACE_INVALID_ID;

   iface_id_internal = StripAppIdFromIfaceId(iface_id);

   return iface_id_internal;

}

int dss_reg_ip_filter
(
  sint15                     sockfd,
  dss_iface_id_type          iface_id,
  ipfltr_type_enum_type      filter_type,
  uint8                      num_filters,
  void                       *filters,
  sint15                     *dss_errno
)
{
   ip_filter_type*        ipfltr_ptr;
   int                    loop_var;
   sint15                 ret;
   IIPFilterRegPriv*        filterReg = NULL;
   IIPFilterManagerPriv     *pDSNetIPFilterMgr = NULL;
   INetworkPriv*        pIDSNetworkPriv = NULL;
   INetworkFactoryPriv* pIDSNetworkFactoryPriv = NULL;
   DSSIDSNetPolicyPrivScope IDSNetPolicyPrivScope;
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY("dss_reg_ip_filter(): socket:%d, iface_id:%u, filter_type:%d", sockfd, iface_id, filter_type);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_reg_ip_filter: dss_errno is NULL", 0, 0, 0);
      return DSS_ERROR;
   }

   *dss_errno = DSS_SUCCESS;

   if (NULL == filters || 0 == num_filters)
   {
     *dss_errno = DS_EFAULT;
     LOG_MSG_ERROR("dss_reg_ip_filter: Bad arg - filters 0x%p, num_filters %d",
               filters, num_filters, 0);
     return DSS_ERROR;
   }

   if (IPFLTR_MAX_TYPE <= filter_type)
   {
     *dss_errno = DS_EFAULT;
     LOG_MSG_ERROR("dss_reg_ip_filter: Bad arg - filter_type %d",
               filter_type, 0, 0);
     return DSS_ERROR;
   }

   if (DSS_IFACE_INVALID_ID == iface_id)
   {
     *dss_errno = DS_EBADF;
     LOG_MSG_ERROR("dss_reg_ip_filter: Bad iface_id 0x%x", iface_id, 0, 0);
     return DSS_ERROR;
   }

   // Allocate the memory for the filters pointers.
   IIPFilterPriv** ppFilters = (IIPFilterPriv**)ps_system_heap_mem_alloc(sizeof(IIPFilterPriv*)*num_filters);
   if (NULL == ppFilters) {
      *dss_errno = DS_ENOMEM;
      return DSS_ERROR;
   }

   // Pass over all the filters and convert them to the new API filters in order
   // to set the filters on the corresponding IDSNetwork object.

   // Get the NetworkFactory from DSSGlobals
   INetworkFactoryPriv *pNetworkFactoryPriv;
   res = DSSGlobals::Instance()->GetNetworkFactoryPriv(&pNetworkFactoryPriv);
   if(AEE_SUCCESS != res){
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      ret = DSS_ERROR;
      goto bail;
   }


   for (loop_var = 0; loop_var < num_filters; loop_var++)
   {
      ipfltr_ptr = (((ip_filter_type*)filters) + loop_var);

      // Create IDSNetFilter object.
      res = pNetworkFactoryPriv->CreateIPFilterSpec(&ppFilters[loop_var]);
      if (AEE_SUCCESS != res) {
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         ret = DSS_ERROR;
         goto bail;
      }

      res = DSSConversion::DS2IDSIPFilter(ipfltr_ptr, ppFilters[loop_var]);
      if (AEE_SUCCESS != res) {
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         ret = DSS_ERROR;
         goto bail;
      }
   }

   // Create IDSNetPolicyPriv with the provided iface_id
   res = IDSNetPolicyPrivScope.Init();
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't create DSNetPolicy", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      ret = DSS_ERROR;
      goto bail;
   }

   res = IDSNetPolicyPrivScope.Fetch()->SetIfaceId((IfaceIdType)((iface_id & 0xFF000000) | 0x00FFFF00));
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("SetIfaceId failed.", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      ret = DSS_ERROR;
      goto bail;
   }

   res = IDSNetPolicyPrivScope.Fetch()->SetRoutable(TRUE);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("SetRouteable failed.", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      ret = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetNetworkFactoryPriv(&pIDSNetworkFactoryPriv);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't create AEECLSID_DSNetNetworkPrivFactory", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      ret = DSS_ERROR;
      goto bail;
   }

   res = pIDSNetworkFactoryPriv->CreateNetworkPriv(IDSNetPolicyPrivScope.Fetch(), &pIDSNetworkPriv);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("CreateNetworkPriv failed", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      ret = DSS_ERROR;
      goto bail;
   }

   //Perform LookupInterface() to set the iface id in the network object.
   res = pIDSNetworkPriv->LookupInterface();
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("LookupInterface() failed", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      ret = DSS_ERROR;
      goto bail;
   }

   res = pIDSNetworkPriv->QueryInterface(AEEIID_IIPFilterManagerPriv, (void**)&pDSNetIPFilterMgr);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("QueryInterface(AEEIID_IDSNetIPFilterMgr) failed: %d", res, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      ret = DSS_ERROR;
      goto bail;
   }

   res = pDSNetIPFilterMgr->RegisterFilters((int)sockfd, ppFilters, num_filters, &filterReg);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("RegisterFilters() failed: %d", res, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      ret = DSS_ERROR;
      goto bail;
   }

   // Need to save the filter in the DSSGlobals in order to know when to release it.
   ret = DSSGlobals::Instance()->AddFilterRegObjectToList(filterReg, iface_id, sockfd);

bail:
   // Release the objects.
   for (loop_var = 0; loop_var < num_filters; loop_var++)
   {
      if (ppFilters[loop_var]) {
         DSSCommon::ReleaseIf((IQI**)&ppFilters[loop_var]);
      }
   }

   PS_SYSTEM_HEAP_MEM_FREE (ppFilters);

   DSSCommon::ReleaseIf((IQI**)&pIDSNetworkPriv);
   DSSCommon::ReleaseIf((IQI**)&pDSNetIPFilterMgr);
   DSSCommon::ReleaseIf((IQI**)&filterReg);
   DSSCommon::ReleaseIf((IQI**)&pNetworkFactoryPriv);

   return ret;
}

int dss_dereg_ip_filter
(
  sint15             sockfd,
  dss_iface_id_type  iface_id,
  sint15            *dss_errno
)
{
  LOG_MSG_FUNCTION_ENTRY("dss_dereg_ip_filter(): socket:%d, iface_id:%u", sockfd, iface_id, 0);

   AEEResult res = DSSGlobals::Instance()->RemoveFilterRegObjectFromList(iface_id, sockfd);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Failed to remove filter Reg object from list", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   return DSS_SUCCESS;
}
