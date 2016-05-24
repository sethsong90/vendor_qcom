/*===========================================================================

                          P S _ S T A T _ F L O W . C

GENERAL DESCRIPTION
  This program specifically deals with statistics at the flow layer of the
  data servcies stack

EXTERNALIZED FUNCTIONS
  ps_stat_get_flow
    This function supplies the required flow stats.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

Copyright (c) 2005-2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_stat_flow.c#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
12/08/10   rp      RAM optimization changes.
04/11/07   scb     Fixed High LINT errors
02/27/07   scb     Added changes to fix RCVT compilation warning
02/27/07   scb     Fixed high LINT errors
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

#include "ps_stat_flow.h"
#include "ps_stat_common.h"
#include "ps_stat_logging.h"

#include "ps_flow.h"
#include "ps_flowi.h"
#include "ps_iface.h"
#include "ps_utils.h"
#include "ps_crit_sect.h"
#include "ds_Utils_DebugMsg.h"

#include <stddef.h>
#include <string.h>


/*===========================================================================

                             INTERNAL DEFINITIONS

===========================================================================*/

/*===========================================================================
MACRO FLOW_STAT_OFFSET_SIZE(INDEX,FIELD, SIZE)

DESCRIPTION
  This macro fills the offset and size fields of the ps_stat_flow_i_table.
  INDEX is used only for readability, not for usage

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
#define FLOW_STAT_OFFSET_SIZE(INDEX,FIELD, SIZE)                \
{																\
  FPOS_STAT(ps_flow_type,FIELD),								\
  sizeof(SIZE)													\
}

/*---------------------------------------------------------------------------
  FLOW stats access table
-------------------------------------------------------------------------- */
const ps_stat_access_table_s_type ps_stat_flow_i_table[PS_STAT_FLOW_ALL + 1] = 
{
  FLOW_STAT_OFFSET_SIZE(0,flow_i_stats.pkts_tx,   uint32),
  FLOW_STAT_OFFSET_SIZE(1,flow_i_stats.bytes_tx,  uint64),
  FLOW_STAT_OFFSET_SIZE(2,flow_private.state,	  uint16),
  /*---------------------------------------------------------------------------
  Indices 3 and 4 are unused. Fill with zeros
  - Refer ps_stat_flow_enum_type
  -------------------------------------------------------------------------- */
  STAT_FILL_NULL(3),
  STAT_FILL_NULL(4)
};

/*===========================================================================

                             INTERNAL FORWARD DECLARATIONS

===========================================================================*/

static errno_enum_type ps_stat_get_flow_is_default
(
  ps_stat_flow_enum_type  stat,
  ps_flow_type            *flow_ptr,
  void                    *return_value,
  uint16                  ret_len
);

static errno_enum_type ps_stat_get_flow_instance_all
(
  ps_stat_flow_enum_type  stat,
  ps_flow_type            *flow_ptr,
  void                    *return_value,
  uint16                  ret_len
);

static errno_enum_type ps_stat_get_flow_control_block
(
  int               handle,
  ps_flow_type   ** ps_flow_ptr_ptr
);

static void ps_stat_get_flow_desc_log_pkt
(
  int32                         handle
);

static void ps_stat_get_flow_stat_log_pkt
(
  int32                         handle
);

/*===========================================================================
FUNCTION PS_STAT_GET_FLOW_IS_DEFAULT()

DESCRIPTION
  This function supplies the flow is_default statistic

DEPENDENCIES
  None

RETURN VALUE

  E_SUCCESS (0) for success and any other number for failure . The error
  codes are as defined in errno.h.

PARAMETERS
  stat            - IN - type of stat
  flow_ptr       -  IN - pointer that points to the FLOW control block
  return_value    - IN - pointer for return value
                  - OUT- return value (statistics)
  ret_len         - length of memory allocated

SIDE EFFECTS
  None
===========================================================================*/
errno_enum_type ps_stat_get_flow_is_default
(
  ps_stat_flow_enum_type  stat,
  ps_flow_type            *flow_ptr,
  void                    *return_value,
  uint16                  ret_len
)
{
   uint32 *is_default;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   ps_flow_capability_enum_type flow_capability = PS_FLOW_CAPABILITY_DEFAULT;
  /*-------------------------------------------------------------------------
    validate 'stat'; only PS_STAT_FLOW_IS_DEFAULT should be passed to
    this function
  -------------------------------------------------------------------------*/
  if (stat != PS_STAT_FLOW_IS_DEFAULT)
  {
    LOG_MSG_ERROR("Invalid stat value passed to ps_stat_get_flow_i_all",0,0,0);
    ASSERT(0);
    return E_NOT_ALLOWED ;
  }

  /*-------------------------------------------------------------------------
    validate flow_ptr : should not be NULL
  -------------------------------------------------------------------------*/
  if (flow_ptr == NULL)
  {
    LOG_MSG_ERROR("ps_stat_get_flow_is_default:Null value of flow_ptr passed",
              0,0,0);
    ASSERT(0);
    return E_NOT_ALLOWED ;
  }

  /*-------------------------------------------------------------------------
    return_value validation : should not be null
  -------------------------------------------------------------------------*/

  if (return_value == NULL)
  {
    LOG_MSG_ERROR("return_value should not be NULL ",0,0,0);
    return E_NOT_ALLOWED ;
  }

  /*-------------------------------------------------------------------------
    check the sizeof the stat requested. return appropriate error
  -------------------------------------------------------------------------*/

  if (ret_len < sizeof(uint32))
  {
    LOG_MSG_ERROR(" Insufficient memory allocated for ptr return_value",0,0,0);
    return E_NOT_ALLOWED ;
  }

  is_default = (uint32 *) return_value ;
  *is_default = PS_FLOW_GET_CAPABILITY(flow_ptr,flow_capability);
  return E_SUCCESS ;
} /* ps_stat_get_flow_is_default */


