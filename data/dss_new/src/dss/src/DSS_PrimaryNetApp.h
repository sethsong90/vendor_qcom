#ifndef __DSS_PRIMARYNETAPP_H__
#define __DSS_PRIMARYNETAPP_H__

/*======================================================

FILE:  DSS_PrimaryNetApp.h

SERVICES:
Primary Network Application class in Backward Compatibility Layer

GENERAL DESCRIPTION:

=====================================================

Copyright (c) 2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_PrimaryNetApp.h#1 $
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
#include "customer.h"
#include "dssocket.h"

#include "DSS_NetApp.h"

#include "ds_Net_IPolicy.h"


//-------------------------------------------------------------------
// Forward Declarations
//-------------------------------------------------------------------
class DSSSecondaryNetApp;

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


//===================================================================
//              Macro Definitions
//===================================================================


//===================================================================
//              Class Definitions
//===================================================================

//===================================================================
//  CLASS:      DSSPrimaryNetApp
//
//  DESCRIPTION:
//
//  HOW TO USE:
//
//===================================================================

class DSSPrimaryNetApp : public DSSNetApp
{
//-------------------------------------------------------------------
//  Constructors/Desctructors
//-------------------------------------------------------------------
  public:
    DSSPrimaryNetApp();
    virtual void Destructor(void) throw();

    static DSSPrimaryNetApp* CreateInstance();

//-------------------------------------------------------------------
//  Interface members
//-------------------------------------------------------------------
  public:
    AEEResult FindSecondaryDSSNetApp(dss_iface_id_type iface_id, DSSNetApp** ppNetApp);

//-------------------------------------------------------------------
//  Get/Set functions for protected members access
//
//-------------------------------------------------------------------

    DSSSecondaryNetApp* GetSecNetAppList() {return mSecNetAppList;}
    void SetSecNetAppList(DSSSecondaryNetApp* pSecNetApp) {mSecNetAppList = pSecNetApp;}
//-------------------------------------------------------------------
//  Protected members
//-------------------------------------------------------------------

//-------------------------------------------------------------------
//  Data members
//-------------------------------------------------------------------
  protected:
      DSSSecondaryNetApp* mSecNetAppList;  // List of secondary structures.


private :
   void * operator new (
      unsigned int numBytes
   ) throw();
};

//===================================================================

#endif // __DSS_PRIMARYNETAPP_H__
