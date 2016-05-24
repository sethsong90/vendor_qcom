#ifndef __DSS_NETMCASTMBMSCTRL_H__
#define __DSS_NETMCASTMBMSCTRL_H__

/*======================================================

FILE:  DSS_NetMCastMBMSCtrl.h

SERVICES:
Backward Compatibility Layer MCast class

GENERAL DESCRIPTION:

=====================================================

Copyright (c) 2008-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_NetMCastMBMSCtrl.h#1 $
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
#include "AEEISignal.h"
#include "AEEISignalCtl.h"
#include "DSS_EventHandler.h"
#include "ds_Net_IMCastMBMSCtrlPriv.h"
#include "DSS_MCastMBMSCtrlHandler.h"
#include "ds_Utils_IWeakRef.h"
#include "ds_Utils_IWeakRefSupport.h"

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
//  CLASS:      DSSNetMCastMBMSCtrl
//
//  DESCRIPTION:
//
//  HOW TO USE:
//
//===================================================================

class DSSEventHandler;
class DSSMCastHandler;

using ds::Utils::IWeakRef;

class DSSNetMCastMBMSCtrl : public IWeakRef
{
//-------------------------------------------------------------------
//  Constructors/Desctructors
//-------------------------------------------------------------------
  public:

    DSSNetMCastMBMSCtrl(ds::Net::IMCastMBMSCtrlPriv* pMCastMBMSCtrl, uint32 MCastMBMSHandle);
    void InsertToList(DSSNetMCastMBMSCtrl* pDSSMCastMBMS);
    virtual ~DSSNetMCastMBMSCtrl() throw() {}
    virtual void Destructor() throw();
    void GetMCastMBMSHandle(uint32* pMCastMBMSHandle);
    AEEResult RegEventCB(dss_iface_ioctl_event_cb cback_fn, void* user_data);

//-------------------------------------------------------------------
//  Interface members
//-------------------------------------------------------------------
  public:

/*-------------------------------------------------------------------------
     Defintions of IQI and IWeakRef Methods
-------------------------------------------------------------------------*/
     DS_UTILS_IWEAKREF_IMPL_DEFAULTS()

//-------------------------------------------------------------------
//  Get/Set functions for protected members access
//
//-------------------------------------------------------------------

    inline AEEResult GetMBMSCtrl(ds::Net::IMCastMBMSCtrlPriv** ppMCastMBMS) {
       *ppMCastMBMS = mpNetMCastMBMSCtrl;
       (void)mpNetMCastMBMSCtrl->AddRef();
       return AEE_SUCCESS;
    }

    AEEResult GetNext(DSSNetMCastMBMSCtrl** ppDSSMCastMBMS) throw() { //TODO: should be IDSNetQoSSecondary??
       *ppDSSMCastMBMS = mNext;
       return AEE_SUCCESS;
    }

    // Set() and Get() of parent DSSNetApp object.
    // GetDSSNetApp() returns NetApp with storng ref, or NULL if
    // NetApp freed already. who calls GetDSSNetApp(), should call
    // Release() on NetApp when done.
    AEEResult GetDSSNetApp(DSSNetApp** ppDSSNetApp);

    void SetDSSNetApp(DSSNetApp* pDSSNetApp);

    AEEResult GetEventHandler(dss_iface_ioctl_event_enum_type event, DSSEventHandler** ppEventHandler, bool bInit);

    template<typename HandlerType>
    inline AEEResult FetchHandler(HandlerType** pHandler, DSSEventHandler** ppEventHandler, bool bInit);

//-------------------------------------------------------------------
//  Protected members
//-------------------------------------------------------------------

//-------------------------------------------------------------------
//  Data members
//-------------------------------------------------------------------
  protected:
     ds::Net::IMCastMBMSCtrlPriv* mpNetMCastMBMSCtrl;
     DSSNetMCastMBMSCtrl* mNext;
     uint32 mMCastMBMSHandle;
     DSSMCastMBMSCtrlHandler* mpMCastMBMSHandler;
     DSSNetApp* mparentNetApp;

public :

   void * operator new (
      unsigned int numBytes
   )  throw();

   void operator delete (
      void *  bufPtr
   );
};


//===================================================================

#endif // __DSS_NETMCASTMBMSCTRL_H__
