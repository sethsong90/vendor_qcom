/*======================================================

FILE:  DSS_Socket.cpp

SERVICES:

GENERAL DESCRIPTION:
Implementation of DSSSocket class

=====================================================

Copyright (c) 2008 - 2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_Socket.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

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
#include "dserrno.h"

#include "DSS_Socket.h"
#include "DSS_Globals.h"
#include "DSS_CritScope.h"
#include "DSS_Conversion.h"
#include "ds_Sock_ISocketLocalPriv.h"
#include "ds_Sock_ISocketExt.h"
#include "ps_mem.h"
#include "ds_Errors_Def.h"
#include "ds_Utils_Atomic.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECCritSect.h"
#include "ds_Utils_DebugMsg.h"

using namespace ds::Error;

//-------------------------------------------------------------------
// Constant / Define Declarations
//-------------------------------------------------------------------

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
extern "C" void dssocki_sock_cb(sint15 dss_nethandle, sint15 sockfd, uint32 event_mask, void *user_data_ptr);

//===================================================================
//              Macro Definitions
//===================================================================

//===================================================================
//            DSSSocket Functions Definitions
//===================================================================

// TODO: documentation
DSSSocket::DSSSocket(dss_sock_cb_fcn_type* sockCb, sint15 netApp):
   mSockFd(-1), mNetApp(netApp), mpIDSSock(0), mSockCb(*sockCb), mSigOnOffMask(0),
   mEventOccurredMask(0), mpReadSignal(0), mpWriteSignal(0), mpCloseSignal(0),
   mpAcceptSignal(0), mpSdbAckSignal(0), mpReadSignalCtl(0), mpWriteSignalCtl(0),
   mpCloseSignalCtl(0), mpAcceptSignalCtl(0), mpSdbAckSignalCtl(0) ,mbIpsecDisabled(0),
   mbFlowFrwding(0), mpCritSect(0), mSockKind(0), mProtocol(0)
{
   mSdbAckCb.sdb_ack_cb = NULL;
   mSdbAckCb.user_data = NULL;
   filterReg = NULL;
}

AEEResult DSSSocket::Init()
{
   LOG_MSG_FUNCTION_ENTRY("Init()", 0, 0, 0);

   IDS_ERR_RET(DSSGlobals::Instance()->GetCritSect(&mpCritSect));
   return AEE_SUCCESS;
}

//===================================================================
//  FUNCTION:   DSSSocket::~DSSSocket
//
//  DESCRIPTION:
//  Destructor of the DSSSocket class.
//===================================================================

/*lint -e{1551} */
void DSSSocket::Destructor() throw(){
   DSSCommon::ReleaseIf((IQI**)&mpIDSSock);

   DSSCommon::ReleaseIf((IQI**)&mpReadSignal);
   DSSCommon::ReleaseIf((IQI**)&mpWriteSignal);
   DSSCommon::ReleaseIf((IQI**)&mpCloseSignal);
   DSSCommon::ReleaseIf((IQI**)&mpAcceptSignal);
   DSSCommon::ReleaseIf((IQI**)&mpSdbAckSignal);

   DSSCommon::ReleaseIf((IQI**)&mpReadSignalCtl);
   DSSCommon::ReleaseIf((IQI**)&mpWriteSignalCtl);
   DSSCommon::ReleaseIf((IQI**)&mpCloseSignalCtl);
   DSSCommon::ReleaseIf((IQI**)&mpAcceptSignalCtl);
   DSSCommon::ReleaseIf((IQI**)&mpSdbAckSignalCtl);
   DSSCommon::ReleaseIf((IQI**)&mpCritSect);

   SignalHandlerBase::Destructor();
}
/*lint –restore */

// TODO: implement the signals. The signal for each event should: turn on the
// relevant bit in mEventOccurredMask (this is relevant only for old applications
// but if they use SignalBus we can do it anyway because this mask is used only by
// old apps), and if the relevant bit in mSigOnOffMask is on - call the callback