/*===========================================================================
FUNCTION PS_STAT_GET_FLOW_INSTANCE_ALL()

DESCRIPTION
  This function supplies all the FLOW statistics for the instance handle
  supplied. This is needed because of the need to extract and format data
  available in the flow control block

DEPENDENCIES
  None

RETURN VALUE

  E_SUCCESS (0) for success and any other number for failure . The error
  codes are as defined in errno.h.

PARAMETERS
  stat            - IN - type of stat
  flow_ptr        - IN - pointer that points to the FLOW control block
  return_value    - IN - pointer for return value
                  - OUT- return value (statistics)
  ret_len         - length of memory allocated

SIDE EFFECTS
  None
===========================================================================*/
errno_enum_type ps_stat_get_flow_instance_all
(
  ps_stat_flow_enum_type  stat,
  ps_flow_type            *flow_ptr,
  void                    *return_value,
  uint16                  ret_len
)
{
   ps_stat_flow_i_s_type *flow_i_stats;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

   ps_flow_capability_enum_type flow_capability = PS_FLOW_CAPABILITY_DEFAULT;
  /*-------------------------------------------------------------------------
    validate 'stat'; only PS_STAT_FLOW_ALL should be passed to this function
  -------------------------------------------------------------------------*/
  if (stat != PS_STAT_FLOW_ALL)
  {
    LOG_MSG_ERROR("Invalid stat value passed to ps_stat_get_flow_i_all",0,0,0);
    ASSERT(0);
    return E_NOT_ALLOWED ;
  }

  /*-------------------------------------------------------------------------
    validate flow_ptr : should not be NULL
  -------------------------------------------------------------------------*/
  if (flow_ptr == NULL)
  {
    LOG_MSG_ERROR("ps_stat_get_flow_instance_all:Null value of flow_ptr passed",
              0,0,0);
    ASSERT(0);
    return E_NOT_ALLOWED ;
  }

  /*-------------------------------------------------------------------------
    return_value validation : should not be null
  -------------------------------------------------------------------------*/

  if (return_value == NULL)
  {
    LOG_MSG_ERROR("return_value should not be NULL ",0,0,0);
    return E_NOT_ALLOWED ;
  }

  /*-------------------------------------------------------------------------
    check the sizeof the stat requested. If the size is zero, the stat is
    invalid in the current context. return appropriate error
  -------------------------------------------------------------------------*/

  if (ret_len < sizeof(ps_stat_flow_i_s_type))
  {
    LOG_MSG_ERROR(" Insufficient memory allocated for ptr return_value",0,0,0);
    return E_NOT_ALLOWED ;
  }

  flow_i_stats = (ps_stat_flow_i_s_type *) return_value ;

  flow_i_stats->pkts_tx    = flow_ptr->flow_i_stats.pkts_tx ;
  flow_i_stats->bytes_tx   = flow_ptr->flow_i_stats.bytes_tx;
  flow_i_stats->state      = flow_ptr->flow_private.state ;
  flow_i_stats->is_default = PS_FLOW_GET_CAPABILITY(flow_ptr, flow_capability);

  flow_ptr->flow_i_stats.queried = PS_STAT_QUERIED;

  return E_SUCCESS ;
} /* ps_stat_get_flow_instance_all */


