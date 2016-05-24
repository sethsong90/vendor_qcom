#ifndef __DSS_GLOBALS_H__
#define __DSS_GLOBALS_H__

/*======================================================

FILE:  DSS_Globals.h

SERVICES:
Global definitions of Backward Compatibility Layer set up in
a Singleton class

GENERAL DESCRIPTION:

=====================================================

Copyright (c) 2008-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_Globals.h#1 $
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
#include "comdef.h"
#include "dssocket.h"
#include "dserrno.h"

#include "AEEISignalFactory.h"

#include "customer.h"

#include "AEEStdErr.h"

#include "ds_Net_INetworkFactory.h"
#include "ds_Net_INetworkFactoryPriv.h"
#include "ds_Sock_ISocketFactory.h"
#include "ds_Net_INatSessionPriv.h"

#include "DSS_NetApp.h"
#include "DSS_PrimaryNetApp.h"
#include "DSS_Socket.h"
#include "AEEICritSect.h"
#include "ps_mem.h"
#include "dss_config.h"

//-------------------------------------------------------------------
// Constant / Define Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Type Declarations (typedef, struct, enum, etc.)
//-------------------------------------------------------------------
typedef struct FilterRegInfo
{
   ds::Net::IIPFilterRegPriv* filterReg;
   dss_iface_id_type iface_id;
   sint15 sockfd;
   struct FilterRegInfo* next;

   void * operator new
   (
      unsigned int numBytes
   )  throw()
   {
      return ps_mem_get_buf( PS_MEM_DSAL_FILTER_REG_INFO_TYPE);
   } 

   void operator delete
   (
      void *  bufPtr
   )
   {
      if(NULL != bufPtr)
      {
         PS_MEM_FREE(bufPtr);
         return;
      }    
   } 
} FilterRegInfo;

//-------------------------------------------------------------------
// Global Constant Data Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Global Data Declarations
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------

uint32 BuildIfaceIdWithAppId(uint8 ifaceIndex, sint15 appid);
uint32 BuildIfaceIdWithoutAppId(uint8 ifaceIndex);
dss_iface_id_type StripAppIdFromIfaceId(dss_iface_id_type iface_id);

AEEResult BuildIfaceId(ds::Net::INetwork* pIDSNetwork,sint15 appid,dss_iface_id_type* iface_id);
AEEResult BuildIfaceId(ds::Net::IPolicyPriv* pIDSNetworkPolicyPriv,sint15 appid,dss_iface_id_type* iface_id);
// This function is called from different places in code with different iface_id formats
// If one of iface_id has it policy bit on (meaning that it was created either with dss_get_iface_id_by_policy
// or with IDSNetworkPriv , then just compare the iface index , otherwise compare everything 
bool CompareIfaceIds(dss_iface_id_type iface_id1,dss_iface_id_type iface_id2);

sint15 GetNetHandleFromQoSRequestHandle(dss_iface_ioctl_type ioctl_name,void* argval_ptr);
bool IsQoSIoctl(dss_iface_ioctl_type ioctl_name);


//===================================================================
//              Macro Definitions
//===================================================================


//===================================================================
//              Class Definitions
//===================================================================

//===================================================================
//  CLASS:      DSSGlobals
//
//  DESCRIPTION:
//
//  HOW TO USE:
//
//===================================================================
enum
{
   NUM_OF_FLOW_IDS = 256
};

class DSSGlobals
{
//-------------------------------------------------------------------
//  Constructors/Desctructors
//-------------------------------------------------------------------
  private:
     DSSGlobals();
  public:
     virtual ~DSSGlobals();

//-------------------------------------------------------------------
//  Interface members
//-------------------------------------------------------------------
  public:
    static DSSGlobals* Instance();
    static void Init();
    static void Deinit();
    AEEResult  GetCritSect(ICritSect** ppIcritSect);
    AEEResult  GetNetworkFactory(ds::Net::INetworkFactory **ppNetworkFactory);
    AEEResult  GetNetworkFactoryPriv(ds::Net::INetworkFactoryPriv **ppNetworkFactoryPriv);
    // TODO: the parameter should be of type that implements IDSSockFactory
    void      GetSockFactory(ds::Sock::ISocketFactory **ppFactory);
    AEEResult GetSignalFactory(ISignalFactory **ppFactory);
    // TODO: allow ppNetApp to be NULL if we only want to check if there's such handle
    AEEResult GetNetApp(sint15 netHandle, DSSNetApp **ppNetApp);
    AEEResult GetNetApp(sint15 netHandle, 
                        dss_iface_id_type iface_id,
                        bool bAllowSecondary,
                        bool bForbidTemporary,
                        DSSNetApp** ppNetApp,
                        bool* bIsTempDSSNetApp,
                        dss_iface_ioctl_type ioctl_name,
                        void* argval_ptr);
    AEEResult GetNetApp(dss_iface_id_type iface_id,DSSNetApp** ppNetApp);
    bool      IsValidNetApp(sint15 handle);
    sint15    InsertNetApp(DSSNetApp *pNetApp);
    AEEResult RemoveNetApp(sint15 netHandle);
    AEEResult RemoveSocket(sint15 sockFd);
    AEEResult GetSocketById(sint15 sockfd, DSSSocket **ppDSSSocket, int16 *pSocketIdx = NULL);
    AEEResult FindSocketWithEvents(sint15 dss_nethandle, sint15 *sockfd_ptr, uint32 *eventOccurred, uint32 *sigOnOff);
    // Creates a DSSSocket and inserts it into the global list. kind can be 1 or 2
    AEEResult CreateSocket(dss_sock_cb_fcn_type sockCb, sint15 sNetApp,
                           sint15* psSockFd, DSSSocket** ppDSSSocket, int kind,
                           byte protocol);
    static AEEResult GetFreeQoSFlowID(uint8 *pFlowID);
    static AEEResult GetFreeMCastFlowID(uint8 *pFlowID);
    static void ReleaseQoSFlowID(uint8 flowID) throw();
    static void ReleaseMCastFlowID(uint8 flowID) throw();

    // in this function we AddRef to the filterReg that is being stored inside the DSSGlobals
    AEEResult AddFilterRegObjectToList(ds::Net::IIPFilterRegPriv* filterReg, dss_iface_id_type iface_id, sint15 sockfd);
    AEEResult RemoveFilterRegObjectFromList(dss_iface_id_type iface_id, sint15 sockfd);
    AEEResult RemoveFilterRegObjectFromList(sint15 sockfd);
    AEEResult GetNatManager(ds::Net::INatSessionPriv** natSession);
    AEEResult DeleteNatManager(ds::Net::INatSessionPriv* natSession);
    AEEResult AddNatManager(ds::Net::INatSessionPriv* natSession);

//-------------------------------------------------------------------
//  Get/Set functions for protected members access
//
//-------------------------------------------------------------------

//-------------------------------------------------------------------
//  Protected members
//-------------------------------------------------------------------
  private:
     AEEResult CreateTempDSSNetApp(DSSNetApp** ppNetApp, dss_iface_id_type iface_id);
     sint15    InsertSocket(DSSSocket *pSocket);
     AEEResult RemoveAllMatchingFilterRegObjectFromList(dss_iface_id_type iface_id, sint15 sockfd);

//-------------------------------------------------------------------
//  Data members
//-------------------------------------------------------------------
   private:      
      ds::Net::INetworkFactory* mpNetFactory;
      ds::Net::INetworkFactoryPriv* mpNetFactoryPriv;
      ds::Sock::ISocketFactory* mpSockFactory;
      ISignalFactory* mpSignalFactory;
      ICritSect* mpCritSect;
      static bool mQoSFlowIDs[NUM_OF_FLOW_IDS];
      static bool mMCastFlowIDs[NUM_OF_FLOW_IDS];

      DSSPrimaryNetApp* mapNetAppArray[DSS_MAX_APPS];    
      DSSSocket* mapSocketArray[DSS_MAX_SOCKS];

      static DSSGlobals* pmInstance;

      FilterRegInfo* mFilterRegInfoList;

      int mTcpSocketsNum;
      int mUdpSocketsNum;

      ds::Net::INatSessionPriv* mNatSession;

//-------------------------------------------------------------------
//  Memory management
//-------------------------------------------------------------------

   public :
      void * operator new
      (
         unsigned int numBytes
      )  throw();

      void operator delete
      (
         void *  bufPtr
      );
};

//===================================================================

#endif // __DSS_GLOBALS_H__
