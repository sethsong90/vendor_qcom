/*===========================================================================
  FILE: ps_iface_logical_flowi.c

  OVERVIEW: TODO

  DEPENDENCIES: None

  Copyright (c) 2010 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_iface_logical_flowi.c#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-05-11 msr Created file

===========================================================================*/

/*===========================================================================

                                INCLUDE FILES

===========================================================================*/
#include "customer.h"
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "ps_iface_logical_flow.h"
#include "ps_iface_logical_flowi.h"
#include "dserrno.h"
#include "ds_Utils_DebugMsg.h"
#include "ps_crit_sect.h"
#include "ps_iface_utils.h"
#include "ps_system_heap.h"
#include "ps_flowi.h"
#include "ps_ifacei.h"
#include "ps_tx_meta_info.h"
#include "ps_metai_info.h"
#include "ds_Utils_DebugMsg.h"

/*===========================================================================

                              EXTERNAL FUNCTIONS

===========================================================================*/

void ps_iface_logical_flowi_free_qos_spec
(
  qos_spec_type * qos_spec_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL != qos_spec_ptr)
  {
    if (NULL != qos_spec_ptr->rx.flow_template.aux_flow_list_ptr)
    {
      PS_SYSTEM_HEAP_MEM_FREE
      (
        qos_spec_ptr->rx.flow_template.aux_flow_list_ptr
      );
    }

    if (NULL != qos_spec_ptr->tx.flow_template.aux_flow_list_ptr)
    {
      PS_SYSTEM_HEAP_MEM_FREE
      (
        qos_spec_ptr->tx.flow_template.aux_flow_list_ptr
      );
    }

    if (NULL != qos_spec_ptr->rx.fltr_template.list_ptr)
    {
      PS_SYSTEM_HEAP_MEM_FREE
      ( 
        qos_spec_ptr->rx.fltr_template.list_ptr
      );
    }
  
    if (NULL != qos_spec_ptr->tx.fltr_template.list_ptr)
    {
      PS_SYSTEM_HEAP_MEM_FREE
      ( 
        qos_spec_ptr->tx.fltr_template.list_ptr
      );
    }
  
    PS_SYSTEM_HEAP_MEM_FREE( qos_spec_ptr);
  }
} /* ps_iface_logical_flowi_free_qos_spec() */

