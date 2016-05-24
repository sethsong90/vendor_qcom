#ifndef __DSS_SECONDARYNETAPP_H__
#define __DSS_SECONDARYNETAPP_H__

/*======================================================

FILE:  DSS_SecondaryNetApp.h

SERVICES:
Secondary Network Application class in Backward Compatibility Layer

GENERAL DESCRIPTION:

=====================================================

Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_SecondaryNetApp.h#1 $
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
#include "DSS_PrimaryNetApp.h"

#include "ds_Net_IPolicy.h"

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
//              Class Definitions
//===================================================================

//===================================================================
//  CLASS:      DSSSecondaryNetApp
//
//  DESCRIPTION:
//
//  HOW TO USE:
//
//===================================================================

class DSSSecondaryNetApp : public DSSNetApp
{
//-------------------------------------------------------------------
//  Constructors/Desctructors
//-------------------------------------------------------------------
  public:
    DSSSecondaryNetApp();
    virtual ~DSSSecondaryNetApp() throw();

    static DSSSecondaryNetApp* CreateInstance();

//-------------------------------------------------------------------
//  Interface members
//-------------------------------------------------------------------
  public:
     void InsertToSecList(DSSPrimaryNetApp* pPrimaryDSSNetApp);

//-------------------------------------------------------------------
//  Get/Set functions for protected members access
//
//-------------------------------------------------------------------
     DSSSecondaryNetApp* GetNext() throw() {return mNextSecNetApp;}
//-------------------------------------------------------------------
//  Protected members
//-------------------------------------------------------------------

//-------------------------------------------------------------------
//  Data members
//-------------------------------------------------------------------
  protected:

      // List of secondary structures.
      DSSSecondaryNetApp* mNextSecNetApp;  // DSSSecondaryNetApp list

  private:
      void * operator new (
         unsigned int numBytes
      ) throw();
};

//===================================================================

#endif // __DSS_SECONDARYNETAPP_H__
