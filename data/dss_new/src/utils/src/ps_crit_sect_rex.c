/*==========================================================================*/
/*!
  @file 
  ps_crit_sect.c

  @brief
  This file provides REX specific critical section implementation.

  Copyright (c) 2009 - 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*==========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ps_crit_sect_rex.c#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  12/24/10   ua  Fixed compilation errors. 
  12/03/09   hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ps_crit_sect.h"
#include "ps_system_heap.h"
#include "amssassert.h"

#if !defined(TEST_FRAMEWORK) || !defined(FEATURE_QUBE)
#include "rex.h"
#endif


/*===========================================================================

                          PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
#if !defined(TEST_FRAMEWORK) || !defined(FEATURE_QUBE)
/*---------------------------------------------------------------------------
  REX specific critical section implementation
---------------------------------------------------------------------------*/
void ps_init_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  crit_sect_ptr->handle = 
    (rex_crit_sect_type *) 
      ps_system_heap_mem_alloc (sizeof (rex_crit_sect_type));

  (void) memset (crit_sect_ptr->handle, 0, sizeof (rex_crit_sect_type));

  rex_init_crit_sect ((rex_crit_sect_type *) 
                      crit_sect_ptr->handle);
  
} /* ps_init_crit_section() */


void ps_enter_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  rex_enter_crit_sect ((rex_crit_sect_type *) 
                       crit_sect_ptr->handle);

} /* ps_enter_crit_section() */

void ps_leave_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  rex_leave_crit_sect ((rex_crit_sect_type *) 
                       crit_sect_ptr->handle);

} /* ps_leave_crit_section() */

void ps_destroy_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  (void) rex_del_crit_sect ((rex_crit_sect_type *) 
                            crit_sect_ptr->handle);

  PS_SYSTEM_HEAP_MEM_FREE (crit_sect_ptr->handle);
  
} /* ps_destroy_crit_section() */

#endif /* if !defined(TEST_FRAMEWORK) || !defined(FEATURE_QUBE) */

