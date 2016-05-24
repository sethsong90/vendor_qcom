#ifndef AEEILOCALSTORAGE_H
#define AEEILOCALSTORAGE_H
/*======================================================

FILE:      AEEILocalStorage.h

SERVICES:  ILocalStorage

DESCRIPTION:

    ILocalStorage provides an API to store an opaque 
     void pointer keyed by an AEECLSID.

=======================================================
        Copyright 2005-2008 Qualcomm Technologies, Inc.
                 All Rights Reserved.
           Qualcomm Technologies Proprietary and Confidential
=====================================================*/

#include "AEEIQI.h"

#define AEEIID_ILocalStorage 0x0102fa3e

typedef struct LSEntry {
   uint32 reserved[4];
} LSEntry;

#if defined(AEEINTERFACE_CPLUSPLUS)
struct ILocalStorage : public IQI
{
   virtual void CDECL Lock() = 0;
   virtual void CDECL Unlock() = 0;
   virtual int CDECL Get(uint32 dwKey, LSEntry **ppe) = 0;
   virtual int CDECL Add(uint32 dwKey, LSEntry *pe) = 0;
   virtual void CDECL Remove(LSEntry *pe) = 0;
};

#else /* #if defined(AEEINTERFACE_CPLUSPLUS) */

#define INHERIT_ILocalStorage(iname) \
   INHERIT_IQI(iname); \
   void  (*Lock)(iname *pif); \
   void  (*Unlock)(iname *pif); \
   int   (*Get)(iname *pif, uint32 dwKey, LSEntry **ppe); \
   int   (*Add)(iname *pif, uint32 dwKey, LSEntry *pe); \
   void  (*Remove)(iname *pif, LSEntry *pe)

// declare the actual LocalStorage interface
AEEINTERFACE_DEFINE(ILocalStorage);

static __inline uint32 ILocalStorage_AddRef(ILocalStorage* pif)
{
   return AEEGETPVTBL(pif,ILocalStorage)->AddRef(pif);
}

static __inline uint32 ILocalStorage_Release(ILocalStorage* pif)
{
   return AEEGETPVTBL(pif,ILocalStorage)->Release(pif);
}

static __inline int ILocalStorage_QueryInterface(ILocalStorage* pif, 
                                                 AEECLSID cls, void** ppo)
{
   return AEEGETPVTBL(pif,ILocalStorage)->QueryInterface(pif, cls, ppo);
}

static __inline void ILocalStorage_Lock(ILocalStorage* pif)
{
   AEEGETPVTBL(pif,ILocalStorage)->Lock(pif);
}

static __inline void ILocalStorage_Unlock(ILocalStorage* pif)
{
   AEEGETPVTBL(pif,ILocalStorage)->Unlock(pif);
}

static __inline int ILocalStorage_Get(ILocalStorage* pif, uint32 dwKey, 
                                      LSEntry** ppe)
{
   return AEEGETPVTBL(pif,ILocalStorage)->Get(pif, dwKey, ppe);
}

static __inline int ILocalStorage_Add(ILocalStorage* pif, uint32 dwKey, 
                                      LSEntry* pe)
{
   return AEEGETPVTBL(pif,ILocalStorage)->Add(pif, dwKey, pe);
}

static __inline void ILocalStorage_Remove(ILocalStorage* pif, LSEntry* pe)
{
   AEEGETPVTBL(pif,ILocalStorage)->Remove(pif, pe);
}

#endif /* #if defined(AEEINTERFACE_CPLUSPLUS) */

/*
==============================================================================
DATA STRUCTURES DOCUMENTATION
==============================================================================
LSEntry

Description:
   Opaque data space reserved for use by storage implementation.

Definition:

typedef struct LSEntry {
   uint32 reserved[4];
} LSEntry;

Members:
  reserved           - reserved

Comments:
  None

See Also:
  None

==============================================================================
*/

