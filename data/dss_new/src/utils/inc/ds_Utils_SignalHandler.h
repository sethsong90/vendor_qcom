#ifndef DS_UTILS_SIGNALHANDLER_H
#define DS_UTILS_SIGNALHANDLER_H

/*!
  @file
  ds_Utils_SignalHandler.h

  @details
  This file provides the ds::Utils::SignalHandler class. This class implements
  the AEEISignalHandler interface. The following methods are exported.

  AEEResult Notify(in uint32 uArgA, in uint32 uArgB)
    This method of the client's signal handler object is invoked when a
    signal is set.

  This version relies on the owner to provide a weak reference implementation
  (by requiring IWeakRef) to avoid having to implement weak reference
  functionality by itself.

  @see ds_Utils_SignalHandler.h

  Copyright (c) 2009-2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/inc/ds_Utils_SignalHandler.h#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-06-28  mt Change the implementation to rely on owner-supplied IWeakRef
  2008-11-16  mt Created the module.

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"

#include "AEEISignalHandler.h"

#include "AEEStdErr.h"
#include "ds_Utils_CSSupport.h"
#include "ds_Utils_IWeakRef.h"

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
namespace ds
{
namespace Utils
{

/*lint -esym(1510, IQI) */

/*!
  @class
  ds::Utils::SignalHandler

  @brief
  Implements the ISignalHandler interface.

  @details
  TODO
*/
class SignalHandler : public ISignalHandler
{
private:
  IWeakRef *mpOwner;

  /*-------------------------------------------------------------------------
    An attempt to do "new SignalHandler" should not compile, as the usage is
    SignalHandler as a member. This declaration is left without implementa-
    tion to achieve that.
  -------------------------------------------------------------------------*/
  static void* operator new
  (
    unsigned int numBytes
  ) throw();

public:
  SignalHandler
  (
  ) throw();

  ~SignalHandler
  (
  ) throw();

  /*!
    @brief
    This method is called to initialize the SignalHandler object.
  */
  void Init
  (
     IWeakRef *pOwner
  )
  throw();

  /*!
    @brief
    Inherited from ISignalHandler. A set signal will cause invocation to
    this method.

    @param uArgA
    @param uArgB

    @return
  */
  virtual int CDECL Notify
  (
    uint32 uArgA,
    uint32 uArgB
  );

  /*-------------------------------------------------------------------------
    IQI Methods. Note that SH uses a separate ref counter for itself,
    to be able to determine when the weak reference it owns may be released.

    Also, since it should always be used as a member, Release() doesn't call
    'delete this' - the destructor is called when the owner is destroyed.
  -------------------------------------------------------------------------*/
  DSIQI_DECL_LOCALS()
  DSIQI_ADDREF()
  virtual uint32 CDECL Release() throw();
  DSIQI_QUERY_INTERFACE(ISignalHandler)

}; /* class SignalHandler */

} /* namespace Utils */
}/* namespace ds */

#endif /* DS_UTILS_SIGNALHANDLER_H */

