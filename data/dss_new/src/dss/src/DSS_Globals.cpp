/*======================================================

FILE:  DSSGlobals.cpp

SERVICES:

GENERAL DESCRIPTION:
Implementation of DSSGlobals class

=====================================================

Copyright (c) 2008-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_Globals.cpp#2 $
  $DateTime: 2011/07/07 00:24:16 $$Author: vmordoho $

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
#include "DSS_Common.h"
#include "customer.h"

#include "AEECCritSect.h"
#include "AEECSignalFactory.h"

#include "ds_Net_INetworkPriv.h"
#include "DSS_SecondaryNetApp.h"
#include "ds_Net_IPolicyPriv.h"
#include "ds_Net_CNetworkFactoryPriv.h"

#include "DSS_Conversion.h"
#include "DSS_NetApp.h"
#include "DSS_Globals.h"
#include "DSS_CritScope.h"
#include "ps_mem.h"
#include "DSS_MemoryManagement.h"

#include "ds_Net_CNetworkFactory.h"
#include "ds_Sock_CSocketFactory.h"
#include "dss_config.h"
#include "ds_Net_CreateInstance.h"
#include "ds_Sock_CreateInstance.h"
#include "ds_Utils_FullPrivSet.h"
#include "ds_Utils_CreateInstance.h"
#include "ps_handle_mgr.h"

using namespace ds::Net;
using namespace ds::Error;

DSSGlobals* DSSGlobals::pmInstance = NULL;
bool DSSGlobals::mQoSFlowIDs[NUM_OF_FLOW_IDS] = {0};
bool DSSGlobals::mMCastFlowIDs[NUM_OF_FLOW_IDS] = {0};

//-------------------------------------------------------------------
// Constant / Define Declarations
//-------------------------------------------------------------------
#define POLICY_FLAG 0x00800000
#define NET_HANDLE_MASK 0x7FFF00
#define DSS_MAX_SINT15 32767
//-------------------------------------------------------------------
// Type Declarations (typedef, struct, enum, etc.)
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Global Constant Data Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Global Data Declarations
//-------------------------------------------------------------------


//-------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------

static sint15 FirstFreeSockIndex(DSSSocket *array[], int size);
static sint15 FirstFreeNetHandle(DSSPrimaryNetApp *array[], int size);

//===================================================================
//              Macro Definitions
//===================================================================


//===================================================================
//            DSSGlobals Functions Definitions
//===================================================================


// TODO: implement the constructor
//===================================================================
//  FUNCTION:   DSSGlobals::DSSGlobals
//
//  DESCRIPTION:
//  Constructor of the DSSGlobals class.
//===================================================================
DSSGlobals::DSSGlobals():
   mpNetFactory(0),
   mpNetFactoryPriv(0),
   mpSockFactory(0),
   mpSignalFactory(0),
   mpCritSect(0),
   mFilterRegInfoList(0),
   mTcpSocketsNum(0),
   mUdpSocketsNum(0),
   mNatSession(0)
{
   int i;

   for (i = 0; i < DSS_MAX_APPS; ++i) {
      mapNetAppArray[i] = NULL;
   }
   for (i = 0; i < DSS_MAX_SOCKS; ++i) {
      mapSocketArray[i] = NULL;
   }
}


extern "C" void DSSGlobalInit
(
  void
)
{
  (void)DSSGlobals::Init();
}

extern "C" void DSSGlobalDeinit
(
  void
)
{
  (void)DSSGlobals::Deinit();
}


//===================================================================
//  FUNCTION:   DSSGlobals::~DSSGlobals
//
//  DESCRIPTION:
//  Destructor of the DSSGlobals class.
//===================================================================

/*lint -e{1551} */
DSSGlobals::~DSSGlobals()
{
   DSSCommon::ReleaseIf((IQI**)&mpNetFactory);
   DSSCommon::ReleaseIf((IQI**)&mpNetFactoryPriv);
   DSSCommon::ReleaseIf((IQI**)&mpSockFactory);
   DSSCommon::ReleaseIf((IQI**)&mpSignalFactory);
   DSSCommon::ReleaseIf((IQI**)&mpCritSect);

   // TODO: loop over all the netApps and the sockets and delete all of them.
}
/*lint –restore */

//===================================================================
//  FUNCTION:   DSSGlobals::Instance
//
//  DESCRIPTION:
//    Any object in the DSS that needs any of the globals
//    that DSSGlobals implements uses this Instance member to gain
//    access to the *single* object of DSSGlobals.

//===================================================================
DSSGlobals* DSSGlobals::Instance()
{
   if (NULL == pmInstance) {
      LOG_MSG_ERROR ("DSSGlobals::Init() must be called first ",0, 0, 0);
      ASSERT(0);
   }

   return pmInstance;
}

//===================================================================
//  FUNCTION:   DSSGlobals::GetCritSect
//
//  DESCRIPTION:
//    Getter for global critical section.