// TODO: documentation
void DSSSocket::GeneralSignalFcn(uint32 eventId)
{
   DSSCritScope cs(*mpCritSect);


   LOG_MSG_FUNCTION_ENTRY("GeneralSignalFcn(): event %d", eventId, 0, 0);

   uint32 tmpSigOnOffMask = mSigOnOffMask;

   // mSockCb should be always concurrent with pNetApp.mSockCb if socket was created with netapp - dss_socket
   // mSockCb can't be dssocki_sock_cb if socket wasn't created with netapp - dss_socket2
   // so we don't really need to get the sockCb stored inside the app, we can use the mSockCb directly

   // using old app model, update the mEventOccurredMask
   if (dssocki_sock_cb == mSockCb.sock_cb_fcn)
   {
     mEventOccurredMask |= eventId;
   }
   // using new app model, registration is turned off automatically upon event cb
   else
   {
     mSigOnOffMask &= ~ eventId;
   }

   if (0 != (tmpSigOnOffMask & eventId)) {
      if (NULL != mSockCb.sock_cb_fcn) {
         LOG_MSG_INFO1("GeneralSignalFcn(): Calling user cb, event: %d" , eventId, 0, 0);
         mSockCb.sock_cb_fcn(mNetApp, mSockFd, eventId, mSockCb.sock_cb_user_data);
      }
   }
}

ISignal** DSSSocket::GetSignal(ds::Sock::SocketEventType eventId)
{
   switch (eventId) {
      case ds::Sock::SocketEvent::QDS_EV_WRITE:
         return &mpWriteSignal;
      case ds::Sock::SocketEvent::QDS_EV_READ:
         return &mpReadSignal;
      case ds::Sock::SocketEvent::QDS_EV_CLOSE:
         return &mpCloseSignal;
      case ds::Sock::SocketEvent::QDS_EV_ACCEPT:
         return &mpAcceptSignal;
      case ds::Sock::SocketEvent::QDS_EV_DOS_ACK:
         return &mpSdbAckSignal;
      default:
        LOG_MSG_ERROR("DSSSocket::GetSignal wrong eventID %d",eventId,0,0);
        ASSERT(0);
   }

   return 0;
}

ISignalCtl** DSSSocket::GetSignalCtl(ds::Sock::SocketEventType eventId)
{
   switch (eventId) {
      case ds::Sock::SocketEvent::QDS_EV_WRITE:
         return &mpWriteSignalCtl;
      case ds::Sock::SocketEvent::QDS_EV_READ:
         return &mpReadSignalCtl;
      case ds::Sock::SocketEvent::QDS_EV_CLOSE:
         return &mpCloseSignalCtl;
      case ds::Sock::SocketEvent::QDS_EV_ACCEPT:
         return &mpAcceptSignalCtl;
      case ds::Sock::SocketEvent::QDS_EV_DOS_ACK:
         return &mpSdbAckSignalCtl;
      default:
         LOG_MSG_ERROR("DSSSocket::GetSignalCtl wrong eventID %d",eventId,0,0);
         ASSERT(0);
   }
   
   return 0;
}

uint32 DSSSocket::GetEventHandlerFunc(ds::Sock::SocketEventType eventId)
{
   switch (eventId) {
      case ds::Sock::SocketEvent::QDS_EV_WRITE:
         return reinterpret_cast <uint32> (WriteSignalFcn);
      case ds::Sock::SocketEvent::QDS_EV_READ:
         return reinterpret_cast <uint32> (ReadSignalFcn);
      case ds::Sock::SocketEvent::QDS_EV_CLOSE:
         return reinterpret_cast <uint32> (CloseSignalFcn);
      case ds::Sock::SocketEvent::QDS_EV_ACCEPT:
         return reinterpret_cast <uint32> (AcceptSignalFcn);
      case ds::Sock::SocketEvent::QDS_EV_DOS_ACK:
         return reinterpret_cast <uint32> (StaticSdbAckSignalFcn);
      default:
        LOG_MSG_ERROR("DSSSocket::GetEventHandlerFunc wrong eventID %d",eventId,0,0);
        ASSERT(0);
   }

   return 0;
}

