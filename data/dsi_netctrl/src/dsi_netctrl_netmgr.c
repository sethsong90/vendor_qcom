/*!
  @file
  dsi_netctrl_netmgr.c

  @brief
  This file implements routines to interact with netmgrd

*/

/*===========================================================================

  Copyright (c) 2010-2014 Qualcomm Technologies, Inc. All Rights Reserved

  Qualcomm Technologies Proprietary and Confidential.

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

This section contains comments describing changes made to the module.
Notice that changes are listed in reverse chronological order.

$Header: //depot/restricted/linux/android/ril/qcril_dsi.c $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
04/07/10   js      created file

===========================================================================*/
#include "netmgr.h"
#include "dsi_netctrli.h"
#include "dsi_netctrl_platform.h"
#include "dsi_netctrl_netmgr.h"
#include "dsi_netctrl_cb_thrd.h"

dsi_netmgr_link_map_type dsi_netmgr_link_map_tbl[DSI_MAX_IFACES] =
{
  { NETMGR_LINK_RMNET_0, 0 },
  { NETMGR_LINK_RMNET_1, 1 },
  { NETMGR_LINK_RMNET_2, 2 },
  { NETMGR_LINK_RMNET_3, 3 },
  { NETMGR_LINK_RMNET_4, 4 },
  { NETMGR_LINK_RMNET_5, 5 },
  { NETMGR_LINK_RMNET_6, 6 },
  { NETMGR_LINK_RMNET_7, 7 },
  { NETMGR_LINK_RMNET_8, 8 },
  { NETMGR_LINK_RMNET_9, 9 },
  { NETMGR_LINK_RMNET_10, 10 },
  { NETMGR_LINK_RMNET_11, 11 },
  { NETMGR_LINK_RMNET_12, 12 },
  { NETMGR_LINK_RMNET_13, 13 },
  { NETMGR_LINK_RMNET_14, 14 },
  { NETMGR_LINK_RMNET_15, 15 }
};

/*===========================================================================
  FUNCTION:  dsi_netmgr_get_ip_family
===========================================================================*/
/*!
    @brief
    stores relevant parts of event data into the internal structure
    associated with the given dsi_iface_id

    @return
    DSI_ERROR
    DSI_SUCCESS
*/
/*=========================================================================*/
static int dsi_netmgr_find_ip_family
(
  const netmgr_nl_event_info_t *info,
  dsi_ip_family_t              *ipf
)
{
  int ret = DSI_ERROR;

  DSI_LOG_DEBUG("%s","dsi_netmgr_find_ip_family: ENTRY");

  if (NULL == info || NULL == ipf)
  {
    DSI_LOG_ERROR("%s","dsi_netmgr_find_ip_family: invalid param(s)");
    goto bail;
  }

  /* If there's an IP addresses included in the message */
  if (info->param_mask & NETMGR_EVT_PARAM_IPADDR)
  {
    if (AF_INET == info->addr_info.addr.ip_addr.ss_family)
    {
      *ipf = DSI_IP_FAMILY_V4;
      ret = DSI_SUCCESS;
    }
    else if (AF_INET6 == info->addr_info.addr.ip_addr.ss_family)
    {
      *ipf = DSI_IP_FAMILY_V6;
      ret = DSI_SUCCESS;
    }
  }

bail:
  DSI_LOG_DEBUG("dsi_netmgr_find_ip_family: addr=%s, ip_family=%d",
                NULL!=info?DSI_GET_IP_FAMILY(info->addr_info.addr.ip_addr.ss_family):"INVALID",
                (DSI_SUCCESS == ret) ? (int)*ipf : -1);
  return ret;
}