/*===========================================================================
FUNCTION PS_STAT_GET_FLOW_CONTROL_BLOCK()

DESCRIPTION
  This function converts a QXDM handle into a flow control block pointer.
  It is necessary to convert a QXDM handle into a control block pointer here
  because the common stats module does not have access to the flow fctns.

DEPENDENCIES
  None

RETURN VALUE
  E_SUCCESS (0) for success and any other number for failure . The error
  codes are as defined in errno.h.

PARAMETERS
  handle          - IN - index of the ctrl blk of interest
  ps_flow_ptr_ptr    - OUT- ctrl blk ptr.  void** so it can be used directly
                         in the associated get functions

SIDE EFFECTS
  None
===========================================================================*/
errno_enum_type ps_stat_get_flow_control_block
(
  int               handle,
  ps_flow_type   ** ps_flow_ptr_ptr
)
{
  ps_iface_type  * iface_ptr;
  uint32           iface_handle;
  uint32           flow_handle;
#ifdef FEATURE_DATA_PS_QOS
  void           * sec_flow_handle;
  void           * new_sec_flow_handle;
#endif /* FEATURE_DATA_PS_QOS */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Convert the input handle into the correct iface and flow handles
    Flow handle is the lowest 8 bits
    Iface handle is the next 8 bits
  -------------------------------------------------------------------------*/
  // TODO: use RFC-style to depict this
  flow_handle = (handle & 0xFF);
  iface_handle = (handle >> 8);

  /*-------------------------------------------------------------------------
    Sanity check the iface handle value
  -------------------------------------------------------------------------*/
  if (MAX_SYSTEM_IFACES <= iface_handle)
  {
    LOG_MSG_ERROR("Invalid handle: %d", handle, 0, 0);
    return E_INVALID_ARG;
  }

  /*-------------------------------------------------------------------------
    Convert the handle into the instance control block
  -------------------------------------------------------------------------*/
  iface_ptr = global_iface_ptr_array[iface_handle];

  /*-------------------------------------------------------------------------
    Iface instance has not been initialized yet
  -------------------------------------------------------------------------*/
  if (FALSE == PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface ptr, handle 0x%x", handle, 0, 0);
    return E_INVALID_ARG;
  }

  /*-------------------------------------------------------------------------
    Sanity check the flow handle value
  -------------------------------------------------------------------------*/
  if ((iface_ptr->iface_private.flow.default_flow_ptr)->flow_private.cookie ==
       flow_handle)
  {
    *ps_flow_ptr_ptr = iface_ptr->iface_private.flow.default_flow_ptr;
  }
#ifdef FEATURE_DATA_PS_QOS
  else
  {
    /*-------------------------------------------------------------------
      Navigate the list of secondary flow control blocks
      Must occur within a critical section since flows could be added
      or removed on the fly, and we don't want to memset invalid memory
    -------------------------------------------------------------------*/
    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    sec_flow_handle = ps_iface_get_sec_flow_handle(iface_ptr);

    while ( (sec_flow_handle != NULL) &&
            (ps_iface_get_sec_flow_by_handle(iface_ptr,
                                             sec_flow_handle,
                                             ps_flow_ptr_ptr,
                                             &new_sec_flow_handle)) &&
            ((*ps_flow_ptr_ptr)->flow_private.cookie != flow_handle))

    {
      sec_flow_handle = new_sec_flow_handle;
    }

    PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

    if (NULL == ps_flow_ptr_ptr)
    {
      return E_INVALID_ARG;
    }
  }
#endif /* FEATURE_DATA_PS_QOS */

  return E_SUCCESS;
} /* ps_stat_get_flow_control_block */


