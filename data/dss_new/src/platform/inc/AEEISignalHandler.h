#ifndef AEEISIGNALHANDLER_H
#define AEEISIGNALHANDLER_H
/*======================================================================
        Copyright 2006 - 2008 Qualcomm Technologies, Inc.
               All Rights Reserved.
            Qualcomm Technologies Confidential and Proprietary
========================================================================

DESCRIPTION:  Definition of ISignalHandler interface.

======================================================================*/

#include "AEEIQI.h"

#define AEEIID_ISignalHandler 0x01041078

#if defined(AEEINTERFACE_CPLUSPLUS)
struct ISignalHandler : public IQI
{
   virtual int CDECL Notify(uint32 uArgA, uint32 uArgB) = 0;
};

#else /* #if defined(AEEINTERFACE_CPLUSPLUS) */

#define INHERIT_ISignalHandler(iname) \
   INHERIT_IQI(iname); \
   int (*Notify)(iname *, \
      uint32 uArgA, \
      uint32 uArgB \
   )

AEEINTERFACE_DEFINE(ISignalHandler);

static __inline uint32 ISignalHandler_AddRef(ISignalHandler *pif)
{
   return AEEGETPVTBL(pif,ISignalHandler)->AddRef(pif);
}

static __inline uint32 ISignalHandler_Release(ISignalHandler *pif)
{
   return AEEGETPVTBL(pif,ISignalHandler)->Release(pif);
}

static __inline int ISignalHandler_QueryInterface(ISignalHandler *pif, AEEIID iid, void * *ppOut)
{
   return AEEGETPVTBL(pif,ISignalHandler)->QueryInterface(pif, iid, ppOut);
}

static __inline int ISignalHandler_Notify(ISignalHandler *pif, uint32 uArgA, uint32 uArgB)
{
   return AEEGETPVTBL(pif,ISignalHandler)->Notify(pif, uArgA, uArgB);
}

#endif /* #if defined(AEEINTERFACE_CPLUSPLUS) */

/*
===============================================================================
INTERFACES DOCUMENTATION
===============================================================================

ISignalHandler Interface

Description: This interface is to be implemented by the users in order to create
Signals from SignalFactory. Signal object sends asynchronous notifications as invocations
to ISignalHandler_Notify().

===============================================================================

ISignalHandler_AddRef()

Description:
    This function is inherited from IQI_AddRef(). 

See Also:
    ISignalHandler_Release()

===============================================================================

ISignalHandler_Release()

Description:
    This function is inherited from IQI_Release(). 

See Also:
    ISignal_AddRef()

===============================================================================

ISignalHandler_QueryInterface()

Description:
    This function is inherited from IQI_QueryInterface(). 

===============================================================================

ISignalHandler_Notify()

Description:
   A signal when set will cause invocation to this method of associated SignalHandler
object.

Prototype:

   int ISignalHandler_Notify(ISignalHandler *pif, uint32 uArgA, uint32 uArgB)

Parameters:
   pif: Pointer to an ISignalHandler interface
   uArgA: User parameters given to ISignalFactory_CreateSignal()
   uArgB: User parameters given to ISignalFactory_CreateSignal()

Return Value:
   AEE_SUCCESS will re-enable the Signal.
   anything else will leave Signal disabled. Use ISignalCtl_Enable() to re-enable.

See Also:
   ISignal_Set(), ISignalCtl_Enable()

===============================================================================

*/

#endif /* AEEISIGNALHANDLER_H */

