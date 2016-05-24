/*===========================================================================

                          P S _ S T A T _ M E M . C

GENERAL DESCRIPTION
  This program specifically deals with statistics at the mem layer of the
  data servcies stack

EXTERNALIZED FUNCTIONS
  ps_stat_get_mem
    This function supplies the required mem stats.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

Copyright (c) 2005 by Qualcomm Technologies, Inc.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ps_stat_mem.c#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
08/15/05   kr      Initial version

===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "customer.h"
#include "comdef.h"
#ifdef FEATURE_DATA_PS
#include "IxErrno.h"
#include "err.h"

#include "ps_stat_mem.h"
#include "ps_stat_common.h"
#include "ds_Utils_DebugMsg.h"

#include "ps_mem.h"

/*===========================================================================
FUNCTION PS_STAT_GET_MEM()

DESCRIPTION
  This function supplies the required mem layer statistics. The first
  argument passed to the function is an enum that denotes the metric of
  interest.The second argument will be  a pointer to the address of the
  ps_mem_pool_info element corresponding to the pool_id. The third argument
  points to the memory location where the results will be copied.The result
  (statistic) will be copied to this memory address. The fourth argument
  passes the length of memory allocated for the return value and should be
  greater than or equal to the memory size of the statistic requested.

DEPENDENCIES
  None

RETURN VALUE
  E_SUCCESS (0) for success and any other number for failure . The error
  codes are as defined in errno.h.

PARAMETERS
  stat            - IN - type of stat
  instance_ptr    - IN - pointer that has the value of socket descriptor
  return_value    - IN - pointer for return value
                  - OUT- return value (statistics)
  ret_len         - length of memory allocated

SIDE EFFECTS
  None
===========================================================================*/
errno_enum_type ps_stat_get_mem
(
  ps_stat_mem_enum_type  stat,
  void                   *instance_ptr,
  void                   *return_value,
  uint16                 ret_len
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Check Instance pointer; Instance pointer should not be null
  -------------------------------------------------------------------------*/

  if( instance_ptr == NULL)
  {
    LOG_MSG_ERROR(" Instance pointer should NOT be NULL for MEM stats",0,0,0);
    return E_INVALID_ARG ;
  }

#ifdef FEATURE_DATA_PS_MEM_STAT_LOGGING
  /*-------------------------------------------------------------------------
    Call ps_mem_get_stat. This is because ps_mem_pool_info_type is
    strictly local to ps_mem.c and we do not want to expose it to any
    other file. Hence, we do not define ps_stat_mem_i_table in this file
  -------------------------------------------------------------------------*/
  return ps_mem_get_stat(stat,
                         instance_ptr,
                         return_value,
                         ret_len
                        );
#else
  return E_NOT_SUPPORTED;
#endif /* FEATURE_DATA_PS_MEM_STAT_LOGGING */
}

/*===========================================================================
FUNCTION PS_STAT_RESET_MEM()

DESCRIPTION
  This function resets the instance statistics associated with the specified
  instance.

DEPENDENCIES
  None

RETURN VALUE
  E_SUCCESS (0) for success and any other number for failure . The error
  codes are as defined in errno.h.

PARAMETERS
  handle: Handle to the instance

SIDE EFFECTS
  None
===========================================================================*/
void ps_stat_reset_mem
(
  int32        handle
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Call ps_mem_reset_stat. This is because ps_mem_pool_info_type is
    strictly local to ps_mem.c and we do not want to expose it to any
    other file.
  -------------------------------------------------------------------------*/
  ps_mem_reset_stat(handle);

  return;
}
#endif /* FEATURE_DATA_PS */

