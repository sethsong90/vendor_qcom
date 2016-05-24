#ifndef AEEISINGLETON_H
#define AEEISINGLETON_H
/*======================================================
        Copyright 2004-2006 Qualcomm Technologies, Inc.
               All Rights Reserved.
            Qualcomm Technologies Proprietary and Confidential
=====================================================*/

#include "AEEIQI.h"

typedef int SingletonCIFunc(void *pcxCI, AEECLSID cls, void **ppif);

#define AEEIID_ISingleton 0x0101ed1f

#if defined(AEEINTERFACE_CPLUSPLUS)
struct ISingleton : public IQI
{
    virtual int CDECL CreateInstance (AEECLSID cls, void** obj, \
                                         SingletonCIFunc *pfnCI,  \
                                         void* pcxCI) = 0;
    virtual uint32 CDECL Ref (AEECLSID cls, uint32* nRefs, int32 nInc) = 0;
};

#else /* #if defined(AEEINTERFACE_CPLUSPLUS) */

#define INHERIT_ISingleton(iname) \
   INHERIT_IQI(iname); \
   int    (*CreateInstance)(iname* pif,\
                            AEECLSID cls, void** ppif,\
                            SingletonCIFunc* pfnCI, void* pcxCI);\
   uint32 (*Ref)(iname* pif, AEECLSID cls, uint32* puRefs, int nInc)

// declare the actual Singleton interface
AEEINTERFACE_DEFINE(ISingleton);

static __inline uint32 ISingleton_AddRef(ISingleton* pif)
{
   return AEEGETPVTBL(pif,ISingleton)->AddRef(pif);
}
static __inline uint32 ISingleton_Release(ISingleton* pif)
{
   return AEEGETPVTBL(pif,ISingleton)->Release(pif);
}

static __inline uint32 ISingleton_QueryInterface(ISingleton* pif, 
                                                 AEECLSID cls, void** ppo)
{
   return AEEGETPVTBL(pif,ISingleton)->QueryInterface(pif, cls, ppo);
}

static __inline int ISingleton_CreateInstance(ISingleton* pif, AEECLSID cls,
                                              void** ppo, SingletonCIFunc* pfnCI,
                                              void* pcxCI)
{
   return AEEGETPVTBL(pif,ISingleton)->CreateInstance(pif, cls, ppo, 
                                                     pfnCI, pcxCI);
}

static __inline uint32 ISingleton_Ref(ISingleton* pif, AEECLSID cls, 
                                      uint32* puRefs, int nInc)
{
   return AEEGETPVTBL(pif,ISingleton)->Ref(pif, cls, puRefs, nInc);
}

#endif /* #if defined(AEEINTERFACE_CPLUSPLUS) */

/*=====================================================================
  DATA STRUCTURE DOCUMENTATION
=======================================================================
=======================================================================

SingletonCIFunc

Description:

  SingletonCIFunc is a CreateInstance callback passed to 
   ISingleton_CreateInstance().

Definition:

  typedef int SingletonCIFunc(void *pcxCI, AEECLSID cls, void **ppif);

Parameters:
   pcxCI: user context data, the same pcxCI as is passed to 
          ISingleton_CreateInstance().
   cls: the AEECLSID of the class to be created
   ppif: [out] pointer to pointer to new instance

Comments:
  None

See Also:
  ISingleton_CreateInstance()

=======================================================================*/

