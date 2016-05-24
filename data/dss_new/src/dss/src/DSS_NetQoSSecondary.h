#ifndef __DSS_NETQOSSECONDARY_H__
#define __DSS_NETQOSSECONDARY_H__

/*======================================================

FILE:  DSS_NetQoSSecondary.h

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

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_NetQoSSecondary.h#1 $
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
//  CLASS:      DSSNetQoS
//
//  DESCRIPTION:
//
//  HOW TO USE:
//
//===================================================================

// Forward declarations to prevent circular inclusion of DSSNetQoSSecondary.
class DSSEventHandler;
class DSSQoSHandler;
class DSSQoSModifyHandler;
class DSSQoSInfoCodeUpdatedHandler;
class DSSNetApp;

using ds::Utils::IWeakRef;

class DSSNetQoSSecondary : public IWeakRef
{
//-------------------------------------------------------------------
//  Constructors/Desctructors
//-------------------------------------------------------------------
  public:

    DSSNetQoSSecondary(ds::Net::IQoSSecondary* pNetQoSSecondary, uint32 flowID);
    virtual ~DSSNetQoSSecondary() throw() {}
    void InsertToList(DSSNetQoSSecondary* pDSSQoS);
    virtual void Destructor() throw();
    void GetFlowID(uint32* pFlowID);

    /*-------------------------------------------------------------------------
    Defintions of IQI and IWeakRef Methods
    -------------------------------------------------------------------------*/
    DS_UTILS_IWEAKREF_IMPL_DEFAULTS()

//-------------------------------------------------------------------
//  Interface members
//-------------------------------------------------------------------
  public:

//-------------------------------------------------------------------
//  Get/Set functions for protected members access
//
//-------------------------------------------------------------------


    // Set() and Get() of parent DSSNetApp object.
    // GetDSSNetApp() returns NetApp with storng ref, or NULL if
    // NetApp freed already. who calls GetDSSNetApp(), should call
    // Release() on NetApp when done.
    AEEResult GetDSSNetApp(DSSNetApp** ppDSSNetApp);

    void SetDSSNetApp(DSSNetApp* pDSSNetApp);

    AEEResult GetNext(DSSNetQoSSecondary** ppDSSNetQoS) throw(){
       *ppDSSNetQoS = mNext;
       return AEE_SUCCESS;
    }

    AEEResult GetNetQoSSecondary(ds::Net::IQoSSecondary** ppNetQoSSecondary); 

    AEEResult RegEventCB(dss_iface_ioctl_ev_cb_type* pEvArg);
    AEEResult DeregEventCB(dss_iface_ioctl_ev_cb_type* pEvArg);

    template<typename HandlerType>
    AEEResult FetchHandler(HandlerType** pHandler, DSSEventHandler** ppEventHandler, bool bInit);

    AEEResult GetEventHandler(dss_iface_ioctl_event_enum_type event, DSSEventHandler** ppEventHandler, bool bInit);
//-------------------------------------------------------------------
//  Protected members
//-------------------------------------------------------------------

//-------------------------------------------------------------------
//  Data members
//-------------------------------------------------------------------
  protected:
     ds::Net::IQoSSecondary* mpNetQoSSecondary;
     DSSNetQoSSecondary* mNext;     // DSSNetQoS list
     uint32 mFlowID;
     DSSQoSHandler* mpNetQoSHandler;
     DSSQoSModifyHandler* mpNetQoSModifyHandler;
     DSSQoSInfoCodeUpdatedHandler* mpQoSInfoCodeUpdatedHandler;
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

#endif // __DSS_NETQOSSECONDARY_H__
