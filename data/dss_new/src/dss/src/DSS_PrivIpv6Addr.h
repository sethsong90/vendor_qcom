#ifndef __DSS_PRIVIPV6ADDR_H__
#define __DSS_PRIVIPV6ADDR_H__

/*======================================================

FILE:  DSS_PrivIpv6Addr.h

SERVICES:
Backward Compatibility Layer Private Ipv6 address class

GENERAL DESCRIPTION:

=====================================================

Copyright (c) 2009 - 2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_PrivIpv6Addr.h#1 $
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
#include "ds_Net_IIPv6Address.h"
#include "ds_Errors_Def.h"
#include "ds_Utils_IWeakRef.h"
#include "ds_Utils_IWeakRefSupport.h"

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

//===================================================================
//              Macro Definitions
//===================================================================


//===================================================================
//              Class Definitions
//===================================================================

//===================================================================
//  CLASS:      DSSPrivIpv6Addr
//
//  DESCRIPTION:
//
//  HOW TO USE:
//
//===================================================================

// Forward declarations to prevent circular inclusion of DSSPrivIpv6Addr.

class DSSEventHandler;
class DSSPrivIpv6AddrHandler;
class DSSNetApp;

using ds::Utils::IWeakRef;


class DSSPrivIpv6Addr : public IWeakRef
{
   //-------------------------------------------------------------------
   //  Constructors/Desctructors
   //-------------------------------------------------------------------
public:

  DSSPrivIpv6Addr(ds::Net::IIPv6Address* pNetIpv6Address, uint32 flowID, boolean isUnique);
   void InsertToList(DSSPrivIpv6Addr* pDSSPrivIpv6Addr);
   virtual ~DSSPrivIpv6Addr() throw() {};
   virtual void Destructor() throw();

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

   inline AEEResult GetIDSNetIpv6Address(ds::Net::IIPv6Address** ppNetIpv6Address)
   {
      if (NULL == mpNetIpv6Address) {
         return QDS_EINVAL;
      }
      *ppNetIpv6Address = mpNetIpv6Address;
      (void)mpNetIpv6Address->AddRef();
      return AEE_SUCCESS;
   }


   AEEResult GetNext(DSSPrivIpv6Addr** ppDSSPrivIpv6Addr) throw()
   {
      *ppDSSPrivIpv6Addr = mNext;
      return AEE_SUCCESS;
   }

   // Set() and Get() of parent DSSNetApp object.
   // GetDSSNetApp() returns NetApp with storng ref, or NULL if
   // NetApp freed already. who calls GetDSSNetApp(), should call
   // Release() on NetApp when done.
   AEEResult GetDSSNetApp(DSSNetApp** ppDSSNetApp);

   void SetDSSNetApp(DSSNetApp* pDSSNetApp);

   inline void GetFlowID(uint32* pFlowID)
   {
      *pFlowID = mFlowID;
   }

   inline void GetIsUnique(boolean* pIsUnique)
   {
      *pIsUnique = mbIsUnique;
   }


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
   ds::Net::IIPv6Address* mpNetIpv6Address;
   DSSPrivIpv6Addr* mNext;     // DSSNetQoS list
   uint32 mFlowID;
   boolean mbIsUnique;
   DSSPrivIpv6AddrHandler* mpIpv6PrivAddrEventHandler;
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

#endif // __DSS_PRIVIPV6ADDR_H__
