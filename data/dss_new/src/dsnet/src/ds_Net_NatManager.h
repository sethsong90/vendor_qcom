#ifndef DS_NET_NAT_MANAGER_H
#define DS_NET_NAT_MANAGER_H
/*==========================================================================*/ 
/*! 
  @file 
  ds_Net_NatManager.h

  @brief
  This file defines the class that implements the INatSessionPriv
  interface.

  @details
  The NatManager class (ds::Net::NatManager) implements the following 
  interfaces:
  IQI
  INatSessionPriv


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

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_NatManager.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-10-04 dm  Created module.

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
/*lint -esym(1510, INatSessionPriv) */
/*lint -esym(1510, IQI) */
class NatManager : public INatSessionPriv
{

private:
  int32                 mIfaceHandle;
  void                 *mpNatHandle;
  ICritSect            *mpICritSect;
  IPolicy              *mpIPolicy;

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
  virtual ~NatManager
  (
    void
  ) 
  throw();


public:
  /*!
  @brief      
  Constructor of NatManagerobject.

  @param      pIPolicy - Policy for this NAT Manager.
  @return     None.
  */
  NatManager
  (
    const ds::Net::IPolicy *pIPolicy 
  );

  /*-------------------------------------------------------------------------
    Functions from INatSessionPriv interface.
  -------------------------------------------------------------------------*/
  /*!
  @function
  Enable()

  @brief      
  This function enables the NAT Manager. 

  @details
  TODO

  @param      None. 

  @return     SUCCESS - on success
  @return     Other ds errors could be returned.
  */
  virtual ds::ErrorType CDECL Enable
  (
    void 
  );

  /*!
  @function
  Disable()

  @brief      
  This function disables the NAT Manager. 

  @details
  TODO

  @param      None. 

  @return     SUCCESS - on success
  @return     Other ds errors could be returned.
  */
  virtual ds::ErrorType CDECL Disable
  (
    void 
  );


  /*!
  @function
  Enable()

  @brief      
  This function enables roaming autoconnect on NAT Manager. 

  @details
  TODO

  @param      None. 

  @return     SUCCESS - on success
  @return     Other ds errors could be returned.
  */
  virtual ds::ErrorType CDECL EnableRoamingAutoconnect
  (
    void 
  );

  /*!
  @function
  Disable()

  @brief      
  This function disables roaming autocnnect on NAT Manager. 

  @details
  TODO

  @param      None. 

  @return     SUCCESS - on success
  @return     Other ds errors could be returned.
  */
  virtual ds::ErrorType CDECL DisableRoamingAutoconnect
  (
    void 
  );

  /*-------------------------------------------------------------------------
    IQI interface Methods
  -------------------------------------------------------------------------*/
  DSIQI_IMPL_DEFAULTS(INatSessionPriv)

  /*-------------------------------------------------------------------------
    Overload new/delete operators.
  -------------------------------------------------------------------------*/
  DSNET_OVERLOAD_OPERATORS(PS_MEM_DS_NET_NAT_MANAGER)

};/* class NatManager */
} /* namespace Net */
} /* namespace ds */

#endif /* DS_NET_NAT_MANAGER_H */