/*===========================================================================
FUNCTION PS_STAT_GET_FLOW_DESC_LOG_PKT()

DESCRIPTION
  This function fills the description of the specified instance handle into
  a log packet.  If the handle matches the handle for all descriptions, it
  copies all of the active instance descriptions into the log packet.  If the
  handle does not specify all instances but specifies a valid active instance,
  the description associated with that instance is copied into the log packet.
  If the handle is invalid, nothing is done.

PARAMETERS
  handle          : Specifies which instance to retrieve the description from

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_stat_get_flow_desc_log_pkt
(
  int32                         handle
)
{
#ifndef FEATURE_DATA_PS_SOCK_REARCH
  ps_iface_type               * iface_ptr;
  ps_flow_type                * flow_ptr;
  ps_stat_inst_desc_flow_type * flow_desc_ptr;
  uint32                        iface_handle;
  uint32                        flow_handle;
#ifdef FEATURE_DATA_PS_QOS
  void                        * sec_flow_handle;
  void                        * new_sec_flow_handle;
#endif /* FEATURE_DATA_PS_QOS */
  uint8                         idx                = 0;
  uint8                         iface_ind;
  int32                         desc_size          = 0;
  uint8                         inst_count         = 0;
  int32                         log_pkt_avail_size = 0;
  uint8                       * log_pkt_ptr        = NULL;
  uint8                       * log_pkt_fill_ptr   = NULL;

  errno_enum_type                              ret;
  ps_stat_desc_all_inst_hdr_log_pkt_type     * desc_all_inst_pkt_ptr = NULL;
  ps_stat_desc_delta_inst_hdr_log_pkt_type   * delta_pkt_ptr         = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (PS_STAT_INST_DESC_ALL == handle)
  {
    for (idx = 0; idx < MAX_SYSTEM_IFACES; ++idx)
    {
      /*---------------------------------------------------------------------
        Convert the handle into the instance control block
      ---------------------------------------------------------------------*/
      iface_ptr = global_iface_ptr_array[idx];
      iface_handle = (idx << 8);

      /*---------------------------------------------------------------------
        Interface has not been initialized yet, so skip
      ---------------------------------------------------------------------*/
      if (NULL == iface_ptr)
      {
        continue;
      }

      /*---------------------------------------------------------------------
        All interfaces have a default flow - get the description of the
        default flow first
      ---------------------------------------------------------------------*/
      flow_ptr = PS_IFACE_GET_DEFAULT_FLOW(iface_ptr);

      if (NULL == flow_ptr)
      {
          LOG_MSG_ERROR("Unable to get default flow", 0, 0, 0);
          return;
      }

      flow_handle = 0;

      desc_size = sizeof(ps_stat_inst_desc_flow_type);

      /*-------------------------------------------------------------------
        Insufficient amount of space to fit this description in the log
        packet, so commit the existing log
      -------------------------------------------------------------------*/
      if (log_pkt_avail_size < desc_size)
      {
        if (NULL != log_pkt_ptr)
        {
          desc_all_inst_pkt_ptr =
            (ps_stat_desc_all_inst_hdr_log_pkt_type *) log_pkt_ptr;

          desc_all_inst_pkt_ptr->count = inst_count;
          memset(desc_all_inst_pkt_ptr->reserved, 0, 3);

          ps_stat_logging_commit_log_pkt
          (
            log_pkt_ptr,
            (PS_STAT_MAX_LOG_PKT_SIZE - log_pkt_avail_size)
          );
        }

        inst_count         = 0;
        log_pkt_avail_size = PS_STAT_MAX_LOG_PKT_SIZE;
        log_pkt_ptr        = NULL;
        log_pkt_fill_ptr   = NULL;

        /*-------------------------------------------------------------------
          Get the pointer to the log packet
        -------------------------------------------------------------------*/
        ps_stat_logging_get_log_pkt(log_pkt_avail_size,
                                    LOG_PS_STAT_DESC_ALL_FLOW_INST_C,
                                    &log_pkt_ptr);

        /*-------------------------------------------------------------------
          If we fail to get a log packet, just return
        -------------------------------------------------------------------*/
        if (NULL == log_pkt_ptr)
        {
          LOG_MSG_ERROR("Unable to allocate log packet", 0, 0, 0);
          return;
        }

        /*-------------------------------------------------------------------
          Leave space for the describe all instance header
        -------------------------------------------------------------------*/
        log_pkt_fill_ptr =
          (log_pkt_ptr +
            (uint8) sizeof(ps_stat_desc_all_inst_hdr_log_pkt_type));

        log_pkt_avail_size -= sizeof(ps_stat_desc_all_inst_hdr_log_pkt_type);

      }

      flow_desc_ptr = (ps_stat_inst_desc_flow_type *) log_pkt_fill_ptr;

      /*---------------------------------------------------------------------
        The flow is bound to a phys link.  The phys link identifier is its
        instance number.  The phys link handle is a combination of an iface
        handle and the phys link instance.  The flow must be bound to an
        phys link that is on the same iface as it is - thus we can reuse
        the flow iface handle in order to form the right physlink handle.
        Otherwise we would have to navigate through the entire list of
        ifaces since the phys link doesn't have a back ptr to the iface
      ---------------------------------------------------------------------*/
      flow_desc_ptr->handle            = (uint16)( iface_handle | flow_handle);
      flow_desc_ptr->phys_link_handle  = (uint8)
        ( iface_handle |
          (flow_ptr->flow_private.phys_link_ptr)->phys_private.instance);
      flow_desc_ptr->capability        =
        flow_ptr->flow_private.capability_mask;
      flow_desc_ptr->qos_handle        =
        PS_FLOWI_GET_QOS_HANDLE (iface_ptr, flow_ptr);          /*lint !e64 */

      /*---------------------------------------------------------------------
        Increment the fill pointer by the size, and decrement the count
        by the same value
      ---------------------------------------------------------------------*/
      log_pkt_fill_ptr += (uint8) desc_size;
      log_pkt_avail_size -= desc_size;
      inst_count++;

#ifdef FEATURE_DATA_PS_QOS
      /*---------------------------------------------------------------------
        Get the description of all of the secondary flows
      ---------------------------------------------------------------------*/
      if (iface_ptr->iface_private.flow.num_sec_flows > 0)
      {
        /*-------------------------------------------------------------------
          Navigate the list of secondary flow control blocks
          Must occur within a critical section since flows could be added
          or removed on the fly, and we don't want to access invalid memory
        -------------------------------------------------------------------*/
        PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

        sec_flow_handle = ps_iface_get_sec_flow_handle(iface_ptr);

        while( (sec_flow_handle != NULL) &&
               (ps_iface_get_sec_flow_by_handle(iface_ptr,
                                                sec_flow_handle,
                                                &flow_ptr,
                                                &new_sec_flow_handle)) )
        {
          flow_handle = flow_ptr->flow_private.cookie;

          desc_size = sizeof(ps_stat_inst_desc_flow_type);

          /*-----------------------------------------------------------------
            Insufficient amount of space to fit this description in the log
            packet, so commit the existing log
          -----------------------------------------------------------------*/
          if (log_pkt_avail_size < desc_size)
          {
            if (NULL != log_pkt_ptr)
            {
              desc_all_inst_pkt_ptr =
                (ps_stat_desc_all_inst_hdr_log_pkt_type *) log_pkt_ptr;

              desc_all_inst_pkt_ptr->count = inst_count;
              memset(desc_all_inst_pkt_ptr->reserved, 0, 3);

              ps_stat_logging_commit_log_pkt
              (
                log_pkt_ptr,
                (PS_STAT_MAX_LOG_PKT_SIZE - log_pkt_avail_size)
              );
            }

            inst_count         = 0;
            log_pkt_avail_size = PS_STAT_MAX_LOG_PKT_SIZE;
            log_pkt_ptr        = NULL;
            log_pkt_fill_ptr   = NULL;

            /*----------------------------------------------------------------
              Get the pointer to the log packet
            ----------------------------------------------------------------*/
            ps_stat_logging_get_log_pkt(log_pkt_avail_size,
                                        LOG_PS_STAT_DESC_ALL_FLOW_INST_C,
                                        &log_pkt_ptr);

            /*---------------------------------------------------------------
            If we fail to get a log packet, just return
            ---------------------------------------------------------------*/
            if (NULL == log_pkt_ptr)
            {
              LOG_MSG_ERROR("Unable to allocate log packet", 0, 0, 0);
              PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
             return;
            }

            /*---------------------------------------------------------------
              Leave space for the describe all instance header
            ---------------------------------------------------------------*/
            log_pkt_fill_ptr =
              (log_pkt_ptr +
                (uint8) sizeof(ps_stat_desc_all_inst_hdr_log_pkt_type));

            log_pkt_avail_size -=
              sizeof(ps_stat_desc_all_inst_hdr_log_pkt_type);
          }

          flow_desc_ptr = (ps_stat_inst_desc_flow_type *) log_pkt_fill_ptr;

          /*-----------------------------------------------------------------
            The flow is bound to a phys link.  The phys link identifier is its
            instance number.  The phys link handle is a combination of an iface
            handle and the phys link instance.  The flow must be bound to an
            phys link that is on the same iface as it is - thus we can reuse
            the flow iface handle in order to form the right physlink handle.
            Otherwise we would have to navigate through the entire list of
            ifaces since the phys link doesn't have a back ptr to the iface
          -----------------------------------------------------------------*/
          flow_desc_ptr->handle            = (uint16)( iface_handle | flow_handle);
          flow_desc_ptr->phys_link_handle  = (uint8)
             ( iface_handle |
               (flow_ptr->flow_private.phys_link_ptr)->phys_private.instance);
          flow_desc_ptr->capability        =
                                       flow_ptr->flow_private.capability_mask;
          flow_desc_ptr->qos_handle        =
            PS_FLOWI_GET_QOS_HANDLE (iface_ptr, flow_ptr);      /*lint !e64 */

          /*-----------------------------------------------------------------
            Increment the fill pointer by the size, and decrement the count
            by the same value
          -----------------------------------------------------------------*/
          log_pkt_fill_ptr += (uint8) desc_size;
          log_pkt_avail_size -= desc_size;
          inst_count++;

          sec_flow_handle = new_sec_flow_handle;
        }

        PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      } /* else secondary flow */
#endif /* FEATURE_DATA_PS_QOS */
    } /* for all interfaces on the system */

    /*-----------------------------------------------------------------------
      Dispatch the last packet
    -----------------------------------------------------------------------*/
    if (NULL != log_pkt_ptr)
    {
      desc_all_inst_pkt_ptr =
        (ps_stat_desc_all_inst_hdr_log_pkt_type *) log_pkt_ptr;

      desc_all_inst_pkt_ptr->count = inst_count;
      memset(desc_all_inst_pkt_ptr->reserved, 0, 3);

      ps_stat_logging_commit_log_pkt
      (
        log_pkt_ptr,
        (PS_STAT_MAX_LOG_PKT_SIZE - log_pkt_avail_size)
      );
    }
  } /* if all instances */
  else
  {
    iface_handle = handle & 0xFF00;

    iface_ind = (uint8) (iface_handle >> 8);
    if (MAX_SYSTEM_IFACES <= iface_ind)
    {
      LOG_MSG_ERROR ("Inv iface handle 0x%x", iface_handle, 0, 0);
      return;
    }

    iface_ptr = global_iface_ptr_array[iface_ind];
    if (NULL == iface_ptr)
    {
      LOG_MSG_ERROR ("NULL iface pointer for handle 0x%x", iface_handle, 0, 0);
      return;
    }

    flow_ptr = NULL;
    ret = ps_stat_get_flow_control_block(handle, &flow_ptr);

    if (E_SUCCESS != ret || NULL == flow_ptr)
    {
      LOG_MSG_ERROR("Unable to get flow control block for get desc.  Handle: %d",
                 handle, 0, 0);
      return;
    }

    desc_size = sizeof(ps_stat_inst_desc_flow_type);

    /*-----------------------------------------------------------------------
      We need to allocate enough memory for the description and the delta
      header
    -----------------------------------------------------------------------*/
    log_pkt_avail_size =
              (desc_size + sizeof(ps_stat_desc_delta_inst_hdr_log_pkt_type));

    ps_stat_logging_get_log_pkt(log_pkt_avail_size,
                                LOG_PS_STAT_DESC_DELTA_INST_C,
                                &log_pkt_ptr);

    /*-----------------------------------------------------------------------
      If we fail to get a log packet, just return
    -----------------------------------------------------------------------*/
    if (NULL == log_pkt_ptr)
    {
      LOG_MSG_ERROR("Unable to allocate log packet", 0, 0, 0);
      return;
    }

    /*-----------------------------------------------------------------------
      Fill in the delta log parameters
    -----------------------------------------------------------------------*/
    delta_pkt_ptr = (ps_stat_desc_delta_inst_hdr_log_pkt_type *) log_pkt_ptr;

    delta_pkt_ptr->type    = PS_STAT_MODULE_INST_FLOW;
    delta_pkt_ptr->deleted = FALSE;
    memset(delta_pkt_ptr->reserved, 0, 2);

    log_pkt_fill_ptr =
      (log_pkt_ptr + (uint8) sizeof(ps_stat_desc_delta_inst_hdr_log_pkt_type));

    /*-----------------------------------------------------------------
      The flow is bound to a phys link.  The phys link identifier is its
      instance number.  The phys link handle is a combination of an iface
      handle and the phys link instance.  The flow must be bound to an
      phys link that is on the same iface as it is - thus we can reuse
      the flow iface handle in order to form the right physlink handle.
      Otherwise we would have to navigate through the entire list of
      ifaces since the phys link doesn't have a back ptr to the iface
    -----------------------------------------------------------------*/
    flow_desc_ptr = (ps_stat_inst_desc_flow_type *) log_pkt_fill_ptr;

    flow_desc_ptr->handle            = (uint16)handle;
    flow_desc_ptr->phys_link_handle  = (uint8)
      ( iface_handle |
         (flow_ptr->flow_private.phys_link_ptr)->phys_private.instance);
    flow_desc_ptr->capability        =
                             flow_ptr->flow_private.capability_mask;
    flow_desc_ptr->qos_handle        =
      PS_FLOWI_GET_QOS_HANDLE (iface_ptr, flow_ptr);            /*lint !e64 */

    /*-----------------------------------------------------------------------
      Dispatch the packet
    -----------------------------------------------------------------------*/
    ps_stat_logging_commit_log_pkt
    (
      log_pkt_ptr,
      (desc_size + sizeof(ps_stat_desc_delta_inst_hdr_log_pkt_type))
    );
  } /* else only fill one instance */
