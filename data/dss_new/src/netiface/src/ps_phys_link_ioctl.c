/*===========================================================================

                      P S _ P H Y S _ L I N K _ I O C T L. C

DESCRIPTION
  This file contains functions used by various modules to access
  network interface.

EXTERNALIZED FUNCTIONS

Copyright (c) 2002-2008 Qualcomm Technologies, Inc. 
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $PVCSPath: L:/src/asw/MM_DATA/vcs/ps_iface_ioctl.c_v   1.8   13 Feb 2003 14:15:20   ubabbar  $
  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_phys_link_ioctl.c#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
11/29/05    msr    Lint changes.
05/12/05    mct    Lint changes.
05/03/05    msr    Changed signature of ps_phys_link_ioctl()
06/29/04    usb    Initial version.
===========================================================================*/

/*===========================================================================

                       INCLUDE FILES FOR THE MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"
#ifdef FEATURE_DATA_PS
#include "ps_phys_link_ioctl.h"
#include "ps_phys_link.h"
#include "msg.h"
#include "err.h"
#include "dserrno.h"
#include "ds_Utils_DebugMsg.h"


/*===========================================================================

            GLOBAL DEFINITIONS AND DECLARATIONS FOR MODULE

===========================================================================*/

/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains local definitions for constants, macros, types,
variables and other items needed by this module.

===========================================================================*/

/*===========================================================================

                    FORWARD  DECLARATIONS FOR MODULE

===========================================================================*/
/*===========================================================================

                      GLOBAL FUNCTION DEFINITIONS

===========================================================================*/

/*===========================================================================
FUNCTION PS_PHYS_LINK_IOCTL()

DESCRIPTION
  This function performs various operations on the given phys link.
  Typically, these operations are to get or set a value.

DEPENDENCIES
  None.

PARAMETERS

  phys_link_ptr             - Phys link on which the specified operations
                              is to be performed

  ioctl_name                - The operation name

  argval_ptr                - Pointer to operation specific structure

  ps_errno                   - Error code returned in case of failure (Error
                               values are those defined in dserrno.h)

                              DS_EINVAL - Returned when the specified IOCTL
                              does not belong to the common set of IOCTLs
                              and there is no IOCTL mode handler registered
                              for the specified interface.

                              DS_EOPNOTSUPP - Returned by the lower level
                              IOCTL mode handler when specified IOCTL is not
                              supported by the interface. For instance, this
                              would be returned by interfaces that do not
                              support a certain "iface specific common IOCTL"
                              (i.e. these are common IOCTLs, but the
                              implementation is mode specific)

                              DS_EFAULT - This error code is returned if the
                              specified arguments for the IOCTL are correct
                              but an error is encountered while executing
                              the IOCTL.

                              DS_NOMEMORY - This error code is returned if we
                              run out of mempory buffers during execution.

RETURN VALUE
  0 - on success
  -1 - on failure

SIDE EFFECTS
  None.

===========================================================================*/
int ps_phys_link_ioctl
(
  ps_phys_link_type        *phys_link_ptr,
  ps_phys_link_ioctl_type   ioctl_name,
  void                     *argval_ptr,
  sint15                   *ps_errno
)
{
  phys_link_state_type *    phys_state;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  *ps_errno = 0;

  if(!(PS_PHYS_LINK_IS_VALID(phys_link_ptr)))
  {
    LOG_MSG_ERROR("IOCTL 0x%x failed, invalid phys link 0x%p",
              ioctl_name, phys_link_ptr, 0);
    *ps_errno = DS_EBADF;
    return -1;
  }

  if (PS_PHYS_LINK_IOCTL_GET_STATE == ioctl_name)
  {
    phys_state = (phys_link_state_type *)argval_ptr;
    *phys_state = PS_PHYS_LINK_GET_STATE(phys_link_ptr);
    return 0;
  }

  /*---------------------------------------------------------------------
    Phys link specific operation. Call the registered callback.
  ---------------------------------------------------------------------*/
  if(phys_link_ptr->ioctl_f_ptr != NULL)
  {
    LOG_MSG_INFO2("Interface specific ioctl=0x%x, calling callback",
             ioctl_name, 0, 0);
    return phys_link_ptr->ioctl_f_ptr(phys_link_ptr,
                                      ioctl_name,
                                      argval_ptr,
                                      ps_errno);
  }
  else
  {
    LOG_MSG_INFO1("Unsupported IOCTL 0x%x on phys link 0x%p",
             ioctl_name, phys_link_ptr, 0);
    *ps_errno = DS_EINVAL;
    return -1;
  }
} /* ps_phys_link_ioctl() */

#endif /* FEATURE_DATA_PS */