/*===========================================================================
  FUNCTION:  dsi_netmgr_abort_pending_start
===========================================================================*/
/*!
  @brief
  This function aborts any pending start_nw_if() attempts on the given iface

  @return
  None
*/
/*=========================================================================*/
static void dsi_netmgr_abort_pending_start
(
  int dsi_iface_id
)
{
  int          i, rc, qmi_err_code;
  dsi_store_t  *st = NULL;

  for (i = 0; i < DSI_MAX_DATA_CALLS; ++i)
  {
    st = (dsi_store_t *)dsi_store_table[i].dsi_store_ptr;

    if (st != NULL &&
        st->priv.dsi_iface_id == dsi_iface_id &&
        DSI_STATE_CALL_CONNECTING == st->priv.call_state)
    {
      DSI_LOG_DEBUG("pending start_nw_if for qdi_hndl[%d], qdi_call_hndl[%p], iface[%d]",
                    DSI_GET_QDI_HNDL(dsi_iface_id),
                    st->priv.qdi_call_hndl,
                    dsi_iface_id);

      if (DSI_INVALID_WDS_TXN != DSI_GET_WDS_TXN(dsi_iface_id))
      {
        DSI_LOG_DEBUG("%s", "dsi_netmgr_abort_pending_start: aborting call");

        rc = qdi_wds_abort(DSI_GET_QDI_HNDL(dsi_iface_id),
                           st->priv.qdi_call_hndl,
                           DSI_GET_WDS_TXN(dsi_iface_id),
                           NULL,
                           NULL,
                           &qmi_err_code);
      }
      else
      {
        DSI_LOG_DEBUG("%s", "dsi_netmgr_abort_pending_start: stopping call");

        rc = qdi_wds_stop_nw_if(DSI_GET_QDI_HNDL(dsi_iface_id),
                                st->priv.qdi_call_hndl,
                                dsi_qmi_wds_cmd_cb,
                                NULL,
                                &qmi_err_code);

      }

      if (rc < 0)
      {
        DSI_LOG_ERROR("dsi_netmgr_abort_pending_start: failed rc[%d], qmi_err[%d]",
                      rc,
                      qmi_err_code);
      }
      else
      {
        DSI_LOG_DEBUG("dsi_netmgr_abort_pending_start: successfully aborted rc=0x%x",
                      rc);
      }
    }
  }
}