AEEResult DSSSocket::CreateSignal(ISignalCtl** ppiSignalCtl,
                                  ISignal**    ppiSignal,
                                  uint32       eventHandlerFunc)
{
   ISignalFactory *pSignalFactory = 0;
   IDS_ERR_RET(DSSGlobals::Instance()->GetSignalFactory(&pSignalFactory));
   if ((NULL == pSignalFactory) || (NULL == ppiSignalCtl) ||
       (NULL == ppiSignal) || (0 == eventHandlerFunc))
   {
      LOG_MSG_ERROR("DSSSocket::CreateSignal either wrong arguments or failed to fetch Signal Factory",0,0,0);
      ASSERT(0);
      return QDS_EINVAL;
   }

   LOG_MSG_FUNCTION_ENTRY("CreateSignal(): signal 0x%p signalCtl 0x%p", *ppiSignal, *ppiSignalCtl, 0);

   IDS_ERR_RET(pSignalFactory->CreateSignal(&signalHandler,
                                            eventHandlerFunc,
                                            reinterpret_cast <uint32> (this),
                                            ppiSignal,
                                            ppiSignalCtl));

   return AEE_SUCCESS;
}

AEEResult DSSSocket::RegEvent(sint31 eventMask)
{
   ds::Sock::SocketEventType IDSEventId;
   ds::Sock::ISocketExt *pIDSExtSokect = NULL;
   AEEResult res = AEE_SUCCESS;
   DSSCritScope cs(*mpCritSect);
   IQI*   pRegObj = NULL;

   LOG_MSG_FUNCTION_ENTRY("RegEvent(): eventMask: %d" ,eventMask, 0, 0);
   
   res = mpIDSSock->QueryInterface(ds::Sock::AEEIID_ISocketExt, (void**)&(pIDSExtSokect));
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("QueryInterface(ds::Sock::AEEIID_ISocketExt) failed: %d", res, 0, 0);
      goto bail;
   }

   for (uint32 eventId = DS_WRITE_EVENT; eventId <= DS_ACCEPT_EVENT; eventId <<= 1) {
      if (0 != (eventId & eventMask)) {
         // the logic should be performed even if we already registered to maintain backward compatibility, 
         // see implementations of dss_async_select for more details
         mSigOnOffMask |= eventId;
         IDS_ERR_RET(DSSConversion::DS2IDSEventId(eventId, &IDSEventId));

         /* Double pointer is used here and in the Signal pointer to set
            the corresponding member variable of DSSSocket when the signal
            is created. */
         ISignalCtl** ppiSignalCtl = GetSignalCtl(IDSEventId);
         if (NULL == ppiSignalCtl) {
            LOG_MSG_ERROR("DSSSocket::RegEvent could not get Signal Control",0,0,0);
            ASSERT(0);
         }
            
         /* If the signalctl is null - this is the first registration.
            Create the signal and register it with the socket.
            Otherwise just enable the already registered signal. */
         if (NULL == *ppiSignalCtl) {
            ISignal** ppiSignal = GetSignal(IDSEventId);
            uint32 eventHandlerFunc = GetEventHandlerFunc(eventId);
            res = CreateSignal(ppiSignalCtl, ppiSignal, eventHandlerFunc);
            if (AEE_SUCCESS != res) {
               goto bail;
            }

            LOG_MSG_INFO1("Registering to event %d, SocketExt obj 0x%p", IDSEventId, pIDSExtSokect, 0);
            res = pIDSExtSokect->RegEvent(*ppiSignal, IDSEventId, &pRegObj);
            // regObj mechanism is currently not in effect by dssock layer. 
            // No need for DSS to hold the allocated object to maintain the event registration. 
            // For design simplicity we release the regObj immediately.
            // If and when dssock layer enforces the registration object concept this code need to be adapted accordingly.
            DSSCommon::ReleaseIf(&pRegObj);
            if (AEE_SUCCESS != res) {
               LOG_MSG_ERROR("RegEvent operation failed: %d", res, 0, 0);
               mSigOnOffMask &= ~eventId;
               goto bail;
            }
         }
         else {
            (void)(*ppiSignalCtl)->Enable();
         }
      }//(0 != (eventId & eventMask))
   }//for

