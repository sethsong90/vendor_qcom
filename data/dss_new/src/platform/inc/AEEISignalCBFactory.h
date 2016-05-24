#ifndef AEEISIGNALCBFACTORY_H
#define AEEISIGNALCBFACTORY_H
/*======================================================================
        Copyright 2006 - 2008 Qualcomm Technologies, Inc.
               All Rights Reserved.
            Qualcomm Technologies Confidential and Proprietary
========================================================================

DESCRIPTION:  Definition of ISignalCBFactory interface.

======================================================================*/

#include "AEEIQI.h"
#include "AEEISignal.h"
#include "AEEISignalCtl.h"

#define AEEIID_ISignalCBFactory 0x01043541

#if defined(AEEINTERFACE_CPLUSPLUS)
struct ISignalCBFactory : public IQI
{
   virtual int CDECL CreateSignal(void (*pfn)(void *pCx),
                                  void *pCx,
                                  ISignal * *ppiSig,
                                  ISignalCtl * *ppiSigCtl) = 0;
};

#else /* #if defined(AEEINTERFACE_CPLUSPLUS) */

#define INHERIT_ISignalCBFactory(iname) \
   INHERIT_IQI(iname); \
   int (*CreateSignal)(iname *, \
      void (*pfn)(void *pCx), \
      void *pCx, \
      ISignal * *ppiSig, \
      ISignalCtl * *ppiSigCtl \
   )

AEEINTERFACE_DEFINE(ISignalCBFactory);

static __inline uint32 ISignalCBFactory_AddRef(ISignalCBFactory *pif)
{
   return AEEGETPVTBL(pif,ISignalCBFactory)->AddRef(pif);
}

static __inline uint32 ISignalCBFactory_Release(ISignalCBFactory *pif)
{
   return AEEGETPVTBL(pif,ISignalCBFactory)->Release(pif);
}

static __inline int ISignalCBFactory_QueryInterface(ISignalCBFactory *pif, AEEIID iid, void * *ppOut)
{
   return AEEGETPVTBL(pif,ISignalCBFactory)->QueryInterface(pif, iid, ppOut);
}

static __inline int ISignalCBFactory_CreateSignal(ISignalCBFactory *pif, void (*pfn)(void *pCx), void *pCx, ISignal * *ppiSig, ISignalCtl * *ppiSigCtl)
{
   return AEEGETPVTBL(pif,ISignalCBFactory)->CreateSignal(pif, pfn, pCx, ppiSig, ppiSigCtl);
}

#endif /* #if defined(AEEINTERFACE_CPLUSPLUS) */

