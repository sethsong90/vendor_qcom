/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                           D S S O C K . C P P

GENERAL DESCRIPTION
  This header file defines the class which implements IDSSock interface

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

Copyright (c) 2008 by Qualcomm Technologies, Inc.  All Rights Reserved.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ds_Utils_CritSect.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/
/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ds_Utils_CSSupport.h"
#include "ds_Utils_DebugMsg.h"
#include "AEEStdErr.h"
#include "ds_Utils_CritSect.h"
#include "ps_crit_sect.h"

using namespace ds::Utils;
using namespace ds::Error;

/*===========================================================================

                     EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/
CritSect::CritSect
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3 ("Initing crit sect 0x%p", this, 0, 0);
  PS_INIT_CRIT_SECTION(&mutex);
  refCnt = 1;

} /* CritSect() */

CritSect::~CritSect
(
  void
)
throw()
{

  LOG_MSG_INFO3 ("Destroying crit sect 0x%p", this, 0, 0);
  PS_DESTROY_CRIT_SECTION(&mutex);

} /* ~CritSect() */

void CritSect::Enter
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3  ("Crit sect 0x%p", this, 0, 0);
  PS_ENTER_CRIT_SECTION(&mutex);

} /* Enter() */

int CritSect::TryEnter
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3  ("Crit sect 0x%p", this, 0, 0);
  return QDS_EOPNOTSUPP;

} /* Enter() */

void CritSect::Leave
(
  void
)
throw()
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO3  ("Crit sect 0x%p", this, 0, 0);
  PS_LEAVE_CRIT_SECTION(&mutex);

} /* Leave() */