bail:
   DSSCommon::ReleaseIf((IQI**)&pIDSExtSokect);
   return res;
}

AEEResult DSSSocket::DeRegEvent(sint31 eventMask)
{
   LOG_MSG_FUNCTION_ENTRY("DeRegEvent(): eventMask: %d" ,eventMask, 0, 0);
   DSSCritScope cs(*mpCritSect);

   for (uint32 eventId = DS_WRITE_EVENT; eventId <= DS_ACCEPT_EVENT; eventId <<= 1) {
      if (0 != (eventId & eventMask)) {
         // The event is in the mask
         mSigOnOffMask &= ~eventId;
      }
   }

   return AEE_SUCCESS;
}


AEEResult DSSSocket::SetSdbAckCb(dss_so_sdb_ack_cb_type *pSdbAckCb)
{
   DSSCritScope cs(*mpCritSect);
   ds::Sock::ISocketExt *pIDSExtSocket = NULL;
   AEEResult res = AEE_SUCCESS;
   ISignalCtl** ppiSignalCtl = NULL;
   ISignal** ppiSignal = NULL;
   uint32 eventHandlerFunc = 0;
   IQI*   pRegObj = NULL;
   
   res = mpIDSSock->QueryInterface(ds::Sock::AEEIID_ISocketExt, (void**)&(pIDSExtSocket));
   if (AEE_SUCCESS != res) {

      LOG_MSG_ERROR("QueryInterface(ds::Sock::AEEIID_ISocketExt) failed: %d", res, 0, 0);
     
      goto bail;
   }
   /* Double pointer is used here and in the Signal pointer to set
   the corresponding member variable of DSSSocket when the signal
   is created. */
   ppiSignalCtl = GetSignalCtl(ds::Sock::SocketEvent::QDS_EV_DOS_ACK);
   if (NULL == ppiSignalCtl) {

      LOG_MSG_ERROR("DSSSocket::SetSdbAckCb could not get Signal Control",0,0,0);

      ASSERT(0);
   }

   /* If the SignalCtl is null - this is the first registration.
   Create the signal and register it with the socket.
   Otherwise just enable the already registered signal. */
   if (NULL == *ppiSignalCtl) {
      ppiSignal = GetSignal(ds::Sock::SocketEvent::QDS_EV_DOS_ACK);

      eventHandlerFunc = GetEventHandlerFunc(ds::Sock::SocketEvent::QDS_EV_DOS_ACK);
      
      res = CreateSignal(ppiSignalCtl, ppiSignal, eventHandlerFunc);

   if (AEE_SUCCESS != res) {
         
         LOG_MSG_ERROR("DSSSocket::SetSdbAckCb could not create a Signal/Signal Control",0,0,0);
         
         goto bail;
      }

      LOG_MSG_INFO1("Registering to QDS_EV_DOS_ACK, SocketExt obj 0x%p", pIDSExtSocket, 0, 0);

      res = pIDSExtSocket->RegEvent(mpSdbAckSignal, ds::Sock::SocketEvent::QDS_EV_DOS_ACK, &pRegObj);
      // regObj mechanism is currently not in effect by dssock layer. 
      // No need for DSS to hold the allocated object to maintain the event registration. 
      // For design simplicity we release the regObj immediately.
      // If and when dssock layer enforces the registration object concept this code need to be adapted accordingly.
      DSSCommon::ReleaseIf(&pRegObj);
      if (AEE_SUCCESS != res) {
      goto bail;
   }

      mSdbAckCb = *pSdbAckCb;

   } 
   else {
      (void)(*ppiSignalCtl)->Enable();
   }

/* fall through */

bail:

   DSSCommon::ReleaseIf((IQI**)&pIDSExtSocket);

   return res;
}

