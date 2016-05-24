#ifndef DS_NET_NAT_SESSION_H
#define DS_NET_NAT_SESSION_H
/*==========================================================================*/
/*!
  @file
  ds_Net_NatSession.h

  @brief
  This file defines the class that implements the INatSession
  interface.

  @details
  The NatSession class (ds::Net::NatSession) implements the following
  interfaces:
  IQI
  INatSession


  @todo
  Write details

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NatSession.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-20 bq  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Errors_Def.h"
#include "ds_Utils_CSSupport.h"
#include "ds_Utils_Factory.h"
#include "AEEICritSect.h"
#include "ds_Net_Utils.h"
#include "ds_Net_Handle.h"
#include "ds_Net_MemManager.h"
#include "ds_Net_IPolicy.h"
#include "ds_Net_INatSession.h"
#include "ds_Net_INatSessionPriv.h"

/*===========================================================================

                     PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace ds
{
namespace Net
{
/*lint -esym(1510, INatSession) */
/*lint -esym(1510, IQI) */
class NatSession : public INatSession
{

private:
  int32                 mIfaceHandle;

  /*!
  @brief
  Private destructor.

  @details
  The destructor of this object is private. Use Release() method of the
  IQI interface to free the object.

  @param      None.
  @return     None.
  @see        IQI::Release()
  */
  virtual ~NatSession
  (
    void
  )
  throw();


public:
  /*!
  @brief      
  Constructor of NatSession object.

  @param      ifaceHandle - Iface handle for this NAT session.
  @return     None.
  */
  NatSession(int32 ifaceHandle);

  /*-------------------------------------------------------------------------
    Inherited functions from INatSession.
  -------------------------------------------------------------------------*/
  /*!
  @function
  AddStaticNatEntry()

  @brief
  This function add static NAT Entry.

  @details
  TODO

  @param[in]  entry - Static NAT Entry.

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  @see        ds::IPAddrType
  */
  virtual ds::ErrorType CDECL AddStaticNatEntry
  (
    const ds::Net::IPNatStaticEntryType* entry
  );

  /*!
  @function
  DeleteStaticNatEntry()

  @brief
  This function delete static NAT Entry.

  @details
  TODO

  @param[in]  entry - Static NAT Entry.

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  @see        ds::IPAddrType
  */
  virtual ds::ErrorType CDECL DeleteStaticNatEntry
  (
    const ds::Net::IPNatStaticEntryType* entry
  );

  /*!
  @function
  GetStaticNatEntry()

  @brief
  This function get static NAT Entry.

  @details
  TODO

  @param[in]  entry - Static NAT Entries.

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  @see        ds::IPAddrType
  */
  virtual ds::ErrorType CDECL GetStaticNatEntry
  (
    ds::Net::IPNatStaticEntryType* entries,
    int entriesLen,
    int* entriesLenReq
  );

  /*!
  @function
  SetDynamicNatEntryTimeout()

  @brief
  This function set Dynamic NAT Entry Timeout in sec.

  @details
  TODO

  @param[in]  timeout - Entry Timeout in Second.

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  @see        ds::IPAddrType
  */
  virtual ds::ErrorType CDECL SetDynamicNatEntryTimeout
  (
    unsigned short int timeout
  );

  /*!
  @function
  GetDynamicNatEntryTimeout()

  @brief
  This function set Dynamic NAT Entry Timeout in sec.

  @details
  TODO

  @param[in]  timeout - Entry Timeout in Second.

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  @see        ds::IPAddrType
  */
  virtual ds::ErrorType CDECL GetDynamicNatEntryTimeout
  (
    unsigned short int *timeout
  );

  /*!
  @function
  SetIpSecVpnPassThrough()

  @brief
  This function set IPSEC VPN PassThrough.

  @details
  TODO

  @param[in]  isVpnPassThrough - VPN PassThrough.

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  */
  virtual ds::ErrorType CDECL SetIpSecVpnPassThrough
  (
    boolean isVpnPassThrough
  );

  /*!
  @function
  GetIpSecVpnPassThrough()

  @brief
  This function get IPSEC VPN PassThrough.

  @details
  TODO

  @param[in]  isVpnPassThrough - VPN PassThrough.

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  */
  virtual ds::ErrorType CDECL GetIpSecVpnPassThrough
  (
    boolean *isVpnPassThrough
  );

  /*!
  @function
  SetL2TPVpnPassThrough()

  @brief
  This function sets L2TP VPN PassThrough.

  @details
  TODO

  @param[in]  isVpnPassThrough - L2TP VPN PassThrough.

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  */
  virtual ds::ErrorType CDECL SetL2TPVpnPassThrough
  (
    boolean isVpnPassThrough
  );

  /*!
  @function
  GetL2TPVpnPassThrough()

  @brief
  This function gets L2TP VPN PassThrough.

  @details
  TODO

  @param[in]  isVpnPassThrough - L2TP VPN PassThrough.

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  */
  virtual ds::ErrorType CDECL GetL2TPVpnPassThrough
  (
    boolean *isVpnPassThrough
  );

  /*!
  @function
  SetPPTPVpnPassThrough()

  @brief
  This function sets PPTP VPN PassThrough.

  @details
  TODO

  @param[in]  isVpnPassThrough - PPTP VPN PassThrough.

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  */
  virtual ds::ErrorType CDECL SetPPTPVpnPassThrough
  (
    boolean isVpnPassThrough
  );

  /*!
  @function
  GetPPTPVpnPassThrough()

  @brief
  This function gets PPTP VPN PassThrough.

  @details
  TODO

  @param[in]  isVpnPassThrough - PPTP VPN PassThrough.

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  */
  virtual ds::ErrorType CDECL GetPPTPVpnPassThrough
  (
    boolean *isVpnPassThrough
  );

  /*!
  @function
  AddDMZ()

  @brief      
  Enables and creates DMZ Entry.

  @details
  This function enables DMZ and adds a specified DMZ address.
  Only one DMZ address can be added.

  @param[in]  entry - DMZ Entry.

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  */
  virtual ds::ErrorType CDECL AddDMZ 
  (
    const ds::Net::DMZEntryType* pDMZEntry
  );

    /*!
  @function
  GetDMZ()

  @brief      
  Gets a DMZ Entry.

  @details
  This function gets the DMZ entry which was added before.

  @param[out]  entry - DMZ Entry.

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  */
  virtual ds::ErrorType CDECL GetDMZ 
  (
    ds::Net::DMZEntryType* pDMZEntry
  );

    /*!
  @function
  DeleteDMZ()

  @brief      
  Deletes DMZ.

  @details
  This function disables and deletes DMZ address that was added before

  @return     SUCCESS - on success
  @return     DSS_EFAULT - Invalid arguments

  */
  virtual ds::ErrorType CDECL DeleteDMZ
  (
    void
  );

  /*-------------------------------------------------------------------------
    IQI interface Methods
  -------------------------------------------------------------------------*/
  DSIQI_IMPL_DEFAULTS(INatSession)

  /*-------------------------------------------------------------------------
    Overload new/delete operators.
  -------------------------------------------------------------------------*/
  DSNET_OVERLOAD_OPERATORS(PS_MEM_DS_NET_NAT_SESSION)

};/* class NatSession */
} /* namespace Net */
} /* namespace DS */

#endif /* DS_NET_NAT_SESSION_H */