/*=====================================================================
INTERFACES DOCUMENTATION
=======================================================================
=======================================================================
ISingleton Interface

This interface facilitates implementing classes that support singleton
instances.  Objects that export ISingleton allow creation of singletons
within their scope.

A class A is said to be a "singleton within the scope of object X" when
there is at most one instance of A per X.  The first client requesting A
will cause it to be instantiated, and subsequent requestors will get an
AddRef'ed pointer to the same instance.  Once all clients release their
references, the singleton instance will be deleted.

ISingleton encapsulated the tracking of class IDs and instances and the
thread-safety issues for doing addref, release, create and delete.

ISingleton's methods are thread-safe.

To use ISingleton, follow these steps:

 1. Modify your CreateInstance function to defer to ISingleton so that 
    ISingleton can return an existing instance, if any.

 Original:

===pre>
   // exported newfunc
   int Foo_CreateInstance(IEnv* piEnv, AEECLSID cls, void** ppo)
   {
      int    nErr;
      Foo*   pif = 0;
   
      nErr = IENV_ERRMALLOCREC(piEnv, Foo, &pif);
   
      if (AEE_SUCCESS == nErr) {
         nErr = Foo_CtorZ(pif, piEnv);
         if (AEE_SUCCESS != nErr) {
            Foo_Dtor(pif);
            IENV_FREEIF(piEnv,pif);
         }
      }
   
      *ppo = pif;
   
      return nErr;
   }
===/pre>

 Using ISingleton:
===pre>
   typedef struct FooCI
   {
      IEnv*       piEnv;
      ISingleton* piSing;
   } FooCI;
   
   static int Foo_SingletonCI(void* p, AEECLSID cls, void** ppo)
   {
      int    nErr;
      Foo*   pif = 0;
      FooCI* pci = (FooCI*)p;
   
      nErr = IENV_ERRMALLOCREC(pci->piEnv, Foo, &pif);
   
      if (AEE_SUCCESS == nErr) {
         // pass the instance of ISingleton to CtorZ(), need it later
         nErr = Foo_CtorZ(pif, pci->piEnv, pci->piSing);
         if (AEE_SUCCESS != nErr) {
            Foo_Dtor(pif);
            IENV_FREEIF(pci->piEnv,pif);
         }
      }
   
      *ppo = pif;
   
      return nErr;
   }

   // exported newfunc
   int Foo_CreateInstance(IEnv* piEnv, AEECLSID cls, void** ppo)
   {
      int   nErr;
      FooCI ci;
   
      ci.piSing = 0;
      ci.piEnv  = piEnv;
   
      nErr = IEnv_QueryInterface(piEnv, AEEIID_ISingleton, 
                                 (void**)&ci.piSing);
   
      if (AEE_SUCCESS == nErr) {
         nErr = ISingleton_CreateInstance(ci.piSing, AEECLSID_FOO, ppo,
                                          Foo_SingletonCI, &ci);
      }

      IQI_RELEASEIF(ci.piSing);
      
      return nErr;
   }

===/pre>

 2. Modify your refcount functions to defer to ISingleton, so ISingleton
     knows when your refcount reaches zero.

===pre>
   static uint32 Foo_AddRef(Foo* pif)
   {
      return ISingleton_Ref(pif->piSing, AEECLSID_FOO, &pif->uRefs, 1);
   }
   static uint32 Foo_Release(Foo* pif)
   {
      uint32 uRefs = ISingleton_Ref(pif->piSing, AEECLSID_FOO, &pif->uRefs, -1);
      if (0 == uRefs) {
         Foo_Delete(pif);
      }
      return uRefs;
   }
===/pre>

 That's it!  The ISingleton interface ensures that only one instance of
 your object is instantiated, that refcounting is thread-safe, and that
 when the last reference to your object is released, the instance is
 removed from the ISingleton.
 


=============================================================================

ISingleton_AddRef()

Description:
   This function is inherited from IQI_AddRef(). 

See Also:
   ISingleton_Release()

=============================================================================

ISingleton_Release()


Description:
   This function is inherited from IQI_Release(). 

See Also:
   ISingleton_AddRef()

=============================================================================

ISingleton_QueryInterface()


Description:
   This function is inherited from IQI_QueryInterface().

See Also:
   ISingleton_AddRef(), ISingleton_Release()

=============================================================================

ISingleton_CreateInstance()

Description:
   This function wraps the actual CreateInstance (or new function) of a 
    singleton class.  If ISingleton has already created the class, it
    is addref'd and returned, otherwise, pfnCI is called to create the
    class.

Prototype:
   int ISingleton_CreateInstance(ISingleton *pif,
                            AEECLSID cls, void **ppo,
                            SingletonCIFunc* pfnCI, void *pcxCI);


Parameters:
    pif : [in] : Pointer to the ISingleton Interface object.
    cls : [in] : 32-bit ClassID of the requested interface.
    ppo : [out] Pointer to be filled with a reference to the instance
    pfnCI : [in] where to call to create the instance if not already created
    pcxCI : [in] : Pointer to user data to be passed to pfnCI

Return Value:
    AEE_SUCCESS, class instance found or created
    other AEEError, on failure, depends on pfnCI

Comments:
    None

See Also:
    IModInfo_CreateInstance()

=============================================================================

ISingleton_Ref()

Description:
   This function is a thread-safe, per-class refcount modifier function.  
     When the refcount for the indicated class id goes to 0, ISingleton
     stops tracking the instance.

Prototype:
   uint32 ISingleton_Ref(ISingleton *pif,
                         AEECLSID cls, uint32 *puRefs, int nInc)

Parameters:
    pif : [in] : Pointer to the ISingleton Interface object.
    cls : [in] : 32-bit ClassID of the ref to be modified, used to 
                untrack if Ref() returns 0
    puRefs: [in/out] pointer to the singleton's refcount, is adjusted by 
                     nInc
    nInc : [in] value to add to *puRefs, use 1 for "AddRef" and -1 for 
              "Release"

Return Value:
    the resulting value of puRefs

Comments:
    If ISingleton_Ref() returns 0, the ISingleton has stopped tracking
     the singleton instance, and the caller can safely delete himself.

See Also:
    IQI_AddRef(), IQI_Release()

=======================================================================*/


#endif /* #ifndef AEEISINGLETON_H */