/*===========================================================================
  FUNCTION:  dsi_netmgr_store_event_data
===========================================================================*/
/*!
    @brief
    stores relevant parts of event data into the internal structure
    associated with the given dsi_iface_id

    @return
    dsi_net_evt_t. Propagate the event to the upper layers only if the return
    value isn't DSI_EVT_INVALID
*/
/*=========================================================================*/
static dsi_net_evt_t dsi_netmgr_store_event_data
(
  int dsi_iface_id,
  int dsi_event,
  const netmgr_nl_event_info_t * info
)
{
  int ret = DSI_ERROR;
  int reti = DSI_SUCCESS;
  dsi_net_evt_t   evt_ret = DSI_EVT_INVALID;
  dsi_ip_family_t ipf;
  dsi_store_t *st = NULL;
  int i;

  if (!DSI_IS_ID_VALID(dsi_iface_id))
  {
    DSI_LOG_ERROR("%s","dsi_netmgr_store_event_data: bad param");
    return DSI_EVT_INVALID;
  }

  DSI_LOG_DEBUG("%s","dsi_netmgr_store_event_data: ENTRY");

  ret = DSI_ERROR;
  do
  {
    if (NULL == info)
    {
      DSI_LOG_ERROR("%s","programming err: NULL info ptr rcvd");
      break;
    }

    reti = DSI_SUCCESS;
    evt_ret = dsi_event;


    switch(dsi_event)
    {
      case DSI_EVT_NET_IS_CONN:
      case DSI_EVT_NET_RECONFIGURED:
      {
        /* Determine the IP family of the received msg */
        if (DSI_SUCCESS != dsi_netmgr_find_ip_family(info, &ipf))
        {
          DSI_LOG_ERROR("%s","unable to determine IP family");
          reti = DSI_ERROR;
          break;
        }

        /* fill ip addr */
        dsi_fill_addr_info( dsi_iface_id, ipf, info );

        for (i = 0; i < DSI_MAX_DATA_CALLS; i++)
        {
          st = (dsi_store_t *)dsi_store_table[i].dsi_store_ptr;
          if (st != NULL &&
              dsi_iface_id == st->priv.dsi_iface_id)
          {
            DSI_LOCK_MUTEX(&(st->priv.mutex));

            DSI_LOG_DEBUG("DSI call state logging: Handle %d, call state %d",
                st->priv.dsi_iface_id, st->priv.call_state);

            /* If a new call has been attempted (partial retry) move to CONNECTED state */
            if (DSI_STATE_CALL_CONNECTING == st->priv.call_state)
            {
              DSI_UPDATE_CALL_STATE(st, DSI_STATE_CALL_CONNECTED);
            }

            /* Don't send the event if the call is not yet connected */
            if (DSI_STATE_CALL_CONNECTED != st->priv.call_state)
            {
              evt_ret = DSI_EVT_INVALID;
            }

            DSI_UNLOCK_MUTEX(&(st->priv.mutex));
          }
        }
      }
      break;

      case DSI_EVT_NET_NEWADDR:
      {
        /* Determine the IP family of the received msg */
        if (DSI_SUCCESS != dsi_netmgr_find_ip_family(info, &ipf))
        {
          DSI_LOG_ERROR("%s","unable to determine IP family");
          reti = DSI_ERROR;
          break;
        }

        /* If we already don't have a valid address for the IP family, store it*/
        if (!DSI_IS_ADDR_VALID(dsi_iface_id, ipf, iface_addr))
        {
          DSI_LOG_DEBUG("storing new address for ip family=%d", ipf);
          dsi_fill_addr_info( dsi_iface_id, ipf, info );

          for (i = 0; i < DSI_MAX_DATA_CALLS; i++)
          {
            st = (dsi_store_t *)dsi_store_table[i].dsi_store_ptr;
            if (st != NULL &&
                dsi_iface_id == st->priv.dsi_iface_id)
            {
              DSI_LOCK_MUTEX(&(st->priv.mutex));

              DSI_LOG_DEBUG("DSI call state logging: Handle %d, call state %d",
                  st->priv.dsi_iface_id, st->priv.call_state);

              /* If a new call has been attempted (partial retry) move to CONNECTED state */
              if (TRUE == st->priv.partial_retry &&
                  DSI_STATE_CALL_CONNECTING == st->priv.call_state)
              {
                DSI_UPDATE_CALL_STATE(st, DSI_STATE_CALL_CONNECTED);
              }

              /* Send the NEWADDR event only if the call is already connected */
              if (DSI_STATE_CALL_CONNECTED != st->priv.call_state)
              {
                evt_ret = DSI_EVT_INVALID;
              }

              DSI_UNLOCK_MUTEX(&(st->priv.mutex));
            }
          }
        }
        else
        {
          /* Don't send the event if we already have a valid address.
             We can get this event multiple times from netmgr for an IPv6 addr */
          evt_ret = DSI_EVT_INVALID;
        }
      }
      break;

      case DSI_EVT_NET_DELADDR:
      {
        /* Determine the IP family of the received msg */
        if (DSI_SUCCESS != dsi_netmgr_find_ip_family(info, &ipf))
        {
          DSI_LOG_ERROR("%s","unable to determine IP family");
          reti = DSI_ERROR;
          break;
        }

        /* If the address is already invalid, don't send the event */
        if (!DSI_IS_ADDR_VALID(dsi_iface_id,ipf,iface_addr))
        {
          DSI_LOG_DEBUG("DSI_EVT_NET_DELADDR: address family=%d already invalid", ipf);
          evt_ret = DSI_EVT_INVALID;
        }
        else
        {
          DSI_LOG_ERROR("DSI_EVT_NET_DELADDR: invalidating address family=%d", ipf);

          /* Invalidate the address */
          DSI_SET_INVALID_ADDR(dsi_iface_id,ipf,iface_addr);

          /* Don't send this event if we haven't sent the DSI_EVT_NET_IS_CONN event */
          for (i = 0; i < DSI_MAX_DATA_CALLS; i++)
          {
            st = (dsi_store_t *)dsi_store_table[i].dsi_store_ptr;
            if (st != NULL &&
                dsi_iface_id == st->priv.dsi_iface_id)
            {
              DSI_LOCK_MUTEX(&(st->priv.mutex));

              DSI_LOG_DEBUG("DSI call state logging: Handle %d, call state %d",
                  st->priv.dsi_iface_id, st->priv.call_state);

              /* Send the DELADDR event only if the call is already connected */
              if (DSI_STATE_CALL_CONNECTED != st->priv.call_state)
              {
                evt_ret = DSI_EVT_INVALID;
              }

              DSI_UNLOCK_MUTEX(&(st->priv.mutex));
            }
          }
        }
      }
      break;

      case DSI_EVT_NET_NO_NET:
        /* Abort any pending start_nw_if() requests on this iface */
        dsi_netmgr_abort_pending_start(dsi_iface_id);

        /* Update the call_state for all active calls on this iface */
        for (i = 0; i < DSI_MAX_DATA_CALLS; i++)
        {
          st = (dsi_store_t *)dsi_store_table[i].dsi_store_ptr;

          if (st != NULL && dsi_iface_id == st->priv.dsi_iface_id )
          {
            DSI_LOCK_MUTEX(&(st->priv.mutex));
            DSI_UPDATE_CALL_STATE(st, DSI_STATE_CALL_IDLE);
            DSI_UNLOCK_MUTEX(&(st->priv.mutex));
          }
        }
        break;

      case DSI_EVT_QOS_STATUS_IND:
        /* fill event payload */
        dsi_fill_qos_info( dsi_iface_id, info );
        DSI_LOG_DEBUG( "qos flow [0x%08x] activated on iface [%d]",
                       info->flow_info.flow_id, dsi_iface_id );
        break;

      default:
        DSI_LOG_DEBUG("%s","programming err: invalid dsi_evt rcvd");
        break;

    }
    if (DSI_ERROR == reti)
    {
      break;
    }
    ret = DSI_SUCCESS;
  } while(0);

  if (DSI_SUCCESS == ret)
  {
    DSI_LOG_DEBUG("dsi_netmgr_store_event_data: EXIT success, evt_ret %d", (int)evt_ret);
  }
  else
  {
    DSI_LOG_DEBUG("dsi_netmgr_store_event_data: EXIT error, evt_ret %d", (int)evt_ret);
  }

  return evt_ret;
}

