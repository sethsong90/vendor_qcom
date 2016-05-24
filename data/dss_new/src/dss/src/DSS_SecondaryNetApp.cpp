/*======================================================

FILE:  DSS_SecondaryNetApp.cpp

SERVICES:

GENERAL DESCRIPTION:
Implementation of DSSSecondaryNetApp class

=====================================================

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_SecondaryNetApp.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/


//===================================================================
//   Includes and Public Data Declarations
//===================================================================
#include "DSS_NetApp.h"
#include "DSS_SecondaryNetApp.h"
#include "ps_mem.h"
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
//            DSSSecondaryNetApp Functions Definitions
//===================================================================


//===================================================================
//  FUNCTION:   DSSSecondaryNetApp::DSSSecondaryNetApp
//
//  DESCRIPTION:
//  Constructor of the DSSSecondaryNetApp class.
//===================================================================

DSSSecondaryNetApp::DSSSecondaryNetApp():
   mNextSecNetApp(0)
{
}

//===================================================================
//  FUNCTION:   DSSSecondaryNetApp::~DSSSecondaryNetApp
//
//  DESCRIPTION:
//  Destructor of the DSSSecondaryNetApp class.
//===================================================================

DSSSecondaryNetApp::~DSSSecondaryNetApp()
throw()
{
   mNextSecNetApp = NULL; // done in order to avoid Lint errro. We don't manage
                 // reference count for DSSSecondaryNetApp and this zeroing
                 // has no meaning at this point
}

//===================================================================
//  FUNCTION:   DSSSecondaryNetApp::InsertToSecList
//
//  DESCRIPTION:
//
//===================================================================

void DSSSecondaryNetApp::InsertToSecList(DSSPrimaryNetApp* pPrimaryDSSNetApp)
{
   DSSSecondaryNetApp* pSecNetAppList = pPrimaryDSSNetApp->GetSecNetAppList();

   if (NULL != pSecNetAppList) {
      mNextSecNetApp = pSecNetAppList->mNextSecNetApp;
   }
   pPrimaryDSSNetApp->SetSecNetAppList(this);
}

void * DSSSecondaryNetApp::operator new
(
   unsigned int numBytes
)
throw()
{
   return ps_mem_get_buf(PS_MEM_DSAL_SECONDARY_NET_APP_TYPE);
} /* DSSSecondaryNetApp::operator new() */

DSSSecondaryNetApp * DSSSecondaryNetApp::CreateInstance()
{
   return new DSSSecondaryNetApp();
}