#endif
} /* ps_stat_get_flow_desc_log_pkt */


/*===========================================================================
FUNCTION PS_STAT_GET_FLOW_STAT_LOG_PKT()

DESCRIPTION
  This function fills the statistics of the specified instance handle into
  a log packet.  If the handle matches the handle for all statistics, it
  copies all of the active instance statistics into the log packet.  If the
  handle does not specify all instances but specifies a valid active instance,
  the statistics associated with that instance is copied into the log packet.
  If the handle is invalid, nothing is done.

PARAMETERS
  handle          : Specifies which instance to retrieve the stats from

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_stat_get_flow_stat_log_pkt
(
  int32                         handle
)
{
  ps_iface_type               * iface_ptr;
  ps_flow_type                * flow_ptr;
  uint32                        iface_handle;
  uint32                        flow_handle;
#ifdef FEATURE_DATA_PS_QOS
  void                        * sec_flow_handle;
  void                        * new_sec_flow_handle;
#endif /* FEATURE_DATA_PS_QOS */
  int32                         idx;

  errno_enum_type                        ret;
  uint8                                * log_pkt_ptr           = NULL;
  ps_stat_inst_flow_log_pkt_type       * inst_flow_log_pkt_ptr = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (PS_STAT_INST_DESC_ALL == handle)
  {
    for (idx = 0; idx < MAX_SYSTEM_IFACES; ++idx)
    {
      /*---------------------------------------------------------------------
        Convert the handle into the instance control block
      ---------------------------------------------------------------------*/
      iface_ptr = global_iface_ptr_array[idx];
      iface_handle = (idx << 8);

      /*---------------------------------------------------------------------
        Interface has not been initialized yet, so skip
      ---------------------------------------------------------------------*/
      if ( (NULL == iface_ptr) ||
           (NULL == iface_ptr->iface_private.this_iface) )
      {
        continue;
      }

      /*---------------------------------------------------------------------
        All interfaces have a default flow - get the description of the
        default flow first
      ---------------------------------------------------------------------*/
      flow_ptr = iface_ptr->iface_private.flow.default_flow_ptr;
      flow_handle = 0;

      ps_stat_logging_get_log_pkt(sizeof(ps_stat_inst_flow_log_pkt_type),
                                  LOG_PS_STAT_FLOW_INST_C,
                                  &log_pkt_ptr);

      if (NULL == log_pkt_ptr)
      {
        LOG_MSG_ERROR("Log packet allocated for flow get stats failed", 0, 0, 0);
        return;
      }

      inst_flow_log_pkt_ptr = (ps_stat_inst_flow_log_pkt_type *) log_pkt_ptr;

      inst_flow_log_pkt_ptr->handle  = (iface_handle | flow_handle);

      (void) ps_stat_get_flow(PS_STAT_FLOW_ALL,
                              (void *) flow_ptr,
                              &(inst_flow_log_pkt_ptr->inst_flow_stats),
                              sizeof(ps_stat_flow_i_s_type));

      ps_stat_logging_commit_log_pkt
      (
        log_pkt_ptr,
        sizeof(ps_stat_inst_flow_log_pkt_type)
      );

#ifdef FEATURE_DATA_PS_QOS
      /*---------------------------------------------------------------------
        Get the description of all of the secondary flows
      ---------------------------------------------------------------------*/
      if (iface_ptr->iface_private.flow.num_sec_flows > 0)
      {
        /*-------------------------------------------------------------------
          Navigate the list of secondary flow control blocks
          Must occur within a critical section since flows could be added
          or removed on the fly, and we don't want to access invalid memory
        -------------------------------------------------------------------*/
        PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

        sec_flow_handle = ps_iface_get_sec_flow_handle(iface_ptr);

        while( (sec_flow_handle != NULL) &&
               (ps_iface_get_sec_flow_by_handle(iface_ptr,
                                                sec_flow_handle,
                                                &flow_ptr,
                                                &new_sec_flow_handle)) )
        {
          flow_handle = flow_ptr->flow_private.cookie;

          ps_stat_logging_get_log_pkt(sizeof(ps_stat_inst_flow_log_pkt_type),
                                      LOG_PS_STAT_FLOW_INST_C,
                                      &log_pkt_ptr);

          if (NULL == log_pkt_ptr)
          {
            LOG_MSG_ERROR("Log packet allocated for flow get stats failed",
                       0, 0, 0);
            PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
            return;
          }

          inst_flow_log_pkt_ptr =
            (ps_stat_inst_flow_log_pkt_type *) log_pkt_ptr;

          inst_flow_log_pkt_ptr->handle  = (iface_handle | flow_handle);

          (void) ps_stat_get_flow(PS_STAT_FLOW_ALL,
                                  (void *) flow_ptr,
                                  &(inst_flow_log_pkt_ptr->inst_flow_stats),
                                  sizeof(ps_stat_flow_i_s_type));

          ps_stat_logging_commit_log_pkt
          (
            log_pkt_ptr,
            sizeof(ps_stat_inst_flow_log_pkt_type)
          );

          sec_flow_handle = new_sec_flow_handle;
        }

        PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      } /* else secondary flow */
#endif /* FEATURE_DATA_PS_QOS */
    } /* for all interfaces on the system */
  } /* if all instances */
  else
  {
    ret = ps_stat_get_flow_control_block(handle, &flow_ptr);

    if (E_SUCCESS != ret)
    {
      LOG_MSG_ERROR("Unable to get flow control block for get stats.  Handle: %d",
                 handle, 0, 0);
      return;
    }

    ps_stat_logging_get_log_pkt(sizeof(ps_stat_inst_flow_log_pkt_type),
                                LOG_PS_STAT_FLOW_INST_C,
                                &log_pkt_ptr);

    if (NULL == log_pkt_ptr)
    {
      LOG_MSG_ERROR("Log packet allocated for flow get stats failed", 0, 0, 0);
      return;
    }

    inst_flow_log_pkt_ptr = (ps_stat_inst_flow_log_pkt_type *) log_pkt_ptr;

    inst_flow_log_pkt_ptr->handle  = handle;

    (void) ps_stat_get_flow(PS_STAT_FLOW_ALL,
                            (void *) flow_ptr,
                            &(inst_flow_log_pkt_ptr->inst_flow_stats),
                            sizeof(ps_stat_flow_i_s_type));

    ps_stat_logging_commit_log_pkt
    (
      log_pkt_ptr,
      sizeof(ps_stat_inst_flow_log_pkt_type)
    );

  } /* else only fill one instance */
} /* ps_stat_get_flow_stat_log_pkt */


