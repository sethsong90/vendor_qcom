/*==========================================================================*/
/*!
  @file 
  ps_crit_sect.c

  @brief
  This file provides QTF specific critical section implementation.

  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
*/
/*==========================================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ps_crit_sect_qtf.c#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2009-12-03 hm  Created module.

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "ps_crit_sect.h"
#include "ps_system_heap.h"
#include "amssassert.h"

#if defined(TEST_FRAMEWORK) && defined(FEATURE_QUBE)
#include "qube.h"
#endif


/*===========================================================================

                          PUBLIC FUNCTION DEFINITIONS

===========================================================================*/
#if defined(TEST_FRAMEWORK) && defined(FEATURE_QUBE)
/*---------------------------------------------------------------------------
  QTF crit section definitions.
---------------------------------------------------------------------------*/
void ps_init_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
  int             qm_status;
  qmutex_attr_t   attr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  crit_sect_ptr->handle = 
    (qmutex_t *) ps_system_heap_mem_alloc (sizeof (qmutex_t));

  ASSERT (NULL != crit_sect_ptr->handle);

  memset (crit_sect_ptr->handle, 0, sizeof (qmutex_t));

  qmutex_attr_settype (&attr, QMUTEX_SHARED);

  qm_status = 
    qmutex_create ((qmutex_t *)crit_sect_ptr->handle,
                   &attr);

  /*lint -save -e1534 */
  ASSERT (qm_status == EOK);
  /*lint -restore */

} /* ps_init_crit_section() */


void ps_enter_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
  int             qm_status;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  qm_status = 
    qmutex_lock (* ((qmutex_t *)crit_sect_ptr->handle) );

  /*lint -save -e1534 */
  ASSERT( qm_status == EOK );
  /*lint -restore */

} /* ps_enter_crit_section() */

void ps_leave_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
  int             qm_status;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  qm_status = 
    qmutex_unlock (* ((qmutex_t *)crit_sect_ptr->handle) );

  /*lint -save -e1534 */
  ASSERT( qm_status == EOK );
  /*lint -restore */

} /* ps_leave_crit_section() */

void ps_destroy_crit_section
(
  ps_crit_sect_type*  crit_sect_ptr
)
{
  int             qm_status;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  qm_status = 
    qmutex_delete (* ((qmutex_t *)crit_sect_ptr->handle) );
  
  /*lint -save -e1534 */
  ASSERT( qm_status == EOK );
  /*lint -restore */

  PS_SYSTEM_HEAP_MEM_FREE ((void *) crit_sect_ptr->handle);

} /* ps_destroy_crit_section() */

#endif /* if defined(TEST_FRAMEWORK) && defined(FEATURE_QUBE) */


