/*===========================================================================
  FILE: DS_Sock_SocketFactoryPriv.cpp

  OVERVIEW: This file provides implementation of the SocketFactoryPriv class.

  DEPENDENCIES: None

  Copyright (c) 2008 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_SocketFactoryPriv.cpp#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2008-05-14 msr Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#include "DS_Sock_SocketFactoryPriv.h"
#include "DS_Utils_DebugMsg.h"
#include "ps_mem.h"

using namespace DS::Error;
using namespace DS::Sock;



/*===========================================================================

                         PUBLIC DATA DECLARATIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  Declaration of static member of SocketFactoryPriv class
---------------------------------------------------------------------------*/
SocketFactoryPriv * SocketFactoryPriv::sockFactoryPrivPtr = 0;


/*===========================================================================

                     PUBLIC MEMBER FUNCTIONS

===========================================================================*/
SocketFactoryPriv * SocketFactoryPriv::CreateInstance
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Allocate a SocketFactoryPriv object if it is not already allocated.

    Since factory pattern is used, a new object is not allocated each time
    CreateInstance() is called
  -------------------------------------------------------------------------*/
  if (0 == sockFactoryPrivPtr)
  {
    sockFactoryPrivPtr = new SocketFactoryPriv();

    if (0 == sockFactoryPrivPtr)
    {
      LOG_MSG_ERROR( "No mem for SocketFactoryPriv", 0, 0, 0);
      goto bail;
    }
  }

  LOG_MSG_FUNCTION_EXIT( "Returning 0x%p", sockFactoryPrivPtr, 0, 0);
  return sockFactoryPrivPtr;

bail:
  return 0;

} /* SocketFactoryPriv::CreateInstance() */


/*===========================================================================

                          PROTECTED MEMBER FUNCTIONS

===========================================================================*/
bool SocketFactoryPriv::IsICMPSupported
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return true;
} /* SocketFactoryPriv::IsICMPSupported() */


/*===========================================================================

                         PRIVATE MEMBER FUNCTIONS

===========================================================================*/
void * SocketFactoryPriv::operator new
(
  unsigned int numBytes
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void) numBytes;
  return ps_mem_get_buf( PS_MEM_SOCKET_FACTORY_PRIV_TYPE);

} /* SocketFactoryPriv::operator new() */


void SocketFactoryPriv::operator delete
(
  void *  bufPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == bufPtr)
  {
    LOG_MSG_ERROR( "NULL ptr", 0, 0, 0);
    ASSERT( 0);
    return;
  }

  PS_MEM_FREE( sockFactoryPrivPtr);
  return;

} /* SocketFactoryPriv::operator delete() */

#endif /* FEATURE_DATA_PS */
