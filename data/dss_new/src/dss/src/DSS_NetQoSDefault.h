#ifndef __DSS_NETQOSDEFAULT_H__
#define __DSS_NETQOSDEFAULT_H__

/*======================================================

FILE:  DSS_NetQoSDefault.h

SERVICES:
Backward Compatibility Layer Secondary QoS class

GENERAL DESCRIPTION:

=====================================================

Copyright (c) 2008-2010 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_NetQoSDefault.h#1 $
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

#include "AEEStdErr.h"
#include "DSS_NetApp.h"
#include "AEEISignal.h"
#include "AEEISignalCtl.h"

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
//  CLASS:      DSSNetQoS
//
//  DESCRIPTION:
//
//  HOW TO USE:
//
//===================================================================

class DSSNetQoSDefault
{
//-------------------------------------------------------------------
//  Constructors/Desctructors
//-------------------------------------------------------------------
  public:

    DSSNetQoSDefault(ds::Net::IQoSDefault* pNetQoSDefault);
    ~DSSNetQoSDefault();

//-------------------------------------------------------------------
//  Interface members
//-------------------------------------------------------------------
  public:

//-------------------------------------------------------------------
//  Get/Set functions for protected members access
//
//-------------------------------------------------------------------

    inline AEEResult GetNetQoSDefault(ds::Net::IQoSDefault** ppNetQoSDefault) {
       *ppNetQoSDefault = mpNetQoSDefault;
       (void)mpNetQoSDefault->AddRef();
       return AEE_SUCCESS;
    }

//-------------------------------------------------------------------
//  Protected members
//-------------------------------------------------------------------

//-------------------------------------------------------------------
//  Data members
//-------------------------------------------------------------------
  protected:
     ds::Net::IQoSDefault* mpNetQoSDefault;

  public :

   void * operator new (
      unsigned int numBytes
   )  throw();

   void operator delete (
      void *  bufPtr
   );
};

//===================================================================

#endif // __DSS_NETQOSDEFAULT_H__