/*===========================================================================
  FUNCTION:  dsi_netmgr_map_link
===========================================================================*/
/*!
    @brief
    maps netmgr link to dsi iface id. if a match is found, dsi iface id
    is returned in dsi_iface_id placeholder.

    @return
    DSI_ERROR
    DSI_SUCCESS
*/
/*=========================================================================*/
static int dsi_netmgr_map_link
(
  netmgr_link_id_t netmgr_link,
  int * dsi_iface_id
)
{
  int i=0;
  int ret=DSI_ERROR;

  DSI_LOG_DEBUG("%s","dsi_netmgr_map_link: ENTRY");

  do
  {
    if (NULL == dsi_iface_id)
    {
      DSI_LOG_DEBUG("%s","programming err: NULL dsi_iface_id ptr");
      break;
    }

    /* find the matching dsi iface id */
    for(i=0; i<DSI_MAX_IFACES; i++)
    {
      if (dsi_netmgr_link_map_tbl[i].netmgr_link == netmgr_link)
      {
        break;
      }
    }

    if (i == DSI_MAX_IFACES)
    {
      DSI_LOG_ERROR("we don't recognize netlink link [%d]. Check " \
                    "dsi_netmgr_link_map_tbl", netmgr_link);
      break;
    }

    DSI_LOG_DEBUG("netmgr link [%d] maps to  dsi iface [%d]",
                  netmgr_link, i);

    *dsi_iface_id = i;
    ret = DSI_SUCCESS;
  } while (0);

  DSI_LOG_DEBUG("%s","dsi_netmgr_map_link: EXIT");

  return ret;
}

