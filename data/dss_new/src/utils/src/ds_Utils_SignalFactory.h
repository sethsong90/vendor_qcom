#ifndef DS_UTILS_SIGNAL_FACTORY_H
#define DS_UTILS_SIGNAL_FACTORY_H

/*==========================================================================*/
/*!
  @file
  ds_Utils_SignalFactory.h

  @brief
  This file prvovides the ds::Utils::SignalFactory class.

  @details
  This file provides the ds::Utils::SignalFactory implementation. This
  class implements the ISignalFactory interface for REX/L4 environment.

  SignalFactory()
    Create a SignalFactory object.

  ~SignalFactory()
    Destroy the SignalFactory object and all the signals associated with
    the signal factory.

  Copyright (c) 2008-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*=========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_SignalFactory.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2009-11-26 mt  Created module based on SignalCBFactory.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "AEEStdErr.h"
#include "ds_Utils_CSSupport.h"
#include "ds_Utils_Factory.h"
#include "ds_Utils_MemManager.h"

#include "AEEISignalFactory.h"
#include "AEEISignal.h"
#include "AEEISignalCtl.h"
#include "AEECSignalFactory.h"

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/

/*---------------------------------------------------------------------------
  Class ds::Utils::SignalFactory
  --------------------------------

  Implements interface      : ISignalFactory
  Derives from classes      : ISignalFactory
                                -->IQI
                              ds::Utils::Factory
                                -->ITraverser
  Abstract class?           : No.
---------------------------------------------------------------------------*/
namespace ds
{
namespace Utils
{
/*lint -esym(1510, ISignalFactory) */
/*lint -esym(1510, IQI) */
class SignalFactory : public ISignalFactory,
                      public Factory
{

private:
 /**
  @brief
  This is the constructor of the SignalFactory object.

  @details
  Use the static method provided by this class instead to create instances
  of this object.

  @param      None.
  @see        None.
  @return     The constructed object.
  */
  SignalFactory
  (
    void
  );

public:

  /**
  @brief
  Destructor.

  @param      None.
  @see        None.
  @return     The constructed object.
  */
  virtual ~SignalFactory
  (
    void
  )
  throw();

  /**
  @brief
  Creates the Signal/SignalCtl objects.

  @details
  The SignalFactory class provides a factory method to create
  instances of Signal and SignalCtl objects. This method takes a
  SignalHandler object and two parameters that the signal object
  should be created with.

  @param[in]  piSignalHandler - Pointer to a SignalHandler class.
  @param[in]  uArgA - Pointer to the callback function.
  @param[in]  uArgB - Pointer to client data.
  @param[out] ppISig - Returned ISignal interface pointer.
  @param[out] ppISigCtl - Returned ISignalCtl object.

  @see        None.
  @return     AEE_SUCCESS - On success.
  @return     AEE_ENOMEMORY - no memory to construct object.
  @note       uArgA function cannot be NULL.
  */
  virtual int CDECL CreateSignal
  (
    ISignalHandler * piHandler,
    uint32           uArgA,
    uint32           uArgB,
    ISignal**        ppISig,
    ISignalCtl**     ppiSigCtl
  )
  throw();

  /* Overloaded new operator */
  void* operator new (unsigned int num_bytes)
  throw()
  {
    (void) num_bytes;
    return ps_mem_get_buf (PS_MEM_DS_UTILS_SIGNAL_FACTORY);
  }

  /* Overloaded delete operator */
  void operator delete (void* objPtr)
  throw()
  {
    PS_MEM_FREE(objPtr);
  }

  /*-------------------------------------------------------------------------
    Defintions of IQI Methods
  -------------------------------------------------------------------------*/
  DSIQI_IMPL_DEFAULTS_SINGLETON(ISignalFactory)

  /*-------------------------------------------------------------------------
    CreateInstance method for SignalFactory.
  -------------------------------------------------------------------------*/
  static int CreateInstance
  (
    void*    env,
    AEECLSID clsid,
    void*    privset,
    void**   newObj
  );

}; /* class SignalFactory */
}  /* namespace Utils */
}  /* namespace ds */


#endif /* DS_UTILS_SIGNAL_CB_FACTORY_H */

