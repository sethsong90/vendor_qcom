#ifndef __DSS_MCAST_H__
#define __DSS_MCAST_H__

/*======================================================

FILE:  DSS_MCast.h

SERVICES:
Backward Compatibility Layer MCast class

GENERAL DESCRIPTION:

=====================================================

Copyright (c) 2008-2010 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_MCast.h#1 $
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
#include "AEEICritSect.h"
#include "AEEISignal.h"
#include "AEEISignalCtl.h"

#include "DSS_EventHandler.h"

#include "DSS_MCastStatusHandler.h"
#include "DSS_MCastRegisterHandler_1_0.h"
#include "DSS_MCastRegisterHandler_2_0.h"
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
//  CLASS:      DSSMCast
//
//  DESCRIPTION:
//
//  HOW TO USE:
//
//===================================================================

class DSSEventHandler;
class DSSMCastHandler;
class DSSNetApp;

using ds::Utils::IWeakRef;

class DSSMCast : public IWeakRef
{
//-------------------------------------------------------------------
//  Constructors / Destructors
//-------------------------------------------------------------------
  public:

    DSSMCast(ds::Net::IMCastSessionPriv* pMCastSession, uint32 flowID);
    void InsertToList(DSSMCast* pDSSMCast);
    virtual ~DSSMCast() throw() {}
    virtual void Destructor() throw();
    void GetMCastHandle(uint32* pMCastHandle);
    AEEResult RegEventCB(dss_iface_ioctl_event_cb cback_fn,
                         void* user_data,
                         dss_iface_ioctl_event_enum_type event_cb);
    void * operator new (unsigned int numBytes) throw();
    void operator delete (void *  bufPtr);

    //-------------------------------------------------------------------------
    //  Defintions of IQI and IWeakRef Methods
    //-------------------------------------------------------------------------
    DS_UTILS_IWEAKREF_IMPL_DEFAULTS()
  
  public:
       
    inline AEEResult GetMCastSession(ds::Net::IMCastSessionPriv** ppMCastSession) {
       *ppMCastSession = mpNetMCastSession;
       (void)mpNetMCastSession->AddRef();
       return AEE_SUCCESS;
    }

    // Set() and Get() of parent DSSNetApp object.
    // GetDSSNetApp() returns NetApp with storng ref, or NULL if
    // NetApp freed already. who calls GetDSSNetApp(), should call
    // Release() on NetApp when done.
    AEEResult GetDSSNetApp(DSSNetApp** ppDSSNetApp);

    void SetDSSNetApp(DSSNetApp* pDSSNetApp);

    AEEResult GetNext(DSSMCast** ppDSSMCastSession) throw() {
       *ppDSSMCastSession = mNext;
       return AEE_SUCCESS;
    }

    //-------------------------------------------------------------------
    //  Deregisters MCastRegister handler, in case both are deregistered
    //  returns TRUE, indicating that this DSSMCast object can be 
    //  deleted, otherwise returns FALSE
    //-------------------------------------------------------------------
    boolean DeregisterHandler(ds::Net::EventType handlerType);

//-------------------------------------------------------------------
//  Private members
//-------------------------------------------------------------------
  private:
     ds::Net::IMCastSessionPriv* mpNetMCastSession;
     DSSMCast* mNext;
     uint32 mMCastHandle;
     DSSMCastStatusHandler* mpMCastStatusHandler;
     DSSMCastRegisterHandler1_0* mpMCastRegisterHandler1_0;
     DSSMCastRegisterHandler2_0* mpMCastRegisterHandler2_0;
     DSSNetApp* mparentNetApp;
     template<typename HandlerType>
     inline AEEResult FetchHandler(HandlerType** pHandler, DSSEventHandler** ppEventHandler, bool bInit);

};


//===================================================================

#endif // __DSS_MCAST_H__
