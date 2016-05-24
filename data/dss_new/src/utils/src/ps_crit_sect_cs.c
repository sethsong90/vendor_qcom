/*==========================================================================*/
/*!
  @file
  ps_crit_sect.c

  @brief
  This file provides REX specific critical section implementation.

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*==========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ps_crit_sect_cs.c#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2009-12-03 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "amssassert.h"
#include "ps_crit_sect.h"

#include "AEEICritSect.h"
#include "AEECCritSect.h"

#include "AEEIEnv.h"
#include "AEEenv.h"
#include "AEEStdErr.h"

/*===========================================================================

                          PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
void ps_init_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
  IEnv* piEnv = 0;
  int nErr;
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  nErr = env_GetCurrent(&piEnv);
  ASSERT(AEE_SUCCESS == nErr);

  nErr = IEnv_CreateInstance(piEnv, AEECLSID_CCritSect,
                             (void**)&crit_sect_ptr->handle);
  ASSERT(AEE_SUCCESS == nErr);

} /* ps_init_crit_section() */

void ps_enter_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ICritSect_Enter((ICritSect*)crit_sect_ptr->handle);

} /* ps_enter_crit_section() */

void ps_leave_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ICritSect_Leave(crit_sect_ptr->handle);

} /* ps_leave_crit_section() */

void ps_destroy_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  IQI_RELEASEIF(crit_sect_ptr->handle);

} /* ps_destroy_crit_section() */