/*
===============================================================================
INTERFACES DOCUMENTATION
===============================================================================

ISignalCBFactory Interface

Description:

	 This interface allows creation of signal objects that are associated
	 with a C function pointer and a 32-bit parameter.  These signals,
	 when "set", will cause their associated function to be called with the
	 associated parameters.  

    Signal objects enable an asynchronous form of communication.  Signal
	 objects can be used within the local domain (e.g. applet or process), as
	 an asynchronous alternative to ordinary C function pointer callbacks.
	 Unlike C function pointers, they can also be passed to other domains to
	 enable inter-process notification of events.

    ISignalCBFactory is intended for use by clients that reside in a
    single-threaded environment, such as applets.  Clients designed to work
    in a multi-threaded environment should use ISignalFactory.

Usage:

   ISignalCBFactory and the related interfaces ISignal, ISignalHandler, etc.,
   are designed for use with the asynchronous signal framework implemented
   in AEECLSID_SignalFactory and AEECLSID_SignalCBFactory.  This usage
   discussion focuses on that framework.

   Signal objects provide a way to communicate notifications asynchronously.
   The term "notification" refers to a type of message sent to a specific
   destination, but with no payload.  Notifications are used as a kind of
   "wake up call", that can alert a target object that some event has
   happened and give the target object an opportunity to find out more
   information and perform some work.  It is asynchronous in the sense that
   the sender can proceed without waiting on the receiver to receive and
   process the notification.  Instead, the notification will be queued and
   the sender can proceed with other operations.  In a multi-threaded
   environment, a receiving thread can process the message while the sender
   continues to execute.  When the sending and receiving objects live in the
   same single-threaded environment, the message will be delivered after the
   sender returns control to the thread's dispatch loop.

   In a typical scenario, an "observer" (an object that wants to be
   asynchronously notified) creates a signal object and passes it to the
   "observed" object (an object that will provide notification).  Here is a
   function that performs an example registration using a fictional
   IBatteryLevel interface:
   
===pre>
   void MyClass_Register(MyClass *me)
   {
      ISignalCBFactory *piSignalCBFactory;
      ISignal *piSignal;
      int nErr;
      
      nErr = IEnv_CreateInstance(me->piEnv, AEECLSID_SignalCBFactory,
                                 (void**)&piSignalCBFactory);
      if (AEE_SUCCESS == nErr) {
         nErr = ISignalCBFactory_CreateSignal(piSignalCBFactory,
                   MyClass_LevelChangeCB, (void*)me, 
                   &piSignal, &me->piSignalCtl);
      }
      if (AEE_SUCCESS == nErr) {
         nErr = IBatteryLevel_OnLevelChange(me->piBL, piSignal);
      }
      IQI_RELEASEIF(piSignalCBFactory);
      IQI_RELEASEIF(piSignal);
   }
===/pre>

   The returned ISignalCtl reference is retained by MyClass for later use,
   but it has no use for the ISignal after passing it to the batter object.
   
   As the observed object provides notifications, the observer then receives
   invocations or callbacks.  An observer could trigger a notification using
   the following code:

===pre>
       ISignal_Set(me->piSignal);
===/pre>

   This would result in a call to the following function (which was
   registered above):

===pre>
   void MyClass_LevelChangeCB(void *pvCxt)
   {
      MyClass *me = (MyClass*) pvCxt;
      int nLevel;

      nLevel = IBatteryLevel_GetLevel(me->piBL);
      ... update UI ...
      ISignalCtl_Enable(me->piSignalCtl);
   }
===/pre>

   Signal callbacks are called in the environment in which the signal was
   created, even when the object that triggered the notification resides in
   a different process or the kernel.  The callbacks are called
   automatically from the environment's "dispatch loop".  Each process or
   applet in the system has a default dispatch loop that is responsible for
   waiting on inbound invocations and calling the appropriate local object.
   Dispatch loops are provided by the system; developers of applets or
   extension classes do not provide them.

   While the observed object can directly call ISignal_Set() to enqueue a
   signal, as shown above, the observer will more typically hand the signal
   object reference to a signal bus object, and then use ISignalBus to
   enqueue the signal based on either an event-triggered or level-triggered
   basis.  Here is an example of how this would be done:

===pre>
   BatteryLevel_OnLevelChange(IBatteryLevel *piBL, ISignal *piSignal)
   {
       BatteryLevel *me = BatteryLevelFromIBatteryLevel(piBL);
       
       if (piSignal) {
          ISignalBus_AddSignal(me->piSignalBus, piSignal);
       }
   }
===/pre>

   Note that the battery level object does not retain a direct reference to
   the signal, and as a result it can serve an arbitrary number of observers
   without having to maintain a list and allocate memory for each observer.
   When using a signal bus, the the battery object would trigger a
   notification using the following code snippet:

===pre>
       ISignalBus_Strobe(me->piSignalBus);
===/pre>

   The following diagram illustrates the objects that are involved in
   delivering notifications in a case where the observer and the observed
   object both live outside the kernel (in a process or applet), and where a
   signal bus is used:

===pre>
            +-----------+      :       +-----------+
            | Observer  |      :       | Observed  |
            +-----------+      :       +-----------+
    User A      ^  |           :            |          User B
    ............|..|...........:............|................
    Kernel      |  v                        v
            +-----------+        +------------+
            |  Signal   |<-------| Signal Bus |
            +-----------|        +------------+
===/pre>
   
   Uses of ISignalCtl

   We noted earlier that the MyClass example class retained a reference to
   the control interface, ISignalCtl, associated with the signal.  You can
   see in the example callback function, above, one use of this interface:
   re-enabling the signal, allowing the callback to "fire" again.

   Another important use for the control interface is during cleanup.  When
   the MyClass object is deleted it must ensure that the callback will not
   be called after the MyClass instance is deleted.  (Otherwise, it would be
   passed a dangling pointer, and potentially corrupt memory.)  It can do
   this by calling ISignalCtl_Detach() when MyClass_Release() decrements its
   reference count to zero.

   Refer to ISignalCtl for more information on enabling and detaching signals.

   Summary of Signals

   Keep in mind the following properties of signals:
   
   1.  Notifications via ISignalHandler or callbacks are asynchronous, and
   therefore occur sometime after the event that triggered them.  As a
   result, the condition that caused them to be set may not be true when
   they are received, so they must be treated as "hints".
   
   2.  Signal messages carry no payload.  A sender identifies which signal
   to enqueue, and a receiver obtains notification about which of its
   signals were enqueued.  No other information is passed.

   In order to receive notifications of different events, two approaches can
   be taken.  The first is to use a different signal for each event.  The
   alternative is to use one signal for multiple events, and invoke the
   observed object to obtain information about the current state.

   3.  Notifications can be coalesced.  While a signal is already enqueued,
   further attempts to enqueue it will have no affect until after the
   recipient has received notification.  Calling ISignal_Set() a hundred
   times may result in the handler being called a hundred times or only
   once, depending on the relative priorities of the sending and receiving
   threads.
   
   Together, these first three properties allow signals to be both
   asynchronous and reliable.  Signals pre-allocate the memory required for
   queueing when they are created, so clients do not need to deal with
   failure resulting from resource exhaustion.

   4.  Signals have an "enabled" state that the observer manipulates via
   ISignalCtl.  This allows the observer to stop receiving notifications or
   resume receiving notifications without returning to the observed object.
   See ISignalCtl_Enable() for more information on this.

   5.  The observed object does not have a direct reference to any object in
   the process or applet of the observer.  This allows ISignalCtl_Detach()
   to prevent future invocations or callbacks even when the observed object
   is uncooperative.

   6.  The asynchronous nature of signal notification derives from the
   queueing performed by this *implementation* of signals.  There is nothing
   "special" about the ISignal interface.  It is an ordinary synchronous
   interface, and a call to ISignal_Set() operates synchronously in the
   sense that the caller will be blocked until the signal object processes
   the Set() call.
   
===============================================================================

ISignalCBFactory_AddRef()

Description:
    This function is inherited from IQI_AddRef(). 

See Also:
    ISignalCBFactory_Release()

===============================================================================

ISignalCBFactory_Release()

Description:
    This function is inherited from IQI_Release(). 

See Also:
    ISignalCBFactory_AddRef()

===============================================================================

ISignalCBFactory_QueryInterface()

Description:
    This function is inherited from IQI_QueryInterface(). 

===============================================================================

ISignalCBFactory_CreateSignal()

Description:
         Create a signal object that invokes a callback function when set.

Prototype:

   int ISignalCBFactory_CreateSignal(ISignalCBFactory* pif, 
                                     void (*pfn)(void *pCx),
                                     void *pCx,
                                     ISignal** ppiSig, 
                                     ISignalCtl** ppiSigCtl)

Parameters:
   pif: Pointer to an ISignalCBFactory interface
   pfn: Pointer to the function to be invoked when signal is set.
   pCx: Argument to be passed to the callback when signal is set.
   ppiSig : [OUT] Optional. resulting signal object's ISignal interface.
   ppiSigCtl : [OUT] resulting signal object's ISignalCtl interface.

Return Value:

   Error code.

Comments:
   None

Side Effects:
   None

===============================================================================
*/

#endif /* AEEISIGNALCBFACTORY_H */

