#ifndef TECH_UMTS_FACTORY_H
#define TECH_UMTS_FACTORY_H

/*===========================================================================
  @file TechUMTSFactory.h

  This file defines the class that implements the ITechUMTS interface.

  TODO: Detailed explaination about the class here.

  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dsnet/rel/11.03/src/ds_Net_TechUMTSFactory.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-03-10 hm  Created module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "AEEStdErr.h"
#include "ds_Errors_Def.h"
#include "AEEISignal.h"
#include "ds_Utils_CSSupport.h"
#include "ds_Net_Utils.h"
#include "ds_Net_MemManager.h"
#include "ds_Net_ITechUMTS.h"
#include "ds_Net_IPolicy.h"


namespace ds
{

namespace Net
{
   /*lint -esym(1510, ITechUMTS) */
   /*lint -esym(1510, IQI) */
// There is no need to AddRef/Release on this class, it's a singleton
class TechUMTSFactory : public ITechUMTS
{
  static TechUMTSFactory * _instance;
  TechUMTSFactory () throw();
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
  virtual ~TechUMTSFactory
  (
    void 
  ) 
  throw();

  /*-------------------------------------------------------------------------
    Methods to overload new/delete operators.
  -------------------------------------------------------------------------*/
  DSNET_OVERLOAD_OPERATORS (PS_MEM_DS_NET_TECH_UMTS_FACTORY)


public:
   /*-------------------------------------------------------------------------
     Instance is a static method used to get an instance of the 
     singleton TechUMTSFactory object
   -------------------------------------------------------------------------*/
   static TechUMTSFactory* Instance
   (
     void
   )
   throw();

  /*-------------------------------------------------------------------------
    Functions inherited from ITechUMTS interface
  -------------------------------------------------------------------------*/
  virtual ds::ErrorType CDECL RegMTPD 
  (
    IPolicy* Policy, 
    ISignal* signalObj, 
    IQI**    mtpdRegObj
  );

  /*-------------------------------------------------------------------------
    Functions inherited from IQI interface
  -------------------------------------------------------------------------*/
  DSIQI_IMPL_DEFAULTS(ITechUMTS);

  /*-------------------------------------------------------------------------
    Mobile Terminated Packet Data event
  -------------------------------------------------------------------------*/
  const static EventType QDS_EV_MTPD  = 0x0106e612;

}; /* class TechUMTSFactory */
} /* namespace Net */
} /* namespace ds */

/* Declaration CreateInstance method for TechUMTSFactory */
DSIQI_DECL_CREATE_INSTANCE2(DS,Net,TechUMTSFactory)

#endif /* TECH_UMTS_FACTORY_H */

