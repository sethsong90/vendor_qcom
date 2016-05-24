#ifndef AEEIWEAKREF_H
#define AEEIWEAKREF_H
/*
=======================================================================
                  Copyright 2005-2008 Qualcomm Technologies, Inc.
                         All Rights Reserved.
                      Qualcomm Technologies Proprietary and Confidential
=======================================================================
*/
#include "AEEIQI.h"

#define AEEIID_IWeakRef 0x0104709f

#if defined(AEEINTERFACE_CPLUSPLUS)
struct IWeakRef : public IQI
{
   virtual int CDECL QueryReferent(AEEIID iid, void** ppo) = 0;
};

#else /* #if defined(AEEINTERFACE_CPLUSPLUS) */

#define INHERIT_IWeakRef(iname) \
   INHERIT_IQI(iname);\
   int (*QueryReferent) (iname*  me, AEEIID iid, void** ppo)

AEEINTERFACE_DEFINE(IWeakRef);

static __inline uint32 IWeakRef_AddRef(IWeakRef* me)
{
   return AEEGETPVTBL(me,IWeakRef)->AddRef(me);
}

static __inline uint32 IWeakRef_Release(IWeakRef* me)
{
   return AEEGETPVTBL(me,IWeakRef)->Release(me);
}

static __inline int IWeakRef_QueryInterface(IWeakRef* me, 
                                                    AEEIID iid, void** ppo)
{
   return AEEGETPVTBL(me,IWeakRef)->QueryInterface(me, iid, ppo);
}

static __inline int IWeakRef_QueryReferent(IWeakRef* me, AEEIID iid, 
                                           void** ppo)
{
   return AEEGETPVTBL(me,IWeakRef)->QueryReferent(me, iid, ppo);
}

#endif /* #if defined(AEEINTERFACE_CPLUSPLUS) */

/*=====================================================================
INTERFACES DOCUMENTATION
=======================================================================
=======================================================================
IWeakRef Interface

Description:

   This interface exposes the functionality of a weak reference.  A weak
   reference is an object that keeps track of some other object without
   preventing the deletion of that object.  The tracked object is referred to
   a the "referent".
   
   A weak reference can be used to attempt to obtain a reference to the
   referent, using IWeakRef_QueryReferent().  This operation will either
   succeed, returning a "strong" (ordinary) reference, or fail safely.

   Note that in terms of the object model, the weak reference is itself an
   instance distinct from the referent.  The weak reference has its own set
   of supported interfaces (usually only IWeakRef), and one cannot navigate
   between the weak reference and its referent via QueryInterface().

   In order to obtain a weak reference, a client can query the referent for
   ISupportsWeakRef, and then use that interface to retrieve the weak
   reference object.

Usage:

   When building object-based systems in a multi-threaded environment, weak
   references are a valuable tool for dealing with reference cycles
   ("circular reference counts").  Reference cycles can occur commonly
   whenever there is bidirectional communication between two objects.

   Ordinarily, a cycle of references will introduce memory leaks.  When two
   objects refer to each other, each object will have a non-zero reference
   count even after all other objects have released their references to
   those two objects.  This can also occur when the two objects refer to
   each other indirectly through other objects.

 ===pre>
        +-----------+   
        |     A     |
        +-----------+   
            |   ^
            v   |        
        +-----------+   
        |     B     |
        +-----------|   
===/pre>

   In a single-threaded environment this situation is often dealt with using
   uncounted references.  Software designers ensure that the reference
   counts in their system form a directed acyclic graph.  Any pointer that
   points "back" to introduce a reference cycle must refrain from
   incrementing a reference count.  Having a pointer without a reference
   count is potentially dangerous, so wherever these exist the programmer
   must be careful to remove them *before* deleting the referenced object.

   In a multi-threaded environment it is impractical to ensure that other
   uncounted references are not in use by other threads, and as a result
   safely deleting an object is problematic.  Weak reference can fulfill the
   role of uncounted references in a safe manner.

   Here is an example of using a weak reference to perform notification:
   
===pre>
   nErr = IWeakRef_QueryReferent(piWeakRef, AEEIID_ISignalHandler,
                                 (void**)&piSH);
   if (AEE_SUCCESS == nErr) {
      // The referent is still alive, and now we hold a strong reference
      // to it in 'piSH' to keep the object alive while we are using it.
      ISignalHandler_Notify(piSH, uA, uB);
      ISignalHandler_Release(piSH);
   } else {
      // The referent has been deleted or is being deleted.  We hold
      // no reference to it.
   }
===/pre>

    In order to obtain a weak reference to an object, the object's
    implementation must support weak references, because the weak reference
    mechanics are tightly coupled with the object's destruction logic.
    AEECObjWR.h provides a C language base class implementation that
    supports weak references.
    
See Also:
   ISupportsWeakRef
   AEECObjWR.h

=============================================================================

IWeakRef_AddRef()

Description:
   This function is inherited from IQI_AddRef(). 

See Also:
   IWeakRef_Release()

=============================================================================

IWeakRef_Release()


Description:
   This function is inherited from IQI_Release(). 

See Also:
   IWeakRef_AddRef()

=============================================================================

IWeakRef_QueryInterface()


Description:
   This function is inherited from IQI_QueryInterface().

See Also:
   IWeakRef_AddRef(), IWeakRef_Release()

=============================================================================

IWeakRef_QueryReferent()

Description:

   This function queries the referenced object for the requested interface.

   If the referent's reference count is non-zero and the referent supports
   the requested interface, this attempt will succeed and return a new
   "strong" (ordinary) reference, incrementing the reference count.

   If the referent's reference count has dropped to zero, the attempt will
   fail safely.  This will indicate that the referent has been deleted or is
   being deleted.

Prototype:
   int IWeakRef_QueryReferent(IWeakRef* me, AEEIID iid, 
                                           void** ppo)

Parameters:
    me : [in] : Pointer to the IWeakRef Interface object.
    iid : [in] : ID of the requested interface.
    ppo : [out] Pointer to be filled with a reference to the object 

Return Value:
    AEE_SUCCESS: QueryInterface was successful
    AEE_EFAILED: reference no longer exists
    other: some other failure

Comments:
    None

See Also:
    IQI_QueryInterface()

=======================================================================*/


#endif //AEEIWEAKREF_H

