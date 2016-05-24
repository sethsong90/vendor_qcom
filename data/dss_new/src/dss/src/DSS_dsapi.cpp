/*======================================================

FILE:  DSS_dsapi.cpp

SERVICES:

GENERAL DESCRIPTION:
Implementation of legacy ds API - entry point for legacy applications.

=====================================================

Copyright (c) 2008-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_dsapi.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-10-28 aa  Support for truncate flags added to dss_recvmsg and dss_recvfrom
  2010-04-13 en  History added.

===========================================================================*/

// TODO: if anything here needs locking, it should use the relevant DSSSocket's
// critical section - using DSSSocket::GetCritSect().


//===================================================================
//   Includes and Public Data Declarations
//===================================================================

//-------------------------------------------------------------------
// Include Files
//-------------------------------------------------------------------
#include "DSS_Common.h"
#include "customer.h"
#include "dssocket.h"
#include "dssicmp_api.h"
#include "dserrno.h"
#include "DSS_Conversion.h"
#include "AEEStdErr.h"
#include "ds_Utils_Conversion.h"
#include "IDSMUtils.h"
#include "ds_Net_CreateInstance.h"
#include "ds_Net_INetworkFactoryPriv.h"
#include "ds_Net_CNetworkFactoryPriv.h"
#include "ds_Sock_Def.h"
#include "ds_Sock_IICMPErrInfoPriv.h"
#include "ds_Sock_IRecvIFInfoPriv.h"
#include "ds_Sock_ISocketLocalPriv.h"
#include "ds_Sock_AddrUtils.h"
#include "DSS_Globals.h"
#include "DSS_Socket.h"
#include "DSS_IDSNetPolicyPrivScope.h"
#include "DSS_IDSNetPolicyScope.h"
#include "DSS_MemoryManagement.h"
#include "ds_Addr_Def.h"
#include "DSS_GenScope.h"
#include "ds_Utils_SockAddrInternalTypes.h"
#include "ps_in.h"
#include "ds_Net_CreateInstance.h"
#include "DSS_IDSNetworkScope.h"
#include "ps_system_heap.h"

using namespace ds::Net;
using namespace ds::Sock;
#ifndef min
#define min(a,b)  ((a) < (b) ? (a):(b))
#endif

//-------------------------------------------------------------------
// Constant / Define Declarations
//-------------------------------------------------------------------

const struct ps_in6_addr ps_in6addr_any = PS_IN6ADDR_ANY_INIT;            /* :: */
const struct ps_in6_addr ps_in6addr_loopback = PS_IN6ADDR_LOOPBACK_INIT ; /* ::1 */


//-------------------------------------------------------------------
// Type Declarations (typedef, struct, enum, etc.)
//-------------------------------------------------------------------
// Structures for notifying ICMP errors to applications
typedef struct {
  dss_sock_extended_err ee;
  struct ps_sockaddr_in ps_sin_addr;
} dssocki_err_msg_in;

//-------------------------------------------------------------------
// Global Constant Data Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Global Data Declarations
//-------------------------------------------------------------------

extern "C"
{
//-------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------
void dssocki_net_cb(sint15 dss_nethandle, dss_iface_id_type iface_id, sint15 dss_errno, void *user_data_ptr);
void dssocki_sock_cb(sint15 dss_nethandle, sint15 sockfd, uint32 event_mask, void *user_data_ptr);

//===================================================================
//                 Helper / Internal Functions
//===================================================================

/*===========================================================================
FUNCTION DSSOCKI_NET_CB()
DESCRIPTION
   This function is a common network call back function for application
   using old opennetlib function.

DEPENDENCIES
   NOTE: This function must be called in a TASKLOCK().

RETURN VALUE
   None

SIDE EFFECTS
   Calls the user specified network call back which is passed in user_data_ptr.
===========================================================================*/
void dssocki_net_cb
(
   sint15            dss_nethandle,
   dss_iface_id_type iface_id,
   sint15            dss_errno,
   void            * user_data_ptr
)
{
   /*lint -save -e611 we need to cast one function type to another, for that we first cast net/socket_callback_fcn to "void*" and then cast it back to another
   function type, using helper functions dssocki_net/sock_cb */
   if(user_data_ptr != NULL)
   {
      ((void (*)(void *))user_data_ptr)((void *)&dss_nethandle);
   }
   /*lint -restore Restore lint error 611*/
} /* dssocki_net_cb() */

/*===========================================================================
FUNCTION DSSOCKI_SOCK_CB()
DESCRIPTION
   This function is a common socket call back function for application
   using old opennetlib function.

DEPENDENCIES
   NOTE: This function must be called in a TASKLOCK().

RETURN VALUE
   None

SIDE EFFECTS
   Calls the user specified socket call back which is passed in user_data_ptr.
===========================================================================*/
void dssocki_sock_cb
(
   sint15 dss_nethandle,
   sint15 sockfd,
   uint32 event_mask,
   void * user_data_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
   /*lint -save -e611 we need to cast one function type to another, for that we first cast net/socket_callback_fcn to "void*" and then cast it back to another
   function type, using helper functions dssocki_net/sock_cb */
   if(user_data_ptr != NULL) {
      ((void (*)(void *))user_data_ptr)((void *)&dss_nethandle);
   }
   /*lint -restore Restore lint error 611*/
} /* dssocki_sock_cb() */

//===================================================================
//                 ds API Functions
//===================================================================
/*==========================================================================
FUNCTION DSS_OPEN_NETLIB()

DESCRIPTION

  Opens up the network library.  Assigns application ID and sets the
  application-defined callback functions to be called when library and
  socket calls would make progress.  The callback are called with a pointer
  to a sint15 containing the application ID for the callback.
  NOTE: the memory for the application ID is ephemeral and likely will not be
    available after the callback returns - if it is desired to use this
    information outside the scope of the callback it should be COPIED, a
    pointer MUST not be used.

  Puts data services manager into "socket" mode.

  This function is called from the context of the socket client's task.

  This function is wrapper around dss_opennetlib2. This calls opennetlib2
  with standard network and socket call back functions and user data as
  the user specified call back functions. The standard socket and network
  call back functions call the user specified call back functions.

DEPENDENCIES
  None.

RETURN VALUE
  Returns application ID on success.

  On error, return DSS_SUCCESS and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EMAPP      no more applications available - max apps exceeded.

SIDE EFFECTS
  Puts data services manager into "socket" mode.
===========================================================================*/
sint15 dss_open_netlib
(
  void   (*net_callback_fcn)(void *),         /* network callback function */
  void   (*socket_callback_fcn)(void *),       /* socket callback function */
  sint15 *dss_errno                               /* error condition value */
)
{

   LOG_MSG_FUNCTION_ENTRY("dss_open_netlib(): net_callback_fcn 0x%p, socket_callback_fcn 0x%p", net_callback_fcn, socket_callback_fcn, 0);
   /*lint -save -e611 we need to cast one function type to another, for that we first cast net/socket_callback_fcn to "void*" and then cast it back to another
     function type, using helper functions dssocki_net/sock_cb */
   return dss_open_netlib2(dssocki_net_cb, (void *)net_callback_fcn,
                           dssocki_sock_cb, (void *)socket_callback_fcn,
                           NULL, dss_errno);

   /*lint -restore Restore lint error 611*/
}

/*===========================================================================
FUNCTION DSS_OPEN_NETLIB2()

DESCRIPTION

  Opens up the network library.  Assigns application ID and sets the
  application-defined callback functions to be called when library and
  socket calls would make progress. Stores the network policy info and
  uses it in further calls.

  Puts data services manager into "socket" mode.

  This function is called from the context of the socket client's task.

DEPENDENCIES
  None.

RETURN VALUE
  Returns application ID on success.

  On error, return DSS_SUCCESS and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EMAPP      no more applications available - max apps exceeded.
  DS_EFAULT     Policy structure is uninitialized.

SIDE EFFECTS
  Puts data services manager into "socket" mode.
===========================================================================*/
sint15 dss_open_netlib2
(
  dss_net_cb_fcn net_cb,                      /* network callback function */
  void * net_cb_user_data,              /* User data for network call back */
  dss_sock_cb_fcn sock_cb,                     /* socket callback function */
  void * sock_cb_user_data,               /*User data for socket call back */
  dss_net_policy_info_type * policy_info_ptr,       /* Network policy info */
  sint15 * dss_errno                              /* error condition value */
)
{
   DSSNetApp* pNetApp = NULL;
   dss_sock_cb_fcn_type socketCallback;
   DSSIDSNetPolicyPrivScope IDSNetPolicyPrivScope;
   INetworkFactoryPriv* pIDSNetworkFactoryPriv = NULL;
   INetworkPriv* pIDSNetworkPriv = NULL;
   sint15 sNetHandle;
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY("dss_open_netlib2(): net_cb 0x%p, sock_cb 0x%p policy_info ptr 0x%p", net_cb, sock_cb, policy_info_ptr);

   IDS_ERR_RET_ERRNO(IDSNetPolicyPrivScope.Init());

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      return DSS_ERROR;
   }

   // let's check if there any space left
   sNetHandle = DSSGlobals::Instance()->InsertNetApp(NULL);
   if (-1 == sNetHandle) {
      LOG_MSG_ERROR("No more applications available", 0, 0, 0);
      *dss_errno = DS_EMAPP;
      return DSS_ERROR;
   }

   pNetApp = DSSPrimaryNetApp::CreateInstance();
   if (NULL == pNetApp) {
      LOG_MSG_ERROR("Can't allocate DSSNetApp", 0, 0, 0);
      *dss_errno = DS_ENOMEM;
      return DSS_ERROR;
   }

   sNetHandle = DSSGlobals::Instance()->InsertNetApp(pNetApp);

   pNetApp->SetNetHandle(sNetHandle);
   pNetApp->SetIfaceId(INVALID_IFACE_ID);

   pNetApp->SetNetworkCallback(net_cb);
   pNetApp->SetNetworkUserData(net_cb_user_data);

   socketCallback.sock_cb_fcn = sock_cb;
   socketCallback.sock_cb_user_data = sock_cb_user_data;
   pNetApp->SetSocketCallback(&socketCallback);

   res = DSSConversion::DS2IDSNetPolicy(policy_info_ptr, IDSNetPolicyPrivScope.Fetch());
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't convert Net Policy", 0, 0, 0);
      if (AEE_SUCCESS != DSSGlobals::Instance()->RemoveNetApp(sNetHandle)) {
         LOG_MSG_ERROR("Internal error: Failed to remove NetApp from apps list", 0, 0, 0);
      }
      (void) pNetApp->Release();
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   res = DSSGlobals::Instance()->GetNetworkFactoryPriv(&pIDSNetworkFactoryPriv);

   if (AEE_SUCCESS != res) {
      if (AEE_SUCCESS != DSSGlobals::Instance()->RemoveNetApp(sNetHandle)) {
         LOG_MSG_ERROR("Internal error: Failed to remove NetApp from apps list", 0, 0, 0);
      }
      (void) pNetApp->Release();
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   res = pIDSNetworkFactoryPriv->CreateNetworkPriv(IDSNetPolicyPrivScope.Fetch(), &pIDSNetworkPriv);
   if (AEE_SUCCESS != res) {
      if (AEE_SUCCESS != DSSGlobals::Instance()->RemoveNetApp(sNetHandle)) {
         LOG_MSG_ERROR("Internal error: Failed to remove NetApp from apps list", 0, 0, 0);
      }

      (void) pNetApp->Release();
      DSSCommon::ReleaseIf((IQI**)&pIDSNetworkPriv);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   // DSSNetApp takes care to AddRef pIDSNetworkPriv once it is set
   res = pNetApp->Init(pIDSNetworkPriv);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't Initialize DSSNetApp", 0, 0, 0);
      if (AEE_SUCCESS != DSSGlobals::Instance()->RemoveNetApp(sNetHandle)) {
         LOG_MSG_ERROR("Internal error: Failed to remove NetApp from apps list", 0, 0, 0);
      }
      (void) pNetApp->Release();
      DSSCommon::ReleaseIf((IQI**)&pIDSNetworkPriv);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   res = pNetApp->SetPolicy(IDSNetPolicyPrivScope.Fetch());
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("SetPolicy operation failed", 0, 0, 0);
      if (AEE_SUCCESS != DSSGlobals::Instance()->RemoveNetApp(sNetHandle)) {
         LOG_MSG_ERROR("Internal error: Failed to remove NetApp from apps list", 0, 0, 0);
      }
      (void) pNetApp->Release();
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   res = pNetApp->SetLegacyPolicy(policy_info_ptr);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("SetPolicy operation failed", 0, 0, 0);
      if (AEE_SUCCESS != DSSGlobals::Instance()->RemoveNetApp(sNetHandle)) {
         LOG_MSG_ERROR("Internal error: Failed to remove NetApp from apps list", 0, 0, 0);
      }
      (void) pNetApp->Release();
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }


   pNetApp->SetNumOfSockets(0);

   DSSCommon::ReleaseIf((IQI**)&pIDSNetworkPriv);

   return sNetHandle;
}
#ifndef FEATURE_DSS_LINUX
/*===========================================================================
FUNCTION DSS_SOCKET()

DESCRIPTION
 Create a socket and related data structures,and return a socket descriptor.

 The mapping to actual protocols is as follows:

 ADDRESS FAMILY         Stream          Datagram

 DSS_AF_INET                TCP             UDP

  Note this function must be called to obtain a valid socket descriptor, for
  use with all other socket-related functions.  Thus, before any socket
  functions can be used (e.g. I/O, asynchronous notification, etc.), this
  call must have successfully returned a valid socket descriptor.  The
  application must also have made a call to dss_open_netlib() to obtain
  a valid application ID, and to put the Data Services task into "sockets"
  mode.

  Note:  This implementation version has no support for Raw IP sockets, and
         will return an error, if the application attempts to create one.

         Sockets created using this call are bound to the dss_nethandle used in
         creating this socket.

DEPENDENCIES
  The function dss_open_netlib() must be called to open the network library
  and put the ds/PS managers into sockets mode.

RETURN VALUE
  On successful creation of a socket, this function returns socket file
  descriptor which is a sint15 value between 0x1000 and 0x1FFF.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EAFNOSUPPORT     address family not supported
  DS_EBADAPP          invalid application ID
  DS_ESOCKNOSUPPORT   invalid or unsupported socket parameter specified
  DS_EPROTONOSUPPORT  invalid or unsupported protocol
  DS_EMFILE           too many descriptors open.  A socket is already open
                      or has not closed completely.

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_socket
(
  sint15 dss_nethandle,
  byte   family,                      /* Address family - DSS_AF_INET only */
  byte   type,                                              /* socket type */
  byte   protocol,                                        /* protocol type */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSNetApp *pNetApp = NULL;
   ds::Sock::ISocketFactory *pSockFactory = NULL;
   ds::AddrFamilyType IDSAddrFamily;
   ds::Sock::SocketType IDSSockType;
   ds::Sock::ProtocolType IDSProtocol;
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   INetwork* pIDSNetwork = NULL;
   DSSIDSNetPolicyScope IDSNetPolicyScope;
   dss_sock_cb_fcn_type sockCb;
   sint15 sSockFd, sRet;
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY("dss_socket(): NetHandle: %d, family:%d, type:%d", dss_nethandle, family, type);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetNetApp(dss_nethandle, &pNetApp);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid application ID", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);;
      sRet = DSS_ERROR;
      goto bail;
   }

   pNetApp->GetSocketCallback(&sockCb);

   res = DSSConversion::DS2IDSAddrFamily((int) family,&IDSAddrFamily);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't convert Address Family", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }
   res = DSSConversion::DS2IDSSockType((int)type, &IDSSockType);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't convert Socket type", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }
   if (0 == protocol) {
      protocol = ((byte)DSS_SOCK_STREAM == type) ? (byte)PS_IPPROTO_TCP:(byte)PS_IPPROTO_UDP;
   }
   res = DSSConversion::DS2IDSProtocol((int)protocol, &IDSProtocol);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't convert Protocol type", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->CreateSocket(sockCb, dss_nethandle, &sSockFd, &pDSSSocket, 1, protocol);

   if (AEE_ENOMEMORY == res || 0 == pDSSSocket) {
      res = QDS_EMFILE;
   }

   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   DSSGlobals::Instance()->GetSockFactory(&pSockFactory);
   if (NULL == pSockFactory) {
      LOG_MSG_ERROR ("dss_socket: pSockFactory is NULL",0, 0, 0); 
      ASSERT(0);
   }

   res = pNetApp->GetIDSNetworkObject(&pIDSNetwork);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Failed to fetch IDSNetwork object: %d", res, 0, 0);
   }
   if (NULL != pIDSNetwork) {
      res = pSockFactory->CreateSocketByNetwork(IDSAddrFamily, IDSSockType, IDSProtocol, pIDSNetwork, &pIDSSocket);
   }
   else {
      res = IDSNetPolicyScope.Init(pNetApp); //TODO: Is this correct? do we need this check if we have the second one
      if (AEE_SUCCESS != res) { //If Init failed then we will check if the member of the scop is NULL (?)
         LOG_MSG_ERROR("Warning: There is no policy set ", 0, 0, 0);
      }

      res = pSockFactory->CreateSocket(
                                         IDSAddrFamily,
                                         IDSSockType,
                                         IDSProtocol,
                                         IDSNetPolicyScope.Fetch(),
                                         FALSE,
                                         &pIDSSocket
                                      );
   }

   if (AEE_ENOMEMORY == res) {
      res = QDS_EMFILE;
   }

   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't create IDSSocket", 0, 0, 0);
      (void) DSSGlobals::Instance()->RemoveSocket(sSockFd);

      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->SetIDSSocket(pIDSSocket);

   sRet = sSockFd;

bail:
   DSSCommon::ReleaseIf((IQI**)&pSockFactory);
   DSSCommon::ReleaseIf((IQI**)&pIDSNetwork);
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_SOCKET2()