//===================================================================
AEEResult DSSGlobals::GetCritSect(ICritSect** ppICritSect)
{
   if (NULL == pmInstance) {
      LOG_MSG_ERROR ("DSSGlobals::Init() must be called first ",0, 0, 0);
      ASSERT(0);
   }

   if (NULL == ppICritSect) {
      return QDS_EINVAL;
   }

   *ppICritSect = mpCritSect;
   (void) mpCritSect->AddRef();

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSGlobals::Init
//
//  DESCRIPTION:
//    Init the singletone DSSGlobals. Must call this before using 
//    DSSGlobals::Instance()

//===================================================================
void DSSGlobals::Init()
{
   int res;
   
   if (NULL != pmInstance) {
      LOG_MSG_ERROR ("Init has been already called! ",0, 0, 0);
      return;
   }

   DSSmem_pool_init();

   pmInstance = PS_MEM_NEW(DSSGlobals);
   if (NULL == pmInstance){
      LOG_MSG_ERROR ("Can't allocate DSSGlobals singleton ",0, 0, 0);
      ASSERT(0);
   }

   res = DS_Utils_CreateInstance(NULL, AEECLSID_CCritSect,
                                 NULL, (void**)&pmInstance->mpCritSect);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't create Critical Section (%d)", res, 0, 0);
      ASSERT(0);
   }
   // initialize ps_handle_mgr used for Socket FD allocation
   ps_handle_mgr_init_client(PS_HANDLE_MGR_CLIENT_DSS_SOCK_FD,
                             DSS_MAX_SOCKS,
                             1,
                             DSS_MAX_SINT15);
   // limitation for maximal handle comes from 
   // prototype of DS API socket related functions,
   // see dss_socket() for example
   // limitation for minimal handle comes from
   // definition that 0 is not a valid socket handle
}

//===================================================================
//  FUNCTION:   DSSGlobals::Deinit
//
//  DESCRIPTION:
//    Any object in the DSS that needs any of the globals
//    that DSSGlobals implements uses this Instance member to gain
//    access to the *single* object of DSSGlobals.

//===================================================================
void DSSGlobals::Deinit()
{
   if (NULL != pmInstance) {
      delete pmInstance;
      pmInstance = NULL;
   }
}

#ifndef FEATURE_DSS_LINUX
// TODO: do we need to lock the getters?
//===================================================================
//  FUNCTION:   DSSGlobals::GetSockFactory
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
void DSSGlobals::GetSockFactory(ds::Sock::ISocketFactory **ppFactory)
{
   if (NULL == mpSockFactory) {
      AEEResult res;

      res = DSSockCreateInstance(NULL, ds::Sock::AEECLSID_CSocketFactory,
                                 NULL, (void**)&mpSockFactory);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("Can't create SocketFactory", 0, 0, 0);
         return;
      }
   }

   (void)mpSockFactory->AddRef();
   *ppFactory = mpSockFactory;
}
#endif

//===================================================================
//  FUNCTION:   DSSGlobals::GetNetworkFactory
//
//  DESCRIPTION:
//     Returns the network factory to the user
//===================================================================
AEEResult DSSGlobals:: GetNetworkFactory(ds::Net::INetworkFactory **ppNetworkFactory)
{
   if (NULL == mpNetFactory) {
      AEEResult res;

      ds::Utils::FullPrivSet *fullPrivSetPtr =
         ds::Utils::FullPrivSet::Instance();

      LOG_MSG_ERROR ("GetNetworkFactory: fullPrivSetPtr is NULL",0, 0, 0);
      ASSERT(0 != fullPrivSetPtr);

      res = DSNetCreateInstance(NULL, AEECLSID_CNetworkFactory,
                                fullPrivSetPtr, (void**)&mpNetFactory);

      (void)fullPrivSetPtr->Release();

      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("Can't create NetworkFactory", 0, 0, 0);
         return res;
      }
   }

   (void)mpNetFactory->AddRef();

   *ppNetworkFactory = mpNetFactory;

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSGlobals::GetNetworkFactoryPriv
//
//  DESCRIPTION:
//     Returns the network factory to the user
//===================================================================
AEEResult DSSGlobals::GetNetworkFactoryPriv(ds::Net::INetworkFactoryPriv **ppNetworkFactoryPriv)
{
   if (NULL == mpNetFactoryPriv) {
      AEEResult res;

      ds::Utils::FullPrivSet *fullPrivSetPtr =
         ds::Utils::FullPrivSet::Instance();

      if (0 == fullPrivSetPtr) {
         LOG_MSG_ERROR ("GetNetworkFactoryPriv: fullPrivSetPtr is NULL",0, 0, 0);
         ASSERT(0);
      }
      
      res = DSNetCreateInstance(NULL, AEECLSID_CNetworkFactoryPriv,
                                fullPrivSetPtr, (void**)&mpNetFactoryPriv);

      (void)fullPrivSetPtr->Release();

      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("Can't create NetworkPrivFactory", 0, 0, 0);
         return res;
      }
   }

   (void)mpNetFactoryPriv->AddRef();

   *ppNetworkFactoryPriv = mpNetFactoryPriv;

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSGlobals::GetSignalFactory
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
AEEResult DSSGlobals::GetSignalFactory(ISignalFactory **ppFactory)
{
   if (NULL == mpSignalFactory) {
      AEEResult res;

      res = DS_Utils_CreateInstance(NULL, AEECLSID_CSignalFactory,
                                    NULL, (void**)&mpSignalFactory);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("Can't create SignalFactory", 0, 0, 0);
         return res;
      }
   }

   (void)mpSignalFactory->AddRef();
   *ppFactory = mpSignalFactory;

   return AEE_SUCCESS;
}

