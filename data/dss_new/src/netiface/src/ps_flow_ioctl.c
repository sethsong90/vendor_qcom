/*===========================================================================

                      P S _ F L O W _ I O C T L. C

DESCRIPTION
  Defines API to use ps_flow specific IOCTLs.

EXTERNALIZED FUNCTIONS
  PS_FLOW_IOCTL()
    Function which supports ps_flow specific IOCTLs

  PS_FLOW_DEFAULT_LOGICAL_FLOW_IOCTL_HDLR()
    Function which performs the default handling of ioctls for logical flows

INITIALIZING AND SEQUENCING REQUIREMENTS
  None

Copyright (c) 2005 by Qualcomm Technologies, Inc.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_flow_ioctl.c#1 $
  $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
09/01/07    ssh    QoS support for logical ifaces
04/25/06    msr    L4/Tasklock code review changes
02/22/06    msr    Using single critical section
02/06/06    msr    Updated for L4 tasklock/crit sections.
08/16/05    msr    Fixed PS_BRANCH_TASKFREE()
08/12/05    msr    Updated messages
05/12/05    ks     Fixed Lint errors.
05/03/05    msr    TASKLOCKing ps_flow_ioctl()
04/17/05    msr    Created the file.
===========================================================================*/

/*===========================================================================

                       INCLUDE FILES FOR THE MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#include "ps_flow_ioctl.h"
#include "ps_flow.h"
#include "ps_flowi.h"
#include "msg.h"
#include "err.h"
#include "amssassert.h"
#include "dserrno.h"
#include "ps_qos_defs.h"
#include "dss_iface_ioctl.h"
#include "ps_system_heap.h"
#include "ds_Utils_DebugMsg.h"


/*===========================================================================

                               PUBLIC FUNCTIONS

===========================================================================*/
/*===========================================================================
FUNCTION PS_FLOW_IOCTL()

DESCRIPTION
  This function performs various operations on the given flow.
  Typically, these operations are to get or set a value.

PARAMETERS
  flow_ptr   : ps_flow on which the specified operation is to be performed
  ioctl_name : the operation name
  argval_ptr : pointer to operation specific structure
  ps_errno   : error code returned in case of failure (Error values are
               those defined in dserrno.h)

                 DS_EINVAL - Returned when the specified IOCTL does not
                 belong to the common set of IOCTLs and there is no IOCTL
                 mode handler registered for the specified interface.

                 DS_EOPNOTSUPP - Returned by the lower level IOCTL mode
                 handler when specified IOCTL is not supported by the
                 interface. For instance, this would be returned by
                 interfaces that do not support a certain
                 "ps_flow specific common IOCTL" (i.e. these are common
                 IOCTLs, but the implementation is mode specific)

                 DS_EFAULT - This error code is returned if the specified
                 arguments for the IOCTL are correct but an error is
                 encountered while executing the IOCTL.

                 DS_NOMEMORY - This error code is returned if we run out of
                 mempory buffers during execution.

RETURN VALUE
   0 : on success
  -1 : on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None.
===========================================================================*/
int ps_flow_ioctl
(
  ps_flow_type         * flow_ptr,
  ps_flow_ioctl_type     ioctl_name,
  void                 * argval_ptr,
  int16                * ps_errno
)
{
  dss_iface_ioctl_primary_qos_get_granted_flow_spec_type * get_flow_ptr = NULL;

  ip_flow_type         * temp_flow_ptr = NULL;
  ps_flow_type         * assoc_flow_ptr;
  int                    ret_val;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2("PS FLOW IOCTL 0x%x flow 0x%p", ioctl_name, flow_ptr, 0);

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_FLOW_IS_VALID(flow_ptr))
  {
    LOG_MSG_ERROR("PS FLOW IOCTL, 0x%x failed, invalid ps_flow, 0x%p",
              ioctl_name, flow_ptr, 0);
    *ps_errno = DS_EBADF;
    return -1;
  }

  switch (ioctl_name)
  {
    case PS_FLOW_IOCTL_QOS_GET_STATUS:
      *(ps_flow_state_enum_type*)argval_ptr = PS_FLOW_GET_STATE(flow_ptr);
      return 0;

    case PS_FLOW_IOCTL_QOS_GET_GRANTED_FLOW_SPEC:
      /* This IOCTL should be handled at the DSS layer itself. It is
         not supported anymore from PS Flow layer. */
      ASSERT (0);
      *ps_errno = DS_EINVAL;
      return -1;

    case PS_FLOW_IOCTL_QOS_GET_GRANTED_FLOW_SPEC2:

      /*-------------------------------------------------------------------
        Return granted flow spec if ps_flow is ACTIVATED, else return 0
        since no QOS is granted by the network in any other state
      -------------------------------------------------------------------*/
      get_flow_ptr =
        (dss_iface_ioctl_primary_qos_get_granted_flow_spec_type *)argval_ptr;

      temp_flow_ptr = PS_FLOWI_GET_RX_GRANTED_FLOW2(flow_ptr);
      if (NULL != temp_flow_ptr)
      {
        (void) memcpy (&get_flow_ptr->rx_ip_flow,
                       temp_flow_ptr,
                       sizeof (ip_flow_type));
      }
      else
      {
        memset (&get_flow_ptr->rx_ip_flow, 0, sizeof(ip_flow_type));
      }

      temp_flow_ptr = PS_FLOWI_GET_TX_GRANTED_FLOW2(flow_ptr);
      if (NULL != temp_flow_ptr)
      {
        (void) memcpy (&get_flow_ptr->tx_ip_flow,
                       temp_flow_ptr,
                       sizeof (ip_flow_type));
      }
      else
      {
        memset (&get_flow_ptr->tx_ip_flow, 0, sizeof(ip_flow_type));
      }

      return 0;

    default:
      /* No-op */
      break;

  } /* Common IOCTL switch */

  assoc_flow_ptr = flow_ptr;
  while (PS_FLOW_IS_VALID(flow_ptr))
  {
    assoc_flow_ptr = flow_ptr;
    if (assoc_flow_ptr->ps_flow_ioctl_f_ptr != NULL)
    {
      break;
    }

    flow_ptr = PS_FLOWI_GET_ASSOC_PS_FLOW(assoc_flow_ptr);
  }

  if (assoc_flow_ptr->ps_flow_ioctl_f_ptr != NULL)
  {
    LOG_MSG_INFO2("PS FLOW IOCTL 0x%x calling MH callback on flow 0x%p",
                  ioctl_name, assoc_flow_ptr, 0);
    ret_val = assoc_flow_ptr->ps_flow_ioctl_f_ptr(assoc_flow_ptr,
                                                  ioctl_name,
                                                  argval_ptr,
                                                  ps_errno);
  }
  else
  {
    LOG_MSG_INFO1("PS FLOW IOCTL 0x%x is not supported on ps_flow 0x%p",
                  ioctl_name, assoc_flow_ptr, 0);
    *ps_errno = DS_EINVAL;
    ret_val = -1;
  }

  return ret_val;

} /* ps_flow_ioctl() */