/*===========================================================================
  FUNCTION:  dsi_netmgr_post_event
===========================================================================*/
/*!
    @brief
    post the dsi event as a result of receiving
    netmgr event
    netmgr_link is the link returned by netmgr callback
    event is the dsi event to be repoted to the client(s)

    @return
    none
*/
/*=========================================================================*/
static void dsi_netmgr_post_event
(
  int dsi_iface_id,
  dsi_net_evt_t event
)
{
  dsi_ce_reason_t ce_reason;
  dsi_store_t * st = NULL;
  int count;
  boolean handle_found = FALSE;

  DSI_LOG_DEBUG("posting [%d] for dsi_iface_id [%d]",
                event, dsi_iface_id);

  /* notify clients that are associated with this dsi iface */
  dsi_notify_users(dsi_iface_id, event);

  if (event == DSI_EVT_NET_NO_NET)
  {
    dsi_ip_family_t ipf;

    /* this would notify the only clients who are
       still attached with this interface */
    DSI_LOG_DEBUG("notify users on interface [%d] of event "
                  "DSI_EVT_NET_NO_NET", dsi_iface_id);

    /* detach interface from this call store handle */
    for(count = 0; count < DSI_MAX_DATA_CALLS; count++)
    {
      st = (dsi_store_t *)dsi_store_table[count].dsi_store_ptr;
      if (st != NULL && st->priv.dsi_iface_id == dsi_iface_id)
      {
        handle_found = TRUE;
        break;
      }
    }
    if (TRUE == handle_found)
    {
      DSI_LOCK_MUTEX(&(st->priv.mutex));
      dsi_detach_dsi_iface(st);
      DSI_UNLOCK_MUTEX(&(st->priv.mutex));
    }
    else
    {
      DSI_LOG_INFO("No call state for index [%d]", dsi_iface_id);
    }

    /* we determine ce reason from QMI
     * set reason_code to UNKNOWN here */
    ce_reason.reason_code = DSI_CE_REASON_UNKNOWN;
    ce_reason.reason_type = DSI_CE_TYPE_UNINIT;

    /* set this interface free... */
    dsi_release_dsi_iface(dsi_iface_id, &ce_reason);
  }
}