AEEResult DSSGlobals::GetNetApp(dss_iface_id_type iface_id,DSSNetApp** ppNetApp)
{
   DSSCritScope cs(*mpCritSect);
   sint15 netHandle;
   AEEResult res = AEE_SUCCESS;

   // verify that policy bit is turned on.
   uint32 appIdFlag = iface_id & POLICY_FLAG;
   if (!appIdFlag) {
      // extract nethandle form iface_id
      netHandle = static_cast<sint15>((iface_id & NET_HANDLE_MASK) >> 8);
   } else {
      return QDS_EBADAPP;
   }

   if (!IsValidNetApp(netHandle)) {
      LOG_MSG_ERROR("Invalid Application descriptor", 0, 0, 0);
      return QDS_EBADAPP;
   }

   res = GetNetApp(netHandle,ppNetApp);
   if (AEE_SUCCESS != res) {
      return res;
   }
   // Check if the provided iface id is the same as the one specified in the primary DSSNetApp.
   dss_iface_id_type PriIfaceId;
   (*ppNetApp)->GetIfaceId(&PriIfaceId);
   if (!CompareIfaceIds((dss_iface_id_type)PriIfaceId,iface_id)) {
      return QDS_EBADAPP;
   }

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSGlobals::GetNetApp
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
AEEResult DSSGlobals::GetNetApp(sint15 netHandle, DSSNetApp **ppNetApp)
{
   DSSCritScope cs(*mpCritSect);
// TODO: What about zero nethandle?
   if ((1 > netHandle) || (DSS_MAX_APPS < netHandle)) {
      // TODO: return an error code that indicates 'out of bound'
      return QDS_EBADAPP;
   }


   if (NULL == mapNetAppArray[netHandle-1]) {
      return QDS_EBADAPP;
   }

   *ppNetApp = mapNetAppArray[netHandle-1];
   (void)(*ppNetApp)->SetNetHandle(netHandle);
   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSGlobals::GetNetApp
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
AEEResult DSSGlobals::GetNetApp(sint15 netHandle,
                                dss_iface_id_type iface_id,
                                bool bAllowSecondary,
                                bool bForbidTemporary,
                                DSSNetApp** ppNetApp,
                                bool* bIsTempDSSNetApp,
                                dss_iface_ioctl_type ioctl_name,
                                void* argval_ptr)
{
   DSSCritScope cs(*mpCritSect);
   *bIsTempDSSNetApp = false;
   DSSPrimaryNetApp* pPriDSSNetApp = NULL;

   // verify that policy bit is turned on.
   uint32 appIdFlag = iface_id & POLICY_FLAG;
   if (!appIdFlag) {
      // extract nethandle form iface_id
      sint15 ifaceIdNetHandle = static_cast<sint15>((iface_id & NET_HANDLE_MASK) >> 8);
      if (-1 == netHandle) {
         // From this point on we shall use the netHandle as if it was directly provided by the app
         netHandle = ifaceIdNetHandle;
      }
      else {
         if (netHandle != ifaceIdNetHandle) {
            // Conflict between netHandle provided by app in the netHandle parameter and the netHandle
            // provided inside the iface_id (misuse by app!)
            return QDS_EBADAPP;
         }
      }
   }
   // netHandle was not provided as a direct input nor found inside in the iface_id.
   // if current ioctl is qos , we can get it from qos_request->handle
   if (-1 == netHandle) {
      netHandle = GetNetHandleFromQoSRequestHandle(ioctl_name,argval_ptr);
   }

   // netHandle was not provided as a direct input nor found inside in the iface_id.
   // => The operation requested by the app shall be serviced via a *temporary* IDSNetwork object.
   if (-1 == netHandle) {
      if (TRUE == bForbidTemporary) {
         return QDS_EBADF;
      }
      IDS_ERR_RET(CreateTempDSSNetApp(ppNetApp, iface_id));
      *bIsTempDSSNetApp = true;  // TODO: maybe the flag should be inside DSSNetApp
      return AEE_SUCCESS;
   }

   // netHandle is known. Search in the primary and secondary DSSNetApps of this netHandle to find an appropriate DSSNetApp.
   // If not found, create a temporary DSSNetApp
   if (!IsValidNetApp(netHandle)) {
      LOG_MSG_ERROR("Invalid Application descriptor", 0, 0, 0);
      return QDS_EBADF;
   }

   pPriDSSNetApp = mapNetAppArray[netHandle-1];
   if (NULL == pPriDSSNetApp) {
      LOG_MSG_ERROR("Invalid Application descriptor", 0, 0, 0);
      return QDS_EBADAPP;
   }

   // Check if the provided iface id is the same as the one specified in the primary DSSNetApp.
   dss_iface_id_type PriIfaceId;
   pPriDSSNetApp->GetIfaceId(&PriIfaceId);
   if (CompareIfaceIds((dss_iface_id_type)PriIfaceId,iface_id)) {
      *ppNetApp = pPriDSSNetApp;
      return AEE_SUCCESS;
   }

   // bAllowSecondary == TRUE. Find corresponding secondary DSSNetApp or create one
   if (TRUE == bAllowSecondary) {
      IDS_ERR_RET(pPriDSSNetApp->FindSecondaryDSSNetApp(iface_id, ppNetApp));\
      return AEE_SUCCESS;
   }

   // The iface id of the Primary DSSNetApp does not correspond with the iface_id provided by the app

   if (TRUE == bForbidTemporary) {
      return QDS_EBADAPP;
   }
   // Primary is incompatible with specified iface_id and secondary is not needed (synchronic operation).
   // Create a temporary DSSNetApp for this operation
   IDS_ERR_RET(CreateTempDSSNetApp(ppNetApp, iface_id));
   *bIsTempDSSNetApp = true;  // TODO: maybe the flag should be inside DSSNetApp
   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSGlobals::CreateTempDSSNetApp
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
AEEResult DSSGlobals::CreateTempDSSNetApp(DSSNetApp** ppNetApp, dss_iface_id_type iface_id)
{
   AEEResult            res;
   IPolicyPriv*         pPolicyPriv = NULL;
   INetworkFactoryPriv* pIDSNetworkFactoryPriv = NULL;
   INetworkPriv*        pIDSNetworkPriv = NULL;

   // Create temporary DSSNetApp
   *ppNetApp = DSSSecondaryNetApp::CreateInstance();
   if (NULL == *ppNetApp) {
      LOG_MSG_ERROR( "Couldn't allocate DSSNetApp", 0, 0, 0);
      res = AEE_ENOMEMORY;
      goto bail;
   }

   if (NULL == mpNetFactoryPriv) {

     res = DSSGlobals::Instance()->GetNetworkFactoryPriv(&mpNetFactoryPriv);

     if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("Can't create NetworkPrivFactory", 0, 0, 0);
         goto bail;
      }
   }

   res = mpNetFactoryPriv->CreatePolicyPriv(&pPolicyPriv);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't create DSNetPolicyPriv", 0, 0, 0);
      goto bail;
   }

   res = pPolicyPriv->SetIfaceId((iface_id & 0xFF000000) | 0x00FFFF00);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't set iface id %u in policy", iface_id, 0, 0);
      goto bail;
   }

   res = DSSGlobals::Instance()->GetNetworkFactoryPriv(&pIDSNetworkFactoryPriv);

   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't create IDSNetworkPrivFactory", 0, 0, 0);
      goto bail;
   }

   // Create IDSNetwork in Monitored Mode
   res = pIDSNetworkFactoryPriv->CreateNetworkPriv(pPolicyPriv, &pIDSNetworkPriv);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't create IDSNetworkPriv", 0, 0, 0);
      goto bail;
   }

   (void) pIDSNetworkPriv->LookupInterface();

   // DSSNetApp takes care to AddRef pIDSNetworkPriv once it is set
   res = (*ppNetApp)->Init(pIDSNetworkPriv);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't Initialize DSSNetApp", 0, 0, 0);
      goto bail;
   }

   DSSCommon::ReleaseIf((IQI**)&pPolicyPriv);
   DSSCommon::ReleaseIf((IQI**)&pIDSNetworkPriv);

   return AEE_SUCCESS;