int32 ps_iface_logical_flowi_handle_nw_init_qos_flow_selective
(
  ps_iface_type * ps_iface_ptr,
  ps_flow_type  * assoc_flow_ptr
)
{
  ps_flow_type * logical_flow_ptr;
  boolean        is_logical_flow_created;
  int32          ret_val;
  int16          ps_errno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p", ps_iface_ptr, 0, 0);

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid Iface 0x%p", ps_iface_ptr, 0, 0);
      break;
    }

    if (!PS_IFACEI_IS_LOGICAL( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    if (!PS_FLOW_IS_VALID( assoc_flow_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid Flow 0x%p", assoc_flow_ptr, 0, 0);
      break;
    }

    if (PS_FLOW_GET_CAPABILITY( assoc_flow_ptr, PS_FLOW_CAPABILITY_DEFAULT))
    {
      LOG_MSG_INVALID_INPUT( "Associated Flow 0x%p cannot be default flow",
                             assoc_flow_ptr, 0, 0);
      break;
    }
        
    logical_flow_ptr = 
      list_peek_front( &(ps_iface_ptr->iface_private.flow.sec_flow_list));

    is_logical_flow_created = FALSE;

    while (logical_flow_ptr != NULL)
    {
      if ( PS_FLOW_GET_ASSOC_PS_FLOW(logical_flow_ptr) == assoc_flow_ptr)
      {
        is_logical_flow_created = TRUE;
        break;
      }

      logical_flow_ptr =
        list_peek_next(&(ps_iface_ptr->iface_private.flow.sec_flow_list),
                       &(logical_flow_ptr->link));
    }

    if ( !is_logical_flow_created)
    {
      ret_val = ps_iface_logical_flowi_handle_nw_init_qos_flow(ps_iface_ptr,
                                                               assoc_flow_ptr,
                                                               &ps_errno);

      if ( 0 != ret_val)
      {
        LOG_MSG_INFO3( "ps_iface_logical_flowi_handle_nw_init_qos_flow() "
                       "failed, iface 0x%x:%d flow 0x%p",
                       ps_iface_ptr->name, ps_iface_ptr->instance,
                       assoc_flow_ptr);
        break;
      }

    }

    LOG_MSG_FUNCTION_EXIT( "Success, iface 0x%x:%d",
                           ps_iface_ptr->name, ps_iface_ptr->instance, 0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;

  } while (0);

  LOG_MSG_FUNCTION_EXIT( "Fail, iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance, 0);
  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return -1;

} /* ps_iface_logical_flowi_handle_nw_init_qos_flow_selective() */

int32 ps_iface_logical_flowi_handle_nw_init_qos
(
  ps_iface_type  * ps_iface_ptr
)
{
  ps_iface_type  * assoc_iface_ptr;
  ps_flow_type   * assoc_flow_ptr;
  int32            ret_val;
  int16            ps_errno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p", ps_iface_ptr, 0, 0);

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      break;
    }

    if (!PS_IFACEI_IS_LOGICAL( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    assoc_iface_ptr = PS_IFACEI_GET_ASSOC_IFACE( ps_iface_ptr);
    if (NULL == assoc_iface_ptr)
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%p not associated",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    assoc_flow_ptr =
      list_peek_front( &( assoc_iface_ptr->iface_private.flow.sec_flow_list));

    while ( assoc_flow_ptr != NULL)
    {
      ret_val = ps_iface_logical_flowi_handle_nw_init_qos_flow( ps_iface_ptr,
                                                                assoc_flow_ptr,
                                                                &ps_errno);
      if ( 0 != ret_val)
      {
        LOG_MSG_INFO3( "ps_iface_logical_flowi_handle_nw_init_qos_flow() "
                       "failed, iface 0x%x:%d flow 0x%p",
                       ps_iface_ptr->name, ps_iface_ptr->instance,
                       assoc_flow_ptr);
      }

      assoc_flow_ptr =
        list_peek_next( &( assoc_iface_ptr->iface_private.flow.sec_flow_list),
                        &( assoc_flow_ptr->link));
    }

    LOG_MSG_FUNCTION_EXIT( "Success, iface 0x%x:%d",
                           ps_iface_ptr->name, ps_iface_ptr->instance, 0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;
    
  } while (0);

  LOG_MSG_FUNCTION_EXIT( "Fail, iface 0x%x:%d",
                         ps_iface_ptr->name, ps_iface_ptr->instance, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);

  return -1;

} /* ps_iface_logicali_handle_nw_init_qos() */

int32 ps_flowi_get_qos_spec_from_flow
(
  ps_flow_type   * flow_ptr,
  boolean          is_modify,
  qos_spec_type ** qos_spec_ptr_ptr,  
  int16          * ps_errno
)
{
  qos_info_type * qos_info_ptr;
  ip_flow_type  * rx_aux_flow_list_ptr = NULL;
  ip_flow_type  * tx_aux_flow_list_ptr = NULL;
  int32           rx_aux_flow_cnt;
  int32           tx_aux_flow_cnt;
  int32           ret_val = 0;
  uint8           rx_fltr_cnt;
  uint8           tx_fltr_cnt;
  boolean         bool_ret_val;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Flow 0x%p isModify %d", flow_ptr, is_modify, 0);
  
  if (NULL == ps_errno)
  {
    LOG_MSG_INVALID_INPUT( "NULL ps_errno", 0, 0, 0);
    return -1;
  }
  
  if (NULL == qos_spec_ptr_ptr)
  {
    LOG_MSG_INVALID_INPUT( "NULL qos_spec_ptr_ptr", 0, 0, 0);
    *ps_errno = DS_EFAULT;
    return -1;
  }

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    if (!PS_FLOW_IS_VALID( flow_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid flow 0x%p", flow_ptr, 0, 0);
      break;
    }

    if (is_modify)
    {
      qos_info_ptr = flow_ptr->flow_private.qos_modify_info_ptr;
    }
    else
    {
      qos_info_ptr = flow_ptr->flow_private.qos_info_ptr;
    }
    
    if (NULL == qos_info_ptr)
    {
      LOG_MSG_INVALID_INPUT( "Invalid qos_info_ptr, flow 0x%p",
                             flow_ptr, 0, 0);
      break;
    }

    *qos_spec_ptr_ptr = ps_system_heap_mem_alloc( sizeof( qos_spec_type));
    if (NULL == *qos_spec_ptr_ptr)
    {
      LOG_MSG_INFO3( "ps_system_heap_mem_alloc failed, flow 0x%p",
                     flow_ptr, 0, 0);
      break;					  
    }

    memset( *qos_spec_ptr_ptr, 0, sizeof( qos_spec_type));
    (*qos_spec_ptr_ptr)->field_mask = qos_info_ptr->field_mask;

    /*-----------------------------------------------------------------------
      Store Rx flow spec
    -----------------------------------------------------------------------*/
    if (qos_info_ptr->field_mask & QOS_MASK_RX_FLOW)
    {
      LOG_MSG_INFO1( "Storing Rx flow spec, flow 0x%p", flow_ptr, 0, 0);

      (*qos_spec_ptr_ptr)->rx.flow_template.req_flow = 
        qos_info_ptr->rx.ipflow.req;

      if (qos_info_ptr->field_mask & QOS_MASK_RX_MIN_FLOW)
      {
        (*qos_spec_ptr_ptr)->rx.flow_template.min_req_flow =
          qos_info_ptr->rx.ipflow.min_req;
      }

      /*-----------------------------------------------------------------------
        Store auxiliary Rx flow specs
      -----------------------------------------------------------------------*/
      if (qos_info_ptr->field_mask & QOS_MASK_RX_AUXILIARY_FLOWS)
      {
        rx_aux_flow_cnt = ps_flow_get_aux_flow_spec_cnt( flow_ptr,
                                                         QOS_MASK_RX_FLOW,
                                                         is_modify);

        rx_aux_flow_list_ptr =
          ps_system_heap_mem_alloc( rx_aux_flow_cnt * sizeof( ip_flow_type));

        if (NULL == rx_aux_flow_list_ptr)
        {
           LOG_MSG_INFO3( "ps_system_heap_mem_alloc() failed for "
                          "rx_aux_flow_list_ptr, flow 0x%p",
                          flow_ptr, 0, 0);
           break;
        }

        ret_val = ps_flow_get_aux_flow_spec( flow_ptr,
                                             &rx_aux_flow_cnt,
                                             rx_aux_flow_list_ptr,
                                             QOS_MASK_RX_FLOW,
                                             is_modify,
                                             ps_errno);
        if (0 != ret_val)
        {
          LOG_MSG_INFO3( "ps_flow_get_aux_flow_spec() failed while copying "
                         "rx aux flow spec, flow 0x%p, err %d",
                         flow_ptr, *ps_errno, 0);
          break;
        }

        (*qos_spec_ptr_ptr)->rx.flow_template.aux_flow_list_ptr = 
          rx_aux_flow_list_ptr;
        (*qos_spec_ptr_ptr)->rx.flow_template.num_aux_flows     = 
          (uint8)rx_aux_flow_cnt;
      }

      /*-----------------------------------------------------------------------
        Store Rx Filters
      -----------------------------------------------------------------------*/
      rx_fltr_cnt = ps_flowi_get_rx_fltr_cnt_ex(flow_ptr, is_modify);
      (*qos_spec_ptr_ptr)->rx.fltr_template.list_ptr =
        ps_system_heap_mem_alloc( rx_fltr_cnt * sizeof( ip_filter_type));


      /*-----------------------------------------------------------------------
        ps_flow_get_rx_fltr_spec returns boolean. So taking complement of
        ret_val
      -----------------------------------------------------------------------*/
      bool_ret_val = ps_flow_get_rx_fltr_spec
                     ( 
                       flow_ptr->flow_private.iface_ptr,
                       flow_ptr,
                       &( (*qos_spec_ptr_ptr)->rx.fltr_template),
                       is_modify,
                       (uint8)rx_fltr_cnt,
                       ps_errno
                     );

      if (!bool_ret_val)
      {
        LOG_MSG_INFO1( "ps_flow_get_rx_fltr_spec() failed while copying "
                       "rx fltr spec, flow 0x%p, err %d",
                       flow_ptr, *ps_errno, 0);
        ret_val = -1;
        break;
      }
    }

    /*-----------------------------------------------------------------------
      Store Tx flow spec
    -----------------------------------------------------------------------*/
    if (qos_info_ptr->field_mask & QOS_MASK_TX_FLOW)
    {
      LOG_MSG_INFO1( "Storing Tx flow spec, flow 0x%p", flow_ptr, 0, 0);

      (*qos_spec_ptr_ptr)->tx.flow_template.req_flow = 
        qos_info_ptr->tx.ipflow.req;

      if (qos_info_ptr->field_mask & QOS_MASK_TX_MIN_FLOW)
      {
        (*qos_spec_ptr_ptr)->tx.flow_template.min_req_flow =
          qos_info_ptr->tx.ipflow.min_req;
      }

      /*-----------------------------------------------------------------------
        Store auxiliary Tx flow specs
      -----------------------------------------------------------------------*/
      if (qos_info_ptr->field_mask & QOS_MASK_TX_AUXILIARY_FLOWS)
      {
        tx_aux_flow_cnt = ps_flow_get_aux_flow_spec_cnt( flow_ptr,
                                                         QOS_MASK_TX_FLOW,
                                                         is_modify);

        tx_aux_flow_list_ptr =
          ps_system_heap_mem_alloc( tx_aux_flow_cnt * sizeof( ip_flow_type));

        if (NULL == tx_aux_flow_list_ptr)
        {  
          LOG_MSG_INFO3( "ps_system_heap_mem_alloc() failed for "
                         "tx_aux_flow_list_ptr, flow 0x%p",
                         flow_ptr, 0, 0);
          break;
        }

        ret_val = ps_flow_get_aux_flow_spec( flow_ptr,
                                             &tx_aux_flow_cnt,
                                             tx_aux_flow_list_ptr,
                                             QOS_MASK_TX_FLOW,
                                             is_modify,
                                             ps_errno);
        if (0 != ret_val)
        {
          LOG_MSG_INFO3( "ps_flow_get_aux_flow_spec() failed while copying "
                         "tx aux flow spec, flow 0x%p, err %d",
                         flow_ptr, *ps_errno, 0);
          break;
        }

        (*qos_spec_ptr_ptr)->tx.flow_template.aux_flow_list_ptr = 
          tx_aux_flow_list_ptr;
        (*qos_spec_ptr_ptr)->tx.flow_template.num_aux_flows     = 
          (uint8)tx_aux_flow_cnt;
      }

      /*-----------------------------------------------------------------------
        Store Tx Filters
      -----------------------------------------------------------------------*/
      tx_fltr_cnt = ps_ifacei_get_tx_fltr_cnt_ex
                    ( 
                      flow_ptr->flow_private.iface_ptr,
                      flow_ptr, 
                      is_modify
                    );

      (*qos_spec_ptr_ptr)->tx.fltr_template.list_ptr =
        ps_system_heap_mem_alloc( tx_fltr_cnt * sizeof( ip_filter_type));

      /*-----------------------------------------------------------------------
        ps_iface_flow_get_tx_fltr_spec returns boolean. So taking complement of
        ret_val
      -----------------------------------------------------------------------*/
      bool_ret_val =
        ps_iface_flow_get_tx_fltr_spec
        ( 
          flow_ptr->flow_private.iface_ptr,
          flow_ptr,
          &( (*qos_spec_ptr_ptr)->tx.fltr_template),
          is_modify,
          tx_fltr_cnt,
          ps_errno
        );
      if (!bool_ret_val)
      {
        LOG_MSG_INFO3( "ps_iface_flow_get_tx_fltr_spec() failed while "
                       "copying tx fltr spec, flow 0x%p, err %d",
                       flow_ptr, *ps_errno, 0);
        ret_val = -1;
        break;
      }
    }

    LOG_MSG_FUNCTION_EXIT( "Created qos_spec_ptr 0x%p from flow 0x%p",
                           *qos_spec_ptr_ptr, flow_ptr, 0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;

  } while(0);
 
  ps_iface_logical_flowi_free_qos_spec(*qos_spec_ptr_ptr);

  LOG_MSG_FUNCTION_EXIT( "Fail, flow 0x%p", flow_ptr, 0, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return -1;

} /* ps_flowi_get_qos_spec_type_from_flow() */

int32 ps_flowi_get_modify_qos_spec_from_flow
(
  ps_flow_type   * logical_flow_ptr,
  qos_spec_type ** qos_spec_ptr_ptr,  
  int16          * ps_errno
)
{
  qos_info_type             * qos_modify_info_ptr;
  ip_flow_type              * rx_aux_flow_list_ptr = NULL;
  ip_flow_type              * tx_aux_flow_list_ptr = NULL;
  ps_flow_type              * assoc_flow_ptr;
  qos_spec_field_mask_type    phys_flow_mask;
  qos_spec_field_mask_type    logical_flow_mask;
  int32                       rx_aux_flow_cnt;
  int32                       tx_aux_flow_cnt;
  int32                       ret_val = 0;
  uint8                       rx_fltr_cnt;
  uint8                       tx_fltr_cnt;
  boolean                     bool_ret_val;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Logical flow 0x%p", logical_flow_ptr, 0, 0);
  
  if (NULL == ps_errno)
  {
    LOG_MSG_INVALID_INPUT( "NULL ps_errno", 0, 0, 0);
    return -1;
  }
  
  if (NULL == qos_spec_ptr_ptr)
  {
    LOG_MSG_INVALID_INPUT( "NULL qos_spec_ptr_ptr", 0, 0, 0);
    *ps_errno = DS_EFAULT;
    return -1;
  }

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    if (!PS_FLOW_IS_VALID( logical_flow_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid logical flow 0x%p",
                             logical_flow_ptr, 0, 0);
      *ps_errno = DS_EINVAL;
      break;
    }
    
    assoc_flow_ptr = PS_FLOW_GET_ASSOC_PS_FLOW( logical_flow_ptr);

    if (!PS_FLOW_IS_VALID( assoc_flow_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid assoc flow 0x%p",
                             assoc_flow_ptr, 0, 0);
      *ps_errno = DS_EINVAL;
      break;
    }

    qos_modify_info_ptr = assoc_flow_ptr->flow_private.qos_info_ptr;
    if (NULL == qos_modify_info_ptr)
    {
      LOG_MSG_INVALID_INPUT( "Invalid qos_info_ptr, flow 0x%p",
                             assoc_flow_ptr, 0, 0);
      *ps_errno = DS_EINVAL;
      break;
    }

    *qos_spec_ptr_ptr = ps_system_heap_mem_alloc( sizeof( qos_spec_type));
    if (NULL == *qos_spec_ptr_ptr)
    {
      LOG_MSG_INFO3( "ps_system_heap_mem_alloc failed, flow 0x%p",
                     assoc_flow_ptr, 0, 0);
      *ps_errno = DS_ENOMEM;
      break;					  
    }

    memset( *qos_spec_ptr_ptr, 0, sizeof( qos_spec_type));

    phys_flow_mask    = PS_FLOW_GET_QOS_FIELD_MASK( assoc_flow_ptr);
    logical_flow_mask = PS_FLOW_GET_QOS_FIELD_MASK( logical_flow_ptr);

    //(*qos_spec_ptr_ptr)->field_mask = qos_modify_info_ptr->field_mask;

    /*-----------------------------------------------------------------------
      Store Rx flow spec
    -----------------------------------------------------------------------*/
    if (!( phys_flow_mask & QOS_MASK_RX_FLOW) && 
        ( logical_flow_mask & QOS_MASK_RX_FLOW))
    {
      /*--------------------------------------------------------------------- 
        Delete Rx FLOW
      ---------------------------------------------------------------------*/
      (*qos_spec_ptr_ptr)->field_mask |= QOS_MASK_RX_FLOW;
      (*qos_spec_ptr_ptr)->rx.flow_template.req_flow.field_mask |= 
        IPFLOW_MASK_NONE;
    }
    else if (phys_flow_mask & QOS_MASK_RX_FLOW)
    {
      if (( phys_flow_mask & QOS_MASK_RX_FLOW) &&
          ( logical_flow_mask & QOS_MASK_RX_FLOW))
      {
        /*------------------------------------------------------------------- 
          Modify Rx FLOW
        -------------------------------------------------------------------*/
        (*qos_spec_ptr_ptr)->field_mask |= QOS_MODIFY_MASK_RX_FLTR_MODIFY;
      }
        
      LOG_MSG_INFO1( "Storing Rx flow spec, flow 0x%p", assoc_flow_ptr, 0, 0);

      (*qos_spec_ptr_ptr)->field_mask |= QOS_MASK_RX_FLOW;
      (*qos_spec_ptr_ptr)->rx.flow_template.req_flow = 
        qos_modify_info_ptr->rx.ipflow.req;

      if (qos_modify_info_ptr->field_mask & QOS_MASK_RX_MIN_FLOW)
      {
        (*qos_spec_ptr_ptr)->field_mask |= QOS_MASK_RX_MIN_FLOW;
        (*qos_spec_ptr_ptr)->rx.flow_template.min_req_flow =
          qos_modify_info_ptr->rx.ipflow.min_req;
      }

      /*-----------------------------------------------------------------------
        Store auxiliary Rx flow specs
      -----------------------------------------------------------------------*/
      if (qos_modify_info_ptr->field_mask & QOS_MASK_RX_AUXILIARY_FLOWS)
      {
        (*qos_spec_ptr_ptr)->field_mask |= QOS_MASK_RX_AUXILIARY_FLOWS;
        rx_aux_flow_cnt = ps_flow_get_aux_flow_spec_cnt( assoc_flow_ptr,
                                                         QOS_MASK_RX_FLOW,
                                                         FALSE);

        rx_aux_flow_list_ptr =
          ps_system_heap_mem_alloc( rx_aux_flow_cnt * sizeof( ip_flow_type));

        if (NULL == rx_aux_flow_list_ptr)
        {
           LOG_MSG_INFO3( "ps_system_heap_mem_alloc() failed for "
                          "rx_aux_flow_list_ptr, flow 0x%p",
                          assoc_flow_ptr, 0, 0);
           break;
        }

        ret_val = ps_flow_get_aux_flow_spec( assoc_flow_ptr,
                                             &rx_aux_flow_cnt,
                                             rx_aux_flow_list_ptr,
                                             QOS_MASK_RX_FLOW,
                                             FALSE,
                                             ps_errno);
        if (0 != ret_val)
        {
          LOG_MSG_INFO3( "ps_flow_get_aux_flow_spec() failed while copying "
                         "rx aux flow spec, flow 0x%p, err %d",
                         assoc_flow_ptr, *ps_errno, 0);
          break;
        }

        (*qos_spec_ptr_ptr)->rx.flow_template.aux_flow_list_ptr = 
          rx_aux_flow_list_ptr;
        (*qos_spec_ptr_ptr)->rx.flow_template.num_aux_flows     = 
          (uint8)rx_aux_flow_cnt;
      }

      /*-----------------------------------------------------------------------
        Store Rx Filters
      -----------------------------------------------------------------------*/
      rx_fltr_cnt = ps_flowi_get_rx_fltr_cnt_ex(assoc_flow_ptr, FALSE);
      (*qos_spec_ptr_ptr)->rx.fltr_template.list_ptr =
        ps_system_heap_mem_alloc( rx_fltr_cnt * sizeof( ip_filter_type));

      /*-----------------------------------------------------------------------
        ps_flow_get_rx_fltr_spec returns boolean. So taking complement of
        ret_val
      -----------------------------------------------------------------------*/
      bool_ret_val = ps_flow_get_rx_fltr_spec
                     ( 
                       assoc_flow_ptr->flow_private.iface_ptr,
                       assoc_flow_ptr,
                       &( (*qos_spec_ptr_ptr)->rx.fltr_template),
                       FALSE,
                       (uint8)rx_fltr_cnt,
                       ps_errno
                     );

      if (!bool_ret_val)
      {
        LOG_MSG_INFO1( "ps_flow_get_rx_fltr_spec() failed while copying "
                       "rx fltr spec, flow 0x%p, err %d",
                       assoc_flow_ptr, *ps_errno, 0);
        ret_val = -1;
        break;
      }
    }

    /*-----------------------------------------------------------------------
      Store Tx flow spec
    -----------------------------------------------------------------------*/
    if (!( phys_flow_mask & QOS_MASK_TX_FLOW) && 
        ( logical_flow_mask & QOS_MASK_TX_FLOW))
    {
      /*--------------------------------------------------------------------- 
        Delete Tx FLOW
      ---------------------------------------------------------------------*/
      (*qos_spec_ptr_ptr)->field_mask |= QOS_MASK_TX_FLOW;
      (*qos_spec_ptr_ptr)->tx.flow_template.req_flow.field_mask |= 
        IPFLOW_MASK_NONE;
    }
    else if (phys_flow_mask & QOS_MASK_TX_FLOW)
    {
      if (( phys_flow_mask & QOS_MASK_TX_FLOW) &&
          ( logical_flow_mask & QOS_MASK_TX_FLOW))
      {
        /*------------------------------------------------------------------- 
          Modify Tx FLOW
        -------------------------------------------------------------------*/
        (*qos_spec_ptr_ptr)->field_mask |= QOS_MODIFY_MASK_TX_FLTR_MODIFY;
      }

      LOG_MSG_INFO1( "Storing Tx flow spec, flow 0x%p", assoc_flow_ptr, 0, 0);
      (*qos_spec_ptr_ptr)->field_mask |= QOS_MASK_TX_FLOW;
      (*qos_spec_ptr_ptr)->tx.flow_template.req_flow = 
        qos_modify_info_ptr->tx.ipflow.req;

      if (qos_modify_info_ptr->field_mask & QOS_MASK_TX_MIN_FLOW)
      {
        (*qos_spec_ptr_ptr)->field_mask |= QOS_MASK_TX_MIN_FLOW;
        (*qos_spec_ptr_ptr)->tx.flow_template.min_req_flow =
          qos_modify_info_ptr->tx.ipflow.min_req;
      }

      /*---------------------------------------------------------------------
        Store auxiliary Tx flow specs
      ---------------------------------------------------------------------*/
      if (qos_modify_info_ptr->field_mask & QOS_MASK_TX_AUXILIARY_FLOWS)
      {
        (*qos_spec_ptr_ptr)->field_mask |= QOS_MASK_TX_AUXILIARY_FLOWS;
        tx_aux_flow_cnt = ps_flow_get_aux_flow_spec_cnt( assoc_flow_ptr,
                                                         QOS_MASK_TX_FLOW,
                                                         FALSE);

        tx_aux_flow_list_ptr =
          ps_system_heap_mem_alloc( tx_aux_flow_cnt * sizeof( ip_flow_type));

        if (NULL == tx_aux_flow_list_ptr)
        {  
          LOG_MSG_INFO3( "ps_system_heap_mem_alloc() failed for "
                         "tx_aux_flow_list_ptr, flow 0x%p",
                         assoc_flow_ptr, 0, 0);
          break;
        }

        ret_val = ps_flow_get_aux_flow_spec( assoc_flow_ptr,
                                             &tx_aux_flow_cnt,
                                             tx_aux_flow_list_ptr,
                                             QOS_MASK_TX_FLOW,
                                             FALSE,
                                             ps_errno);
        if (0 != ret_val)
        {
          LOG_MSG_INFO3( "ps_flow_get_aux_flow_spec() failed while copying "
                         "tx aux flow spec, flow 0x%p, err %d",
                         assoc_flow_ptr, *ps_errno, 0);
          break;
        }

        (*qos_spec_ptr_ptr)->tx.flow_template.aux_flow_list_ptr = 
          tx_aux_flow_list_ptr;
        (*qos_spec_ptr_ptr)->tx.flow_template.num_aux_flows     = 
          (uint8)tx_aux_flow_cnt;
      }

      /*---------------------------------------------------------------------
        Store Tx Filters
      ---------------------------------------------------------------------*/
      tx_fltr_cnt = ps_ifacei_get_tx_fltr_cnt_ex
                    ( 
                      assoc_flow_ptr->flow_private.iface_ptr,
                      assoc_flow_ptr, 
                      FALSE
                    );

      (*qos_spec_ptr_ptr)->tx.fltr_template.list_ptr =
        ps_system_heap_mem_alloc( tx_fltr_cnt * sizeof( ip_filter_type));

      /*---------------------------------------------------------------------
        ps_iface_flow_get_tx_fltr_spec returns boolean. So taking complement of
        ret_val
      ---------------------------------------------------------------------*/
      bool_ret_val =
        ps_iface_flow_get_tx_fltr_spec
        ( 
          assoc_flow_ptr->flow_private.iface_ptr,
          assoc_flow_ptr,
          &( (*qos_spec_ptr_ptr)->tx.fltr_template),
          FALSE,
          tx_fltr_cnt,
          ps_errno
        );
      if (!bool_ret_val)
      {
        LOG_MSG_INFO3( "ps_iface_flow_get_tx_fltr_spec() failed while "
                       "copying tx fltr spec, flow 0x%p, err %d",
                       assoc_flow_ptr, *ps_errno, 0);
        ret_val = -1;
        break;
      }
    }

    LOG_MSG_FUNCTION_EXIT( "Created qos_spec_ptr 0x%p from flow 0x%p",
                           *qos_spec_ptr_ptr, assoc_flow_ptr, 0);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;

  } while(0);
 
  ps_iface_logical_flowi_free_qos_spec(*qos_spec_ptr_ptr);

  LOG_MSG_FUNCTION_EXIT( "Fail, logical flow 0x%p", logical_flow_ptr, 0, 0);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return -1;

} /* ps_flowi_get_modify_qos_spec_from_flow() */

int32 ps_iface_logical_flowi_handle_nw_init_qos_flow
(
 ps_iface_type  * ps_iface_ptr,
 ps_flow_type   * assoc_flow_ptr,
 int16          * ps_errno
)
{
  ps_flow_create_param_type             qos_req_param;
  ps_iface_type                       * assoc_iface_ptr; 
  int32                                 ret_val = -1;
  ps_flow_type                        * ps_flow_ptr = NULL;
  ps_tx_meta_info_type                * default_flow_meta_info_ptr;
  ps_tx_meta_info_type                * logical_flow_meta_info_ptr = NULL;
  ps_flow_type                        * default_flow_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p flow 0x%p",
                          ps_iface_ptr, assoc_flow_ptr, 0);

  memset( &qos_req_param, 0, sizeof(ps_flow_create_param_type));

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    if (!PS_IFACE_IS_VALID( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", ps_iface_ptr, 0, 0);
      break;
    }

    if (!PS_IFACEI_IS_LOGICAL( ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    assoc_iface_ptr = PS_IFACEI_GET_ASSOC_IFACE( ps_iface_ptr);
    if (NULL == assoc_iface_ptr)
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%p not associated",
                             ps_iface_ptr->name, ps_iface_ptr->instance, 0);
      break;
    }

    if (!PS_FLOW_IS_VALID( assoc_flow_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid flow 0x%p", assoc_flow_ptr, 0, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Construct QoS request from flow ptr
    -----------------------------------------------------------------------*/
    ret_val = ps_flowi_get_qos_spec_from_flow(assoc_flow_ptr,
                                              FALSE,
                                              &qos_req_param.qos_spec,
                                              ps_errno);
    if (0 != ret_val)
    {
      LOG_MSG_INFO3( "ps_flowi_get_qos_spec_type_from_flow() failed, "
                     "flow 0x%p err %d", assoc_flow_ptr, *ps_errno, 0);
      break;
    }

    /*-----------------------------------------------------------------------
        Set Filtering enable for qos_req_param
    -----------------------------------------------------------------------*/
    ret_val = ps_iface_ipfltr_is_handle_enabled
              (
                assoc_iface_ptr,
                IP_FLTR_CLIENT_QOS_OUTPUT,
                PS_FLOWI_GET_TX_FLTR_HANDLE( assoc_flow_ptr),
                &qos_req_param.enable_filtering,
                ps_errno
              );

    if (0 != ret_val)
    {
      LOG_MSG_INFO3( "ps_iface_ipfltr_is_handle_enabled() failed, "
                     "flow 0x%p err %d", assoc_flow_ptr, *ps_errno, 0);
      break;
    }

    qos_req_param.fltr_priority = PS_IFACE_IPFLTR_PRIORITY_DEFAULT;
    qos_req_param.subset_id     = 0;

    /*-----------------------------------------------------------------------
      Create PS Flow based on the created qos_req_param
    -----------------------------------------------------------------------*/
    ret_val = ps_iface_create_flow( ps_iface_ptr,
                                    NULL,
                                    &qos_req_param,
                                    &ps_flow_ptr,
                                    ps_errno);
    if (0 != ret_val)
    {
      LOG_MSG_INFO3( "ps_iface_create_flow() failed, flow 0x%p err %d",
                     assoc_flow_ptr, *ps_errno, 0);
      break;
    }

    default_flow_ptr = PS_IFACEI_GET_DEFAULT_FLOW( ps_iface_ptr);
    default_flow_meta_info_ptr =
      PS_FLOW_GET_META_INFO_FROM_FLOW( default_flow_ptr);

    /*-----------------------------------------------------------------------
      Allocate meta-info for flow and copy default flow's meta-info into it
    -----------------------------------------------------------------------*/
    if (NULL != default_flow_meta_info_ptr)
    {
      PS_TX_META_INFO_AND_RT_META_INFO_GET( logical_flow_meta_info_ptr);

      if (logical_flow_meta_info_ptr == NULL ||
          PS_TX_META_GET_RT_META_INFO_PTR( logical_flow_meta_info_ptr) == NULL)
      {
        LOG_MSG_ERROR( "Couldn't alloc metainfo, flow 0x%p",
                       assoc_flow_ptr, 0, 0);
        *ps_errno = DS_ENOMEM;
        break;
      }

      PS_TX_META_INFO_COPY( default_flow_meta_info_ptr,
                            logical_flow_meta_info_ptr);
    }

    ps_flow_ptr->flow_private.logical_flow.mi_ptr = logical_flow_meta_info_ptr;

    ret_val =
      ps_flow_set_assoc_flow( ps_flow_ptr, assoc_flow_ptr, NULL, ps_errno);

    if (0 != ret_val)
    {
      LOG_MSG_INFO3( "ps_flow_set_assoc_flow() failed, flow 0x%p err %d",
                     assoc_flow_ptr, *ps_errno, 0);
      break;
    }

  } while (0);

  /*-----------------------------------------------------------------------
    Release memory allocated for the qos_req_param
  -----------------------------------------------------------------------*/
  ps_iface_logical_flowi_free_qos_spec(qos_req_param.qos_spec);

  if (0 != ret_val)
  {
    if (NULL != ps_flow_ptr)
    {
      ps_flow_go_null_ind( ps_flow_ptr, PS_EIC_NOT_SPECIFIED );
      (void) ps_iface_delete_flow( ps_iface_ptr, ps_flow_ptr, ps_errno);
    }

    LOG_MSG_FUNCTION_EXIT( "Fail, flow 0x%p", assoc_flow_ptr, 0, 0);
  }
  else
  {
    LOG_MSG_FUNCTION_EXIT( "Created flow 0x%p for assoc flow 0x%p",
                           ps_flow_ptr, assoc_flow_ptr, 0);
  }

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return ret_val;

} /* ps_iface_logical_flowi_handle_nw_init_qos_flow() */

int32 ps_iface_logical_flowi_process_fltr_aux_info_updated_ev
(
  ps_iface_type * assoc_ps_iface_ptr,
  ps_flow_type  * assoc_flow_ptr,
  ps_flow_type  * logical_flow_ptr,
  int16         * ps_errno
)
{
  ip_filter_spec_type   rx_ip_filter_spec; 
  ip_filter_type      * rx_ip_filter_list; 
  ip_filter_spec_type   tx_ip_filter_spec; 
  ip_filter_type      * tx_ip_filter_list;
  int32                 ret_val;
  uint8                 rx_fltr_cnt;   
  uint8                 tx_fltr_cnt;   
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "assoc iface 0x%p, assoc flow 0x%p "
                          "logical flow 0x%p",
                          assoc_ps_iface_ptr, assoc_flow_ptr, logical_flow_ptr);

  do
  {
    PS_ENTER_CRIT_SECTION( &global_ps_crit_section);

    if ( !PS_IFACE_IS_VALID( assoc_ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid iface 0x%p", assoc_ps_iface_ptr, 0, 0);
      *ps_errno = DS_EINVAL;
      break;
    }

    if ( !PS_IFACEI_IS_LOGICAL( assoc_ps_iface_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Iface 0x%x:%d not logical",
                             assoc_ps_iface_ptr->name, 
                             assoc_ps_iface_ptr->instance, 0);
      *ps_errno = DS_EINVAL;
      break;
    }

    if ( !PS_FLOW_IS_VALID( assoc_flow_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid flow 0x%p", assoc_flow_ptr, 0, 0);
      *ps_errno = DS_EINVAL;
      break;
    }

    if ( PS_FLOW_GET_CAPABILITY( assoc_flow_ptr, PS_FLOW_CAPABILITY_DEFAULT))
    {
      LOG_MSG_INVALID_INPUT( "Associated Flow 0x%p cannot be default flow",
                             assoc_flow_ptr, 0, 0);
      *ps_errno = DS_EINVAL;
      break;
    }

    if ( !PS_FLOW_IS_VALID( logical_flow_ptr))
    {
      LOG_MSG_INVALID_INPUT( "Invalid Flow 0x%p", logical_flow_ptr, 0, 0);
      *ps_errno = DS_EINVAL;
      break;
    }

    if ( PS_FLOW_GET_CAPABILITY( logical_flow_ptr, PS_FLOW_CAPABILITY_DEFAULT))
    {
      LOG_MSG_INVALID_INPUT( "Logical Flow 0x%p cannot be default flow",
                             logical_flow_ptr, 0, 0);
      *ps_errno = DS_EINVAL;
      break;
    }
        
    if ( PS_FLOW_GET_QOS_FIELD_MASK( logical_flow_ptr) & QOS_MASK_RX_FLOW)
    {
      /*-----------------------------------------------------------------------
        Handle Rx Filter Spec
      -----------------------------------------------------------------------*/
      rx_fltr_cnt = 
        ps_flowi_get_rx_fltr_cnt_ex( assoc_flow_ptr, FALSE);

      rx_ip_filter_list =
        ps_system_heap_mem_alloc( rx_fltr_cnt * sizeof( ip_filter_type));
      rx_ip_filter_spec.list_ptr = rx_ip_filter_list;

      ret_val = ps_flow_get_rx_fltr_spec( assoc_ps_iface_ptr,
                                          assoc_flow_ptr,
                                          &rx_ip_filter_spec,
                                          FALSE,
                                          rx_fltr_cnt,
                                          ps_errno);
      
      if (0 != ret_val)
      {
        LOG_MSG_INFO3( "ps_flow_get_rx_fltr_spec() failed while "
                       "copying rx fltr spec, flow 0x%p, err %d",
                       assoc_flow_ptr, *ps_errno, 0);

        *ps_errno = DS_EINVAL;
        PS_SYSTEM_HEAP_MEM_FREE( rx_ip_filter_list);
        break;
      }
      
      ret_val =
        ps_flow_set_rx_filter_spec( 
          logical_flow_ptr->flow_private.iface_ptr,
          logical_flow_ptr,
          &rx_ip_filter_spec,
          FALSE);
      
      PS_SYSTEM_HEAP_MEM_FREE( rx_ip_filter_list);

      if (0 != ret_val)
      {
        LOG_MSG_INFO3( "ps_flow_set_rx_filter_spec() failed while "
                       "setting rx fltr spec, flow 0x%p",
                       logical_flow_ptr, 0, 0);
        *ps_errno = DS_EINVAL;
        break;
      }
    }

    if ( PS_FLOW_GET_QOS_FIELD_MASK( logical_flow_ptr) & QOS_MASK_TX_FLOW)
    {  
      /*-----------------------------------------------------------------------
        Handle Tx Filter Spec
      -----------------------------------------------------------------------*/
      tx_fltr_cnt = 
        ps_ifacei_get_tx_fltr_cnt_ex( assoc_ps_iface_ptr, 
                                      assoc_flow_ptr, 
                                      FALSE);
      tx_ip_filter_list =
        ps_system_heap_mem_alloc( tx_fltr_cnt * sizeof( ip_filter_type));
      tx_ip_filter_spec.list_ptr = tx_ip_filter_list;

      ret_val = ps_iface_flow_get_tx_fltr_spec( assoc_ps_iface_ptr,
                                                assoc_flow_ptr,
                                                &tx_ip_filter_spec,
                                                FALSE,
                                                tx_fltr_cnt,
                                                ps_errno);
      
      if (0 != ret_val)
      {
        LOG_MSG_INFO3( "ps_iface_flow_get_tx_fltr_spec() failed while "
                       "copying tx fltr spec, flow 0x%p, err %d",
                       assoc_flow_ptr, *ps_errno, 0);

        *ps_errno = DS_EINVAL;
        PS_SYSTEM_HEAP_MEM_FREE( tx_ip_filter_list);
        break;
      }
      
      ret_val =
        ps_iface_flow_set_tx_filter_spec( 
          logical_flow_ptr->flow_private.iface_ptr,
          logical_flow_ptr,
          &tx_ip_filter_spec,
          FALSE);
      
      PS_SYSTEM_HEAP_MEM_FREE( tx_ip_filter_list);

      if (0 != ret_val)
      {
        LOG_MSG_INFO3( "ps_iface_flow_set_tx_filter_spec() failed while "
                       "setting tx fltr spec, flow 0x%p",
                       logical_flow_ptr, 0, 0);
        *ps_errno = DS_EINVAL;
        break;
      }

    }

    
    LOG_MSG_FUNCTION_EXIT( "assoc iface 0x%p, assoc flow 0x%p "
                           "logical flow 0x%p",
                           assoc_ps_iface_ptr, assoc_flow_ptr, logical_flow_ptr);

    PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
    return 0;

  } while (0);

  LOG_MSG_FUNCTION_EXIT( "assoc iface 0x%p, assoc flow 0x%p "
                         "logical flow 0x%p",
                         assoc_ps_iface_ptr, assoc_flow_ptr, logical_flow_ptr);

  PS_LEAVE_CRIT_SECTION( &global_ps_crit_section);
  return -1;

} /* ps_iface_logical_flowi_process_fltr_aux_info_updated_ev() */

#endif /* FEATURE_DATA_PS */
