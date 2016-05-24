/*===========================================================================
  FILE: DS_Sock_ICMPErrInfo.cpp

  OVERVIEW: This file provides implementation of the ICMPErrInfo class.

  DEPENDENCIES: None

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datacommon/dssock/rel/09.02.01/src/DS_Sock_ICMPErrInfo.cpp#1 $
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
#include "DS_Sock_ICMPErrInfo.h"
#include "DS_Utils_DebugMsg.h"
#include "ps_mem.h"

using namespace DS::Sock;
using namespace DS::Error;


/*===========================================================================

                     PUBLIC MEMBER FUNCTIONS

===========================================================================*/
void * ICMPErrInfo::operator new
(
  unsigned int numBytes
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void)numBytes;
  return ps_mem_get_buf( PS_MEM_ICMP_ERR_INFO_TYPE);

} /* ICMPErrInfo::operator new() */


void ICMPErrInfo::operator delete
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

} /* ICMPErrInfo::operator delete() */


ICMPErrInfo::ICMPErrInfo
(
  struct ps_sock_extended_err  psExtendedErrInfo,
  SockAddrStorageType          _addr

) :
    addr( _addr)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO1( "Obj 0x%x", this, 0, 0);

  extendedErrInfo.error_number = psExtendedErrInfo.ee_errno;
  extendedErrInfo.origin       = psExtendedErrInfo.ee_origin;
  extendedErrInfo.type         = psExtendedErrInfo.ee_type;
  extendedErrInfo.code         = psExtendedErrInfo.ee_code;
  extendedErrInfo.info         = psExtendedErrInfo.ee_info;

  /*-------------------------------------------------------------------------
    Using explicit scope to shut up lint. It complains because AddRef() is
    declared as virtual to shut up compiler
  -------------------------------------------------------------------------*/
  (void) ICMPErrInfo::AddRef();
  return;

} /* ICMPErrInfo::ICMPErrInfo() */


DS::ErrorType CDECL ICMPErrInfo::GetAncID
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

  *ancIDPtr = AncData::ICMP_ERROR_INFO;
  return SUCCESS;

} /* ICMPErrInfo::GetAncID() */


DS::ErrorType CDECL ICMPErrInfo::SetAncID
(
  DS::Sock::AncDataIDType  ancID
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  (void)ancID;
  return SUCCESS;

} /* ICMPErrInfo::SetAncID() */


DS::ErrorType CDECL ICMPErrInfo::GetExtendedErr
(
  ExtendedErrType *  extendedErrInfoPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Obj 0x%x", this, 0, 0);

  if (0 == extendedErrInfoPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL arg, obj 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  *extendedErrInfoPtr = extendedErrInfo;

  LOG_MSG_FUNCTION_EXIT( "Success, obj 0x%x", this, 0, 0);
  return SUCCESS;

} /* ICMPErrInfo::GetExtendedErr() */


DS::ErrorType CDECL ICMPErrInfo::GetAddr
(
  SockAddrStorageType *  addrPtr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Obj 0x%x", this, 0, 0);

  if (0 == addrPtr)
  {
    LOG_MSG_INVALID_INPUT( "NULL arg, obj 0x%x", this, 0, 0);
    return DSS_EFAULT;
  }

  *addrPtr = addr;

  LOG_MSG_FUNCTION_EXIT( "Success, obj 0x%x", this, 0, 0);
  return SUCCESS;

} /* ICMPErrInfo::GetAddr() */


DS::ErrorType CDECL ICMPErrInfo::QueryInterface
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

    case AEEIID_IICMPErrInfo:
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

} /* ICMPErrInfo::QueryInterface() */

#endif /* FEATURE_DATA_PS */
