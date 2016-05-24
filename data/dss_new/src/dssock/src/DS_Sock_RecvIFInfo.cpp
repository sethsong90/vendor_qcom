/*===========================================================================
  FILE: DS_Sock_RecvIFInfo.cpp

  OVERVIEW: This file provides implementation of the RecvIFInfo class.

  DEPENDENCIES: None

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_RecvIFInfo.cpp#1 $
  $DateTime: 2009/09/28 14:45:31 $$Author: dmudgal $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2009-03-02 msr Created module

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"
#include "target.h"

#ifdef FEATURE_DATA_PS
#include "DS_Sock_RecvIFInfo.h"
#include "DS_Utils_DebugMsg.h"
#include "ps_mem.h"

using namespace DS::Sock;
using namespace DS::Error;


/*===========================================================================

                     PUBLIC MEMBER FUNCTIONS

===========================================================================*/
void * RecvIFInfo::operator new
(
  unsigned int numBytes
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void)numBytes;
  return ps_mem_get_buf( PS_MEM_RECV_IF_INFO_TYPE);

} /* RecvIFInfo::operator new() */


void RecvIFInfo::operator delete
(
  void *  bufPtr
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == bufPtr)
  {
    LOG_MSG_ERROR( "NULL ptr", 0, 0, 0);
    ASSERT( 0);
    return;
  }

  PS_MEM_FREE( bufPtr);
  return;

} /* RecvIFInfo::operator delete() */


RecvIFInfo::RecvIFInfo
(
  uint32  _recvIFHandle
) :
    recvIFHandle( _recvIFHandle)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1( "Obj 0x%x, RecvIF %d", this, recvIFHandle, 0);

  /*-------------------------------------------------------------------------
    Using explicit scope to shut up lint. It complains because AddRef() is
    declared as virtual to shut up compiler
  -------------------------------------------------------------------------*/
  (void) RecvIFInfo::AddRef();
  return;

} /* RecvIFInfo::RecvIFInfo() */


DS::ErrorType CDECL RecvIFInfo::GetAncID
(
  DS::Sock::AncDataIDType *  ancIDPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (0 == ancIDPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL arg", 0, 0, 0);
    return DSS_EFAULT;
  }

  *ancIDPtr = AncData::RECV_IF_INFO;
  return SUCCESS;

} /* RecvIFInfo::GetAncID() */


DS::ErrorType CDECL RecvIFInfo::SetAncID
(
  DS::Sock::AncDataIDType  ancID
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void)ancID;
  return SUCCESS;

} /* RecvIFInfo::SetAncID() */


DS::ErrorType CDECL RecvIFInfo::GetRecvIFHandle
(
  uint32 *  recvIFHandlePtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Obj 0x%x", this, 0, 0);

  if (0 == recvIFHandlePtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL arg, obj 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  *recvIFHandlePtr = recvIFHandle;

  LOG_MSG_FUNCTION_EXIT( "Success, obj 0x%x", this, 0, 0);
  return SUCCESS;

} /* RecvIFInfo::GetRecvIFHandle() */


DS::ErrorType CDECL RecvIFInfo::QueryInterface
(
  AEEIID   iid,
  void **  objPtrPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Obj 0x%x iid %d", this, iid, 0);

  if (0 == objPtrPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL arg, obj 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  switch (iid)
  {
    case AEEIID_IAncData:
    {
      *objPtrPtr = static_cast <IAncData *> ( this);
      (void) AddRef();
      break;
    }

    case AEEIID_IRecvIFInfo:
    {
      *objPtrPtr = this;
      (void) AddRef();
      break;
    }

    case AEEIID_IQI:
    {
      *objPtrPtr = static_cast <IQI *> ( this);
      (void) AddRef();
      break;
    }

    default:
    {
      *objPtrPtr = 0;
      LOG_MSG_INVALID_INPUT( "Unknown iid %d, obj 0x%x", iid, this, 0);
      return AEE_ECLASSNOTSUPPORT;
    }
  } /* switch */

  LOG_MSG_FUNCTION_EXIT( "Success, obj 0x%x", this, 0, 0);
  return SUCCESS;

} /* RecvIFInfo::QueryInterface() */

#endif /* FEATURE_DATA_PS */