DESCRIPTION
 Create a socket and related data structures, and return a socket descriptor.

 The mapping to actual protocols is as follows:

 ADDRESS FAMILY         Stream          Datagram

 DSS_AF_INET                TCP             UDP

  Note this function must be called to obtain a valid socket descriptor, for
  use with all other socket-related functions.  Thus, before any socket
  functions can be used (e.g. I/O, asynchronous notification, etc.), this
  call must have successfully returned a valid socket descriptor.  The
  application must also have made a call to dss_open_netlib() to obtain
  a valid application ID, and to put the Data Services task into "sockets"
  mode.

  Note:  This implementation version has no support for Raw IP sockets, and
         will return an error, if the application attempts to create one.

         Sockets created using socket2 are not bound to any particular dss_nethandle.

DEPENDENCIES
  Netpolicy structure needs to be initialized by calling dss_init_netpolicy.

RETURN VALUE
  On successful creation of a socket, this function returns socket file
  descriptor which is a sint15 value between 0x1000 and 0x1FFF.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EAFNOSUPPORT     address family not supported
  DS_EBADAPP          invalid application ID
  DS_EPROTOTYPE       specified protocol invalid for socket type
  DS_ESOCKNOSUPPORT   invalid or unsupported socket parameter specified
  DS_EPROTONOSUPPORT  specified protocol not supported
  DS_EMFILE           too many descriptors open.  A socket is already open or
                      has not closed completely.

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_socket2
(
  byte   family,                                         /* Address family */
  byte   type,                                              /* socket type */
  byte   protocol,                                        /* protocol type */
  dss_sock_cb_fcn sock_cb,                     /* socket callback function */
  void * sock_cb_user_data,              /* User data for socket call back */
  dss_net_policy_info_type * policy_info_ptr,       /* Network policy info */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocketFactory *pSockFactory = NULL;
   ds::AddrFamilyType IDSAddrFamily;
   ds::Sock::SocketType IDSSockType;
   ds::Sock::ProtocolType IDSProtocol;
   DSSIDSNetPolicyPrivScope IDSNetPolicyPrivScope;
   ds::Sock::ISocket *pIDSSocket = NULL;
   sint15 sSockFd;
   sint15 sRet = DSS_SUCCESS;
   AEEResult res;
   ds::Net::IPolicyPriv* pIPolicy = NULL;

   LOG_MSG_FUNCTION_ENTRY("dss_socket2(): family:%d, type:%d, protocol:%d", family, type, protocol);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSConversion::DS2IDSAddrFamily((int) family, &IDSAddrFamily);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't convert Address Family", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }
   res = DSSConversion::DS2IDSSockType((int)type, &IDSSockType);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't convert Socket type", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }
   if (0 == protocol) {
      protocol = ((byte)DSS_SOCK_STREAM == type) ? (byte)PS_IPPROTO_TCP:(byte)PS_IPPROTO_UDP;
   }
   res = DSSConversion::DS2IDSProtocol((int)protocol, &IDSProtocol);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't convert Protocol type", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   if (NULL != policy_info_ptr) {
      res = IDSNetPolicyPrivScope.Init();
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("Can't create DSNetPolicy", 0, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
         goto bail;
      }

      res = DSSConversion::DS2IDSNetPolicy(policy_info_ptr, IDSNetPolicyPrivScope.Fetch());
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("Can't convert Net Policy", 0, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   dss_sock_cb_fcn_type sockCb;
   sockCb.sock_cb_fcn = sock_cb;
   sockCb.sock_cb_user_data = sock_cb_user_data;
   res = DSSGlobals::Instance()->CreateSocket(sockCb, -1, &sSockFd, &pDSSSocket, 2, protocol);
   if (AEE_ENOMEMORY == res) {
      res = QDS_EMFILE;
   }
   if ((AEE_SUCCESS != res) || (NULL == pDSSSocket)){
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   if ((NULL != policy_info_ptr)) {
      res = pDSSSocket->SetLegacyPolicy(policy_info_ptr);
      if (AEE_SUCCESS != res) {
         (void) DSSGlobals::Instance()->RemoveSocket(sSockFd);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   DSSGlobals::Instance()->GetSockFactory(&pSockFactory);
   if (NULL == pSockFactory) {
      LOG_MSG_ERROR ("dss_socket2: pSockFactory is NULL",0, 0, 0); 
      ASSERT(0);
   }

   pIPolicy = IDSNetPolicyPrivScope.Fetch();

   res = pSockFactory->CreateSocket(
                                     IDSAddrFamily,
                                     IDSSockType,
                                     IDSProtocol,
                                     pIPolicy,
                                     FALSE,
                                     &pIDSSocket
                                   );

   if (AEE_ENOMEMORY == res) {
      res = QDS_EMFILE;
   }

   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't create IDSSocket", 0, 0, 0);

      (void) DSSGlobals::Instance()->RemoveSocket(sSockFd);

      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->SetIDSSocket(pIDSSocket);

   sRet = sSockFd;

bail:
   DSSCommon::ReleaseIf((IQI**)&pSockFactory);
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}

sint15 dssicmp_socket
(
  dssicmp_icmp_ver_type  icmp_ver,
  uint8  type,
  uint8  code,
  dss_sock_cb_fcn sock_cb,                     /* socket callback function */
  void * sock_cb_user_data,              /* User data for socket call back */
  dss_net_policy_info_type * policy_info_ptr,       /* Network policy info */
  sint15 *dss_errno                               /* error condition value */
)
{
    DSSSocket *pDSSSocket = NULL;
    ds::Sock::ISocketFactory *pSockFactory = NULL;
    ds::Sock::ISocket *pIDSSocket = NULL;
    ds::Sock::ISocketLocalPriv *pIDSPrivSokect = NULL;
    sint15 sSockFd;
    AEEResult res;
    DSSIDSNetPolicyPrivScope IDSNetPolicyPrivScope;
    ds::AddrFamilyType IDSAddrFamily;
    ds::Sock::SocketType IDSSockType;
    ds::Sock::ProtocolType IDSProtocol;
    ds::Sock::OptLevelType IDSLevel;
    ds::Sock::OptNameTypePriv IDSOptName;
    byte protocol = 0;

    LOG_MSG_FUNCTION_ENTRY("dssicmp_socket(): type:%u, code:%u", type, code, 0);

    if (NULL == dss_errno) {
       LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
       return DSS_ERROR;
    }

    if(icmp_ver == DSSICMP_ICMP_V4){
      protocol = (byte)PS_IPPROTO_ICMP;
    }
    else{
      protocol = (byte)PS_IPPROTO_ICMP6;
    }

    dss_sock_cb_fcn_type sockCb;
    sockCb.sock_cb_fcn = sock_cb;
    sockCb.sock_cb_user_data = sock_cb_user_data;
    res = DSSGlobals::Instance()->CreateSocket(sockCb, -1, &sSockFd, &pDSSSocket, 2, protocol);
    if (AEE_ENOMEMORY == res) {
       res = QDS_EMFILE;
    }
    if ((AEE_SUCCESS != res) || (NULL == pDSSSocket)){
       *dss_errno = DSSConversion::IDS2DSErrorCode(res);
       return DSS_ERROR;
    }

    if (NULL != policy_info_ptr) {
       res = pDSSSocket->SetLegacyPolicy(policy_info_ptr);
       if (AEE_SUCCESS != res) {
          (void) DSSGlobals::Instance()->RemoveSocket(sSockFd);

          *dss_errno = DSSConversion::IDS2DSErrorCode(res);
          return DSS_ERROR;
       }
    }

    if(icmp_ver != DSSICMP_ICMP_V4 && icmp_ver != DSSICMP_ICMP_V6)
    {
       *dss_errno = DS_EFAULT;
        return DSS_ERROR;
    }

    if (DSSICMP_ICMP_V4 == icmp_ver) {
       IDSAddrFamily =ds::AddrFamily::QDS_AF_INET;
    } else {
       IDSAddrFamily = ds::AddrFamily::QDS_AF_INET6;
    }

    IDSSockType = ds::Sock::Type::QDS_DGRAM;

    IDSProtocol =  ds::Sock::Protocol::QDS_ICMP;

    DSSGlobals::Instance()->GetSockFactory(&pSockFactory);
    if (NULL == pSockFactory) {
       LOG_MSG_ERROR ("dssicmp_socket: pSockFactory is NULL",0, 0, 0); 
       ASSERT(0);
    }

    if (NULL != policy_info_ptr) {
       res = IDSNetPolicyPrivScope.Init();
       if (AEE_SUCCESS != res) {
          LOG_MSG_ERROR("Can't create DSNetPolicy", 0, 0, 0);
          *dss_errno = DSSConversion::IDS2DSErrorCode(res);;
          return DSS_ERROR;
       }

       res = DSSConversion::DS2IDSNetPolicy(policy_info_ptr, IDSNetPolicyPrivScope.Fetch());
       if (AEE_SUCCESS != res) {
          LOG_MSG_ERROR("Can't convert Net Policy", 0, 0, 0);
          *dss_errno = DSSConversion::IDS2DSErrorCode(res);
          return DSS_ERROR;
       }
    }

    res = pSockFactory->CreateSocket(
                                       IDSAddrFamily,
                                       IDSSockType,
                                       IDSProtocol,
                                       IDSNetPolicyPrivScope.Fetch(),
                                       FALSE,
                                       &pIDSSocket
                                    );

    if (AEE_ENOMEMORY == res) {
       res = QDS_EMFILE;
    }

    if (AEE_SUCCESS != res) {
       LOG_MSG_ERROR("Can't create IDSSocket", 0, 0, 0);
       (void) DSSGlobals::Instance()->RemoveSocket(sSockFd);

       *dss_errno = DSSConversion::IDS2DSErrorCode(res);
       return DSS_ERROR;
    }
    DSSGenScope scopeSocket(pIDSSocket,DSSGenScope::IDSIQI_TYPE);

    res = pIDSSocket->QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv, (void**)&(pIDSPrivSokect));
    if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv) failed: %d", res, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sSockFd = DSS_ERROR;
      goto bail;
    }

    IDSLevel = ds::Sock::OptLevel::QDS_LEVEL_ICMP;
    IDSOptName = ds::Sock::OptNamePriv::QDS_ICMP_CODE;

    res = pIDSPrivSokect->SetOptPriv(IDSLevel, IDSOptName,(int)code);
    if (AEE_SUCCESS != res) {
       LOG_MSG_ERROR("SetOpt operation failed", 0, 0, 0);
       *dss_errno = DSSConversion::IDS2DSErrorCode(res);
       (void) DSSGlobals::Instance()->RemoveSocket(sSockFd);
       sSockFd = DSS_ERROR;
       goto bail;
    }

    IDSOptName = ds::Sock::OptNamePriv::QDS_ICMP_TYPE;

    res = pIDSPrivSokect->SetOptPriv(IDSLevel, IDSOptName,(int)type);
    if (AEE_SUCCESS != res) {
       LOG_MSG_ERROR("SetOpt operation failed", 0, 0, 0);
       *dss_errno = DSSConversion::IDS2DSErrorCode(res);
       (void) DSSGlobals::Instance()->RemoveSocket(sSockFd);
       sSockFd = DSS_ERROR;
       goto bail;
    }

    pDSSSocket->SetIDSSocket(pIDSSocket);

bail:
    DSSCommon::ReleaseIf((IQI**)&pIDSPrivSokect);
    return sSockFd;
}

/*===========================================================================
FUNCTION DSS_BIND()

DESCRIPTION
  For all client sockets, attaches a local address and port value to the
  socket.  If the call is not explicitly issued, the socket will implicitly
  bind in calls to dss_connect() or dss_sendto().  Note that this function
  does not support binding to a local IPv4 address, but rather ONLY a local
  port number.  The local IPv4 address is assigned automatically by the sockets
  library.  This function does support binding to a local IPv6 address,
  however, as this is required for using IPv6 Privacy Extensions (RFC 3041).
  The function must receive (as a parameter) a valid socket descriptor,
  implying a previous successful call to dss_socket().

DEPENDENCIES
  None.

RETURN VALUE
  Returns DSS_SUCCESS on success.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specified
  DS_EAFNOSUPPORT     address family not supported
  DS_EOPNOTSUPP       operation not supported
  DS_EADDRINUSE       the local address is already in use.
  DS_EINVAL           the socket is already attached to a local name
  DS_EFAULT           invalid address and/or invalid address length

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_bind
(
  sint15 sockfd,                                      /* socket descriptor */
  struct ps_sockaddr *localaddr,                          /* local address */
  uint16 addrlen,                                     /* length of address */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   AEEResult res;
   sint15 sRet;
   ds::SockAddrStorageType tempSockAddr;
   ds::AddrFamilyType      tempFamily;
   int tmpDSFamily;

   LOG_MSG_FUNCTION_ENTRY("dss_bind(): socket:%d", sockfd, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (AEE_SUCCESS != res)
   {
      LOG_MSG_ERROR("GetSocketById failed:%d ", res, 0, 0);
   }

   if (NULL == pDSSSocket) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_bind: pIDSSocket is NULL",0, 0, 0); 
      ASSERT(0);
   }

   // There is a backward compatibility issue here: the old API supports unbinding and the new API does not
   if (NULL == localaddr) {
      LOG_MSG_ERROR("Local Address is NULL - this is not supported", 0, 0, 0);
      *dss_errno = DS_EOPNOTSUPP;
      sRet = DSS_ERROR;
      goto bail;
   }

   if (DSS_AF_INET != localaddr->ps_sa_family &&
       DSS_AF_INET6 != localaddr->ps_sa_family)
   {
      LOG_MSG_ERROR("Illegal Address Family", 0, 0, 0);
      *dss_errno = DS_EAFNOSUPPORT;
      sRet = DSS_ERROR;
      goto bail;
   }

   if (DSS_AF_INET == localaddr->ps_sa_family){
      if (sizeof(struct ps_sockaddr_in) != addrlen) {
         LOG_MSG_ERROR("Illegal IPv4 address length", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   if (DSS_AF_INET6 == localaddr->ps_sa_family){
      if (sizeof(struct ps_sockaddr_in6) != addrlen) {
         LOG_MSG_ERROR("Illegal IPv6 address length", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   // Memcpy ps_sockaddr buffer into ps_sockaddr storage buffer
   // The copy is necessary since the parameter to bind must be the specific type declared
   // for bind's parameter in the ISocket interface. This parameter is ds::SockAddrStorageType.
   // If we attempt to merely cast the provided struct ps_sockaddr to ds::SockAddrStorageType
   // and provide that as a parameter, remoting code may attempt to copy unallocated memory (depends
   // on how localaddr was allocated) which may result in an exception (because remoting code
   // expects ds::Sock::SockAddrStorageType)
   addrlen = min(addrlen,sizeof(ds::SockAddrStorageType));
   memcpy(tempSockAddr, localaddr, addrlen);

   res = DSSConversion::DS2IDSAddrFamily(localaddr->ps_sa_family, &tempFamily);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid Address Family %", localaddr->ps_sa_family, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::SetFamily(tempSockAddr, ps_htons(tempFamily));

   res = pIDSSocket->Bind(tempSockAddr);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Bind operation failed", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = pIDSSocket->GetSockName(tempSockAddr);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("GetSockName operation failed", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }
   // copy the address, now including the allocated port, to localaddr
   memcpy(localaddr, tempSockAddr, addrlen);
   (void)ds::Sock::AddrUtils::GetFamily( tempSockAddr,&tempFamily);
   tempFamily = ps_ntohs(tempFamily);

   res = DSSConversion::IDS2DSAddrFamily(tempFamily,&tmpDSFamily) ;
   if (AEE_SUCCESS != res) {
    LOG_MSG_ERROR( "Invalid Address Family %", tempFamily, 0, 0);
    *dss_errno = DSSConversion::IDS2DSErrorCode(res);
    sRet = DSS_ERROR;
    goto bail;
   }

   localaddr->ps_sa_family = (uint16)tmpDSFamily;

   sRet = DSS_SUCCESS;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_CONNECT()

DESCRIPTION
  For TCP, attempts to establish the TCP connection.  Upon
  successful connection, calls the socket callback function asserting that
  the DS_WRITE_EVENT is TRUE.  For udp it fills in the peers ipaddr in the
  socket control block and binds the socket to an interface.
  The function must receive (as a parameter) a valid socket descriptor,
  implying a previous successful call to dss_socket().

DEPENDENCIES
  Network subsystem must be established and available.

RETURN VALUE
  Return DSS_ERROR and places the error condition value in *dss_errno.

  dss_errno Values
  ----------------
  DS_EWOULDBLOCK      operation would block
  DS_EBADF            invalid socket descriptor is specified
  DS_ECONNREFUSED     connection attempt refused
  DS_ETIMEDOUT        connection attempt timed out
  DS_EFAULT           addrlen parameter is invalid
  DS_EIPADDRCHANGED   IP address changed due to PPP resync
  DS_EINPROGRESS      connection establishment in progress
  DS_EISCONN          a socket descriptor is specified that is already
                      connected
  DS_ENETDOWN         network subsystem unavailable
  DS_EOPNOTSUPP       invalid server address specified
  DS_EADDRREQ         destination address is required.
  DS_NOMEM            not enough memory to establish connection

SIDE EFFECTS
  For TCP, initiates active open for connection.
===========================================================================*/
sint15 dss_connect
(
  sint15 sockfd,                                      /* Socket descriptor */
  struct ps_sockaddr *servaddr,                     /* destination address */
  uint16 addrlen,                                    /* length of servaddr */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   AEEResult res;
   sint15 sRet;
   ds::SockAddrStorageType tempSockAddr;
   ds::AddrFamilyType tempFamily;

   LOG_MSG_FUNCTION_ENTRY("dss_connect(): socket:%d", sockfd, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_connect: pIDSSocket is NULL",0, 0, 0); 
      ASSERT(0);
   }

   if (NULL == servaddr) {
      LOG_MSG_ERROR("Server Address is NULL", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   if (DSS_AF_INET != servaddr->ps_sa_family && DSS_AF_INET6 != servaddr->ps_sa_family){
      LOG_MSG_ERROR("Illegal Address Family", 0, 0, 0);
      *dss_errno = DS_EAFNOSUPPORT;
      sRet = DSS_ERROR;
      goto bail;
   }

   if (DSS_AF_INET == servaddr->ps_sa_family){
      if (sizeof(struct ps_sockaddr_in) != addrlen) {
         LOG_MSG_ERROR("Illegal IPv4 address length", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   if (DSS_AF_INET6 == servaddr->ps_sa_family){
      if (sizeof(struct ps_sockaddr_in6) != addrlen) {
         LOG_MSG_ERROR("Illegal IPv6 address length", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   // Memcpy ps_sockaddr buffer into ps_sockaddr storage buffer
   addrlen = min(addrlen,sizeof(ds::SockAddrStorageType));
   memcpy(tempSockAddr, servaddr, addrlen);

   res = DSSConversion::DS2IDSAddrFamily(servaddr->ps_sa_family, &tempFamily);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid Address Family %", servaddr->ps_sa_family, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::SetFamily(tempSockAddr, ps_htons(tempFamily));

   res = pIDSSocket->Connect(tempSockAddr);
   if (AEE_SUCCESS != res) {
      if (AEE_EWOULDBLOCK == res) {
         LOG_MSG_ERROR("Connect operation in progress (flow controlled)", 0, 0, 0);
      } else if (QDS_EISCONN == res) {
         LOG_MSG_ERROR("Connect operation completed, dss_errno = DS_EISCONN", 0, 0, 0);
      } else {
         LOG_MSG_ERROR("Connect operation failed", 0, 0, 0);
      }
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }
   sRet = DSS_SUCCESS;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_LISTEN()

DESCRIPTION

  For TCP, this starts a passive open for connections.  Upon a
  successful connection, the socket callback function is invoked
  asserting DS_ACCEPT_EVENT as TRUE.  The application should respond
  with a call to dss_accept(). If a connection is received and there
  are no free queue slots the new connection is rejected
  (QDS_ECONNREFUSED).  The backlog queue is for ALL unaccepted sockets
  (half-open, or completely established).

  A listening UDP doesn't make sense, and as such isn't supported.
  DS_EOPNOTSUPP is returned.

  The sockfd parameter is a created (dss_socket) and bound (dss_bind)
  socket that will become the new listening socket.  The backlog
  parameter indicates the maximum length for the queue of pending
  sockets.  If backlog is larger than the maximum, it will be
  reduced to the maximum (see DSS_SOMAXCONN). This is the BSD behavior.

  The argument dss_error should point to a memory location in which
  error conditions can be recorded.

DEPENDENCIES

  Network subsystem must be established and available.

  The sockfd should get a valid socket descriptor (implying a
  previously successful call to dss_socket) This socket should be
  bound to a specific port number (implying a previously successful
  call to dss_bind) .

RETURN VALUE

  Returns DSS_SUCCESS on success.  If the backlog was truncated
  DS_EFAULT will be set in errno, but the call will still be
  successful.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EWOULDBLOCK      operation would block (PJ: I don't think this CAN happen)
  DS_EBADF            invalid socket descriptor is specified
  DS_EOPNOTSUPP       The socket is not capable of listening (UDP)
  DS_EFAULT           backlog parameter is invalid
  DS_ENETNONET        network subsystem unavailable for some unknown reason
  DS_ENETINPROGRESS   network subsystem establishment currently in progress
  DS_ENETCLOSEINPROGRESS network subsystem close in progress.
  DS_NOMEM            not enough memory to establish backlog connections.
  DS_EINVAL           Socket already open, closed, unbound or not one
                      you can listen on.

SIDE EFFECTS
  For TCP, initiates passive open for new connections.
===========================================================================*/
sint15 dss_listen
(
  sint15 sockfd,                                      /* Socket descriptor */
  sint15 backlog,                      /* Number of connections to backlog */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   AEEResult res;
   sint15 sRet;

   LOG_MSG_FUNCTION_ENTRY("dss_listen(): socket:%d", sockfd, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL == pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_listen: pIDSSocket is NULL",0, 0, 0); 
      ASSERT(0);
   }

   // Check for errors in backlog parameter - It must be greater than 0
   if (0 >= backlog) {
      LOG_MSG_ERROR("Backlog parameter must be greater than 0", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   // Silently truncate queue to SOMAXCONN
   if (DSS_SOMAXCONN < backlog) {
      LOG_MSG_INFO1("Backlog parameter is too high", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      backlog = DSS_SOMAXCONN;
   }

   res = pIDSSocket->Listen((long)backlog);
   if (AEE_SUCCESS != res) {
      if (AEE_EWOULDBLOCK == res) {
         LOG_MSG_ERROR("Listen operation in progress (flow controlled)", 0, 0, 0);
      } else {
      LOG_MSG_ERROR("Listen operation failed", 0, 0, 0);
      }
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   sRet = DSS_SUCCESS;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_ACCEPT()

DESCRIPTION

  The accept function is used on listening sockets to respond when
  DS_ACCEPT_EVENT is asserted.  The first backlog queued connection is
  removed from the queue, and bound to a new connected socket (as if
  you called dss_socket).  The newly created socket is in the
  connected state.  The listening socket is unaffected and the queue size is
  maintained (i.e. there is not need to call listen again.)

  The argument sockfd is the file descriptor of the listening socket

  The argument remote addr is a pointer to a struct ps_sockaddr.  This
  structure is populated with the address information for the remote
  end of the new connection. addrlen should initially contain the
  length of the struct ps_sockaddr passed in.  The length of the real
  address is placed in this location when the struct is populated.

  The argument dss_error should point to a memory location in which
  error conditions can be recorded.

DEPENDENCIES

  Network subsystem must be established and available.

  The sockfd should get a valid socket descriptor (implying a
  previously successful call to dss_socket) This socket should be
  bound to a specific port number (implying a previously successful
  call to dss_bind).  The socket should be listening (implying a
  previously successful call to dss_listen).

RETURN VALUE
  Returns the socket descriptor of the new socket on success.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EWOULDBLOCK      operation would block
  DS_EBADF            invalid socket descriptor is specified
  DS_EOPNOTSUPP       The socket is not of type DSS_SOCK_STREAM
  DS_EINVAL           Socket is not listening.
  DS_EFAULT           The addr parameter is bogus.
  DS_ENETNONET        network subsystem unavailable for some unknown reason
  DS_ENETINPROGRESS   network subsystem establishment currently in progress
  DS_ENETCLOSEINPROGRESS network subsystem close in progress.
  DS_NOMEM            not enough memory to establish backlog connections.

SIDE EFFECTS

  The head backlog item from the queue of the listening socket is
  removed from that queue.
===========================================================================*/
sint15 dss_accept
(
  sint15 sockfd,                                      /* Socket descriptor */
  struct ps_sockaddr *remoteaddr,                    /* new remote address */
  uint16 *addrlen,                                   /* length of servaddr */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL, *pNewDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL, *pNewIDSSocket = NULL;
   AEEResult res;
   sint15 sRet;
   ds::SockAddrStorageType tempSockAddr;
   int tmpDSFamily;
   ds::AddrFamilyType family = ds::AddrFamily::QDS_AF_UNSPEC;
   size_t copylen;

   LOG_MSG_FUNCTION_ENTRY("dss_accept(): socket:%d", sockfd, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      ASSERT(0);
   }

   // Verify remoteaddr and addrlen are valid
   if ((NULL == remoteaddr) || (NULL == addrlen)) {
      LOG_MSG_ERROR("Remote Address or addrlen parameter is NULL", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   memset(tempSockAddr, 0, sizeof(tempSockAddr));

   res = pIDSSocket->Accept(tempSockAddr, &pNewIDSSocket);
   if (AEE_SUCCESS != res) {
      if (AEE_EWOULDBLOCK == res) {
         LOG_MSG_ERROR("Accept operation in progress (flow controlled)", 0, 0, 0);
      } else {
         LOG_MSG_ERROR("Accept operation failed", 0, 0, 0);
      }
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::GetFamily(tempSockAddr,&family);
   family = ps_ntohs(family);

   res = DSSConversion::IDS2DSAddrFamily(family, &tmpDSFamily);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid Address Family %", family, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::SetFamily(tempSockAddr, tmpDSFamily);


   if (ds::AddrFamily::QDS_AF_INET == family) {
      if (sizeof(struct ps_sockaddr_in) < *addrlen){
         *addrlen = sizeof(struct ps_sockaddr_in);
      }
   }
   else if (ds::AddrFamily::QDS_AF_INET6 == family) {
      if (sizeof(struct ps_sockaddr_in6) < *addrlen){
         *addrlen = sizeof(struct ps_sockaddr_in6);
      }
   }
   else {
      LOG_MSG_ERROR("Invalid Address Family", 0, 0, 0);
      *dss_errno = DS_EAFNOSUPPORT;
      sRet = DSS_ERROR;
      goto bail;
   }

   copylen = min(*addrlen,sizeof(ds::SockAddrStorageType));
   memcpy(remoteaddr, tempSockAddr, copylen);

   // Fetch the NetApp of the socket in order to assign it later to the NetApp field of the new DSS socket.
   sint15 sNewSockFd;
   int kind;
   pDSSSocket->GetSockKind(&kind);
   res = DSSGlobals::Instance()->CreateSocket(pDSSSocket->GetCb(), pDSSSocket->GetNetApp(),
                                              &sNewSockFd, &pNewDSSSocket, kind, PS_IPPROTO_TCP);

   if (AEE_ENOMEMORY == res) {
      res = QDS_EMFILE;
   }

   if ((AEE_SUCCESS != res) || (NULL == pNewDSSSocket)){
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   pNewDSSSocket->SetIDSSocket(pNewIDSSocket);

   sRet = sNewSockFd;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   DSSCommon::ReleaseIf((IQI**)&pNewIDSSocket);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_ASYNC_SELECT()

DESCRIPTION
  Enables the events to be notified about through the asynchronous
  notification mechanism.  Application specifies a bitmask of events that it
  is interested in, for which it will receive asynchronous notification via
  its application callback function. This function also performs a real-time
  check to determine if any of the events have already occurred, and if so
  invokes the application callback.

DEPENDENCIES
  None.

RETURN VALUE

  Returns DSS_SUCCESS on success.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specified

SIDE EFFECTS
  Sets the relevant event mask in the socket control block.  Will also
  notify the application via the callback function.

IMPORTAND NOTE REGARDING BACKWARD COMPATIBILITY
  In legacy code, whenever dss_async_select is invoked, and appropriate event can be asserted - it is asserted, regardless if 
  dss_getnextevent was called afterwards, meaning that we can call several dss_async_select-s in a raw and each time get the event asserted if 
  it can be asserted
  
  In current implementation if 2 dss_async_select-s are invoked, then the second call would cause events to be asserted *only* if the callback for the 
  first dss_async_select was invoked, before user called the second dss_async_select. In order to reach full compatibilty with the legacy code, we will
  have to enable checking the status of signal buses in DSNet/DSSock layers inside DSS. 
===========================================================================*/
sint31 dss_async_select
(
  sint15 sockfd,                                      /* socket descriptor */
  sint31 interest_mask,                        /* bitmask of events to set */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY("dss_async_select(): socket:%d, bitmask:%d", sockfd, interest_mask, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      return DSS_ERROR;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL == pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      return DSS_ERROR;
   }

   res = pDSSSocket->RegEvent(interest_mask);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   return DSS_SUCCESS;
}

/*===========================================================================
FUNCTION DSS_ASYNC_DESELECT()

DESCRIPTION
  Clears events of interest in the socket control block interest mask.  The
  application specifies a bitmask of events that it wishes to clear; events
  for which it will no longer receive notification.

DEPENDENCIES
  None.

RETURN VALUE
  Returns DSS_SUCCESS on success.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specified

SIDE EFFECTS
  Clears specified events from the relevant event mask.
===========================================================================*/
sint15 dss_async_deselect
(
  sint15 sockfd,                                      /* socket descriptor */
  sint31 clr_interest_mask,                  /* bitmask of events to clear */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY("dss_async_deselect(): socket:%d, clear_bitmask:%d", sockfd, clr_interest_mask, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      return DSS_ERROR;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      return DSS_ERROR;
   }

   res = pDSSSocket->DeRegEvent(clr_interest_mask);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   return DSS_SUCCESS;
}

/*===========================================================================
FUNCTION DSS_GETNEXTEVENT()

DESCRIPTION
  This function performs a real-time check to determine if any of the events
  of interest specified in the socket control block's event mask have
  occurred.  It also clears any bits in the event mask that have occurred.
  The application must re-enable these events through a subsequent call to
  dss_async_select().  The application may pass in a pointer to a single
  socket descriptor to determine if any events have occurred for that socket

  Alternatively, the application may set this pointer's value to NULL (0)
  (note, not to be confused with a NULL pointer, but rather a pointer whose
  value is 0) in which case the function will return values for the next
  available socket.  The next available socket's descriptor will be placed
  in the socket descriptor pointer, and the function will return.  If no
  sockets are available (no events have occurred across all sockets for
  that application) the pointer value will remain NULL (originally value
  passed in), and the function will return 0, indicating that no events
  have occurred.

DEPENDENCIES
  None.

RETURN VALUE
  Returns an event mask of the events that were asserted for sockets whose
  app ID matches that passed in.  A value of 0 indicates that no events have
  occurred.

  On passing a pointer whose value is NULL into the function for
  the socket descriptor (not to be confused with a NULL pointer), places
  the next available socket descriptor in *sockfd_ptr and returns the
  event mask for that socket. If no sockets are available (no events have
  occurred across all sockets for that application) the pointer value
  will remain NULL (originally value passed in), and the function will
  return 0, indicating that no events have occurred.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADAPP           invalid app descriptor is specified
  DS_EBADF             invalid socket descriptor is specified

SIDE EFFECTS
  Clears the bits in the socket control block event mask, corresponding to
  the events that have occurred.
===========================================================================*/
sint31 dss_getnextevent
(
  sint15 dss_nethandle,                                         /* application ID */
  sint15 *sockfd_ptr,                                 /* socket descriptor */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   sint15 socketNetHandle;
   uint32 eventOccurred, sigOnOff, relevantEvents;
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY("dss_getnextevent(): net_handle:%d, socket:%d", dss_nethandle, (NULL == sockfd_ptr)? 0:(*sockfd_ptr), 0);

   if (NULL == dss_errno) {
     LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
     return DSS_ERROR;
   }

   if (NULL == sockfd_ptr) {
     LOG_MSG_ERROR("sockfd_ptr is NULL", 0, 0, 0);
     *dss_errno = DS_EBADF;
     return DSS_ERROR;
   }

   if (!DSSGlobals::Instance()->IsValidNetApp(dss_nethandle)) {
      LOG_MSG_ERROR("Invalid Application descriptor", 0, 0, 0);
      *dss_errno = DS_EBADAPP;
      return DSS_ERROR;
   }

   if (0 != *sockfd_ptr) {
      res = DSSGlobals::Instance()->GetSocketById(*sockfd_ptr, &pDSSSocket);
      if (NULL == pDSSSocket || AEE_SUCCESS != res) {
         LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
         *dss_errno = DS_EBADF;
         return DSS_ERROR;
      }
      socketNetHandle = pDSSSocket->GetNetApp();
      if (socketNetHandle != dss_nethandle) {
         LOG_MSG_ERROR("The socket specified is not attached to the application specified", 0, 0, 0);
         *dss_errno = DS_EBADF;
         return DSS_ERROR;
      }
      pDSSSocket->GetEventOccurredMask(&eventOccurred);
      pDSSSocket->GetSigOnOffMask(&sigOnOff);

      relevantEvents = eventOccurred & sigOnOff;
      if (relevantEvents) {
         pDSSSocket->SetSigOnOffMask(sigOnOff & ~relevantEvents);
         pDSSSocket->SetEventOccurredMask(eventOccurred & ~relevantEvents);
         return relevantEvents;
      }
   }
   else {
      res = DSSGlobals::Instance()->FindSocketWithEvents(dss_nethandle, sockfd_ptr, &eventOccurred, &sigOnOff);
      if (NULL != *sockfd_ptr && AEE_SUCCESS == res) {
         res = DSSGlobals::Instance()->GetSocketById(*sockfd_ptr, &pDSSSocket);
         if(AEE_SUCCESS == res)
         {
            LOG_MSG_ERROR("GetSocketById failed:%d", res, 0, 0);
         }
         relevantEvents = eventOccurred & sigOnOff;
         if (relevantEvents) {
            pDSSSocket->SetSigOnOffMask(sigOnOff & ~relevantEvents);
            pDSSSocket->SetEventOccurredMask(eventOccurred & ~relevantEvents);
            return relevantEvents;
         }
      }
   }

   // No events have occurred, so just return 0.
   return 0;
}

/*===========================================================================
FUNCTION DSS_READ()

DESCRIPTION
  Reads up to nbytes of data into buffer from the transport specified
  by the socket descriptor

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes read, which could be less than the number of
      bytes specified. A return of 0 indicates that an End-of-File condition
      has occurred.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specified
  DS_ENOTCONN         socket not connected
  DS_ECONNRESET       TCP connection reset by server
  DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
  DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
  DS_EPIPE            broken pipe
  DS_EADDRREQ         destination address required
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           bad memory address
  DS_EWOULDBLOCK      operation would block
  DS_EINVAL           Can't read a listen socket.

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_read
(
  sint15 sockfd,                                      /* socket descriptor */
  void   *buffer,                     /* user buffer to which to copy data */
  uint16 nbytes,                 /* number of bytes to be read from socket */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   int nBufLenReq;
   AEEResult res;
   sint15 sRet = DSS_ERROR;

   LOG_MSG_FUNCTION_ENTRY("dss_read(): socket:%d", sockfd, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_read: pIDSSocket is NULL",0, 0, 0); 
      ASSERT(0);
   }

   if (NULL == buffer && 0 != nbytes) {
      LOG_MSG_ERROR("Buffer is NULL", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
   }

   res = pIDSSocket->Read((byte*)buffer, (int)nbytes, &nBufLenReq);
   if (AEE_SUCCESS != res && QDS_EEOF != res) {
      if (AEE_EWOULDBLOCK == res) {
         LOG_MSG_ERROR("Read operation in progress (flow controlled)", 0, 0, 0);
      } else {
      LOG_MSG_ERROR("Read operation failed", 0, 0, 0);
      }
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   *dss_errno = DS_EEOF;
   sRet = nBufLenReq;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_READV()

DESCRIPTION
  Provides the scatter read variant of the dss_read() call, which
  allows the application to read into non-contiguous buffers.

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes read, which could be less than the number of
      bytes specified. A return of 0 indicates that an End-of-File condition
      has occurred.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ------------
  DS_EBADF            invalid socket descriptor is specified
  DS_ENOTCONN         socket not connected
  DS_ECONNRESET       TCP connection reset by server
  DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
  DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
  DS_EPIPE            broken pipe
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           bad memory address
  DS_EWOULDBLOCK      operation would block
  DS_EINVAL           Can't read from a listen socket

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_readv
(
  sint15 sockfd,                                      /* socket descriptor */
  struct ps_iovec iov[],        /* array of data buffers to copy data into */
  sint15 iovcount,               /* number of bytes to be read from socket */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   ds::Sock::ISocketLocalPriv *pIDSPrivSokect = NULL;
   IPort1::SeqBytes *bufs = NULL;
   int nBufLenReq, i;
   AEEResult res;
   sint15 sRet;

   LOG_MSG_FUNCTION_ENTRY("dss_readv(): socket:%d", sockfd, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_readv: pIDSSocket is NULL",0, 0, 0); 
      ASSERT(0);
   }

   if (NULL == iov) {
      if (0 == iovcount) {
         sRet = 0;
      }
      else {
         LOG_MSG_ERROR("iov parameter is NULL", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
      }
      goto bail;
   }

   // array entities validation
   for (i = 0; i < iovcount; i++) {
      if ((NULL == iov[i].ps_iov_base) && (0 != iov[i].ps_iov_len)) {
         LOG_MSG_ERROR("One of the buffers is NULL", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   // allocate space for bufs
   if (iovcount > 0) {
      bufs = (IPort1::SeqBytes*)ps_system_heap_mem_alloc(sizeof(IPort1::SeqBytes)*iovcount);
      if (NULL == bufs) {
         LOG_MSG_ERROR("Can't allocate memory for the buffers", 0, 0, 0);
         *dss_errno = DS_EMSGSIZE;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   // fill bufs
   for (i = 0; i < iovcount; i++) {
      bufs[i].data = iov[i].ps_iov_base;
      bufs[i].dataLen = (int)iov[i].ps_iov_len;
   }

   res = pIDSSocket->QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv, (void**)&(pIDSPrivSokect));
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv) failed: %d", res, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = pIDSPrivSokect->ReadV(bufs, (int)iovcount, &nBufLenReq);

   if (AEE_SUCCESS != res && QDS_EEOF != res) {
      if (AEE_EWOULDBLOCK == res) {
         LOG_MSG_ERROR("ReadV operation in progress (flow controlled)", 0, 0, 0);
      } else {
      LOG_MSG_ERROR("ReadV operation failed", 0, 0, 0);
      }
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   *dss_errno = DS_EEOF;
   sRet = nBufLenReq;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSPrivSokect);
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   PS_SYSTEM_HEAP_MEM_FREE (bufs);

   return sRet;
}

/*===========================================================================
FUNCTION DSS_RECVFROM()

DESCRIPTION
  Reads 'nbytes' bytes in the buffer from the transport specified by the
  socket descriptor.  Fills in address structure with values from who sent
  the data.

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes to be written, which could be less than the number
      of bytes specified.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specified
  DS_EAFNOSUPPORT     address family not supported
  DS_EWOULDBLOCK      operation would block
  DS_EFAULT           bad memory address
  DS_EOPNOTSUPP       option not supported
  DS_EINVAL           can't recv from a listen socket.

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_recvfrom
(
  sint15 sockfd,                                      /* socket descriptor */
  void   *buffer,               /* user buffer from which to copy the data */
  uint16 nbytes,                          /* number of bytes to be written */
  uint32 flags,                                                  /* unused */
  struct ps_sockaddr *fromaddr,                     /* destination address */
  uint16 *addrlen,                                       /* address length */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   int nBufLenReq;
   ds::SockAddrStorageType idsSockAddrStorage;
   AEEResult res;
   sint15 sRet = DSS_ERROR;
   ds::AddrFamilyType family = ds::AddrFamily::QDS_AF_UNSPEC;
   int tmpDSFamily;
   unsigned int outFlags;
   int nControlLenReq; // used as parameter to RecvMsg, but never used.

   LOG_MSG_FUNCTION_ENTRY("dss_recvfrom(): socket:%d, flags:%u", sockfd, flags, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   if ((NULL != fromaddr) && (NULL == addrlen)) {
      LOG_MSG_ERROR("addrlen is NULL", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   if (NULL == buffer && 0 != nbytes) {
      LOG_MSG_ERROR("Buffer is NULL", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_recvfrom: pIDSSocket is NULL",0, 0, 0); 
      ASSERT(0);
   }

   memset(idsSockAddrStorage, 0, sizeof(idsSockAddrStorage));

   res = pIDSSocket->RecvMsg(idsSockAddrStorage,
                             (byte *) buffer,
                             (int) nbytes,
                              &nBufLenReq,
                              0,
                              0,
                              &nControlLenReq,
                              &outFlags,
                              flags);

   *dss_errno = DSSConversion::IDS2DSErrorCode(res);

   if (AEE_SUCCESS != res && QDS_EEOF != res) {
      
      if (AEE_EWOULDBLOCK == res) {
         LOG_MSG_ERROR("RecvFrom operation in progress (flow controlled)", 0, 0, 0);
      } else {
         LOG_MSG_ERROR("RecvFrom operation failed", 0, 0, 0);
      }
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }
   
   if(0 != (outFlags & RecvFlags::QDS_MSG_TRUNC)) {
        *dss_errno = DS_EMSGTRUNC;
   }

   (void)ds::Sock::AddrUtils::GetFamily(idsSockAddrStorage,&family);
   family = ps_ntohs(family);

   res = DSSConversion::IDS2DSAddrFamily(family, &tmpDSFamily);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid Address Family %", family, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::SetFamily(idsSockAddrStorage, tmpDSFamily);

   if (NULL != addrlen) {
      switch (family)
      {
         case ds::AddrFamily::QDS_AF_INET:
            if (sizeof(struct ps_sockaddr_in) < *addrlen)
            {
               *addrlen = sizeof(struct ps_sockaddr_in);
            }
            break;
         case ds::AddrFamily::QDS_AF_INET6:
            if (sizeof(struct ps_sockaddr_in6) < *addrlen)
            {
               *addrlen = sizeof(struct ps_sockaddr_in6);
            }
            break;

         default:
            LOG_MSG_ERROR("dss_recvfrom: The address family must be INET or INET6 %i", family, 0, 0);
            ASSERT(0);
      }
   }

   if (NULL != addrlen){
      size_t copylen = min(sizeof(ds::SockAddrStorageType), *addrlen);

      if (NULL != fromaddr){
         memcpy(fromaddr, idsSockAddrStorage, copylen);
      }
      else{
         *addrlen = 0;
      }
   }

   sRet = nBufLenReq;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_RECVMSG()
DESCRIPTION
  This function is a common read function for all the socket input
  functions. The message header contains an array of scattered buffers, a
  socket descriptor and an address field for filling the source address
  of the received packet.The function reads data into the scattered buffers
  over the transport specified by the socket descriptor

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes to be written, which could be less than the number
      of bytes specified.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specified
  DS_EAFNOSUPPORT     address family not supported
  DS_EWOULDBLOCK      operation would block
  DS_EFAULT           bad memory address
  DS_EOPNOTSUPP       option not supported
  DS_EINVAL           can't recv from a listen socket.

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_recvmsg
(
  sint15                  sockfd,   /* socket descriptor                   */
  struct dss_msghdr     * msg,      /* Message header for filling in data  */
  int                     flags,    /* flags from dss_recvfrom             */
  sint15                * dss_errno /* error condition value               */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   ds::Sock::ISocketLocalPriv *pIDSsocketPriv = NULL;
   IPort1::SeqBytes *bufs = NULL;
   ds::Sock::IAncDataPriv **pOutAncData = NULL;
   struct dss_cmsghdr *cmsg_ptr = NULL;
   int nOutAncLen = 0, nBufLenReq = 0, nOutAncLenReq = 0, i = 0;
   ds::SockAddrStorageType tempSockAddr;
   ds::AddrFamilyType family = ds::AddrFamily::QDS_AF_UNSPEC;
   unsigned int nOutFlags;
   int isRecvIfSet = 0;
   int isRecvErrSet =0;
   AEEResult res;
   sint15 sRet;
   uint16 controllen  = 0;
   ds::ErrorType tempRes = AEE_SUCCESS;
   unsigned int dsFlags;
   int tmpDSFamily;
   size_t copylen;

   LOG_MSG_FUNCTION_ENTRY("dss_recvmsg(): socket:%d", sockfd, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSConversion::IDS2DSRcvMsgFlag(flags,&dsFlags);
   if (AEE_SUCCESS != res) {
       LOG_MSG_ERROR("Invalid dss_recvmsg flag %ox", flags, 0, 0);
       *dss_errno = DSSConversion::IDS2DSErrorCode(res);
       sRet = DSS_ERROR;
       goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL == pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   if(NULL == msg) {
      LOG_MSG_ERROR("msg parameter is NULL", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_recvmsg: pIDSSocket is NULL",0, 0, 0); 
      ASSERT(0);
   }

   if (NULL == msg->msg_iov) {
      if (0 == msg->msg_iovlen) {
         sRet = 0;
         // TODO: is it possible that msg_iov is NULL and msg_control is not? Currently we don't support this.
      }
      else {
         LOG_MSG_ERROR("msg_iov parameter is NULL", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
      }
      goto bail;
   }

   // array entities validation
   for (i = 0; i < msg->msg_iovlen; i++) {
      if ((NULL ==  msg->msg_iov[i].ps_iov_base) && (0 != msg->msg_iov[i].ps_iov_len)) {
         LOG_MSG_ERROR("One of the buffers is NULL", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   // allocate space for bufs

   bufs = NULL;
   if (msg->msg_iovlen > 0) {
      bufs = (IPort1::SeqBytes*)ps_system_heap_mem_alloc(sizeof(IPort1::SeqBytes)*(msg->msg_iovlen));
      if (NULL == bufs) {
         LOG_MSG_ERROR("Can't allocate memory for the buffers", 0, 0, 0);
         *dss_errno = DS_ENOMEM;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   // fill bufs
   for (i = 0; i < msg->msg_iovlen; i++) {
      bufs[i].data = msg->msg_iov[i].ps_iov_base;
      bufs[i].dataLen = (int)(msg->msg_iov[i].ps_iov_len);
   }

   // calculate the number of pointers to ancillary data that we need to allocate
   // This number is an upper bound of the number of ancillary data we can populate
   // to the user according to the msg_controllen that the user provided.
   nOutAncLen = msg->msg_controllen / sizeof(struct dss_cmsghdr);
   pOutAncData = NULL;

   // allocate array of pointers of type IAncData*
   if (0 != nOutAncLen)
   {
      pOutAncData = (ds::Sock::IAncDataPriv **)ps_system_heap_mem_alloc(sizeof(ds::Sock::IAncDataPriv *)*nOutAncLen);
      if (NULL == pOutAncData) {
         LOG_MSG_ERROR("Can't allocate pOutAncData", 0, 0, 0);
         *dss_errno = DS_ENOMEM;
         PS_SYSTEM_HEAP_MEM_FREE (bufs);
         return DSS_ERROR;
      }
   }

   // We don't need the inAncData so we pass dummy parameters
   memset(tempSockAddr, 0, sizeof(tempSockAddr));
   res = pIDSSocket->QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv, (void**)&pIDSsocketPriv);
   if (AEE_SUCCESS != res) {
     LOG_MSG_ERROR("QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv) failed: %d", res, 0, 0);
     *dss_errno = DSSConversion::IDS2DSErrorCode(res);
     sRet = DSS_ERROR;
     goto bail;
   }
   res = pIDSsocketPriv->RecvMsg(tempSockAddr,
                                 bufs, (int)msg->msg_iovlen, &nBufLenReq,
                                 pOutAncData, nOutAncLen, &nOutAncLenReq, &nOutFlags,
                                 dsFlags);

   *dss_errno = DSSConversion::IDS2DSErrorCode(res);
   if (AEE_SUCCESS != res && QDS_EEOF != res) {       
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   if(0 != (nOutFlags & (RecvFlags::QDS_MSG_TRUNC | RecvFlags::QDS_MSG_CTRUNC))) {
     *dss_errno = DS_EMSGTRUNC;
   }

   (void)ds::Sock::AddrUtils::GetFamily(tempSockAddr,&family);
   family = ps_ntohs(family);

   res = DSSConversion::IDS2DSAddrFamily(family, &tmpDSFamily);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid Address Family %", family, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::SetFamily(tempSockAddr, tmpDSFamily);

   if (ds::AddrFamily::QDS_AF_INET == family) {
      if (sizeof(struct ps_sockaddr_in) < msg->msg_namelen){
         msg->msg_namelen = sizeof(struct ps_sockaddr_in);
      }
   }
   else if (ds::AddrFamily::QDS_AF_INET6 == family) {
      if (sizeof(struct ps_sockaddr_in6) < msg->msg_namelen){
         msg->msg_namelen = sizeof(struct ps_sockaddr_in6);
      }
   }
   else {
      LOG_MSG_ERROR("Invalid Address Family", 0, 0, 0);
      *dss_errno = DS_EAFNOSUPPORT;
      sRet = DSS_ERROR;
      goto bail;
   }

   copylen = min(msg->msg_namelen,sizeof(ds::SockAddrStorageType));

   if (NULL != msg->msg_name){
      memcpy(msg->msg_name, tempSockAddr, copylen);
   }
   else{
      msg->msg_namelen = 0;
   }

   // Check if RECVIF was set to the socket
   res = pIDSSocket->GetOpt(/*in*/ ::ds::Sock::OptLevel::QDS_LEVEL_IP,
                            /*in*/ ::ds::Sock::OptName::QDS_IP_RECVIF,
                            /*rout*/ &isRecvIfSet);
   if (AEE_SUCCESS != res)
   {
      LOG_MSG_ERROR("GetOpt failed: %d", res, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   // Check if RECVERR was set to the socket
   res = pIDSSocket->GetOpt(/*in*/ ::ds::Sock::OptLevel::QDS_LEVEL_IP,
      /*in*/ ::ds::Sock::OptName::QDS_IP_RECVERR,
      /*rout*/ &isRecvErrSet);
   if (AEE_SUCCESS != res)
   {
      LOG_MSG_ERROR("GetOpt failed: %d", res, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   // convert all the ancillary data structures that we got from RecvMsg
   for (i=0 , cmsg_ptr = DSS_CMSG_FIRSTHDR(msg) ; i < nOutAncLenReq; i++)
   {
      dssocki_err_msg_in err_info = {{0,0}};
      ds::Sock::AncDataIDType AncID = 0;
      ds::Sock::ExtendedErrType Err;
      ds::SockAddrStorageType AddrStorage;
      ds::SockAddrIN6InternalType AddrIN6;
      unsigned int RecvIFHandle;
      dss_in_pktinfo_type    in_pktinfo;
      DSSNetApp* pNetApp;
      sint15 socketDSSNetApp = 0;
      dss_iface_id_type DSSNetAppIfaceID;

      if (NULL == cmsg_ptr){
         LOG_MSG_ERROR("DSS_CMSG_FIRSTHDR or DSS_CMSG_NXTHDR failed and returned NULL", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
      }

      // Get the type of this ancillary data
      if (NULL != pOutAncData){
         res = pOutAncData[i]->GetAncID(&AncID);
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("GetAncID failed:%d", res, 0, 0);
         }
      }

      // Extract data from pOutAncData[i] in accordance with the Ancillary data type
      switch (AncID)
      {
         case ::ds::Sock::AncData::ICMP_ERROR_INFO:
         {
            if ((DSS_MSG_ERRQUEUE & flags) && (TRUE == isRecvErrSet))
            {
               res = ((ds::Sock::IICMPErrInfoPriv*)pOutAncData[i])->GetExtendedErr(&Err);
               if (AEE_SUCCESS != res) {
                  LOG_MSG_ERROR("GetErr failed:%d", res, 0, 0);
               }

               res = ((ds::Sock::IICMPErrInfoPriv*)pOutAncData[i])->GetAddr(AddrStorage);
               if (AEE_SUCCESS != res) {
                  LOG_MSG_ERROR("GetAddr failed:%d", res, 0, 0);
               }

               (void)ds::Sock::AddrUtils::GetFamily(AddrStorage, &family);
               family = ps_ntohs(family);

               res = DSSConversion::IDS2DSAddrFamily(family, &tmpDSFamily);
               if (AEE_SUCCESS != res) {
                  LOG_MSG_ERROR("Invalid Address Family %", family, 0, 0);
                  *dss_errno = DSSConversion::IDS2DSErrorCode(res);
                  sRet = DSS_ERROR;
                  goto bail;
               }

               (void)ds::Sock::AddrUtils::SetFamily(AddrStorage, tmpDSFamily);

               res = ::ds::Sock::AddrUtils::GetSockAddrIN6(AddrStorage,&AddrIN6);
               if (AEE_SUCCESS != res) {
                  LOG_MSG_ERROR("GetSockAddrIN6 failed:%d", res, 0, 0);
               }

               // TODO: are the values be the same in both of the APIs, or should we make a conversion?

               err_info.ee.ee_errno = (uint32)Err.error_number;
               err_info.ee.ee_origin = (uint8)Err.origin;
               err_info.ee.ee_type = (uint8)Err.type;
               err_info.ee.ee_code = (uint8)Err.code;
               err_info.ee.ee_info = (uint32)Err.info;
               // We don't update the pad and data fields, which are currently unused
               //Convert the family back to host byte order for legacy API
               err_info.ps_sin_addr.ps_sin_family = AddrIN6.family;
               err_info.ps_sin_addr.ps_sin_port = AddrIN6.port;
               memcpy(&(err_info.ps_sin_addr.ps_sin_addr.ps_s_addr),
                      (const void*)&(AddrIN6.addr[12]), sizeof(uint32));

               // TODO: do we want to fetch the level as well (i.e. to add it to the IDL)
               tempRes =
                  ds::Utils::Conversion::PutCMsg(msg,
                                                 cmsg_ptr,
                                                 (int16)DSS_IPPROTO_IP,
                                                (int16)DSS_IP_RECVERR, sizeof(err_info),
                                                (void*)&err_info,&controllen);

               if (AEE_SUCCESS != tempRes){
                 LOG_MSG_ERROR("Failed to put cmsg with err %d",tempRes , 0, 0);
               }
            }
            break;
         }

         case ::ds::Sock::AncData::RECV_IF_INFO:
         {
            if (TRUE == isRecvIfSet){
               socketDSSNetApp = pDSSSocket->GetNetApp();
               // if the socket is connected to DSSNetApp then we get the iface_id
               // that is stored in this DSSNetApp and pass it to the application via
               // ancillary data.
               if (0 != socketDSSNetApp){
                  res = DSSGlobals::Instance()->GetNetApp(socketDSSNetApp, &pNetApp);
                  if (AEE_SUCCESS != res) {
                   LOG_MSG_ERROR("GetNetApp failed:%d", res, 0, 0);
                  }
                  pNetApp->GetIfaceId(&DSSNetAppIfaceID);
                  in_pktinfo.if_index = DSSNetAppIfaceID;
               }
               else // The socket is not connected to NetApp so we pass the iface_id that we got from ISocket
               {
                  res = ((ds::Sock::IRecvIFInfoPriv*)pOutAncData[i])->GetRecvIFHandle(&RecvIFHandle);
                  if (AEE_SUCCESS != res) {
                   LOG_MSG_ERROR("GetRecvIFHandle failed:%d", res, 0, 0);
                  }
                  in_pktinfo.if_index = RecvIFHandle;
               }
               // Write the ancillary data to the old msg structure
               tempRes =
                  ds::Utils::Conversion::PutCMsg(msg,
                                                 cmsg_ptr,
                                                 (int16)PS_IPPROTO_IP,
                                                 (int16)DSS_IP_RECVIF,
                                                 sizeof(in_pktinfo),
                                                 (void*)&in_pktinfo,
                                                 &controllen );

               if (AEE_SUCCESS != tempRes){
                 LOG_MSG_ERROR("Failed to put cmsg with err %d",tempRes , 0, 0);
               }
            }

            break;
         }

         default:
            LOG_MSG_ERROR("Unsupported Ancillary Data received :%d", AncID, 0, 0);
      }

      DSSCommon::ReleaseIf((IQI**) &(pOutAncData[i]));

      cmsg_ptr = DSS_CMSG_NXTHDR(msg, cmsg_ptr);
   }

   if(controllen < msg->msg_controllen)
   {
      msg->msg_controllen = controllen;
   }

   sRet = nBufLenReq;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   DSSCommon::ReleaseIf((IQI**)&pIDSsocketPriv);
   PS_SYSTEM_HEAP_MEM_FREE (pOutAncData);
   PS_SYSTEM_HEAP_MEM_FREE (bufs);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_WRITE()

DESCRIPTION
  Writes up to nbytes of data from buffer over the transport specified
  by the socket descriptor.

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes written, which could be less than the number of
      bytes specified.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specified
  DS_ENOTCONN         socket not connected
  DS_ECONNRESET       TCP connection reset by server
  DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
  DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
  DS_EPIPE            broken pipe
  DS_EADDRREQ         destination address required
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           bad memory address
  DS_EWOULDBLOCK      operation would block
  DS_EINVAL           can't write to a listen socket

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_write
(
  sint15 sockfd,                                      /* socket descriptor */
  const void *buffer,               /* user buffer from which to copy data */
  uint16 nbytes,                /* number of bytes to be written to socket */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   int nNumWritten = 0;
   AEEResult res;
   sint15 sRet = DSS_ERROR;

   LOG_MSG_FUNCTION_ENTRY("dss_write(): socket:%d, buffer:0x%p", sockfd, buffer, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   if (NULL == buffer && 0 != nbytes) {
      LOG_MSG_ERROR("Buffer is NULL", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_write: pIDSSocket is NULL",0, 0, 0); 
      ASSERT(0);
   }

   res = pIDSSocket->Write((const byte*)buffer, (int)nbytes, &nNumWritten);
   if (AEE_SUCCESS != res) {
      if (AEE_EWOULDBLOCK == res) {
         LOG_MSG_ERROR("Write operation in progress (flow controlled)", 0, 0, 0);
      } else {
      LOG_MSG_ERROR("Write operation failed", 0, 0, 0);
      }
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   sRet = (sint15)nNumWritten;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_WRITEV()

DESCRIPTION
  Provides the gather write variant of the dss_write() call, which
  allows the application to write from non-contiguous buffers.    Sends
  specified number of bytes in the buffer over the TCP transport.

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes written, which could be less than the number of
      bytes specified.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specified
  DS_ENOTCONN         socket not connected
  DS_ECONNRESET       TCP connection reset by server
  DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
  DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
  DS_EPIPE            broken pipe
  DS_EADDRREQ         destination address required
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           bad memory address
  DS_EWOULDBLOCK      operation would block
  DS_EINVAL           Cant write to a listen socket

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_writev
(
  sint15 sockfd,                                      /* socket descriptor */
  struct ps_iovec iov[],  /* array of data buffers from which to copy data */
  sint15 iovcount,                                /* number of array items */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   ds::Sock::ISocketLocalPriv *pIDSPrivSokect = NULL;
   IPort1::SeqBytes *bufs = NULL;
   int nNumWritten;
   int i;
   AEEResult res;
   sint15 sRet;

   LOG_MSG_FUNCTION_ENTRY("dss_writev(): socket:%d", sockfd, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   if (NULL == iov) {
      if (0 == iovcount) {
         sRet = 0;
      }
      else {
         LOG_MSG_ERROR("iov parameter is NULL", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
      }
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_writev: pIDSSocket is NULL",0, 0, 0); 
      ASSERT(0);
   }

   // validate array entities.
   for (i = 0; i < iovcount; i++) {
      if ((NULL == iov[i].ps_iov_base) && (0 != iov[i].ps_iov_len)) {
         LOG_MSG_ERROR("One of the buffers is NULL", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   // allocate space for bufs
   if (iovcount > 0) {
      bufs = (IPort1::SeqBytes*)ps_system_heap_mem_alloc(sizeof(IPort1::SeqBytes)*iovcount);
      if (NULL == bufs) {
         LOG_MSG_ERROR("Can't allocate memory for the buffers", 0, 0, 0);
         *dss_errno = DS_EMSGSIZE;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   // fill bufs
   for (i = 0; i < iovcount; i++) {
      bufs[i].data = iov[i].ps_iov_base;
      bufs[i].dataLen = (int)iov[i].ps_iov_len;
   }

   res = pIDSSocket->QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv, (void**)&(pIDSPrivSokect));
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv) failed: %d", res, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = pIDSPrivSokect->WriteV(bufs, (int)iovcount, &nNumWritten);
   if (AEE_SUCCESS != res) {
      if (AEE_EWOULDBLOCK == res) {
         LOG_MSG_ERROR("WriteV operation in progress (flow controlled)", 0, 0, 0);
      } else {
      LOG_MSG_ERROR("WriteV operation failed", 0, 0, 0);
      }
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }


   sRet = (sint15)nNumWritten;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSPrivSokect);
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   PS_SYSTEM_HEAP_MEM_FREE (bufs);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_SENDTO()

DESCRIPTION
  Sends 'nbytes' bytes in the buffer over the transport specified by the
  socket descriptor.

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes to be written, which could be less than the number
      of bytes specified.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ---------------
  DS_EBADF            invalid socket descriptor is specified
  DS_EAFNOSUPPORT     address family not supported
  DS_EWOULDBLOCK      operation would block
  DS_EADDRREQ         destination address required
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           bad memory address
  DS_EOPNOTSUPP       option not supported
  DS_EMSGSIZE         the msg is too large to be sent all at once

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_sendto
(
  sint15 sockfd,                /* socket descriptor                       */
  const void *buffer,           /* user buffer from which to copy the data */
  uint16 nbytes,                /* number of bytes to be written           */
  uint32 flags,                 /* used for SDB (if enabled)               */
  struct ps_sockaddr *toaddr,      /* destination address                  */
  uint16 addrlen,               /* address length                          */
  sint15 *dss_errno             /* error condition value                   */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   int nNumSent;
   AEEResult res;
   sint15 sRet = DSS_ERROR;
   ds::SockAddrStorageType tempSockAddr;
   ds::AddrFamilyType tempFamily;

   LOG_MSG_FUNCTION_ENTRY("dss_sendto(): socket:%d, buffer:%d", sockfd, buffer, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   if (NULL == buffer && 0 != nbytes) {
      LOG_MSG_ERROR("Buffer is NULL", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
   }

   if (NULL == toaddr) {
      LOG_MSG_ERROR("Destination address is NULL", 0, 0, 0);
      *dss_errno = DS_EADDRREQ;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_sendto: pIDSSocket is NULL",0, 0, 0); 
      ASSERT(0);
   }

   if (DSS_AF_INET != toaddr->ps_sa_family && DSS_AF_INET6 != toaddr->ps_sa_family){
      LOG_MSG_ERROR("Illegal Address Family", 0, 0, 0);
      *dss_errno = DS_EAFNOSUPPORT;
      sRet = DSS_ERROR;
      goto bail;
   }

   if (DSS_AF_INET == toaddr->ps_sa_family){
      if (sizeof(struct ps_sockaddr_in) != addrlen) {
         LOG_MSG_ERROR("Illegal IPv4 address length", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   if (DSS_AF_INET6 == toaddr->ps_sa_family){
      if (sizeof(struct ps_sockaddr_in6) != addrlen) {
         LOG_MSG_ERROR("Illegal IPv6 address length", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   // Memcpy ps_sockaddr buffer into ps_sockaddr storage buffer
   addrlen = min(addrlen,sizeof(ds::SockAddrStorageType));
   memcpy(tempSockAddr, toaddr, addrlen);

   res = DSSConversion::DS2IDSAddrFamily(toaddr->ps_sa_family, &tempFamily);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid Address Family %", toaddr->ps_sa_family, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::SetFamily(tempSockAddr, ps_htons(tempFamily));

   res = pIDSSocket->SendTo((const byte*)buffer, (int)nbytes, tempSockAddr, flags, &nNumSent);

   if (AEE_SUCCESS != res) {
      if (AEE_EWOULDBLOCK == res) {
         LOG_MSG_ERROR("SendTo operation in progress (flow controlled)", 0, 0, 0);
      } else {
      LOG_MSG_ERROR("SendTo operation failed", 0, 0, 0);
      }
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   sRet = (sint15)nNumSent;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_SENDMSG()

DESCRIPTION
  This function is a common write function for all the socket output
  functions. The message header contains an array of scattered buffers, a
  socket descriptor and destination address for unconnected udp sockets.
  The function writes data from the scattered buffers over the transport
  specified by the socket descriptor.
DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes to be written, in case of tcp it could be less
  than the number of bytes specified.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ---------------
  DS_EBADF            invalid socket descriptor is specified
  DS_EAFNOSUPPORT     address family not supported
  DS_EWOULDBLOCK      operation would block
  DS_EADDRREQ         destination address required
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           bad memory address
  DS_EOPNOTSUPP       option not supported
  DS_EMSGSIZE         the msg is too large to be sent all at once
  DS_EISCONN          if the socket is connected and the destination
                      address is other than it is connected to.

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_sendmsg
(
  sint15                  sockfd,  /* socket descriptor                    */
  struct dss_msghdr     * msg,     /* Header containing data and dest addr */
  int                     flags,   /* flags used for SDB (if enabled)      */
  sint15                * dss_errno /* error condition value               */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   ds::Sock::ISocketLocalPriv *pIDSsocketPriv = NULL;
   IPort1::SeqBytes *bufs = NULL;
   ds::Sock::IAncDataPriv *pInAncData = NULL;
   ds::SockAddrStorageType tempSockAddr;
   ds::AddrFamilyType family = ds::AddrFamily::QDS_AF_UNSPEC;
   int nNumWritten;
   int i;
   AEEResult res;
   sint15 sRet;
   ds::AddrFamilyType tmpIDSFamily = ds::AddrFamily::QDS_AF_UNSPEC;
   int lengthToCopy = 0;

   LOG_MSG_FUNCTION_ENTRY("dss_sendmsg(): socket:%d, flags:%d, msg:0x%p", sockfd, flags, msg);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_sendmsg() : dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   if(NULL == msg) {
      LOG_MSG_ERROR("dss_sendmsg() : msg parameter is NULL", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   if(0 >= msg->msg_namelen) {
     LOG_MSG_ERROR("dss_sendmsg() : msg->msg_namelen <= 0 ", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_sendmsg: pIDSSocket is NULL",0, 0, 0); 
      ASSERT(0);
   }

   if (NULL == msg->msg_iov) {
      if (0 == msg->msg_iovlen) {
         sRet = 0;
      }
      else {
         LOG_MSG_ERROR("msg_iov parameter is NULL", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
      }
      goto bail;
   }

   if (0 < msg->msg_controllen) {
      LOG_MSG_ERROR("Ancillary data is not supported", 0, 0, 0);
      *dss_errno = DS_EOPNOTSUPP;
      sRet = DSS_ERROR;
      goto bail;
   }

   // validate array entities.
   for (i = 0; i < msg->msg_iovlen; i++) {
      if ((NULL == msg->msg_iov[i].ps_iov_base) && (0 != msg->msg_iov[i].ps_iov_len)) {
         LOG_MSG_ERROR("One of the buffers is NULL", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   // allocate space for bufs
   if (msg->msg_iovlen > 0) {
      bufs = (IPort1::SeqBytes*)ps_system_heap_mem_alloc(sizeof(IPort1::SeqBytes)*(msg->msg_iovlen));
      if (NULL == bufs) {
         LOG_MSG_ERROR("Can't allocate memory for the buffers", 0, 0, 0);
         *dss_errno = DS_ENOMEM;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   // fill bufs
   for (i = 0; i < msg->msg_iovlen; i++) {
      bufs[i].data = msg->msg_iov[i].ps_iov_base;
      bufs[i].dataLen = (int)(msg->msg_iov[i].ps_iov_len);
   }

   lengthToCopy = min(msg->msg_namelen, sizeof(ds::SockAddrStorageType));

   memcpy(tempSockAddr, msg->msg_name, lengthToCopy);
   // address family is retrieved in DS format
   (void)ds::Sock::AddrUtils::GetFamily(tempSockAddr, &family);
   // need to convert it to IDS format
   res = DSSConversion::DS2IDSAddrFamily(family, &tmpIDSFamily);
   if (AEE_SUCCESS != res) {
     LOG_MSG_ERROR("Invalid Address Family %", family, 0, 0);
     *dss_errno = DSSConversion::IDS2DSErrorCode(res);
     sRet = DSS_ERROR;
     goto bail;
   }

   (void)ds::Sock::AddrUtils::SetFamily(tempSockAddr, ps_htons(tmpIDSFamily));

   res = pIDSSocket->QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv, (void**)&pIDSsocketPriv);
   if (AEE_SUCCESS != res) {
     LOG_MSG_ERROR("QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv) failed: %d", res, 0, 0);
     *dss_errno = DSSConversion::IDS2DSErrorCode(res);
     sRet = DSS_ERROR;
     goto bail;
   }
   res = pIDSsocketPriv->SendMsg(tempSockAddr,
                                 bufs, (int)msg->msg_iovlen, &nNumWritten,
                                &pInAncData, 0, 0);

   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   sRet = (sint15)nNumWritten;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   DSSCommon::ReleaseIf((IQI**)&pIDSsocketPriv);
   PS_SYSTEM_HEAP_MEM_FREE (bufs);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_SHUTDOWN

DESCRIPTION
  Shuts down the connection of the specified socket depending on the
  'how' parameter as follows:

  DSS_SHUT_RD:   Disallow subsequent calls to recv function
  DSS_SHUT_WR:   Disallow subsequent calls to send function
  DSS_SHUT_RDWR: Disallow subsequent calls to both recv and send functions

DEPENDENCIES
  None.

PARAMETERS
  sockfd    -  socket file descriptor
  how       -  action to be performed: shutdown read-half, write-half or
               both
  dss_errno -  error number

RETURN VALUE
  In case of successful completion, returns DSS_SUCCESS. Otherwise, returns
  DSS_ERROR and places the error number in dss_errno.

  Errno Values
  ------------
  DS_EBADF                invalid socket descriptor is specified
  DS_ENOTCONN             the socket is not connected
  DS_EINVAL               invalid operation (e.g., how parameter is invalid)
  DS_ENOMEM               insufficient memory available to complete the
                          operation

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_shutdown
(
  sint15           sockfd,                  /* socket descriptor           */
  uint16           how,                     /* what action to perform      */
  sint15*          dss_errno                /* error number                */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   AEEResult res;
   sint15 sRet;

   LOG_MSG_FUNCTION_ENTRY("dss_shutdown(): socket:%d, action:%u", sockfd, how, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_shutdown: pIDSSocket is NULL",0, 0, 0);
      ASSERT(0);
   }

   res = pIDSSocket->Shutdown((ds::Sock::ShutdownDirType)how);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Shutdown operation failed", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   sRet = DSS_SUCCESS;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_CLOSE()

DESCRIPTION
  Non-blocking close of a socket.  Performs all necessary clean-up of data
  structures and frees the socket for re-use.  For TCP initiates the active
  close for connection termination.  Once TCP has closed, the DS_CLOSE_EVENT
  will become TRUE, and the application can call dss_close() again to free
  the socket for re-use.  UDP sockets also need to call this to
  clean-up the socket and free it for re-use.

DEPENDENCIES
  None.

RETURN VALUE
  Returns DSS_SUCCESS on success.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EWOULDBLOCK      operation would block - TCP close in progress
  DS_EBADF            invalid socket descriptor is specified

SIDE EFFECTS
  Initiates active close for TCP connections.
===========================================================================*/
sint15 dss_close
(
  sint15 sockfd,                                      /* socket descriptor */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   AEEResult res;
   sint15 sRet;

   LOG_MSG_FUNCTION_ENTRY("dss_close(): socket:%d", sockfd, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_close: pIDSSocket is NULL",0, 0, 0);
      ASSERT(0);
   }

   res = pIDSSocket->Close();
   if (AEE_SUCCESS != res) {
      if (AEE_EWOULDBLOCK == res) {
         LOG_MSG_ERROR("Close operation in progress (flow controlled)", 0, 0, 0);
      } else {
         LOG_MSG_ERROR("Close operation failed", 0, 0, 0);
      }
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   // De-Register all IP filter that were registered using this sockfd
   // We should not bail out here on error. App could have already freed
   // the filters using dss_dereg_ip_filter() API.
   // De-reg filters only after FIN is received for TCP sockets
   // (pIDSSock->Close() returns AEE_SUCCESS).
   (void) DSSGlobals::Instance()->RemoveFilterRegObjectFromList(sockfd);

   // Synchronized dss_close: If Stop returns AEE_SUCCESS, it means that the socket is already
   // closed and we don't need to wait for event
   (void) DSSGlobals::Instance()->RemoveSocket(sockfd);

   sRet = DSS_SUCCESS;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}



/*===========================================================================
FUNCTION DSS_SETSOCKOPT()

DESCRIPTION
  Set the options associated with a socket. This function expects the
  following parameters:

DEPENDENCIES
  None.

PARAMETERS
  int sockfd        -     Socket file descriptor.
  int level         -     Socket option level.
  int optname,      -     Option name.
  void *optval      -     Pointer to the option value.
  uint32 *optlen    -     Pointer to the size of the option value.
  sint15 *errno     -     Error condition value.

RETURN VALUE
  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF                invalid socket descriptor is specified
  DS_ENOPROTOOPT          the option is unknown at the level indicated
  DS_EINVAL               invalid option name or invalid option value
  DS_EFAULT               Invalid buffer or argument

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_setsockopt
(
  int sockfd,                            /* socket descriptor              */
  int level,                             /* socket option level            */
  int optname,                           /* option name                    */
  void *optval,                          /* value of the option            */
  uint32 *optlen,                        /* size of the option value       */
  sint15 *dss_errno                      /* error condition value          */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   ds::Sock::ISocketLocalPriv *pIDSPrivSokect = NULL;
   ds::Sock::ISocketExt *pIDSExtSokect = NULL;
   ds::Sock::OptLevelType IDSLevel;
   ds::Sock::OptNameType IDSOptName;
   ds::Sock::OptNameTypePriv IDSOptNamePriv;
   ds::Sock::IPMembershipInfoType IDSMembership;
   ds::Sock::LingerType IDSLinger;
   DSSIDSNetPolicyPrivScope IDSNetPolicyPrivScope;
   AEEResult res = AEE_SUCCESS;
   sint15 sRet = DSS_SUCCESS;

   LOG_MSG_FUNCTION_ENTRY("dss_setsockopt(): socket:%d, level:%d, option_name:%d", sockfd, level, optname);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   if ((NULL == optval) || (NULL == optlen)) {
      LOG_MSG_ERROR("Either optval or optlen is NULL", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet =(DSS_ERROR);
      goto bail;
   }

   // Even though this socket option is deprecated, operation should succeed, no value shall be stored,
   // to support the legacy apps. Some legacy apps expect success on this operation,
   // they don't care that this operation makes no change to any values in the system
   if ((int)DSS_SO_QOS_SHARE_HANDLE == optname){
      LOG_MSG_INVALID_INPUT("DSS_SO_QOS_SHARE_HANDLE socket option is deprecated",0,0,0);
      sRet = DSS_SUCCESS;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_setsockopt: pIDSSocket is NULL",0, 0, 0);
      ASSERT(0);
   }

   /* Silently return success for DISABLE_FLOW_FWDING */
   if ((int)DSS_SO_DISABLE_FLOW_FWDING == optname) {
      pDSSSocket->SetFlowForwarding(*((int*)optval));
      *dss_errno = 0;
      sRet = DSS_SUCCESS;
      goto bail;
   }

   res = pIDSSocket->QueryInterface(ds::Sock::AEEIID_ISocketExt, (void**)&(pIDSExtSokect));
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("QueryInterface(ds::Sock::AEEIID_ISocketExt) failed: %d", res, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   if ((int)DSS_IP_ADD_MEMBERSHIP == optname || (int)DSS_IP_DROP_MEMBERSHIP == optname) {
      dss_ip_mreqn* mcastArg = (dss_ip_mreqn*)optval;
      struct ps_in_addr mcast_addr = mcastArg->mcast_grp;
      IDSMembership.mcastGroup.family = ds::AddrFamily::QDS_AF_INET;
      (void)memmove(IDSMembership.mcastGroup.addr,&mcast_addr.ps_s_addr,4);
      IDSMembership.ifaceId = (mcastArg->iface_id & 0xFF000000UL) | 0x00FFFF00UL ; // lower layers are expecting iface id with FFFF's
      if (PS_IFACE_INVALID_ID == IDSMembership.ifaceId) {
         IDSMembership.ifaceId = INVALID_IFACE_ID;
      }
      if ((int)DSS_IP_ADD_MEMBERSHIP == optname) {
         res = pIDSExtSokect->AddIPMembership(&IDSMembership);
      }
      else {
         res = pIDSExtSokect->DropIPMembership(&IDSMembership);
      }

      if (AEE_SUCCESS != res) {
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
      }
      else {
         sRet = DSS_SUCCESS;
      }

      goto bail;
      }

   if ((int)DSS_IPV6_ADD_MEMBERSHIP == optname || (int)DSS_IPV6_DROP_MEMBERSHIP == optname) {
      dss_ipv6_mreqn* mcastArg = (dss_ipv6_mreqn*)optval;
      struct ps_in6_addr mcast_addr = mcastArg->mcast_grp_v6;
      IDSMembership.mcastGroup.family = ds::AddrFamily::QDS_AF_INET6;
      (void)memmove(IDSMembership.mcastGroup.addr,mcast_addr.ps_s6_addr,16);
      IDSMembership.ifaceId = (mcastArg->iface_id & 0xFF000000UL) | 0x00FFFF00UL ; // lower layers are expecting iface id with FFFF's
      if ((int)DSS_IPV6_ADD_MEMBERSHIP == optname) {
         res = pIDSExtSokect->AddIPMembership(&IDSMembership);
      } else {
         res = pIDSExtSokect->DropIPMembership(&IDSMembership);
      }
      if (AEE_SUCCESS != res) {
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
      }
      else {
         sRet = DSS_SUCCESS;
      }
      goto bail;
   }

   if ((int)DSS_BCMCS_JOIN == optname || (int)DSS_BCMCS_LEAVE == optname) {

      DSSNetApp *pNetApp = NULL;
      DSSIDSNetworkScope IDSNetworkScope;
      struct ps_in_addr *mcast_addr = (struct ps_in_addr*)optval;
      IDSMembership.mcastGroup.family = ds::AddrFamily::QDS_AF_INET;
      (void)memmove(IDSMembership.mcastGroup.addr,&(mcast_addr->ps_s_addr),4);

      int kind;
      pDSSSocket->GetSockKind(&kind);
      if (2 == kind) {
        //No app for sock2
        IDSMembership.ifaceId = INVALID_IFACE_ID;
      }
      else {
        // Get the iface id from the IDSNetwork object from the DSSNetApp.
        sint15 appid = pDSSSocket->GetNetApp();

        res = DSSGlobals::Instance()->GetNetApp(appid, &pNetApp);
        if (AEE_SUCCESS != res) {
           *dss_errno = DSSConversion::IDS2DSErrorCode(res);
           sRet = DSS_ERROR;
           goto bail;
        }
        res = IDSNetworkScope.Init(pNetApp);
        if (AEE_SUCCESS != res) {
           *dss_errno = DSSConversion::IDS2DSErrorCode(res);
           sRet = DSS_ERROR;
           goto bail;
        }

        // Make sure that the network is up
        NetworkStateType netState;
        res = IDSNetworkScope.Fetch()->GetState(&netState);
        if(AEE_SUCCESS != res) {
           *dss_errno = DSSConversion::IDS2DSErrorCode(res);
           sRet = DSS_ERROR;
           goto bail;
        }
        if (NetworkState::QDS_OPEN != netState) {
           *dss_errno = DS_ENETDOWN;
           sRet = DSS_ERROR;
           goto bail;
        }

        ds::Net::IfaceIdType iface_id;
        res = IDSNetworkScope.Fetch()->GetIfaceId(&iface_id);
        if (AEE_SUCCESS != res) {
           *dss_errno = DSSConversion::IDS2DSErrorCode(res);
           sRet = DSS_ERROR;
           goto bail;
        }

        IDSMembership.ifaceId = iface_id;
      }//Sock kind !=2

      if ((int)DSS_BCMCS_JOIN == optname) {
         res = pIDSExtSokect->AddIPMembership(&IDSMembership);
      } else {
         res = pIDSExtSokect->DropIPMembership(&IDSMembership);
      }
      if (AEE_SUCCESS != res) {
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
      }
      else {
         sRet = DSS_SUCCESS;
      }
      goto bail;
   }

   if ((int)DSS_SO_LINGER == optname) {
      res = DSSConversion::DS2IDSLinger((dss_so_linger_type*)optval, &IDSLinger);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("DS2IDSLinger operation failed:%d", res, 0, 0);
      }

      res = pIDSSocket->SetSOLingerReset(&IDSLinger);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("SetSOLinger operation failed", 0, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
      }
      else {
         sRet = DSS_SUCCESS;
      }
      goto bail;
   }

   if ((int)DSS_SO_LINGER_RESET == optname) {
      res = DSSConversion::DS2IDSLinger((dss_so_linger_type*)optval, &IDSLinger);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("DS2IDSLinger operation failed:%d", res, 0, 0);
      }

      res = pIDSSocket->SetSOLingerReset(&IDSLinger);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("SetSOLingerReset operation failed", 0, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
      } else {
         sRet = DSS_SUCCESS;
      }
      goto bail;
   }

   if ((int)DSS_SO_SILENT_CLOSE == optname) {

      IDSLinger.lingerEnabled = TRUE;
      IDSLinger.timeInSec = 0;

      res = pIDSSocket->SetSOLingerReset(&IDSLinger);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("SetSOLingerReset operation failed", 0, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
      } else {
         sRet = DSS_SUCCESS;
      }
      goto bail;
   }

   if ((int)DSS_SO_SDB_ACK_CB == optname) {
      res = pDSSSocket->SetSdbAckCb((dss_so_sdb_ack_cb_type*)optval);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("SetSdbAckSignal operation failed: %d", res, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
      } else {
         sRet = DSS_SUCCESS;
      }

      goto bail;
   }

   res = pIDSSocket->QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv, (void**)&(pIDSPrivSokect));
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv) failed: %d", res, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   if ((int)DSS_SO_NETPOLICY == optname) {
      if (-1 != pDSSSocket->GetNetApp()) {
         // This option is supported only for sockets created using dss_socket2()
         LOG_MSG_ERROR("DSS_SO_CB_FCN is supported only for sockets created using dss_socket2()", 0, 0, 0);
         *dss_errno = DS_EOPNOTSUPP;
         sRet = DSS_ERROR;
      }
      else {
         res = IDSNetPolicyPrivScope.Init();
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("Can't create DSNetPolicy", 0, 0, 0);
            *dss_errno = DSSConversion::IDS2DSErrorCode(res);
            sRet = DSS_ERROR;
            goto bail;
         }

         res = DSSConversion::DS2IDSNetPolicy((dss_net_policy_info_type*)optval, IDSNetPolicyPrivScope.Fetch());
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("Can't convert Net Policy", 0, 0, 0);
            *dss_errno = DSSConversion::IDS2DSErrorCode(res);
            sRet = DSS_ERROR;
            goto bail;
         }

         // TODO: AddRef?
         res = pIDSExtSokect->SetNetPolicy(IDSNetPolicyPrivScope.Fetch());
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("SetNetPolicy operation failed", 0, 0, 0);
            *dss_errno = DSSConversion::IDS2DSErrorCode(res);
            sRet = DSS_ERROR;
            goto bail;
         }
         res = pDSSSocket->SetLegacyPolicy((dss_net_policy_info_type*)optval);
         if (AEE_SUCCESS != res) {
            LOG_MSG_ERROR("SetLegacyPolicy operation failed", 0, 0, 0);
            *dss_errno = DSSConversion::IDS2DSErrorCode(res);
            sRet = DSS_ERROR;
            goto bail;
         }
      }
      goto bail;
   }

   if ((int)DSS_SO_CB_FCN == optname) {
      if (-1 != pDSSSocket->GetNetApp()) {
         // This option is supported only for sockets created using dss_socket2()
         LOG_MSG_ERROR("DSS_SO_CB_FCN is supported only for sockets created using dss_socket2()", 0, 0, 0);
         *dss_errno = DS_EOPNOTSUPP;
         sRet = DSS_ERROR;
      }
      else {
         pDSSSocket->SetCb((dss_sock_cb_fcn_type*)optval);
         sRet = DSS_SUCCESS;
         goto bail;
      }
   }

   if (sizeof(int) > *optlen) {
      LOG_MSG_ERROR("optlen is not valid", 0, 0, 0);
      *dss_errno = DS_EINVAL;
      sRet = DSS_ERROR;
      goto bail;
   }

   if ((int)DSS_SO_SYS_SOCK == optname) {
      ds::Sock::ISocketLocalPriv *pIDSsocketPriv = NULL;
      if (DSS_SOL_SOCKET != (dss_sockopt_levels_type)level) {
         LOG_MSG_ERROR("Wrong Option Level", 0, 0, 0);
         *dss_errno = DS_EINVAL;
         sRet = DSS_ERROR;
         goto bail;
      }
      res = pIDSSocket->QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv, (void**)&pIDSsocketPriv);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv) failed: %d", res, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
         goto bail;
      }
      res = pIDSsocketPriv->SetSystemOption(TRUE);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("SetSystemOption(ds::Sock::AEEIID_ISocketLocalPriv) failed: %d", res, 0, 0);
      }
      DSSCommon::ReleaseIf((IQI**)&pIDSsocketPriv);
      sRet = DSS_SUCCESS;
      goto bail;
   }

   res = DSSConversion::DS2IDSOptLevel((dss_sockopt_levels_type)level, &IDSLevel);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't convert Option Level", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   // Check if the option should be translated to a OptNamePriv
   res = DSSConversion::DS2IDSOptNamePriv((dss_sockopt_names_type)optname, &IDSOptNamePriv);
   if (AEE_SUCCESS == res) {
      // temporarily convert optlen from uint32* to int*
      res = pIDSPrivSokect->SetOptPriv(IDSLevel, IDSOptNamePriv, *(int*)optval);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("SetOptPriv operation failed", 0, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
      }
      else {
         sRet = DSS_SUCCESS;
      }
      goto bail;
   }

   res = DSSConversion::DS2IDSOptName((dss_sockopt_names_type)optname, &IDSOptName);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't convert Option Name", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   // temporarily convert optlen from uint32* to int*
   res = pIDSSocket->SetOpt(IDSLevel, IDSOptName, *(int*)optval);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("SetOpt operation failed", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
   }
   else {
      sRet = DSS_SUCCESS;
   }

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSPrivSokect);
   DSSCommon::ReleaseIf((IQI**)&pIDSExtSokect);
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_GETSOCKOPT()

DESCRIPTION
  Return an option associated with a socket. This function expects the
  following parameters:

DEPENDENCIES
  None.

PARAMETERS
  int sockfd        -     Socket file descriptor.
  int level         -     Socket option level.
  int optname,      -     Option name.
  void *optval      -     Pointer to the option value.
  uint32 *optlen    -     Pointer to the size of the option value.
  sint15 *errno     -     Error condition value.

RETURN VALUE
  optlen is a value-result parameter, initially containing the size of
  the buffer pointed to by optval, and modified on return to indicate the
  actual  size  of the value returned. On error, return DSS_ERROR and places
  the error condition value in *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF                invalid socket descriptor is specified
  DS_ENOPROTOOPT          the option is unknown at the level indicated
  DS_EINVAL               invalid option name or invalid option value
  DS_EFAULT               Invalid buffer or argument

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_getsockopt
(
  int sockfd,                            /* socket descriptor              */
  int level,                             /* socket option level            */
  int optname,                           /* option name                    */
  void *optval,                          /* value of the option            */
  uint32 *optlen,                        /* size of the option value       */
  sint15 *dss_errno                      /* error condition value          */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   ds::Sock::ISocketLocalPriv *pIDSPrivSokect = NULL;
   ds::Sock::OptLevelType IDSLevel;
   ds::Sock::OptNameType IDSOptName;
   ds::Sock::OptNameTypePriv IDSOptNamePriv;
   ds::Sock::LingerType IDSLinger;
   DSSIDSNetPolicyScope IDS2DSNetPolicyScope;
   AEEResult res;
   sint15 sRet = DSS_SUCCESS;

   LOG_MSG_FUNCTION_ENTRY("dss_getsockopt(): socket:%d, level:%d, option_name:%d", sockfd, level, optname);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   if ((NULL == optval) || (NULL == optlen)){
      LOG_MSG_ERROR("Either optval or optlen is NULL", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   // Even though this socket option is deprecated, operation should succeed and arbitrary value shall be returned
   // to support the legacy apps. Some legacy apps expect success on this operation even though the optval is meaningless for them
   if ((int)DSS_SO_QOS_SHARE_HANDLE == optname){
      LOG_MSG_INVALID_INPUT("DSS_SO_QOS_SHARE_HANDLE socket option is deprecated",0,0,0);
      *((unsigned int *) optval) = 17;
      sRet = DSS_SUCCESS;
      goto bail;
   }

   // DSS_BCMCS_JOIN and DSS_BCMCS_LEAVE are not supported for dss_getsockopt
   if ( ((int)DSS_BCMCS_JOIN == optname) || ((int)DSS_BCMCS_LEAVE == optname) ) {
      LOG_MSG_INVALID_INPUT("DSS_BCMCS_JOIN and DSS_BCMCS_LEAVE socket options are not valid for getsockopt",0,0,0);      
      *dss_errno = DS_ENOPROTOOPT;   // DS_ENOPROTOOPT error code preserves backward compatibility
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_setsockopt: pIDSSocket is NULL",0, 0, 0);
      ASSERT(0);
   }

   /* Silently return success for DISABLE_FLOW_FWDING */
   if ((int)DSS_SO_DISABLE_FLOW_FWDING == optname) {
      *dss_errno = 0;
      pDSSSocket->GetFlowForwarding((int*)optval);
      sRet = DSS_SUCCESS;
      goto bail;
   }

   if ((int)DSS_ICMP_ECHO_ID == optname || (int)DSS_ICMP_ECHO_SEQ_NUM == optname) {
      // These options are supported for set only
      LOG_MSG_ERROR("Option is not supported for dss_getsockopt", 0, 0, 0);
      *dss_errno = DS_EINVAL;
      sRet = DSS_ERROR;
      goto bail;
   }

   if ((int)DSS_SO_LINGER == optname) {
      res = pIDSSocket->GetSOLingerReset(&IDSLinger);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetSOLinger failed", 0, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
         goto bail;
      }
      res = DSSConversion::IDS2DSLinger(&IDSLinger, (dss_so_linger_type*)optval);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("IDS2DSLinger failed", 0, 0, 0);
      }

      *optlen = sizeof(dss_so_linger_type);
      sRet = DSS_SUCCESS;
      goto bail;
   }

   if ((int)DSS_SO_LINGER_RESET == optname ||
       (int)DSS_SO_SILENT_CLOSE == optname) {
      res = pIDSSocket->GetSOLingerReset(&IDSLinger);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetSOLingerReset failed", 0, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
         goto bail;
      }
      res = DSSConversion::IDS2DSLinger(&IDSLinger, (dss_so_linger_type*)optval);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("IDS2DSLinger failed:%d", res, 0, 0);
      }

      *optlen = sizeof(dss_so_linger_type);
      sRet = DSS_SUCCESS;
      goto bail;
   }

   if ((int)DSS_SO_SDB_ACK_CB == optname) {
      sRet = pDSSSocket->GetSdbAckCb((dss_so_sdb_ack_cb_type*)optval);
      *optlen = sizeof(dss_so_sdb_ack_cb_type);
      goto bail;
   }

   if ((int)DSS_SO_NETPOLICY == optname) {
      res = IDS2DSNetPolicyScope.Init(pIDSSocket);
         if (AEE_SUCCESS != res) {
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
         goto bail;
         }

      // Convert IDSPOlicy to legacy policy representation. 3rd parameter -1 means app_id is not applicable
      // TODO: check if there is use case that requires us here to actually try and fetch the app_id via the DSSNetApp
      //       to which the DSSSocket is bound (if any), and set it in the iface_id in the policy accordingly.
      res = DSSConversion::IDS2DSNetPolicy(IDS2DSNetPolicyScope.Fetch(), (dss_net_policy_info_type*)optval, -1);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("Can't convert Net Policy", 0, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
         goto bail;
      }
      res = pDSSSocket->GetLegacyPolicy((dss_net_policy_info_type*)optval);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("SetLegacyPolicy operation failed", 0, 0, 0);
         sRet = DSS_ERROR;
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         goto bail;
      }
      *optlen = sizeof(dss_net_policy_info_type);
      goto bail;
   }

   if (sizeof(int) > *optlen) {
      LOG_MSG_ERROR("optlen is not valid", 0, 0, 0);
      *dss_errno = DS_EINVAL;
      sRet = DSS_ERROR;
      goto bail;
   }

   if ((int)DSS_SO_SYS_SOCK == optname) {
      ds::Sock::ISocketLocalPriv *pIDSsocketPriv = NULL;
      boolean bSystemOption;
      if (DSS_SOL_SOCKET != (dss_sockopt_levels_type)level) {
         LOG_MSG_ERROR("Wrong Option Level", 0, 0, 0);
         *dss_errno = DS_EINVAL;
         sRet = DSS_ERROR;
         goto bail;
      }
      res = DSSConversion::IDS2DSErrorCode(pIDSSocket->QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv, (void**)&pIDSsocketPriv));
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv) failed: %d", res, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
         goto bail;
      }
      res = pIDSsocketPriv->GetSystemOption(&bSystemOption);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetSystemOption(ds::Sock::AEEIID_ISocketLocalPriv) failed: %d", res, 0, 0);
      }

      *((int*)optval) = bSystemOption ? 1 : 0;
      *optlen = sizeof(int);
      DSSCommon::ReleaseIf((IQI**)&pIDSsocketPriv);
      sRet = DSS_SUCCESS;
      goto bail;
   }

   res = DSSConversion::DS2IDSOptLevel((dss_sockopt_levels_type)level, &IDSLevel);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't convert Option Level", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = pIDSSocket->QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv, (void**)&(pIDSPrivSokect));
   if (AEE_SUCCESS != res) {
     LOG_MSG_ERROR("QueryInterface(ds::Sock::AEEIID_ISocketLocalPriv) failed: %d", res, 0, 0);
     *dss_errno = DSSConversion::IDS2DSErrorCode(res);
     sRet = DSS_ERROR;
     goto bail;
   }

   // Check if the option should be translated to a OptNamePriv
   res = DSSConversion::DS2IDSOptNamePriv((dss_sockopt_names_type)optname, &IDSOptNamePriv);
   if (AEE_SUCCESS == res) {
      res = pIDSPrivSokect->GetOptPriv(IDSLevel, IDSOptNamePriv, (int*)optval);
      if (AEE_SUCCESS != res) {
         LOG_MSG_ERROR("GetOptPriv operation failed", 0, 0, 0);
         *dss_errno = DSSConversion::IDS2DSErrorCode(res);
         sRet = DSS_ERROR;
      }
      else {
         sRet = DSS_SUCCESS;
      }
      goto bail;
   }

   res = DSSConversion::DS2IDSOptName((dss_sockopt_names_type)optname, &IDSOptName);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't convert Option Name", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   // temporarily convert optlen from uint32* to int*
   res = pIDSSocket->GetOpt(IDSLevel, IDSOptName, (int*)optval);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("GetOpt operation failed", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   if ((int)DSS_SO_ERROR == optname)
   {
      *((int *) optval) = DSSConversion::IDS2DSErrorCode(*((int *) optval));
   }
//   *optlen = sizeof(int);

   sRet = DSS_SUCCESS;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSPrivSokect);
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_GETSOCKNAME

DESCRIPTION
  Returns the current local address assigned to the specified socket.

DEPENDENCIES
  None.

PARAMETERS
  sockfd    -  socket file descriptor
  addr      -  local address currently associated with the socket
  addrlen   -  address length. This parameter is initialized to indicate
               the amount of space pointed by addr and on return, it
               contains the actual size of the address returned.
  dss_errno -  error number

RETURN VALUE
  Returns DSS_SUCCESS upon successful completion and places the socket
  address and the address length in addr and addrlen parameters, resp.

  If the address is larger than the supplied buffer then it is silently
  truncated. The value returned in addrlen indicates the size prior to
  truncation, if any.

  On error, returns DSS_ERROR and places the error number in dss_errno.

  Errno Values
  ------------
  DS_EBADF                invalid socket descriptor is specified
  DS_EFAULT               addr parameter points to an invalid memory
                          location

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_getsockname
(
  sint15              sockfd,               /* socket descriptor           */
  struct ps_sockaddr* addr,                 /* address of the socket       */
  uint16*             addrlen,              /* address length              */
  sint15*             dss_errno             /* error number                */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   AEEResult res;
   sint15 sRet;
   ds::SockAddrStorageType tempSockAddr;
   ds::AddrFamilyType family = ds::AddrFamily::QDS_AF_UNSPEC;
   int tmpDSFamily;
   size_t copylen;

   LOG_MSG_FUNCTION_ENTRY("dss_getsockname(): socket:%d", sockfd, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   if (NULL == addr){
      LOG_MSG_ERROR("Address is NULL", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   if (NULL == addrlen) {
      LOG_MSG_ERROR("addrlen is NULL", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_getsockname: pIDSSocket is NULL",0, 0, 0);
      ASSERT(0);
   }

   memset(tempSockAddr, 0, sizeof(tempSockAddr));

   res = pIDSSocket->GetSockName(tempSockAddr);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("GetSockName operation failed", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::GetFamily(tempSockAddr,&family);
   family = ps_ntohs(family);

   res = DSSConversion::IDS2DSAddrFamily(family, &tmpDSFamily);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid Address Family %", family, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::SetFamily(tempSockAddr, tmpDSFamily);

   if (ds::AddrFamily::QDS_AF_INET == family) {
      if (sizeof(struct ps_sockaddr_in) < *addrlen)
      {
         *addrlen = sizeof(struct ps_sockaddr_in);
      }
   }
   else if (ds::AddrFamily::QDS_AF_INET6 == family) {
      if (sizeof(struct ps_sockaddr_in6) < *addrlen)
      {
         *addrlen = sizeof(struct ps_sockaddr_in6);
      }
   }
   else {
      LOG_MSG_ERROR("Invalid Address Family", 0, 0, 0);
      *dss_errno = DS_EAFNOSUPPORT;
      sRet = DSS_ERROR;
      goto bail;
   }

   copylen = min(*addrlen,sizeof(ds::SockAddrStorageType));

   memcpy(addr, tempSockAddr, copylen);

   sRet = DSS_SUCCESS;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}

/*===========================================================================
FUNCTION DSS_GETPEERNAME

DESCRIPTION
  Returns the address of the peer connected to the specified socket.

DEPENDENCIES
  None.

PARAMETERS
  sockfd    -  socket file descriptor
  addr      -  address of the peer connected with the socket
  addrlen   -  address length. This parameter is initialized to indicate
               the amount of space pointed by addr and on return, it
               contains the actual size of the address returned.
  dss_errno -  error number

RETURN VALUE
  Returns DSS_SUCCESS upon successful completion and places the peer
  address and the address length in addr and addrlen parameters, resp.

  If the address is larger than the supplied buffer then it is silently
  truncated. The value returned in addrlen indicates the size prior to
  truncation, if any.

  On error, returns DSS_ERROR and places the error number in dss_errno.

  Errno Values
  ------------
  DS_EBADF                invalid socket descriptor is specified
  DS_EFAULT               addr parameter points to an invalid memory
                          location
  DS_ENOTCONN             the socket is not connected

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_getpeername
(
  sint15              sockfd,               /* socket descriptor           */
  struct ps_sockaddr* addr,                 /* address of the socket       */
  uint16*             addrlen,              /* address length              */
  sint15*             dss_errno             /* error number                */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   AEEResult res;
   sint15 sRet;
   ds::SockAddrStorageType tempSockAddr;
   ds::AddrFamilyType family = ds::AddrFamily::QDS_AF_UNSPEC;
   int tmpDSFamily;
   size_t copylen;

   LOG_MSG_FUNCTION_ENTRY("dss_getpeername(): socket:%d", sockfd, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   if (NULL == addr) {
      LOG_MSG_ERROR("Address is NULL", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   if (NULL == addrlen) {
      LOG_MSG_ERROR("addrlen is NULL", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL== pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_getpeername: pIDSSocket is NULL",0, 0, 0);
      ASSERT(0);
   }

   memset(tempSockAddr, 0, sizeof(tempSockAddr));

   res = pIDSSocket->GetPeerName(tempSockAddr);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("GetPeerName operation failed", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::GetFamily(tempSockAddr,&family);
   family = ps_ntohs(family);

   res = DSSConversion::IDS2DSAddrFamily(family, &tmpDSFamily);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid Address Family %", family, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::SetFamily(tempSockAddr, tmpDSFamily);

   if (ds::AddrFamily::QDS_AF_INET == family) {
      if (sizeof(struct ps_sockaddr_in) < *addrlen)
      {
         *addrlen = sizeof(struct ps_sockaddr_in);
      }
   }
   else if (ds::AddrFamily::QDS_AF_INET6 == family) {
      if (sizeof(struct ps_sockaddr_in6) < *addrlen)
      {
         *addrlen = sizeof(struct ps_sockaddr_in6);
      }
   }
   else {
      LOG_MSG_ERROR("Invalid Address Family", 0, 0, 0);
      *dss_errno = DS_EAFNOSUPPORT;
      sRet = DSS_ERROR;
      goto bail;
   }

   copylen = min(*addrlen,sizeof(ds::SockAddrStorageType));

   memcpy(addr, tempSockAddr, copylen);

   sRet = DSS_SUCCESS;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
}





/*===========================================================================
FUNCTION DSS_WRITE_DSM_CHAIN()

DESCRIPTION
Sends the specified DSM item chain over the TCP transport.  Lower layers
will be responsible for deallocating the DSM item chain.

DEPENDENCIES
None.

RETURN VALUE
n - the number of bytes written.

On error, return DSS_ERROR and places the error condition value in
*dss_errno.

dss_errno Values
----------------
DS_EBADF            invalid socket descriptor is specfied
DS_ENOTCONN         socket not connected
DS_ECONNRESET       TCP connection reset by server
DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
DS_EPIPE            broken pipe
DS_EADDRREQ         destination address required
DS_ENETDOWN         network subsystem unavailable
DS_EFAULT           bad memory address
DS_EWOULDBLOCK      operation would block
DS_EOPNOTSUPP       invalid server address specified

SIDE EFFECTS
The pointer to the DSM item chain is set to NULL.
===========================================================================*/
sint15 dss_write_dsm_chain
(
 sint15 sockfd,                                      /* socket descriptor */
 dsm_item_type **item_ptr,          /* DSM item chain containing the data */
 sint15 *dss_errno                               /* error condition value */
 )
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   ds::IDSMUtils *pIDSMUtils = NULL;
   sint15 sRet;
   AEEResult res;
   int32 nBytesWritten;

   LOG_MSG_FUNCTION_ENTRY("dss_write_dsm_chain(): socket:%d", sockfd, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL == pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_write_dsm_chain: pIDSSocket is NULL",0, 0, 0);
      ASSERT(0);
   }

   res = pIDSSocket->QueryInterface(ds::AEEIID_IDSMUtils, (void**)&pIDSMUtils);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = pIDSMUtils->WriteDSMChain(item_ptr, &nBytesWritten);  // TODO: implemented as part of ISocket (also in adapter)
   DSSCommon::ReleaseIf((IQI**)&pIDSMUtils);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("dss_write_dsm_chain failed", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   sRet = (sint15)nBytesWritten;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;

} /* dss_write_dsm_chain() */

/*===========================================================================
FUNCTION DSS_READ_DSM_CHAIN()

DESCRIPTION
Reads a DSM item chain from the TCP transport.

DEPENDENCIES
None.

RETURN VALUE
n - the number of bytes read.  A return of 0 indicates that an End-of-File
condition has occurred.

On error, return DSS_ERROR and places the error condition value in
*dss_errno.

dss_errno Values
----------------
DS_EBADF            invalid socket descriptor is specfied
DS_ENOTCONN         socket not connected
DS_ECONNRESET       TCP connection reset by server
DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
DS_EPIPE            broken pipe
DS_EADDRREQ         destination address required
DS_ENETDOWN         network subsystem unavailable
DS_EFAULT           bad memory address
DS_EWOULDBLOCK      operation would block
DS_EINVAL           can't read from a listen socket

SIDE EFFECTS
None.
===========================================================================*/
sint15 dss_read_dsm_chain
(
 sint15 sockfd,                                      /* socket descriptor */
 dsm_item_type  **item_ptr,          /* ptr to item chain containing data */
 sint15 *dss_errno                               /* error condition value */
 )
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   ds::IDSMUtils *pIDSMUtils = NULL;
   sint15 sRet;
   AEEResult res;
   int32 nBytesRead;

   LOG_MSG_FUNCTION_ENTRY("dss_read_dsm_chain(): socket:%d", sockfd, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL == pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_read_dsm_chain: pIDSSocket is NULL",0, 0, 0);
      ASSERT(0);
   }

   res = pIDSSocket->QueryInterface(ds::AEEIID_IDSMUtils, (void**)&pIDSMUtils);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = pIDSMUtils->ReadDSMChain(item_ptr, &nBytesRead);   // TODO: implemented as part of ISocket (also in adapter)
   DSSCommon::ReleaseIf((IQI**)&pIDSMUtils);
   if (AEE_SUCCESS != res && QDS_EEOF != res) {
      LOG_MSG_ERROR("dss_read_dsm_chain failed", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   *dss_errno = DS_EEOF;
   sRet = (sint15)nBytesRead;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
} /* dss_read_dsm_chain() */

/*===========================================================================
FUNCTION DSS_SENDTO_DSM_CHAIN()

DESCRIPTION
  Sends the specified DSM item chain over the TCP transport.  Lower layers
  will be responsible for deallocating the DSM item chain.

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes written.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_ENOTCONN         socket not connected
  DS_ECONNRESET       TCP connection reset by server
  DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
  DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
  DS_EPIPE            broken pipe
  DS_EADDRREQ         destination address required
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           bad memory address
  DS_EWOULDBLOCK      operation would block
  DS_EOPNOTSUPP       invalid server address specified

SIDE EFFECTS
  The pointer to the DSM item chain is set to NULL.
===========================================================================*/
sint15 dss_sendto_dsm_chain
(
  sint15 sockfd,                                      /* socket descriptor */
  dsm_item_type **item_ptr_ptr,      /* DSM item chain containing the data */
  uint32 flags,                               /* used for SDB (if enabled) */
  struct ps_sockaddr *toaddr,                       /* destination address */
  uint16 addrlen,                                        /* address length */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   ds::IDSMUtils *pIDSMUtils = NULL;
   ds::SockAddrStorageType remoteSockAddr;
   sint15 sRet;
   AEEResult res;
   int32 nBytesSent;
   ds::AddrFamilyType tempFamily;

   LOG_MSG_FUNCTION_ENTRY("dss_sendto_dsm_chain(): sock:%d, flags:%u", sockfd, flags, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("NULL args", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   if (NULL == toaddr ||
       NULL == item_ptr_ptr)
   {
      LOG_MSG_ERROR ("Inv. args", 0, 0, 0);
      sRet = DSS_ERROR;
      *dss_errno = DS_EFAULT;
      goto bail;
   }

   if (DSS_AF_INET != toaddr->ps_sa_family && DSS_AF_INET6 != toaddr->ps_sa_family){
      LOG_MSG_ERROR("Illegal Address Family", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   if (DSS_AF_INET == toaddr->ps_sa_family){
      if (sizeof(struct ps_sockaddr_in) != addrlen) {
         LOG_MSG_ERROR("Illegal IPv4 address length", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   if (DSS_AF_INET6 == toaddr->ps_sa_family){
      if (sizeof(struct ps_sockaddr_in6) != addrlen) {
         LOG_MSG_ERROR("Illegal IPv6 address length", 0, 0, 0);
         *dss_errno = DS_EFAULT;
         sRet = DSS_ERROR;
         goto bail;
      }
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL == pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_sendto_dsm_chain: pIDSSocket is NULL",0, 0, 0);
      ASSERT(0);
   }

   res = pIDSSocket->QueryInterface (ds::AEEIID_IDSMUtils, (void**)&pIDSMUtils);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   memset (remoteSockAddr, 0, sizeof(remoteSockAddr));
   addrlen = min(addrlen,sizeof(ds::SockAddrStorageType));
   memcpy (remoteSockAddr, toaddr, addrlen);

   res = DSSConversion::DS2IDSAddrFamily(toaddr->ps_sa_family, &tempFamily);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid Address Family %", toaddr->ps_sa_family, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::SetFamily(remoteSockAddr, ps_htons(tempFamily));

   res = pIDSMUtils->SendToDSMChain(item_ptr_ptr, remoteSockAddr, flags, &nBytesSent);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("dss_sendto_dsm_chain failed", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   sRet = (sint15)nBytesSent;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSMUtils);
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;
} /* dss_sendto_dsm_chain() */

/*===========================================================================
FUNCTION DSS_RECVFROM_DSM_CHAIN()

DESCRIPTION
  Reads a DSM item chain from the TCP transport.

DEPENDENCIES
  None.

RETURN VALUE
  n - the number of bytes read.  A return of 0 indicates that an End-of-File
      condition has occurred.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADF            invalid socket descriptor is specfied
  DS_ENOTCONN         socket not connected
  DS_ECONNRESET       TCP connection reset by server
  DS_ECONNABORTED     TCP connection aborted due to timeout or other failure
  DS_EIPADDRCHANGED   IP address changed, causing TCP connection reset
  DS_EPIPE            broken pipe
  DS_EADDRREQ         destination address required
  DS_ENETDOWN         network subsystem unavailable
  DS_EFAULT           bad memory address
  DS_EWOULDBLOCK      operation would block
  DS_EINVAL           can't read from a listen socket

SIDE EFFECTS
  None.
===========================================================================*/
sint15 dss_recvfrom_dsm_chain
(
  sint15 sockfd,                                      /* socket descriptor */
  dsm_item_type  **item_ptr_ptr,      /* ptr to item chain containing data */
  uint32 flags,                                                  /* unused */
  struct ps_sockaddr *fromaddr,                     /* destination address */
  uint16 *addrlen,                                       /* address length */
  sint15 *dss_errno                               /* error condition value */
)
{

   DSSSocket *pDSSSocket = NULL;
   ds::Sock::ISocket *pIDSSocket = NULL;
   ds::IDSMUtils *pIDSMUtils = NULL;
   ds::SockAddrStorageType remoteSockAddr;
   ds::AddrFamilyType family = ds::AddrFamily::QDS_AF_UNSPEC;
   int32 nBytesRecvd;
   sint15 sRet;
   AEEResult res;
   int tmpDSFamily;

   LOG_MSG_FUNCTION_ENTRY("dss_recvfrom_dsm_chain(): sock: %d, flags: %u", sockfd, flags, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("NULL args", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   if (((NULL != fromaddr) && (NULL == addrlen)) ||
       (NULL == item_ptr_ptr)) {
      LOG_MSG_ERROR ("Inv. args", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetSocketById(sockfd, &pDSSSocket);
   if (NULL == pDSSSocket || AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid socket descriptor", 0, 0, 0);
      *dss_errno = DS_EBADF;
      sRet = DSS_ERROR;
      goto bail;
   }

   pDSSSocket->GetIDSSocket(&pIDSSocket);
   if (NULL == pIDSSocket) {
      LOG_MSG_ERROR ("dss_recvfrom_dsm_chain: pIDSSocket is NULL",0, 0, 0);
      ASSERT(0);
   }

   res = pIDSSocket->QueryInterface (ds::AEEIID_IDSMUtils, (void**)&pIDSMUtils);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   memset (remoteSockAddr, 0, sizeof(remoteSockAddr));

   res = pIDSMUtils->RecvFromDSMChain(item_ptr_ptr, remoteSockAddr, flags, &nBytesRecvd);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      if (*dss_errno != DS_EWOULDBLOCK) {
         LOG_MSG_ERROR("dss_recvfrom_dsm_chain failed %d", *dss_errno, 0, 0);
      }
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::GetFamily(remoteSockAddr,&family);
   family = ps_ntohs(family);

   res = DSSConversion::IDS2DSAddrFamily(family, &tmpDSFamily);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid Address Family %", family, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      sRet = DSS_ERROR;
      goto bail;
   }

   (void)ds::Sock::AddrUtils::SetFamily(remoteSockAddr, tmpDSFamily);

   if (NULL != addrlen) {
      switch (family)
      {
         case ds::AddrFamily::QDS_AF_INET:
            if (sizeof(struct ps_sockaddr_in) < *addrlen)
            {
               *addrlen = sizeof(struct ps_sockaddr_in);
            }
            break;
         case ds::AddrFamily::QDS_AF_INET6:
            if (sizeof(struct ps_sockaddr_in6) < *addrlen)
            {
               *addrlen = sizeof(struct ps_sockaddr_in6);
            }
            break;

         default:
            LOG_MSG_ERROR("dss_recvfrom: The address family must be INET or INET6 %i", family, 0, 0);
            ASSERT(0);
      }
   }

   if (NULL != addrlen){
      size_t copylen = min(sizeof(ds::SockAddrStorageType), *addrlen);

      if (NULL != fromaddr){
         memcpy(fromaddr, remoteSockAddr, copylen);
      }
      else{
         *addrlen = 0;
      }
   }

   sRet = (sint15)nBytesRecvd;

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSMUtils);
   DSSCommon::ReleaseIf((IQI**)&pIDSSocket);
   return sRet;

} /* dss_recvfrom_dsm_chain() */
#endif //FEATURE_DSS_LINUX
/*===========================================================================
FUNCTION DSS_CLOSE_NETLIB()

DESCRIPTION

  Closes the network library for the application.  All sockets must have
  been closed for the application, prior to closing.  If this is the last
  remaining application, the network subsystem (PPP/traffic channel) must
  have been brought down, prior to closing the network library.  If this
  is the last active application using the network library, this function
  takes the data services manager out of "socket" mode.

  This function is called from the context of the socket client's task.

DEPENDENCIES
  None.

RETURN VALUE
  On success, returns DSS_SUCCESS.

  On error, return DSS_ERROR and places the error condition value in
  *dss_errno.

  dss_errno Values
  ----------------
  DS_EBADAPP        invalid application ID
  DS_SOCKEXIST     there are existing sockets
  DS_ENETEXIST      the network subsystem exists

SIDE EFFECTS
  Puts data services manager into "autodetect" mode.
===========================================================================*/
sint15 dss_close_netlib
(
  sint15 dss_nethandle,                                         /* application ID */
  sint15 *dss_errno                               /* error condition value */
)
{
   DSSNetApp *pNetApp = NULL;
   int nNumOfSocks;
   sint15 sRet;
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY("dss_close_netlib(): net_handle:%d", dss_nethandle, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR("dss_errno is NULL", 0, 0, 0);
      sRet = DSS_ERROR;
      goto bail;
   }

   res = DSSGlobals::Instance()->GetNetApp(dss_nethandle, &pNetApp);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Invalid application ID", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);;
      sRet = DSS_ERROR;
      goto bail;
   }

   pNetApp->GetNumOfSockets(&nNumOfSocks);
   if (0 != nNumOfSocks) {
      LOG_MSG_ERROR("There are existing sockets", 0, 0, 0);
      *dss_errno = DS_SOCKEXIST;
      sRet = DSS_ERROR;
      goto bail;
   }

   sRet = dss_pppclose(dss_nethandle, dss_errno);
   if (DSS_SUCCESS != sRet) {
      LOG_MSG_ERROR("dss_pppclose failed: %d", sRet, 0, 0);
   }

   if (AEE_SUCCESS != DSSGlobals::Instance()->RemoveNetApp(dss_nethandle)) {
      LOG_MSG_ERROR("Internal error: Failed to remove NetApp from apps list", 0, 0, 0);
   }
   (void) pNetApp->Release();  // IDSNetworkPriv object of this DSSNetApp is deallocated here

   sRet = DSS_SUCCESS;

bail:
   return sRet;
}

/*===========================================================================
FUNCTION DSS_GET_IFACE_ID_BY_ADDR()

DESCRIPTION
  This function return the interface id matching an IP address

DEPENDENCIES
  None.

PARAMETERS

RETURN VALUE
  iface_id: If a valid iface could be obtained based on address
  On error, return DSS_IFACE_INVALID_ID and places the error condition value
  in *dss_errno.

  dss_errno Values
  ----------------
  DS_EFAULT      Invalid arguments passed to the function.
  DS_ENOROUTE    No interface can be determined for the address.
  DS_EOPNOTSUPP  This operation is not supported.

SIDE EFFECTS
  None.
===========================================================================*/
dss_iface_id_type dss_get_iface_id_by_addr
(
  ip_addr_type * ip_addr_ptr,                     /* IP address info       */
  sint15       * dss_errno                        /* error condition value */
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  LOG_MSG_INFO2("dss_get_iface_id_by_addr() called ", 0, 0, 0);
  if( dss_errno == NULL )
  {
    LOG_MSG_ERROR("Invalid params", 0, 0, 0);
    return(DSS_IFACE_INVALID_ID);
  }

  *dss_errno = DS_EOPNOTSUPP;
  return (DSS_IFACE_INVALID_ID);

} /* dss_get_iface_id_by_addr() */

// Even though this socket option is deprecated, operation should succeed and arbitrary value shall be returned
// to support the legacy apps. Some legacy apps expect success on this operation even though the value is meaningless for them
int32 dss_get_qos_share_handle
(
   sint15  net_handle,
   sint15  *dss_errno
)
{
   LOG_MSG_ERROR("dss_get_qos_share_handle is deprecated",0,0,0);
   return 17;
} /* dss_get_qos_share_handle() */

void* dss_enable_nat
(
   dss_net_policy_info_type  *net_policy_ptr,
   int16                     *dss_errno
)
{
   ds::Net::INatSessionPriv  *pINatSessionPriv = NULL;
   ds::Net::INetworkFactoryPriv  *pINetworkPrivFactory = NULL;
   ds::Net::IPolicyPriv *pIPolicyPriv = NULL;
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY ("Enabling NAT", 0, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR ("NULL errno", 0, 0, 0);
      return NULL;
   }

   // Get the NetworkFactory from DSSGlobals
   DSSGlobals::Instance()->GetNetworkFactoryPriv(&pINetworkPrivFactory);
   DSSGenScope scopeNetworkFactory(pINetworkPrivFactory, DSSGenScope::IDSIQI_TYPE);

   // Create Policy object and set its iface_id
   res = pINetworkPrivFactory->CreatePolicyPriv (&pIPolicyPriv);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return NULL;
   }

   DSSGenScope scopePolicy(pIPolicyPriv, DSSGenScope::IDSIQI_TYPE);

   // Convert the user given policy to IDS policy.
   res = DSSConversion::DS2IDSNetPolicy((const dss_net_policy_info_type *)net_policy_ptr, static_cast <ds::Net::IPolicyPriv *>(pIPolicyPriv));
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return NULL;
   }

   // Create NAT session based on policy. DONT create a scope object, we are returning NatSession interface as a handle to user.
   res = pINetworkPrivFactory->CreateNatSessionPriv (pIPolicyPriv, &pINatSessionPriv);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return NULL;
   }

   // Enable the NAT session
   res = pINatSessionPriv->Enable();
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return NULL;
   }

   // Need to save the Nat Manager in the DSSGlobals in order to know when to release it.
   res = DSSGlobals::Instance()->AddNatManager(pINatSessionPriv);
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return NULL;
   }

   LOG_MSG_INFO1 ("Nat Manager 0x%x successfully added to DSSGlobals", pINatSessionPriv, 0, 0);
   LOG_MSG_FUNCTION_EXIT ("NAT enabled, handle 0x%x", pINatSessionPriv, 0, 0);

   // Return the created NAT session as a handle to user.
   return (void*)pINatSessionPriv;

}

/* TODO: Handle should not be a pointer */
int32 dss_disable_nat
(
   void    *handle,
   int16   *dss_errno
)
{
   ds::Net::INatSessionPriv  *pINatSessionPriv = NULL;

   LOG_MSG_FUNCTION_ENTRY ("Disable NAT, handle 0x%x", handle, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR ("NULL errno", 0, 0, 0);
      return DSS_ERROR;
   }

   if (NULL == handle) {
      LOG_MSG_ERROR ("Invalid handle", 0, 0, 0);
      *dss_errno = DS_EFAULT;
      return DSS_ERROR;
   }

   // Handle passed in is actually pointing to NatSessionPriv object.
   pINatSessionPriv =  reinterpret_cast <ds::Net::INatSessionPriv *> (handle);

   // Disable the NAT session. Also release the Nat Session object.
   IDS_ERR_RET_ERRNO (pINatSessionPriv->Disable());

   // Need to save the Nat Manager in the DSSGlobals in order to know when to release it.
   IDS_ERR_RET_ERRNO(DSSGlobals::Instance()->DeleteNatManager(pINatSessionPriv));
   DSSCommon::ReleaseIf ((IQI **) &pINatSessionPriv);

   LOG_MSG_INFO1 ("Nat Manager 0x%x successfully deleted from DSSGlobals", handle, 0, 0);
   LOG_MSG_FUNCTION_EXIT ("NAT disabled, handle 0x%x", handle, 0, 0);
   return AEE_SUCCESS;

}

int32 dss_softap_enable_roaming_autoconnect
(
   int16   *dss_errno
)
{
   ds::Net::INatSessionPriv  *pINatSessionPriv = NULL;
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY ("Enabling Roaming Autoconnect", 0, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR ("NULL errno", 0, 0, 0);
      return DSS_ERROR;
   }

   res = DSSGlobals::Instance()->GetNatManager(&pINatSessionPriv);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR ("Nat not enabled, cannot start roaming autoconnect", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   LOG_MSG_INFO1 ("Nat Manager 0x%x successfully retrived from DSSGlobals", pINatSessionPriv, 0, 0);

   res = pINatSessionPriv->EnableRoamingAutoconnect();
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   LOG_MSG_FUNCTION_EXIT ("NAT roaming autoconnect enabled", 0, 0, 0);
   return AEE_SUCCESS;
}

int32 dss_softap_disable_roaming_autoconnect
(
   int16   *dss_errno
)
{
   ds::Net::INatSessionPriv  *pINatSessionPriv = NULL;
   AEEResult res;

   LOG_MSG_FUNCTION_ENTRY ("Disabling Roaming Autoconnect", 0, 0, 0);

   if (NULL == dss_errno) {
      LOG_MSG_ERROR ("NULL errno", 0, 0, 0);
      return DSS_ERROR;
   }

   res = DSSGlobals::Instance()->GetNatManager(&pINatSessionPriv);
   if (AEE_SUCCESS != res) {
       LOG_MSG_ERROR ("Nat not enabled, cannot start roaming autoconnect", 0, 0, 0);
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   LOG_MSG_INFO1 ("Nat Manager 0x%x successfully retrived from DSSGlobals", pINatSessionPriv, 0, 0);

   res = pINatSessionPriv->DisableRoamingAutoconnect();
   if (AEE_SUCCESS != res) {
      *dss_errno = DSSConversion::IDS2DSErrorCode(res);
      return DSS_ERROR;
   }

   LOG_MSG_FUNCTION_EXIT ("NAT roaming autoconnect disabled", 0, 0, 0);
   return AEE_SUCCESS;
}

} /* extern "C" */
