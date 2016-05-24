/*===========================================================================
  FILE: ds_Utils_FullPrivSet.cpp

  OVERVIEW: This file provides implementation of the FullPrivSet class.

  DEPENDENCIES: None

                Copyright (c) 2011 Qualcomm Technologies, Inc.
                All Rights Reserved.
                Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_FullPrivSet.cpp#2 $
  $DateTime: 2011/07/25 23:24:15 $$Author: c_rpidap $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2011-07-15 rp QShrink2 changes: MSG_* macros in inline functions are removed.
  2010-02-16 ts Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Utils_MemManager.h"
#include "ds_Utils_FullPrivSet.h"
using namespace ds::Error;
using namespace ds::Utils;


/*===========================================================================

                         PUBLIC DATA DECLARATIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  Declaration of static member of FullPrivSet class
---------------------------------------------------------------------------*/
FullPrivSet * FullPrivSet::fullPrivSetPtr = 0;


/*===========================================================================

                         PUBLIC MEMBER FUNCTIONS

===========================================================================*/
FullPrivSet * FullPrivSet::Instance
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Singelton pattern is used.
    Allocate a FullPrivSet object if it is not already allocated.
  -------------------------------------------------------------------------*/
  if (0 == fullPrivSetPtr)
  {
    fullPrivSetPtr = new FullPrivSet();

    if (0 == fullPrivSetPtr)
    {
      LOG_MSG_ERROR( "No mem for FullPrivSet", 0, 0, 0);
      goto bail;
    }
  }
  
  (void)fullPrivSetPtr->AddRef();

  LOG_MSG_FUNCTION_EXIT( "Returning 0x%p", fullPrivSetPtr, 0, 0);
  return fullPrivSetPtr;

bail:
  return 0;

} /* FullPrivSet::Instance() */


ds::ErrorType CDECL FullPrivSet::CheckPrivileges
(
  AEEPRIVID *aprivs,
  int nPrivsLen
)
{
  (void)aprivs;
  (void)nPrivsLen;
  /*------------------------------------------------------------------------
    Always return AEE_SUCCESS to indicate full privileges
  -------------------------------------------------------------------------*/
  return AEE_SUCCESS;
}

ds::ErrorType CDECL FullPrivSet::GetPrivileges
(
  AEEPRIVID *aprivs,
  int nPrivsLen,
  int* pnPrivsLenReq
)
{
  (void)aprivs;
  (void)nPrivsLen;
  (void)pnPrivsLenReq;

  return QDS_EOPNOTSUPP;
}

ds::ErrorType CDECL FullPrivSet::CreateSubset
(
  const AEEPRIVID* aprivSubset,
  int nNumPrivs,
  IPrivSet** ppipiSubset
)
{
  (void)aprivSubset;
  (void)nNumPrivs;
  (void)ppipiSubset;
  return QDS_EOPNOTSUPP;
}
 
/*===========================================================================

                          PROTECTED MEMBER FUNCTIONS

===========================================================================*/
FullPrivSet::FullPrivSet
(
  void
)
throw()
: refCnt(0)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_EXIT( "Created FullPrivSet 0x%p", this, 0, 0);

} /* FullPrivSet::FullPrivSet() */


FullPrivSet::~FullPrivSet
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  fullPrivSetPtr = 0;
  LOG_MSG_FUNCTION_EXIT( "Deleted FullPrivSet 0x%p", this, 0, 0);

} /* FullPrivSet::~FullPrivSet() */


/*===========================================================================

                         PRIVATE MEMBER FUNCTIONS

===========================================================================*/
inline void * FullPrivSet::operator new
(
  unsigned int numBytes
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void)numBytes;
  return ps_mem_get_buf (PS_MEM_DS_UTILS_FULL_PRIV_SET);
} /* FullPrivSet::operator new() */


inline void FullPrivSet::operator delete
(
  void *  bufPtr
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == bufPtr)
  {
    /*LOG_MSG_ERROR( "NULL ptr", 0, 0, 0);*/
    ASSERT( 0);
    return;
  }

  PS_MEM_FREE(bufPtr);
  return;

} /* FullPrivSet::operator delete() */



#endif /* FEATURE_DATA_PS */