/*===========================================================================
  FUNCTION:  dsi_process_netmgr_ev
===========================================================================*/
/*!
    @brief
    callback function registered with dsi_netctrl_cb thread for netmgr
    events

    @return
    void
*/
/*=========================================================================*/
void dsi_process_netmgr_ev
(
  netmgr_nl_events_t event,
  netmgr_nl_event_info_t * info,
  void * data
)
{
  int ret = DSI_ERROR;
  int reti = DSI_SUCCESS;
  int dsi_event = DSI_EVT_INVALID;
  int dsi_iface_id = DSI_INVALID_IFACE;
  int rc = DSI_ERROR;

  DSI_LOG_VERBOSE("%s", "dsi_process_netmgr_ev ENTRY");
  (void)data;

  DSI_GLOBAL_LOCK;

  do
  {
    ret = DSI_ERROR;

    if (NULL == info)
    {
      DSI_LOG_ERROR("%s", "NULL info ptr received");
      break;
    }

    reti = DSI_SUCCESS;
    switch(event)
    {
      case NET_PLATFORM_UP_EV:
        DSI_LOG_DEBUG("%s", "NET_PLATFORM_UP_EV received");
        dsi_event = DSI_EVT_NET_IS_CONN;
        break;
      case NET_PLATFORM_DOWN_EV:
        DSI_LOG_DEBUG("%s", "NET_PLATFORM_DOWN_EV received");
        dsi_event = DSI_EVT_NET_NO_NET;
        break;
      case NET_PLATFORM_RECONFIGURED_EV:
        DSI_LOG_DEBUG("%s", "NET_PLATFORM_RECONFIGURED_EV received");
        dsi_event = DSI_EVT_NET_RECONFIGURED;
        break;
      case NET_PLATFORM_NEWADDR_EV:
        DSI_LOG_DEBUG("%s", "NET_PLATFORM_NEWADDR_EV received");
        dsi_event = DSI_EVT_NET_NEWADDR;
        break;
      case NET_PLATFORM_DELADDR_EV:
        DSI_LOG_DEBUG("%s", "NET_PLATFORM_DELADDR_EV received");
        dsi_event = DSI_EVT_NET_DELADDR;
        break;
      case NET_PLATFORM_FLOW_ACTIVATED_EV:
        /* Used for mobile & network-initiated flows */
        DSI_LOG_DEBUG("%s", "NET_PLATFORM_FLOW_ACTIVATED_EV received");
        dsi_event = DSI_EVT_QOS_STATUS_IND;
        break;
      case NET_PLATFORM_FLOW_SUSPENDED_EV:
        /* Used for mobile & network-initiated flows */
        DSI_LOG_DEBUG("%s", "NET_PLATFORM_FLOW_SUSPENDED_EV received");
        dsi_event = DSI_EVT_QOS_STATUS_IND;
        break;
      case NET_PLATFORM_FLOW_DELETED_EV:
        /* Used for mobile & network-initiated flows */
        DSI_LOG_DEBUG("%s", "NET_PLATFORM_FLOW_DELETED_EV received");
        dsi_event = DSI_EVT_QOS_STATUS_IND;
        break;
      case NET_PLATFORM_FLOW_MODIFIED_EV:
        /* Used for mobile & network-initiated flows */
        DSI_LOG_DEBUG("%s", "NET_PLATFORM_FLOW_MODIFIED_EV received");
        dsi_event = DSI_EVT_QOS_STATUS_IND;
        break;
      case NETMGR_READY_RESP:
        DSI_LOG_DEBUG("%s","NETMGR_READY_RESP received\n");
        /* only update dsi_inited if previously query for netmgr readiness has been sent */
        if( DSI_TRUE == netmgr_ready_queried )
        {
          DSI_LOG_DEBUG("%s", "update dsi_inited to TRUE netmgr_ready_queried to FALSE\n");
          set_dsi_init_state(DSI_TRUE);
          netmgr_ready_queried = DSI_FALSE;
        }
        break;
      default:
        DSI_LOG_VERBOSE("we don't handle event %d at present",
                        event);
        reti = DSI_ERROR;
        break;
    }
    if(reti == DSI_ERROR)
    {
      break;
    }

    /* map netmgr link to dsi iface id */
    rc = dsi_netmgr_map_link(info->link, &dsi_iface_id);
    if (DSI_SUCCESS != rc)
    {
      DSI_LOG_DEBUG("dsi_netmgr_map_link failed with err [%d]",
                    rc);
      break;
    }

    if(NETMGR_READY_RESP != event)
    {
      /* no need to process further for NETMGR_READY_RESP. For the rest of other events,
         need to notify clients accordingly */

      /* cache info associated with this event */
      if (DSI_EVT_INVALID != (dsi_event = dsi_netmgr_store_event_data(dsi_iface_id, dsi_event, info)))
      {
        DSI_LOG_DEBUG("dsi_process_netmgr_ev posting event [%d]",
                      dsi_event);
        /* now post the dsi event to it's clients */
        dsi_netmgr_post_event(dsi_iface_id, dsi_event);
      }
    }

    ret = DSI_SUCCESS;
  } while(0);

  if (ret == DSI_ERROR)
  {
    DSI_LOG_VERBOSE("%s", "dsi_process_netmgr_ev EXIT with err");
  }
  else
  {
    DSI_LOG_VERBOSE("%s", "dsi_process_netmgr_ev EXIT with suc");
  }

  DSI_GLOBAL_UNLOCK;

}