bail:
   DSSCommon::ReleaseIf((IQI**)&pPolicyPriv);
   DSSCommon::ReleaseIf((IQI**)&pIDSNetworkPriv);
   DSSCommon::ReleaseIf((IQI**)ppNetApp);

   return res;
}

//===================================================================
//  FUNCTION:   DSSGlobals::IsValidNetApp
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
bool DSSGlobals::IsValidNetApp(sint15 handle)
{
   DSSCritScope cs(*mpCritSect);
   // TODO: What about zero nethandle?
   if ((1 > handle) || (DSS_MAX_APPS < handle)) {
      return false;
   }

   return (NULL != mapNetAppArray[handle-1]);
}

// TODO: check if the documentation is relevant
//===================================================================
//  FUNCTION:   DSSGlobals::InsertNetApp
//
//  DESCRIPTION:
//    Inserts the given DSSNetApp object to the DSSNetApps array
//    NULL pNetApp can be passed to just check if there is available space
//===================================================================
sint15 DSSGlobals::InsertNetApp(DSSNetApp *pNetApp)
{
   DSSCritScope cs(*mpCritSect);

   sint15 index = FirstFreeNetHandle(mapNetAppArray, DSS_MAX_APPS);
   if (-1 != index) {
      //mapNetAppArray[index-1] = dynamic_cast<DSSPrimaryNetApp*>(pNetApp);  // Use static_cast for now until we decide to enable RTTI in the compiler (/GR in make.d/defines_vc7.min)
      if (NULL != pNetApp) {
         mapNetAppArray[index-1] = static_cast<DSSPrimaryNetApp*>(pNetApp);
      }
   }

   return index;
}