/*
=============================================================================
INTERFACES DOCUMENTATION
=============================================================================

ILocalStorage Interface

Description:

   This interface provides a simple key/value storage facility.  The keys
   are 32-bit numbers, and the values are opaque memory blocks.

   Each stored memory block contains an LSEntry record, whose contents are
   used by the storage implementation.  The intent is that the storage
   implementation can avoid allocation and freeing on Add & Remove by using
   the members of LSEntry to maintain its data structure.

   Users provide only a pointer to the LSEntry when adding a record, and are
   given only the pointer to the LSEntry when they retrieve the record.
   Users are responsible for computing the memory block pointer from the
   LSEntry pointer.

   A simple use model is to place the LSEntry at the beginning of the memory
   buffer by declaring it as the first member of a structure.  This simplifies
   recovery of the data block pointer, reducing it to a simple cast.

===pre>
  eg : struct my_ls_data_type{
         LSEntry lse;
         ... ;  // other members of data buffer.
       };
===/pre>

   Users of the ILocalStorage object must identify appropriate key that
   prevents collision with other users of the object.  Unless the storage
   class specifies otherwise, the recommended approach for assigning keys is
   to use an address from your module image (such as the address of a
   function or a const char array).  An address that is local to the
   client's code will prevent collision with others.  This could be an
   address of a function.  Collision is not considered an error but only the
   last added is returned during a call to get.  If the last added is
   removed then the second to last is returned during a call to get, and so
   forth.

===pre>
eg : 

   static int save_to_ls(struct my_ls_data_type *pdata)
   {
      ...
      nErr = ILocalStorage_Add(piLocalStorage, (uint32)save_to_ls, 
                               &pdata->lse);
      ...
   }
===/pre>

=============================================================================

ILocalStorage_AddRef()

Description:
    This function is inherited from IQI_AddRef(). 

See Also:
    ILocalStorage_Release()

=============================================================================

ILocalStorage_Release()

Description:
    This function is inherited from IQI_Release(). 

See Also:
    ILocalStorage_AddRef()

=============================================================================

ILocalStorage_QueryInterface()

Description:
    This function is inherited from IQI_QueryInterface(). 

See Also:
    None

=============================================================================

ILocalStorage_Lock()

Description: 

   Lock the storage for exclusive access by the current thread.  The caller
   might block in this request until the lock is available.

   After obtaining the lock, the caller must call ILocalStorage_Unlock() to
   release it.

Prototype:
   void ILocalStorage_Lock(ILocalStorage *pif);

Parameters:
   pif : pointer to the ILocalStorage interface

Return value:
   None

Comments:

   Single-thread-only implementations provide null implementations of Lock
   and UnLock.  That is, they immediately return success without doing
   anything.


See Also:
   ILocalStorage_Unlock()

=============================================================================

ILocalStorage_Unlock()

Description: 

   Unlock the object.

Prototype:
   void ILocalStorage_Unlock(ILocalStorage *pif);

Parameters:
   pif : pointer to the ILocalStorage interface

Return value:
   None

Comments:  
   None.

See Also:
   ILocalStorage_Lock()

=============================================================================

ILocalStorage_Get()

Description: 

   Find and return the last added data pointer associated to the key that has
   not been removed.

Prototype:
   int ILocalStorage_Get(ILocalStorage *pif, uint32 dwKey, LSEntry** ppe);

Parameters:
   pif   : pointer to the ILocalStorage interface
   dwKey : a key against which the entry was previously added.
   ppe   : [out] pointer to be filled with the pointer to entry.

Return value:
   AEE_SUCCESS if data is found (has been set)
   AEE_ENOSUCH if data can't be found (has not been set, or has been removed)

Comments:  
   None

See Also:
   ILocalStorage_Add()
   ILocalStorage_Remove()

=============================================================================

ILocalStorage_Add()

Description: 

   This function stores an entry for the specified key value.

   Multiple entries may be stored using the same key.  When adding multiple
   entries using the same key, the entries stack.  The most recently-added
   entry will be the one returned by Get().

   This function may overwrite some or all of the members of LSEntry.  While
   the entry remains in the storage, the caller must not modify any of the
   members of LSEntry.

Prototype:
   int ILocalStorage_Add(ILocalStorage *pif, uint32 dwKey, LSEntry* pe)

Parameters:
   pif   : pointer to the ILocalStorage interface
   dwKey : key value.
   pe    : pointer to local storage entry

Return value:
   AEE_SUCCESS if data is set
   AEE_EFAILED/other Error code

Comments:
   
  Use an address from the code that uses the data buffer as the key.            

See Also:
   ILocalStorage_Get()
   ILocalStorage_Remove()

=============================================================================

ILocalStorage_Remove()

Description: 

   This function removes an entry from the storage.

   After Remove(), subsequent calls to Get() will not retrieve this entry,
   and the caller is free to overwrite or free the memory.

Prototype:
   void ILocalStorage_Remove(ILocalStorage *pif, LSEntry* pe)

Parameters:
   pif   : pointer to the ILocalStorage interface
   pe    : pointer to local storage entry

Return value:
   None

Comments:  
   None.

See Also:
   ILocalStorage_Get()
   ILocalStorage_Set()

=============================================================================
*/
#endif /* #ifndef AEEILOCALSTORAGE_H */