/*===========================================================================
FUNCTION PS_FLOW_DEFAULT_LOGICAL_FLOW_IOCTL_HDLR()

DESCRIPTION
  This function performs the default handling of ioctls for logical flows.

PARAMETERS
  flow_ptr   : ps_flow on which the specified operation is to be performed
  ioctl_name : the operation name
  argval_ptr : pointer to operation specific structure
  ps_errno   : error code returned in case of failure (Error values are
               those defined in dserrno.h)

                 DS_EINVAL - Returned when the specified IOCTL does not
                 belong to the common set of IOCTLs and there is no IOCTL
                 mode handler registered for the specified interface.

                 DS_EOPNOTSUPP - Returned by the lower level IOCTL mode
                 handler when specified IOCTL is not supported by the
                 interface. For instance, this would be returned by
                 interfaces that do not support a certain
                 "ps_flow specific common IOCTL" (i.e. these are common
                 IOCTLs, but the implementation is mode specific)

                 DS_EFAULT - This error code is returned if the specified
                 arguments for the IOCTL are correct but an error is
                 encountered while executing the IOCTL.

                 DS_NOMEMORY - This error code is returned if we run out of
                 mempory buffers during execution.

RETURN VALUE
   0 : on success
  -1 : on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None.
===========================================================================*/
int ps_flow_default_logical_flow_ioctl_hdlr
(
  ps_flow_type         * flow_ptr,
  ps_flow_ioctl_type     ioctl_name,
  void                 * argval_ptr,
  int16                * ps_errno
)
{
#ifdef FEATURE_DATA_PS_QOS
  ps_flow_type                       ** assoc_flow_ptr_arr;
  ps_flow_type                        * assoc_flow_ptr;
  ps_flow_ioctl_qos_suspend_ex_type   * qos_ioctl_arg_suspend_ex_ptr;
  ps_flow_ioctl_qos_resume_ex_type    * qos_ioctl_arg_resume_ex_ptr;
  ps_flow_ioctl_qos_release_ex_type   * qos_ioctl_arg_release_ex_ptr;
  ps_flow_ioctl_qos_suspend_ex_type     assoc_qos_ioctl_arg_suspend_ex;
  ps_flow_ioctl_qos_resume_ex_type      assoc_qos_ioctl_arg_resume_ex;
  ps_flow_ioctl_qos_release_ex_type     assoc_qos_ioctl_arg_release_ex;
  int                                   retval = 0;
  unsigned long                         num_bytes_to_alloc;
  uint8                                 idx;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL errno", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_FLOW_IS_VALID(flow_ptr))
  {
    LOG_MSG_ERROR("PS FLOW IOCTL, 0x%x failed, invalid ps_flow, 0x%p",
              ioctl_name, flow_ptr, 0);
    *ps_errno = DS_EBADF;
    return -1;
  }

  assoc_flow_ptr = PS_FLOW_GET_ASSOC_PS_FLOW(flow_ptr);
  if (!PS_FLOW_IS_VALID(assoc_flow_ptr))
  {
    LOG_MSG_ERROR("Invalid associated flow ptr", 0, 0, 0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  switch( ioctl_name )
  {
    case PS_FLOW_IOCTL_QOS_SUSPEND:
      (void) ps_flow_suspend_cmd(flow_ptr, ps_errno, NULL);
      retval = ps_flow_ioctl(assoc_flow_ptr, ioctl_name, argval_ptr, ps_errno);
      break;

    case PS_FLOW_IOCTL_QOS_RESUME:
      (void) ps_flow_resume_cmd(flow_ptr, ps_errno, NULL);
      retval = ps_flow_ioctl(assoc_flow_ptr, ioctl_name, argval_ptr, ps_errno);
      break;

    case PS_FLOW_IOCTL_QOS_RELEASE:
      (void) ps_flow_go_null_cmd(flow_ptr, ps_errno, NULL);
      retval = ps_flow_ioctl(assoc_flow_ptr, ioctl_name, argval_ptr, ps_errno);
      break;

    case PS_FLOW_IOCTL_QOS_SUSPEND_EX:
      qos_ioctl_arg_suspend_ex_ptr =
        (ps_flow_ioctl_qos_suspend_ex_type *) argval_ptr;

      num_bytes_to_alloc =
        sizeof(ps_flow_type *) * qos_ioctl_arg_suspend_ex_ptr->num_flows;
      assoc_flow_ptr_arr =
        (ps_flow_type **) ps_system_heap_mem_alloc(num_bytes_to_alloc);

      if (assoc_flow_ptr_arr == NULL)
      {
        LOG_MSG_ERROR("Couldn't alloc memory", 0, 0, 0);
        *ps_errno = DS_ENOMEM;
        break;
      }

      for (idx = 0; idx < qos_ioctl_arg_suspend_ex_ptr->num_flows; idx++)
      {
        (void) ps_flow_suspend_cmd(qos_ioctl_arg_suspend_ex_ptr->flows_ptr[idx],
                                   ps_errno,
                                   NULL);

        assoc_flow_ptr_arr[idx] =
          PS_FLOWI_GET_ASSOC_PS_FLOW(
            qos_ioctl_arg_suspend_ex_ptr->flows_ptr[idx]);
      }

      assoc_qos_ioctl_arg_suspend_ex.num_flows =
        qos_ioctl_arg_suspend_ex_ptr->num_flows;
      assoc_qos_ioctl_arg_suspend_ex.flows_ptr = assoc_flow_ptr_arr;

      retval = ps_flow_ioctl(assoc_flow_ptr,
                             ioctl_name,
                             &assoc_qos_ioctl_arg_suspend_ex,
                             ps_errno);

      PS_SYSTEM_HEAP_MEM_FREE(assoc_flow_ptr_arr);
      break;

    case PS_FLOW_IOCTL_QOS_RESUME_EX:
      qos_ioctl_arg_resume_ex_ptr =
        (ps_flow_ioctl_qos_resume_ex_type *) argval_ptr;

      num_bytes_to_alloc =
        sizeof(ps_flow_type *) * qos_ioctl_arg_resume_ex_ptr->num_flows;
      assoc_flow_ptr_arr =
        (ps_flow_type **) ps_system_heap_mem_alloc(num_bytes_to_alloc);

      if (assoc_flow_ptr_arr == NULL)
      {
        LOG_MSG_ERROR("Couldn't alloc memory", 0, 0, 0);
        *ps_errno = DS_ENOMEM;
        break;
      }

      for (idx = 0; idx < qos_ioctl_arg_resume_ex_ptr->num_flows; idx++)
      {
        (void) ps_flow_resume_cmd(qos_ioctl_arg_resume_ex_ptr->flows_ptr[idx],
                                  ps_errno,
                                  NULL);

        assoc_flow_ptr_arr[idx] =
          PS_FLOWI_GET_ASSOC_PS_FLOW(
            qos_ioctl_arg_resume_ex_ptr->flows_ptr[idx]);
      }

      assoc_qos_ioctl_arg_resume_ex.num_flows =
        qos_ioctl_arg_resume_ex_ptr->num_flows;
      assoc_qos_ioctl_arg_resume_ex.flows_ptr = assoc_flow_ptr_arr;

      retval = ps_flow_ioctl(assoc_flow_ptr,
                             ioctl_name,
                             &assoc_qos_ioctl_arg_resume_ex,
                             ps_errno);

      PS_SYSTEM_HEAP_MEM_FREE(assoc_flow_ptr_arr);
      break;

    case PS_FLOW_IOCTL_QOS_RELEASE_EX:
      qos_ioctl_arg_release_ex_ptr =
        (ps_flow_ioctl_qos_release_ex_type *) argval_ptr;

      num_bytes_to_alloc =
        sizeof(ps_flow_type *) * qos_ioctl_arg_release_ex_ptr->num_flows;
      assoc_flow_ptr_arr =
        (ps_flow_type **) ps_system_heap_mem_alloc(num_bytes_to_alloc);

      if (assoc_flow_ptr_arr == NULL)
      {
        LOG_MSG_ERROR("Couldn't alloc memory", 0, 0, 0);
        *ps_errno = DS_ENOMEM;
        break;
      }

      for (idx = 0; idx < qos_ioctl_arg_release_ex_ptr->num_flows; idx++)
      {
        (void) ps_flow_go_null_cmd(qos_ioctl_arg_release_ex_ptr->flows_ptr[idx],
                                   ps_errno,
                                   NULL);

        assoc_flow_ptr_arr[idx] =
          PS_FLOWI_GET_ASSOC_PS_FLOW(
            qos_ioctl_arg_release_ex_ptr->flows_ptr[idx]);
      }

      assoc_qos_ioctl_arg_release_ex.num_flows =
        qos_ioctl_arg_release_ex_ptr->num_flows;
      assoc_qos_ioctl_arg_release_ex.flows_ptr = assoc_flow_ptr_arr;

      retval = ps_flow_ioctl(assoc_flow_ptr,
                             ioctl_name,
                             &assoc_qos_ioctl_arg_release_ex,
                             ps_errno);

      PS_SYSTEM_HEAP_MEM_FREE(assoc_flow_ptr_arr);
      break;

    case PS_FLOW_IOCTL_GET_TX_QUEUE_LEVEL:
    case PS_FLOW_IOCTL_PRIMARY_QOS_MODIFY:
    case PS_FLOW_IOCTL_PRIMARY_QOS_GET_GRANTED_FLOW_SPEC:
    case PS_FLOW_IOCTL_HDR_GET_RMAC3_INFO:
    case PS_FLOW_IOCTL_707_GET_TX_STATUS:
    case PS_FLOW_IOCTL_707_GET_INACTIVITY_TIMER:
    case PS_FLOW_IOCTL_707_SET_INACTIVITY_TIMER:
    case PS_FLOW_IOCTL_GET_MODIFY_RESULT:
    case PS_FLOW_IOCTL_GET_FLOW_UPDATED_INFO_CODE:
    case PS_FLOW_IOCTL_GET_PRIMARY_QOS_MODIFY_RESULT:
      retval = ps_flow_ioctl(assoc_flow_ptr, ioctl_name, argval_ptr, ps_errno);
      break;

    default:
      LOG_MSG_FATAL_ERROR("Unsupported ioctl 0x%x", ioctl_name, 0, 0 );
      *ps_errno = DS_EOPNOTSUPP;
      retval = -1;
      break;
  } /* switch( ioctl_name ) */

  return retval;
#else
  return -1;
#endif /* FEATURE_DATA_PS_QOS */

} /* ps_flow_default_logical_flow_ioctl_hdlr() */
#endif /* FEATURE_DATA_PS */