//==============================================================================
//  FUNCTION:   DSSGlobals::RemoveNetApp
//
//  DESCRIPTION:
//     TODO: Documentation
//
//  ASSUMPTIONS:
//     It is the responsibility of the caller to delete the DSSNetApp object.
//==============================================================================
AEEResult DSSGlobals::RemoveNetApp(sint15 netHandle)
{
   DSSCritScope cs(*mpCritSect);

   if ((1 > netHandle) || (DSS_MAX_APPS < netHandle)) {
      // TODO: return an error code that indicates 'out of bound'
      return QDS_EBADAPP;
   }

   mapNetAppArray[netHandle-1] = NULL;

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSGlobals::InsertSocket
//
//  DESCRIPTION:
//    Inserts pSocket to sockets array, returns socket handle (FD)
//    allocated to newly inserted socket
//    returns PS_HANDLE_MGR_INVALID_HANDLE (-1) in case of error
//===================================================================
sint15 DSSGlobals::InsertSocket(DSSSocket *pSocket)
{
   DSSCritScope cs(*mpCritSect);
   sint15 index, netHandle;
   int curNumOfSockets;
   int32 socketHandle = PS_HANDLE_MGR_INVALID_HANDLE;

   index = FirstFreeSockIndex(mapSocketArray, DSS_MAX_SOCKS);
   if (-1 == index){
      LOG_MSG_ERROR("InsertSocket cannot find free index for new DSSSocket",
        0, 0, 0);
      goto bail;
   }

   mapSocketArray[index-1] = pSocket;

   // allocate new socket handle (FD)
   // ps_handle_mgr's indexes starts from 0,
   // so decrement received array index by 1
   // see GetSocketById where index is incremented by 1
   socketHandle = ps_handle_mgr_get_handle(PS_HANDLE_MGR_CLIENT_DSS_SOCK_FD, 
                                           index - 1);
   if ( (0 >= socketHandle) ||
        (DSS_MAX_SINT15 < socketHandle) ){
      LOG_MSG_ERROR("InsertSocket cannot allocate socket handle %d for new DSSSocket",
                     socketHandle, 0, 0);
      goto bail;
   }

   LOG_MSG_INFO1("InsertSocket allocated new socket handle %d",
                  socketHandle, 0, 0);

   netHandle = pSocket->GetNetApp();
   if (-1 != netHandle && NULL != mapNetAppArray[netHandle-1]) {
      mapNetAppArray[netHandle-1]->GetNumOfSockets(&curNumOfSockets);
      (void)mapNetAppArray[netHandle-1]->SetNumOfSockets(1 + curNumOfSockets);
   }

/* fall through */
bail:

   // here we can safely cast cause we checked the value before
   return (sint15)socketHandle;
}

//===================================================================
//  FUNCTION:   DSSGlobals::RemoveSocket
//
//  DESCRIPTION:
//     TODO: Documentation
//
//     This function destroys the socket being removed and frees the
//     underlying memory. Existing pointers to the socket may no longer be
//     used after calling this function.
//===================================================================
AEEResult DSSGlobals::RemoveSocket(sint15 sockFd)
{
   DSSCritScope cs(*mpCritSect);
   DSSSocket *pSocket = NULL;
   sint15 netHandle;
   int curNumOfSockets;
   AEEResult res = AEE_SUCCESS;
   byte protocol;
   int16 socketIndex = -1;
#ifndef FEATURE_DSS_LINUX
   if (sockFd <= 0 || sockFd > DSS_MAX_SINT15) {
      LOG_MSG_ERROR("RemoveSocket - bad socket FD %d", sockFd, 0, 0);
      return QDS_EBADF;
   }
#endif
   res = GetSocketById(sockFd, &pSocket, &socketIndex);
   if (AEE_SUCCESS != res){
      LOG_MSG_ERROR("RemoveSocket - cannot find socket FD %d", sockFd, 0, 0);
      goto bail;
   }

   // Decrement the number of sockets associated with the DSSNetApp
   netHandle = pSocket->GetNetApp();
   if (-1 != netHandle) {
      mapNetAppArray[netHandle-1]->GetNumOfSockets(&curNumOfSockets);
      (void)mapNetAppArray[netHandle-1]->SetNumOfSockets(curNumOfSockets - 1);
   }

   // decrement the number of allocated TCP/UDP sockets
   // which is incremented in CreateSocket() on 
   // DSSSocket creation
   pSocket->GetProtocol(&protocol);
   if (PS_IPPROTO_TCP == protocol) {
      mTcpSocketsNum--;
   }
   else if(PS_IPPROTO_UDP == protocol){
      mUdpSocketsNum--;
   }

   DSSCommon::ReleaseIf((IQI**)&pSocket);

   mapSocketArray[socketIndex - 1] = NULL;

   res = AEE_SUCCESS;

/* fall through */

bail:

   return res;
}

//===================================================================
//  FUNCTION:   DSSGlobals::GetSocketById
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
AEEResult DSSGlobals::GetSocketById(sint15 sockfd, 
                                    DSSSocket **ppDSSSocket,
                                    int16 *pSocketIdx)
{
   DSSCritScope cs(*mpCritSect);
   int16        socketIndex = -1;
   sint15       internalSocketFD = PS_HANDLE_MGR_INVALID_HANDLE;

   if (PS_HANDLE_MGR_INVALID_HANDLE == sockfd) {
     LOG_MSG_ERROR("GetSocketById - bad socket FD %d", sockfd, 0, 0);
      return QDS_EBADF;
   }

   socketIndex = ps_handle_mgr_get_index(PS_HANDLE_MGR_CLIENT_DSS_SOCK_FD,
                                         sockfd);
   // socket index in array == index above + 1
   // see InsertSocket where index was deceremented by 1
   socketIndex += 1;
   if ((1 > socketIndex) || (DSS_MAX_SOCKS < socketIndex)) {
     LOG_MSG_ERROR("GetSocketById - bad socket index %d", socketIndex, 0, 0);
     return QDS_EBADF;
   }

   if(NULL == mapSocketArray[socketIndex - 1]){
      LOG_MSG_ERROR("GetSocketById - bad socket index %d", socketIndex, 0, 0);
      return QDS_EBADF;
   }

   // we should check that socket array's item with
   // socketIndex index is not reused
   (mapSocketArray[socketIndex - 1])->GetSockFd(&internalSocketFD);
   if (internalSocketFD != sockfd){
      LOG_MSG_ERROR("GetSocketById - socket FD provided by user %d"
                    "does not match socket FD stored in socket array %d"
                    "at %d index", 
                    sockfd, 
                    internalSocketFD, 
                    socketIndex);
      return QDS_EBADF;
   }

   *ppDSSSocket = mapSocketArray[socketIndex - 1];

   if (NULL != pSocketIdx){
      *pSocketIdx = socketIndex;
   }

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSGlobals::FindSocketWithEvents
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
AEEResult DSSGlobals::FindSocketWithEvents(sint15 dss_nethandle, sint15 *sockfd_ptr, uint32 *pEventOccurred, uint32 *pSigOnOff)
{
   DSSCritScope cs(*mpCritSect);
   for (int i = 0; i < DSS_MAX_SOCKS; i++) {
      if(NULL != mapSocketArray[i]) {
         sint15 nCurNetApp = mapSocketArray[i]->GetNetApp();
         if (dss_nethandle == nCurNetApp) {
            mapSocketArray[i]->GetEventOccurredMask(pEventOccurred);
            mapSocketArray[i]->GetSigOnOffMask(pSigOnOff);
            if (*pEventOccurred & *pSigOnOff) {
               mapSocketArray[i]->GetSockFd(sockfd_ptr);
               return AEE_SUCCESS;
            }
         }
      }
   }

   // nethandle wasn't found.
   return QDS_EBADF;
}

//===================================================================
//  FUNCTION:   DSSGlobals::CreateSocket
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
AEEResult DSSGlobals::CreateSocket(dss_sock_cb_fcn_type sockCb, sint15 sNetApp, sint15* psSockFd, DSSSocket** ppDSSSocket, int kind, byte protocol)
{
   AEEResult res;

   if( ((byte)PS_IPPROTO_TCP == protocol) &&
       (mTcpSocketsNum >= DSS_MAX_TCP_SOCKS) )
   {
      LOG_MSG_ERROR("Can't create any more TCP sockets, maximum number of %d is created",
                     mTcpSocketsNum,0,0);
      return QDS_EMFILE;
   }

   if( ((byte)PS_IPPROTO_UDP == protocol) &&
     (mUdpSocketsNum >= DSS_MAX_UDP_SOCKS) )
   {
      LOG_MSG_ERROR("Can't create any more UDP sockets, maximum number of %d is created",
         mUdpSocketsNum,0,0);
      return QDS_EMFILE;
   }

   *ppDSSSocket = DSSSocket::CreateInstance(&sockCb, sNetApp);
   if (0 == *ppDSSSocket) {
      LOG_MSG_ERROR("Can't allocate DSSSocket", 0, 0, 0);
      return AEE_ENOMEMORY;
   }
   res = (*ppDSSSocket)->Init();
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't init DSSSocket", 0, 0, 0);
      DSSCommon::ReleaseIf((IQI**)ppDSSSocket);
      return res;
   }
   *psSockFd = DSSGlobals::Instance()->InsertSocket(*ppDSSSocket);
   if (PS_HANDLE_MGR_INVALID_HANDLE == *psSockFd) {
      LOG_MSG_ERROR("Too many descriptors are open", 0, 0, 0);
      DSSCommon::ReleaseIf((IQI**)ppDSSSocket);
      return QDS_EMFILE;
   }
   (*ppDSSSocket)->SetSockFd(*psSockFd);
   (void)(*ppDSSSocket)->SetSockKind(kind);
   (*ppDSSSocket)->SetSockProtocol(protocol);

   // increment the number of TCP/UDP sockets
   if((byte)PS_IPPROTO_TCP == protocol){
      mTcpSocketsNum++;
   }
   else if((byte)PS_IPPROTO_UDP == protocol){
      mUdpSocketsNum++;
   }

   return AEE_SUCCESS;
}

//===================================================================
//                 Helper / Internal Functions
//===================================================================

//===================================================================
//  FUNCTION:   FirstFreeSockIndex
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
sint15 FirstFreeSockIndex(DSSSocket *array[], int size)
{
   for (int i = 0; i < size; i++) {
      if (NULL == array[i]) {
         return i + 1;
      }
   }
   return -1;
}

//===================================================================
//  FUNCTION:   FirstFreeNetHandle
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
sint15 FirstFreeNetHandle(DSSPrimaryNetApp *array[], int size)
{
   for (int i = 0; i < size; i++) {
      if (NULL == array[i]) {
         return i+1;
      }
   }
   return -1;
}

//===================================================================
//  FUNCTION:   GetFreeQoSFlowID
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
AEEResult DSSGlobals::GetFreeQoSFlowID(uint8 *pFlowID)
{
   for (int i=0; i<NUM_OF_FLOW_IDS; ++i)
   {
      if (FALSE == mQoSFlowIDs[i])
      {
         *pFlowID = i+1;
         mQoSFlowIDs[i] = TRUE;
         return AEE_SUCCESS;
      }
   }

   return QDS_EINVAL;
}

//===================================================================
//  FUNCTION:   GetFreeMCastFlowID
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
AEEResult DSSGlobals::GetFreeMCastFlowID(uint8 *pFlowID)
{
   for (int i=0; i<NUM_OF_FLOW_IDS; ++i)
   {
      if (FALSE == mMCastFlowIDs[i])
      {
         *pFlowID = i+1;
         mMCastFlowIDs[i] = TRUE;
         return AEE_SUCCESS;
      }
   }

   return QDS_EINVAL;
}

//===================================================================
//  FUNCTION:   GetFreeFlowID
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
void DSSGlobals::ReleaseQoSFlowID(uint8 flowID) throw()
{
   if (flowID > 0){
      mQoSFlowIDs[flowID - 1] = FALSE;
   }
}

//===================================================================
//  FUNCTION:   GetFreeFlowID
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
void DSSGlobals::ReleaseMCastFlowID(uint8 flowID) throw()
{
   if (flowID > 0){
      mMCastFlowIDs[flowID - 1] = FALSE;
   }
}
//===================================================================

//===================================================================
//  FUNCTION:   BuildIfaceIdWithAppId
//
//===================================================================
AEEResult BuildIfaceId(INetwork* pIDSNetwork,sint15 appid,dss_iface_id_type* iface_id)
{
   IfaceIdType ifaceId = 0;
   IDS_ERR_RET(pIDSNetwork->GetIfaceId(&ifaceId));

// we need special treatment in case of STA iface , in this case the original ifaceId from lower layers is returned
// rather than modified by DSS . This is needed because some sta iface tests work directly with ps layers passing them
// the iface id and ps layer expect it to be in the original unmodified format
#ifdef FEATURE_STA_PS_IFACE
   IfaceNameType ifaceName = 0;
   IDS_ERR_RET(pIDSNetwork->GetIfaceName(&ifaceName));
   if (IfaceName::IFACE_STA == ifaceName) {
      *iface_id = (dss_iface_id_type)ifaceId;
      return AEE_SUCCESS;
   }
#endif

   uint8 ifaceIndex;
   ifaceIndex = (uint8)(ifaceId >> 24);
   if (-1 != appid) {
      *iface_id = BuildIfaceIdWithAppId(ifaceIndex, appid);
   } else {
      *iface_id = BuildIfaceIdWithoutAppId(ifaceIndex);
   }

   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   BuildIfaceIdWithAppId
//
//===================================================================
AEEResult BuildIfaceId(IPolicyPriv* pIDSNetworkPolicyPriv,sint15 appid,dss_iface_id_type* iface_id)
{
   IfaceIdType ifaceId = 0;
   IDS_ERR_RET(pIDSNetworkPolicyPriv->GetIfaceId(&ifaceId));

   // we need special treatment in case of STA iface , in this case the original ifaceId from lower layers is returned
   // rather than modified by DSS . This is needed because some sta iface tests work directly with ps layers passing them
   // the iface id and ps layer expect it to be in the original unmodified format
#ifdef FEATURE_STA_PS_IFACE
   IfaceNameType ifaceName;
   IDS_ERR_RET(pIDSNetworkPolicyPriv->GetIfaceName(&ifaceName));
   if (IfaceName::IFACE_STA == ifaceName) {
      *iface_id = (dss_iface_id_type)ifaceId;
      return AEE_SUCCESS;
   }
#endif

   uint8 ifaceIndex;
   ifaceIndex = (uint8)(ifaceId >> 24);
   if (-1 != appid) {
      *iface_id = BuildIfaceIdWithAppId(ifaceIndex, appid);
   } else {
      *iface_id = BuildIfaceIdWithoutAppId(ifaceIndex);
   }

   return AEE_SUCCESS;
}



//===================================================================
//  FUNCTION:   BuildIfaceIdWithAppId
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
uint32 BuildIfaceIdWithAppId(uint8 ifaceIndex, sint15 appid)
{
   uint32 extended_appid;
   uint32 iface_id;

   //ASSERT(appid >= 0);

   // Update the iface id with the iface index, the policy flag will turned off in the returned iface_id.
   iface_id = (uint32)ifaceIndex; // Put the iface index into the iface_id
   iface_id = iface_id << 24; // perform shift left on the iface_id so the iface index will be stored in the MSB byte.

   extended_appid = appid; // put the appid into uint32 buffer.
   extended_appid = extended_appid << 8; // perform shift left on the appid so it will be stored in the second and the third bytes.

   iface_id |= extended_appid; // perform bitwise or in order to store the nethandle into the iface_id buffer.

   return iface_id;
}

//===================================================================
//  FUNCTION:   BuildIfaceIdWithoutAppId
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
uint32 BuildIfaceIdWithoutAppId(uint8 ifaceIndex)
{
   uint32 iface_id;

   iface_id = (uint32)ifaceIndex; // Put the iface index into the iface_id
   iface_id = iface_id << 24; // perform shift left on the iface_id so the iface index will be stored in the MSB byte.
   iface_id |= 0x00800000; // turn on the policy flag that indecated that this iface id was returned calling get_iface_id_by_policy().

   return iface_id;
}

dss_iface_id_type StripAppIdFromIfaceId(dss_iface_id_type iface_id)
{
   return iface_id|= 0x00FFFF00;
}

AEEResult DSSGlobals::AddNatManager(INatSessionPriv* natSession)
{
   DSSCritScope cs(*mpCritSect);
   mNatSession = natSession;
   (void)mNatSession->AddRef();
   return AEE_SUCCESS;
}

AEEResult DSSGlobals::DeleteNatManager(INatSessionPriv* natSession)
{
   DSSCritScope cs(*mpCritSect);
   if (NULL != mNatSession)
   {
       DSSCommon::ReleaseIf ((IQI **) &mNatSession);
   }

   return AEE_SUCCESS;
}

AEEResult DSSGlobals::GetNatManager(INatSessionPriv** natSession)
{
   DSSCritScope cs(*mpCritSect);

   ASSERT (NULL != natSession);
   *natSession = 0;

   while (NULL != mNatSession) 
   {
       *natSession = mNatSession;
       return AEE_SUCCESS;
   }
   
   // We haven't found any Nat Manager
   *natSession = NULL;
   return QDS_EBADF;
}

AEEResult DSSGlobals::AddFilterRegObjectToList(IIPFilterRegPriv* filterReg, dss_iface_id_type iface_id, sint15 sockfd)
{
   DSSCritScope cs(*mpCritSect);
   FilterRegInfo* newFilterRegInfo = PS_MEM_NEW(FilterRegInfo);
   if (NULL == newFilterRegInfo) {
      LOG_MSG_ERROR( "Couldn't allocate FilterRegInfo", 0, 0, 0);
      return AEE_ENOMEMORY;
   }

   newFilterRegInfo->filterReg = filterReg;
   (void)newFilterRegInfo->filterReg->AddRef();
   newFilterRegInfo->iface_id = iface_id;
   newFilterRegInfo->sockfd = sockfd;

   newFilterRegInfo->next = mFilterRegInfoList;
   mFilterRegInfoList = newFilterRegInfo;

   return AEE_SUCCESS;
}

AEEResult DSSGlobals::RemoveFilterRegObjectFromList(dss_iface_id_type iface_id, sint15 sockfd)
{
  // match both socket fd and interface id
  return RemoveAllMatchingFilterRegObjectFromList(iface_id, sockfd);
}

AEEResult DSSGlobals::RemoveFilterRegObjectFromList(sint15 sockfd)
{
   // match only socket fd
   return RemoveAllMatchingFilterRegObjectFromList(DSS_IFACE_INVALID_ID, sockfd);
}

AEEResult DSSGlobals::RemoveAllMatchingFilterRegObjectFromList(dss_iface_id_type iface_id, sint15 sockfd)
{
  FilterRegInfo* save, *curr, *prev;
  FilterRegInfo * head = NULL;
  boolean bFilterIsFound = FALSE;
  boolean bHeadIsFound = FALSE;

  save = NULL;
  prev = NULL;
  curr = mFilterRegInfoList;

  // if the list is empty return.
  if (NULL == mFilterRegInfoList) {
    return AEE_SUCCESS;
  }

  mpCritSect->Enter();

  while (curr){
    boolean bMatchingCondition = 
      (DSS_IFACE_INVALID_ID == iface_id) ? 
      (sockfd == curr->sockfd) : (iface_id == curr->iface_id && sockfd == curr->sockfd); 
    if (TRUE == bMatchingCondition) {
      save = curr->next;
      if (prev){
        prev->next = save;
      }
      curr->next = head;
      head = curr;
      bFilterIsFound = TRUE;
      curr = save;
    }
    else{
      if (!bHeadIsFound){
        mFilterRegInfoList = curr;
        bHeadIsFound = TRUE;
      }
      prev = curr;
      curr = curr->next;
    }
  }

  mpCritSect->Leave();

  if (!bHeadIsFound){
    mFilterRegInfoList = NULL;
  }

  while (head)
  {
    curr = head;
    head = head->next;
      DSSCommon::ReleaseIf((IQI**)&curr->filterReg);
      PS_MEM_DELETE(curr);
  }
  return (TRUE == bFilterIsFound) ? AEE_SUCCESS : QDS_EBADF;
}

// This function is called from different places in code with different iface_id formats
// If one of iface_id has it policy bit on (meaning that it was created either with dss_get_iface_id_by_policy
// or with IDSNetworkPriv) , then just compare the iface index , otherwise compare everything
bool CompareIfaceIds(dss_iface_id_type iface_id1,dss_iface_id_type iface_id2)
{
   if ((iface_id1 & POLICY_FLAG) ||
       (iface_id2 & POLICY_FLAG))
   {
      return ((iface_id1 >> 24) == (iface_id2 >> 24));
   } else {
      return (iface_id1 == iface_id2);
   }
}

bool IsQoSIoctl(dss_iface_ioctl_type ioctl_name)
{
   switch (ioctl_name) {
      case DSS_IFACE_IOCTL_QOS_MODIFY:
      case DSS_IFACE_IOCTL_QOS_RELEASE:
      case DSS_IFACE_IOCTL_QOS_SUSPEND:
      case DSS_IFACE_IOCTL_QOS_RESUME:
      //case DSS_IFACE_IOCTL_PRIMARY_QOS_GET_GRANTED_FLOW_SPEC: // this operation can be performed on temporary app
      case DSS_IFACE_IOCTL_QOS_GET_STATUS:
      case DSS_IFACE_IOCTL_QOS_GET_GRANTED_FLOW_SPEC:
      case DSS_IFACE_IOCTL_QOS_GET_GRANTED_FLOW_SPEC2:
      case DSS_IFACE_IOCTL_QOS_RELEASE_EX:
      case DSS_IFACE_IOCTL_QOS_SUSPEND_EX:
      case DSS_IFACE_IOCTL_QOS_RESUME_EX:
      case DSS_IFACE_IOCTL_707_HDR_GET_RMAC3_INFO:
      case DSS_IFACE_IOCTL_707_GET_TX_STATUS:
      case DSS_IFACE_IOCTL_707_GET_INACTIVITY_TIMER:
      case DSS_IFACE_IOCTL_707_SET_INACTIVITY_TIMER:
         return TRUE;
      default:
         return FALSE;
   }
}



sint15 GetNetHandleFromQoSRequestHandle(dss_iface_ioctl_type ioctl_name,void* argval_ptr)
{
   dss_qos_handle_type qos_handle;
   switch (ioctl_name) {
      case DSS_IFACE_IOCTL_QOS_MODIFY:
         qos_handle = ((dss_iface_ioctl_qos_modify_type*)argval_ptr)->handle;
         break;
      case DSS_IFACE_IOCTL_QOS_RELEASE:
         qos_handle = ((dss_iface_ioctl_qos_release_type*)argval_ptr)->handle;
         break;
      case DSS_IFACE_IOCTL_QOS_SUSPEND:
         qos_handle = ((dss_iface_ioctl_qos_suspend_type*)argval_ptr)->handle;
         break;
      case DSS_IFACE_IOCTL_QOS_RESUME:
         qos_handle = ((dss_iface_ioctl_qos_resume_type*)argval_ptr)->handle;
         break;
      case DSS_IFACE_IOCTL_QOS_GET_STATUS:
         qos_handle = ((dss_iface_ioctl_qos_get_status_type*)argval_ptr)->handle;
         break;
      case DSS_IFACE_IOCTL_QOS_GET_GRANTED_FLOW_SPEC:
         qos_handle = ((dss_iface_ioctl_qos_get_flow_spec_type*)argval_ptr)->handle;
         break;
      case DSS_IFACE_IOCTL_QOS_GET_GRANTED_FLOW_SPEC2:
         qos_handle = ((dss_iface_ioctl_qos_get_granted_flow_spec2_type*)argval_ptr)->handle;
         break;
      case DSS_IFACE_IOCTL_QOS_RELEASE_EX:
      case DSS_IFACE_IOCTL_QOS_SUSPEND_EX:
      case DSS_IFACE_IOCTL_QOS_RESUME_EX:
         if (NULL == ((dss_iface_ioctl_qos_release_ex_type*)argval_ptr)->handles_ptr) {
            return -1;
         }
         qos_handle = ((dss_iface_ioctl_qos_release_ex_type*)argval_ptr)->handles_ptr[0];
         break;

      default:
         return -1;
   }

   // qos_handle does not contain app id (probably error)
   if (qos_handle & POLICY_FLAG) {
      return -1;
   }

   return static_cast<sint15>((qos_handle & NET_HANDLE_MASK) >> 8);

}

void * DSSGlobals::operator new
(
   unsigned int numBytes
) throw()
{
   return ps_mem_get_buf( PS_MEM_DSAL_GLOBALS_TYPE);
} /* DSSGlobals::operator new() */


void DSSGlobals::operator delete
(
   void *  bufPtr
)
{
   PS_MEM_FREE(bufPtr);
   return;
} /* DSSGlobals::operator delete() */