/*===========================================================================

                             EXTERNAL FUNCTIONS

===========================================================================*/

/*===========================================================================
FUNCTION PS_STAT_RESET_FLOW()

DESCRIPTION
  This function resets the instance statistics associated with the specified
  instance.

DEPENDENCIES
  None

RETURN VALUE
  E_SUCCESS (0) for success and any other number for failure . The error
  codes are as defined in errno.h.

PARAMETERS
  handle : Handle to the flow

SIDE EFFECTS
  None
===========================================================================*/
void ps_stat_reset_flow
(
  int32            handle
)
{
  errno_enum_type  ret;
  ps_flow_type   * flow_cb_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  ret = ps_stat_get_flow_control_block(handle, &flow_cb_ptr);

  if (ret != E_SUCCESS)
  {
    LOG_MSG_ERROR("Unable to get flow control block for reset.  Handle: %d",
               handle, 0, 0);
    return;
  }

  if (PS_STAT_QUERIED == flow_cb_ptr->flow_i_stats.queried)
  {
    ps_stat_get_flow_stat_log_pkt(handle);
  }

  memset(&(flow_cb_ptr->flow_i_stats), 0, sizeof(flow_cb_ptr->flow_i_stats));

  return;
} /* ps_stat_reset_flow */


/*===========================================================================
FUNCTION PS_STAT_INIT_FLOW()

DESCRIPTION

DEPENDENCIES
  None

PARAMETERS

RETURN VALUE

SIDE EFFECTS
  None
===========================================================================*/
void ps_stat_init_flow
(
  void
)
{
  ps_stat_logging_inst_reg_fill_desc_cback(PS_STAT_MODULE_INST_FLOW,
                                           ps_stat_get_flow_desc_log_pkt);

  ps_stat_logging_inst_reg_get_stat_cback(PS_STAT_MODULE_INST_FLOW,
                                          ps_stat_get_flow_stat_log_pkt);

  ps_stat_logging_inst_reg_reset_stat_cback(PS_STAT_MODULE_INST_FLOW,
                                            ps_stat_reset_flow);

} /* ps_stat_init_flow */


