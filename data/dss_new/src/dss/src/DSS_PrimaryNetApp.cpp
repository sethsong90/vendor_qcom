/*======================================================

FILE:  DSS_PrimaryNetApp.cpp

SERVICES:

GENERAL DESCRIPTION:
Implementation of DSSPrimaryNetApp class

=====================================================

Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_PrimaryNetApp.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/


//===================================================================
//   Includes and Public Data Declarations
//===================================================================
#include "DSS_Common.h"

#include "ds_Net_INetwork.h"
#include "ds_Net_INetworkFactoryPriv.h"
#include "ds_Net_IPolicyPriv.h"
#include "ds_Net_CNetworkFactoryPriv.h"

#include "customer.h"
#include "DSS_Conversion.h"
#include "DSS_NetApp.h"
#include "DSS_PrimaryNetApp.h"
#include "DSS_SecondaryNetApp.h"
#include "DSS_CritScope.h"
#include "DSS_IDSNetPolicyPrivScope.h"
#include "ps_mem.h"
#include "DSS_Globals.h"
#include "DSS_MemoryManagement.h"
#include "ds_Net_CreateInstance.h"

using namespace ds::Net;

//-------------------------------------------------------------------
// Include Files
//-------------------------------------------------------------------

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

//===================================================================
//              Macro Definitions
//===================================================================



//===================================================================
//            DSSPrimaryNetApp Functions Definitions
//===================================================================


//===================================================================
//  FUNCTION:   DSSPrimaryNetApp::DSSPrimaryNetApp
//
//  DESCRIPTION:
//  Constructor of the DSSPrimaryNetApp class.
//===================================================================

DSSPrimaryNetApp::DSSPrimaryNetApp():
   mSecNetAppList(0)
{
}

//===================================================================
//  FUNCTION:   DSSPrimaryNetApp::~DSSPrimaryNetApp
//
//  DESCRIPTION:
//  Destructor of the DSSPrimaryNetApp class.
//===================================================================

/*lint -e{1551} */
void DSSPrimaryNetApp::Destructor()
throw()
{
   while (NULL != mSecNetAppList) {
      DSSSecondaryNetApp *temp = mSecNetAppList->GetNext();
      (void) mSecNetAppList->Release();
      mSecNetAppList = temp;
   }
   DSSNetApp::Destructor();
}
/*lint –restore */

//===================================================================
//  FUNCTION:   DSSGlobals::FindSecondaryDSSNetApp
//
//  DESCRIPTION:
//     TODO: Documentation
//===================================================================
AEEResult DSSPrimaryNetApp::FindSecondaryDSSNetApp(dss_iface_id_type iface_id, DSSNetApp** ppNetApp)
{
   dss_iface_id_type      DSSNetAppIfaceId;
   DSSSecondaryNetApp*    pSecDSSNetApp = mSecNetAppList;
   AEEResult              res;
   DSSIDSNetPolicyPrivScope IDSNetPolicyPrivScope;
   INetworkFactoryPriv* pIDSNetworkFactoryPriv = NULL;
   INetworkPriv*        pIDSNetworkPriv = NULL;

   while (pSecDSSNetApp) {
      pSecDSSNetApp->GetIfaceId(&DSSNetAppIfaceId);
      if (DSSNetAppIfaceId == iface_id) {
         *ppNetApp = pSecDSSNetApp;  // Assumption: if secondary DSSNetApp exists, it has a valid IDSNetwork
         return AEE_SUCCESS;
      }
      pSecDSSNetApp = pSecDSSNetApp->GetNext();
   }

   // Appropriate DSSSecondaryNetApp not found. Create new one.
   pSecDSSNetApp = DSSSecondaryNetApp::CreateInstance();
   if (NULL == pSecDSSNetApp) {
      LOG_MSG_ERROR( "Couldn't allocate DSSSecondaryNetApp", 0, 0, 0);
      return AEE_ENOMEMORY;
   }

   pSecDSSNetApp->SetIfaceId(iface_id);
   pSecDSSNetApp->SetNetHandle(mNetHandle);

   // TODO: Similar code is in DSSGlobals::CreateTempDSSNetApp. Unite.
   res = IDSNetPolicyPrivScope.Init();
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't create DSNetPolicyPriv", 0, 0, 0);
      goto bail;
   }

   res = IDSNetPolicyPrivScope.Fetch()->SetIfaceId((iface_id & 0xFF000000) | 0x00FFFF00);
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
   res = pIDSNetworkFactoryPriv->CreateNetworkPriv(IDSNetPolicyPrivScope.Fetch(), &pIDSNetworkPriv);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't create INetworkPriv", 0, 0, 0);
      goto bail;
   }


   // DSSNetApp takes care to AddRef pIDSNetworkPriv once it is set
   res = pSecDSSNetApp->Init(pIDSNetworkPriv);
   if (AEE_SUCCESS != res) {
      LOG_MSG_ERROR("Can't Initialize DSSNetApp", 0, 0, 0);
      goto bail;
   }

   // now it safe to add newly created Secondary NetApp
   // to Primary NetApp's list
   pSecDSSNetApp->InsertToSecList(this);

   (void) pIDSNetworkPriv->LookupInterface();

   *ppNetApp = pSecDSSNetApp;

   DSSCommon::ReleaseIf((IQI**)&pIDSNetworkPriv);
   
   return AEE_SUCCESS;

bail:
   DSSCommon::ReleaseIf((IQI**)&pSecDSSNetApp);
   DSSCommon::ReleaseIf((IQI**)&pIDSNetworkPriv);
   return res;
}

void * DSSPrimaryNetApp::operator new
(
   unsigned int numBytes
) throw()
{
   return ps_mem_get_buf( PS_MEM_DSAL_PRIMARY_NET_APP_TYPE);
} /* DSSPrimaryNetApp::operator new() */

DSSPrimaryNetApp * DSSPrimaryNetApp::CreateInstance()
{
   return new DSSPrimaryNetApp();
}