// TODO: documentation
void DSSSocket::ReadSignalFcn(void *pUserData)
{
   LOG_MSG_FUNCTION_ENTRY("ReadSignalFcn()", 0, 0, 0);
   reinterpret_cast<DSSSocket*>(pUserData)->GeneralSignalFcn(DS_READ_EVENT);
}

// TODO: documentation
void DSSSocket::WriteSignalFcn(void *pUserData)
{
   LOG_MSG_FUNCTION_ENTRY("WriteSignalFcn()", 0, 0, 0);
   reinterpret_cast<DSSSocket*>(pUserData)->GeneralSignalFcn(DS_WRITE_EVENT);
}

// TODO: documentation
void DSSSocket::CloseSignalFcn(void *pUserData)
{
   // We don't need to delete the DSSSocket, since we assume that the application will call
   // again to dss_close if this event was received
   LOG_MSG_FUNCTION_ENTRY("CloseSignalFcn()", 0, 0, 0);
   reinterpret_cast<DSSSocket*>(pUserData)->GeneralSignalFcn(DS_CLOSE_EVENT);
}

// TODO: documentation
void DSSSocket::AcceptSignalFcn(void *pUserData)
{
   LOG_MSG_FUNCTION_ENTRY("AcceptSignalFcn()", 0, 0, 0);
   reinterpret_cast<DSSSocket*>(pUserData)->GeneralSignalFcn(DS_ACCEPT_EVENT);
}

// TODO: documentation
void DSSSocket::StaticSdbAckSignalFcn(void *pUserData)
{
   LOG_MSG_FUNCTION_ENTRY("StaticSdbAckSignalFcn()", 0, 0, 0);
   reinterpret_cast<DSSSocket*>(pUserData)->SdbAckSignalFcn();
}

// TODO: documentation
void DSSSocket::SdbAckSignalFcn()
{
   ds::Sock::DoSAckStatusType newAPIStatus;
   dss_sdb_ack_status_enum_type oldAPIStatus;
   dss_sdb_ack_status_info_type statusInfo;
   int overflow;
   AEEResult res = AEE_SUCCESS;
   ds::Sock::ISocketExt *pIDSExtSokect = NULL;

   res = mpIDSSock->QueryInterface(ds::Sock::AEEIID_ISocketExt, (void**)&pIDSExtSokect);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("QueryInterface(AEEIID_ISocketExt) failed: %d", res, 0, 0);
      goto bail;
   }

   res = pIDSExtSokect->GetDoSAckInfo(&newAPIStatus, &overflow);
   if (AEE_SUCCESS != res){
      LOG_MSG_ERROR("GetDoSAckInfo Failed", 0, 0, 0);
      goto bail;
   }

   res = DSSConversion::IDS2DSSDBAckStatus(newAPIStatus, &oldAPIStatus);
   if (AEE_SUCCESS != res){
      LOG_MSG_ERROR("Conversion of AckStatus Failed", 0, 0, 0);
      goto bail;
   }

   if(mSdbAckCb.sdb_ack_cb == NULL) 
   {
      LOG_MSG_INFO1("NULL mSdbAckCb", 0, 0, 0);
   }
   else
   {
   statusInfo.overflow = overflow;
   statusInfo.status   = oldAPIStatus;
   mSdbAckCb.sdb_ack_cb(mSockFd, &statusInfo, mSdbAckCb.user_data);
   }

   (void)mpSdbAckSignalCtl->Enable();

bail:

   DSSCommon::ReleaseIf((IQI**)&pIDSExtSokect);
}

void * DSSSocket::operator new
(
  unsigned int numBytes
) throw()
{
  return ps_mem_get_buf(PS_MEM_DSAL_SOCKET_TYPE);
} /* DSSSocket::operator new() */

DSSSocket * DSSSocket::CreateInstance(dss_sock_cb_fcn_type* sockCb, sint15 netApp)
{
   return new DSSSocket(sockCb, netApp);
}