/*===========================================================================
FUNCTION PS_STAT_GET_FLOW()

DESCRIPTION
  This function supplies the required flow layer statistics. The first
  argument passed to the function is an enum that denotes the metric of
  interest.The second argument will be  a pointer to the address of the
  flow control block. The third argument points to the memory location
  where the results will be copied.The result (statistic) will be copied to
  this memory address. The fourth argument passes the length of memory
  allocated for the return value and should be  greater than or equal to
  the memory size of the statistic requested.

DEPENDENCIES
  None

RETURN VALUE
  E_SUCCESS (0) for success and any other number for failure . The error
  codes are as defined in errno.h.

PARAMETERS
  stat            - IN - type of stat
  instance_ptr    - IN - Handle to instance
  return_value    - IN - pointer for return value
                  - OUT- return value (statistics)
  ret_len         - length of memory allocated

SIDE EFFECTS
  None
===========================================================================*/
errno_enum_type ps_stat_get_flow
(
  ps_stat_flow_enum_type  stat,
  void                    *instance_ptr,
  void                    *return_value,
  uint16                  ret_len
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Check Instance pointer; Instance pointer should not be null
  -------------------------------------------------------------------------*/

  if (instance_ptr == NULL)
  {
    LOG_MSG_ERROR(" Instance pointer should NOT be NULL for FLOW stats",0,0,0);
    return E_INVALID_ARG ;
  }

  if (stat == PS_STAT_FLOW_ALL)
  {
     return ps_stat_get_flow_instance_all(stat,
                                          instance_ptr,
                                          return_value,
                                          ret_len
                                         ) ;
  }
  else if (stat == PS_STAT_FLOW_IS_DEFAULT)
  {
     return ps_stat_get_flow_is_default(stat,
                                        instance_ptr,
                                        return_value,
                                        ret_len
                                       ) ;

  }
  {
    return ps_stat_get_common(stat,
                              ps_stat_flow_i_table,
                              PS_STAT_FLOW_ALL,
                              instance_ptr,
                              return_value,
                              ret_len
                             ) ;
  }

} /* ps_stat_get_flow */

#endif /* FEATURE_DATA_PS */
