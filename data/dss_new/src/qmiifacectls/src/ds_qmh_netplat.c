/*===========================================================================

                       Q M I   M O D E   H A N D L E R

                 N E T W O R K   P L A T F O R M   L A Y E R

GENERAL DESCRIPTION
  This file contains data declarations and function prototypes for the
  QMI Proxy IFACE Network Platform Layer.

EXTERNALIZED FUNCTIONS

INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2008-2012 by Qualcomm Technologies, Inc.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datacommon/qmiifacectls/main/latest/src/ds_qmh_netplat.c#5 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
05/06/08    ar     Created module/initial version.

===========================================================================*/


/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#if defined (FEATURE_DATA_PS) && defined (FEATURE_QMI_CLIENT)
#include <unistd.h>
#include <sys/socket.h>
#include <linux/if.h>
#include <linux/if_addr.h>

#include "AEEStdDef.h"
#include "ps_ifacei_addr_v6.h"
#include "ps_routei_lo.h"
#include "ds_qmhi.h"
#include "ds_qmh_sm_int.h"
#include "ds_qmh_netplat.h"

/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

This section contains local definitions for constants, macros, types,
variables and other items needed by this module.

===========================================================================*/

#define DSQMH_NETPLAT_MAX_CONN       (DSQMH_MAX_PS_IFACES)
#define DSQMH_NETPLAT_INVALID_CONNID (-1)
#define DSQMH_NETPLAT_CONNID_ISVALID(id) \
        (DSQMH_NETPLAT_MAX_CONN > (uint32)id)

/* Module state structure */
struct dsqmh_netplat_s {
  dsqmh_netplat_conn_type conns[DSQMH_NETPLAT_MAX_CONN];  
  int                     sockfd;
  netmgr_client_hdl_t     netmgr_client_hndl;
  boolean                 initialized;
} dsqmh_netplat_state;

#define DSQMH_GET_CONN_PTR(inst)                        \
        ( DSQMH_NETPLAT_MAX_CONN >(uint32)inst )?       \
          &dsqmh_netplat_state.conns[inst] : NULL;

extern void ps_ifacei_v6_prefix_deprecated_handler(void *user_data_ptr);
extern void ps_ifacei_v6_prefix_expired_handler(void *user_data_ptr);

/*===========================================================================

                    INTERNAL FUNCTION DEFINITIONS

===========================================================================*/

LOCAL void ds_qmh_netplat_get_connection
(
   dsqmh_netplat_conn_type  **conn_pptr
)
{
  int32   i;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  DSQMH_MSG_LOW( "QMH ds_qmh_netplat_get_connection", 0, 0, 0 );

  /* Find ununsed connection slot */
  for( i=0; i<DSQMH_NETPLAT_MAX_CONN; i++)
  {
    if( DSQMH_NETPLAT_INVALID_CONNID == 
        dsqmh_netplat_state.conns[i].conn_id )
    {
      *conn_pptr = DSQMH_GET_CONN_PTR( i );
      (*conn_pptr)->conn_id = i;
      break;
    }
  }
}


/*===========================================================================
FUNCTION DS_QMH_NETPLAT_IND_CB

DESCRIPTION
  This function is registered with the NetMgr client interface to receive
  network interface events.

PARAMETERS
  event  - Event identifier
  info   - Event payload structure
  data   - Context data (cookie)
  
DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
LOCAL void ds_qmh_netplat_ind_cb
(
  netmgr_nl_events_t       event,
  netmgr_nl_event_info_t * info,
  void *                   data
)
{
  dsqmh_netplat_conn_type  *conn_ptr = NULL;
  dsqmh_iface_cblk_type    *cblk_ptr = NULL;
  ds_netplat_ind_msg_type   ind_id = DS_NETPLAT_IND_INVALID;
  ds_netplat_info_type      ind_info;
  boolean                   notify = TRUE;
  
  ASSERT( info );
  (void)data;
  DSQMH_MSG_MED( "QMH NETPLAT ind cb: event=%d link=%d flow=0x%08x",
                 event, info->link, info->flow_info.flow_id );
  
  memset( &ind_info, 0x0, sizeof(ind_info) );

  /* QMH Connection ID matches NetMgr link ID */
  ind_info.conn_id = (int32)info->link;

  /* Validate connection ID */
  if( !DSQMH_NETPLAT_CONNID_ISVALID( ind_info.conn_id ) )
  {
    DSQMH_MSG_ERROR( "Invalid connection ID (%d)",ind_info.conn_id,0,0 );
    return;
  }

  conn_ptr = DSQMH_GET_CONN_PTR( ind_info.conn_id );
  cblk_ptr = DSQMH_GET_CBLK_PTR( ind_info.conn_id );
    
  if( conn_ptr->ind_hdlr )
  {
    /* Translate NetMgr event to QMH indication */
    switch( event )
    {
      case NET_PLATFORM_UP_EV:
      case NET_PLATFORM_RECONFIGURED_EV:
        ind_id = DS_NETPLAT_IND_PLATFORM_UP;

        /* Process address information and store for later retrieval.
         * We do this here as the PS Iface layer can only process
         * address assignment after CONFIGURE state. */
        if( NETMGR_EVT_PARAM_ADDRINFO & info->param_mask )
        {
#ifdef FEATURE_DS_LINUX_ANDROID
          memcpy( &conn_ptr->addr_info.addr.ps_ss_align, &info->addr_info.addr.ip_addr.__data,
                  sizeof(conn_ptr->addr_info.addr)-sizeof(conn_ptr->addr_info.addr.ps_ss_family) );
#elif FEATURE_DSS_LINUX
          memcpy( &conn_ptr->addr_info.addr.ps_ss_align, &info->addr_info.addr.ip_addr.__ss_padding,
                  sizeof(conn_ptr->addr_info.addr)-sizeof(conn_ptr->addr_info.addr.ps_ss_family) );
#endif
          conn_ptr->addr_info.flags = info->addr_info.flags;
          DSQMH_MSG_MED( "Stored IP address info: conn=%d ss_family=0x%x flags=0x%x",
                         ind_info.conn_id, info->addr_info.addr.ip_addr.ss_family, conn_ptr->addr_info.flags );
          
          if( AF_INET == info->addr_info.addr.ip_addr.ss_family )
          {
            conn_ptr->addr_info.valid = TRUE;
            conn_ptr->addr_info.addr.ps_ss_family = IPV4_ADDR;
          }
          else if( AF_INET6 == info->addr_info.addr.ip_addr.ss_family )
          {
            conn_ptr->addr_info.valid = TRUE;
            conn_ptr->addr_info.addr.ps_ss_family = IPV6_ADDR;
            
            /* For IPV6 address, need lifetime values */
            if( NETMGR_EVT_PARAM_CACHE & info->param_mask )
            {
              conn_ptr->addr_info.ipv6_lifetime.prefered = info->addr_info.cache_info.prefered;
              conn_ptr->addr_info.ipv6_lifetime.valid    = info->addr_info.cache_info.valid;
              DSQMH_MSG_MED( "Stored IPv6 address lifetime info",0,0,0);
            }
            else
            {
              DSQMH_MSG_ERROR( "No IPV6 address lifetime data for event (%d)",
                               event,0,0 );
            }
          }
          else
          {
            DSQMH_MSG_ERROR( "Unsupported address family (%d)",
                             info->addr_info.addr.ip_addr.ss_family,0,0 );
          }
        }
        else
        {
          DSQMH_MSG_ERROR( "No address info for event (%d)",event,0,0 );
        }
        if( NETMGR_EVT_PARAM_DEVNAME & info->param_mask )
        {
          struct ifreq ifr;
          int fd;

          /*copy the network interface name*/
          std_strlcpy(conn_ptr->dev_info.dev_name,
                      info->dev_name,
                      DSQMH_NETPLAT_DEVICE_NAME_LEN );

          DSQMH_MSG_LOW( "updated device interface name:%s",
                         conn_ptr->dev_info.dev_name,0,0);

          /*fetch the linux network interface index from kernel*/
          /* Open a temporary socket of datagram type to use for issuing the ioctl */
          if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            DSQMH_MSG_ERROR( "Socket Open failed: cannot fetch if index",0,0,0);
            break;
          }

          /* Set device name in the ioctl req struct */
          memset(&ifr, 0x0, sizeof(ifr));

          std_strlcpy(ifr.ifr_name,
                      conn_ptr->dev_info.dev_name,
                      sizeof(ifr.ifr_name));

          /* Issue ioctl on the device */
          if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
            DSQMH_MSG_ERROR( "Issue SIOCGIFINDEX ioctl on the device failed, "
                             "errno: %d", errno,0,0);
            close(fd);
            break;
          }

          conn_ptr->dev_info.if_index = ifr.ifr_ifindex;
          conn_ptr->dev_info.info_valid = TRUE;

          /*  Close temporary socket */
          close(fd);

        }
        break;
        
      case NET_PLATFORM_DOWN_EV:
        /* Clear address info */
        memset( &conn_ptr->addr_info, 0x0, sizeof(conn_ptr->addr_info) );
        memset( &conn_ptr->dev_info, 0x0, sizeof(conn_ptr->dev_info) );
        
        /* Check if iface is already in DOWN state */
        if( DSPROXY_IFACE_STATE_DOWN == stm_get_state( cblk_ptr->sm_ptr ) )
        {
          /* Iface teardown already completed */
          notify = FALSE;
          DSQMH_MSG_MED( "Iface in DOWN state, skipping notify", 0,0,0 );
        }
        else
        {
          ind_id = DS_NETPLAT_IND_PLATFORM_DOWN;
        }
        break;

      case NET_PLATFORM_NEWADDR_EV:
        notify = FALSE;
        
        /* Update IPV6 address state */
        if( NETMGR_EVT_PARAM_IPADDR & info->param_mask )
        {
          DSQMH_MSG_MED( "Processing NEWADDR event: conn=%d family=0x%x",
                         ind_info.conn_id, conn_ptr->addr_info.addr.ps_ss_family,0 );

          if( AF_INET6 == info->addr_info.addr.ip_addr.ss_family )
          {
            struct ps_in6_addr       ip_addr;
            ps_ifacei_v6_addr_type  *v6_addr_ptr = NULL;

            /* Find the IPV6 address object */
#ifdef FEATURE_DS_LINUX_ANDROID
            memcpy( ip_addr.ps_s6_addr64, info->addr_info.addr.ip_addr.__data, sizeof(ip_addr.ps_s6_addr64));
#else
            memcpy( ip_addr.ps_s6_addr64, info->addr_info.addr.ip_addr.__ss_padding, sizeof(ip_addr.ps_s6_addr64));
#endif
            if( TRUE == ps_iface_find_ipv6_addr( &ip_addr, &v6_addr_ptr, NULL ) )
            {
              /* Check the addresss info flags for state change */
              if( IFA_F_DEPRECATED & info->addr_info.flags )
              {
                /* Trigger PS state machine */
                ps_ifacei_v6_prefix_deprecated_handler( v6_addr_ptr );
              }
            }
            else
            {
              DSQMH_MSG_ERROR( "Cannot find IPV6 address on interfaces",0,0,0 );
            }
          }
          
          /* Update state of default address in this module, if applicable */
#ifdef FEATURE_DS_LINUX_ANDROID
          if( 0 == memcmp( &conn_ptr->addr_info.addr.ps_ss_align,
                           &info->addr_info.addr.ip_addr.__data,
                           sizeof(conn_ptr->addr_info.addr.ps_ss_align) ))
#else
            if( 0 == memcmp( &conn_ptr->addr_info.addr.ps_ss_align,
                             &info->addr_info.addr.ip_addr.__ss_padding,
                             sizeof(conn_ptr->addr_info.addr.ps_ss_align) ))
#endif
          {
            if( NETMGR_EVT_PARAM_CACHE & info->param_mask )
            {
              conn_ptr->addr_info.flags = info->addr_info.flags;
              conn_ptr->addr_info.ipv6_lifetime.prefered = info->addr_info.cache_info.prefered;
              conn_ptr->addr_info.ipv6_lifetime.valid    = info->addr_info.cache_info.valid;
              DSQMH_MSG_LOW( "Updated default IPv6 address info",0,0,0);
            }
            else
            {
              DSQMH_MSG_ERROR( "No IPV6 address lifetime data for event (%d)",
                               event,0,0 );
            }
          }
        }
        else
        {
          DSQMH_MSG_ERROR( "No address info for event (%d)",event,0,0 );
        }
        break;
        
      case NET_PLATFORM_DELADDR_EV:
        notify = FALSE;
        
        /* Update IPV6 address state */
        if( NETMGR_EVT_PARAM_IPADDR & info->param_mask )
        {
          DSQMH_MSG_MED( "Processing DELADDR event: conn=%d family=0x%x",
                         ind_info.conn_id, conn_ptr->addr_info.addr.ps_ss_family,0 );

          if( AF_INET6 == info->addr_info.addr.ip_addr.ss_family )
          {
            struct ps_in6_addr       ip_addr;
            ps_ifacei_v6_addr_type  *v6_addr_ptr = NULL;
          
            /* Find the IPV6 address object */
#ifdef FEATURE_DS_LINUX_ANDROID
            memcpy( ip_addr.ps_s6_addr64, 
                    info->addr_info.addr.ip_addr.__data, 
                    sizeof(ip_addr.ps_s6_addr64));
#else
            memcpy( ip_addr.ps_s6_addr64, 
                    info->addr_info.addr.ip_addr.__ss_padding, 
                    sizeof(ip_addr.ps_s6_addr64));
#endif
            if( TRUE == ps_iface_find_ipv6_addr( &ip_addr, &v6_addr_ptr, NULL ) )
            {
              /* Trigger PS state machine */
              ps_ifacei_v6_prefix_expired_handler( v6_addr_ptr );
            }
            else
            {
              DSQMH_MSG_ERROR( "Cannot find IPV6 address on interfaces",0,0,0 );
            }
          }
          
          /* Update state of default address in this module, if applicable */
#ifdef FEATURE_DS_LINUX_ANDROID
          if( 0 == memcmp( &conn_ptr->addr_info.addr.ps_ss_align,
                           &info->addr_info.addr.ip_addr.__data,
                           sizeof(conn_ptr->addr_info.addr.ps_ss_align) ))
#else
            if( 0 == memcmp( &conn_ptr->addr_info.addr.ps_ss_align,
                             &info->addr_info.addr.ip_addr.__ss_padding,
                             sizeof(conn_ptr->addr_info.addr.ps_ss_align) ))
#endif
          {
            /* Should DELADDR ever happen for default address? */
            DSQMH_MSG_HIGH( "Clearing default address on iface",0,0,0 );
            
            /* Clear address info */
            memset( &conn_ptr->addr_info, 0x0, sizeof(conn_ptr->addr_info) );
          }
        }
        else
        {
          DSQMH_MSG_ERROR( "No address info for event (%d)",event,0,0 );
        }
        break;
        
      case NET_PLATFORM_FLOW_ACTIVATED_EV:
        ind_id = DS_NETPLAT_IND_QOS_REQUEST;
        ind_info.flow_id = info->flow_info.flow_id;
        break;
        
      case NET_PLATFORM_FLOW_DELETED_EV:
        ind_id = DS_NETPLAT_IND_QOS_RELEASE;
        ind_info.flow_id = info->flow_info.flow_id;
        break;
        
      default:
        DSQMH_MSG_ERROR( "Uunsupported NetMgr event (%d)",event,0,0 );
        break;
    }

    if( notify )
    {
      /* Netmgr assigns link IDs in order of QMI RMNET device names.  This
       * matches that used here in QMH.  Will need to revisit if ordering
       * changes. */
      ind_info.info_ptr = (void*)info->link;
    
      /* Notify upper layer of platform event */
      conn_ptr->ind_hdlr( ind_id, &ind_info, conn_ptr->ind_hdlr_user_data );
    }
  }
  else
  {
    DSQMH_MSG_ERROR( "Unsupported NetMgr event (%d)",event,0,0 );
  }

} /* ds_qmh_netplat_ind_cb() */



/*===========================================================================
  
                    EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/


/*===========================================================================
FUNCTION DS_QMH_NETPLAT_OPEN

DESCRIPTION
  This function registers the upper layer as a client and stores the
  indication callback.  It also registers with the host platform network
  manager for events. 

PARAMETERS

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int ds_qmh_netplat_open
(
  platform_ind_hdlr_type      platform_ind_hdlr,
  void                       *user_data,
  int32                      *conn_id_ptr
)
{
  dsqmh_netplat_conn_type  *conn_ptr = NULL;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_ASSERT( conn_id_ptr, "DSQMH NETPLAT null conn_id pointer passed" );

  /* Allocation platform connection */
  ds_qmh_netplat_get_connection( &conn_ptr );

  /* Preserve the caller's indication handler. */
  if( NULL == conn_ptr )
  {
    DSQMH_MSG_ERROR( "Failed on get connection",0,0,0 );
    return DSQMH_FAILED;
  }

  conn_ptr->ind_hdlr           = platform_ind_hdlr;
  conn_ptr->ind_hdlr_user_data = user_data;

  *conn_id_ptr = conn_ptr->conn_id;
  DSQMH_MSG_MED( "ds_qmh_netplat_open: hdlr=0x%p conn=%d",
                 platform_ind_hdlr, *conn_id_ptr, 0 );

  return DSQMH_SUCCESS;
} /* ds_qmh_netplat_open() */



/*===========================================================================
FUNCTION DS_QMH_NETPLAT_CLOSE

DESCRIPTION
  This function clears the upper layer client registration. It also
  releases link with the host platform network manager. 

PARAMETERS
  conn_id  - Client connection identifier  

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int ds_qmh_netplat_close
(
  int32                      conn_id
)
{
  ps_iface_type   *iface_ptr = NULL;
  dsqmh_netplat_conn_type  *conn_ptr = NULL;
  int result = DSQMH_SUCCESS;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Validate connection ID */
  if( !DSQMH_NETPLAT_CONNID_ISVALID( conn_id ) )
  {
    DSQMH_MSG_ERROR( "Invalid connection ID (%d)",conn_id,0,0 );
    return DSQMH_FAILED;
  }
  conn_ptr = DSQMH_GET_CONN_PTR( conn_id );
  iface_ptr = DSQMH_GET_IFACE_PTR( conn_id );
  
  DSQMH_MSG_MED( "QMH ds_qmh_netplat_close: conn=%d", conn_id, 0, 0 );
  
  /* Update the loopback routing table */
  ps_route_lo_delete( iface_ptr );

  /* Clear upper layer client registration */
  conn_ptr->conn_id            = DSQMH_NETPLAT_INVALID_CONNID;
  conn_ptr->ind_hdlr           = NULL;
  conn_ptr->ind_hdlr_user_data = 0;
  conn_ptr->handle             = 0;
    
  return result;
} /* ds_qmh_netplat_close() */



/*===========================================================================
FUNCTION DS_QMH_NETPLAT_BRINGUP

DESCRIPTION
  This function sends network interface bringup command to host platform.
  For Linux, this is a no-op as the NetMgr daemon acts on Modem QMI
  events directly and posts asynchronous events to QMH.
  
PARAMETERS
  conn_id     - Client connection identifier  
  cmd_cb      - Command processing callback
  user_data   - Context data (cookie)
  
DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int ds_qmh_netplat_bringup
(
  int32                       conn_id,
  platform_cmd_hdlr_type      cmd_cb,
  void                       *user_data
)
{
  dsqmh_netplat_conn_type  *conn_ptr = NULL;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  
  DSQMH_MSG_MED( "QMH netplat_platform_up:%d conn=%d",
                 (uint32)user_data, conn_id, 0 );

  /* Validate connection ID */
  if( !DSQMH_NETPLAT_CONNID_ISVALID( conn_id ) )
  {
    DSQMH_MSG_ERROR( "Invalid connection ID (%d)",conn_id,0,0 );
    return DSQMH_FAILED;
  }
  conn_ptr = DSQMH_GET_CONN_PTR(conn_id);
  
  /* Send success response for command given this is a no-op */
  if( cmd_cb )
  {
    cmd_cb( DS_NETPLAT_RSP_BRINGUP, DS_NETPLAT_ERR_NONE, user_data );
  }

  /* Wait for Linux NetMgr daemon to post asynchronous event */
  
  return DSQMH_SUCCESS;
} /* ds_qmh_netplat_bringup() */


/*===========================================================================
FUNCTION DS_QMH_NETPLAT_TEARDOWN

DESCRIPTION
  This function sends network interface teardown command to host platform.
  For Linux, this is a no-op as the NetMgr daemon acts on Modem QMI
  events directly and posts asynchronous events to QMH.

PARAMETERS
  conn_id     - Client connection identifier  
  cmd_cb      - Command processing callback
  user_data   - Context data (cookie)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int ds_qmh_netplat_teardown
(
  int32                       conn_id,
  platform_cmd_hdlr_type      cmd_cb,
  void                       *user_data
)
{
  dsqmh_netplat_conn_type  *conn_ptr = NULL;
  ds_netplat_info_type info;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH netplat_platform_down:%d conn=%d",
                 (uint32)user_data, conn_id, 0 );

  /* Validate connection ID */
  if( !DSQMH_NETPLAT_CONNID_ISVALID( conn_id ) )
  {
    DSQMH_MSG_ERROR( "Invalid connection ID (%d)",conn_id,0,0 );
    return DSQMH_FAILED;
  }
  conn_ptr = DSQMH_GET_CONN_PTR(conn_id);
  
  /* Send success response for command given this is a no-op */
  if( cmd_cb )
  {
    cmd_cb( DS_NETPLAT_RSP_TEARDOWN, DS_NETPLAT_ERR_NONE, user_data );
  }

  /* On Linux, NetMgr is a passive QMI client so we generate event
   * here indicating platform down performed.  This will allow call
   * control to proceed with QMI stop network interface, which will
   * eventually trigger NetMgr to post actual platform down event. */
  memset( &info, 0x0, sizeof(info) );
  info.info_ptr = user_data; /* Assuming NetMgr link equals QMH instance */
  info.conn_id =  conn_id;

  conn_ptr->ind_hdlr( DS_NETPLAT_IND_PLATFORM_DOWN, &info, conn_ptr->ind_hdlr_user_data );

  return DSQMH_SUCCESS;
} /* ds_netplat_teardown() */



/*===========================================================================
FUNCTION DS_QMH_NETPLAT_QOS_REQUEST

DESCRIPTION
  This function sends QOS flow request command to host platform.
  For Linux, this is a no-op as the NetMgr daemon acts on Modem QMI
  events directly and posts asynchronous events to QMH.

PARAMETERS
  conn_id     - Client connection identifier  
  cmd_cb      - Command processing callback
  user_data   - Context data (cookie)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int ds_qmh_netplat_qos_request
(
  int32                       conn_id,
  platform_cmd_hdlr_type      cmd_cb,
  void                       *user_data
)
{
  dsqmh_netplat_conn_type  *conn_ptr = NULL;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH netplat_qos_request: conn=%d", conn_id, 0, 0 );

  /* Validate connection ID */
  if( !DSQMH_NETPLAT_CONNID_ISVALID( conn_id ) )
  {
    DSQMH_MSG_ERROR( "Invalid connection ID (%d)",conn_id,0,0 );
    return DSQMH_FAILED;
  }
  conn_ptr = DSQMH_GET_CONN_PTR(conn_id);
  
  /* Send success response for command given this is a no-op */
  if( cmd_cb )
  {
    cmd_cb( DS_NETPLAT_RSP_QOS_REQUEST, DS_NETPLAT_ERR_NONE, user_data );
  }

  /* Wait for Linux NetMgr daemon to post asynchronous event */

  return DSQMH_SUCCESS;
} /* ds_qmh_netplat_qos_request() */



/*===========================================================================
FUNCTION DS_QMH_NETPLAT_QOS_RELEASE

DESCRIPTION
  This function sends QOS flow release command to host platform.
  For Linux, this is a no-op as the NetMgr daemon acts on Modem QMI
  events directly and posts asynchronous events to QMH.

PARAMETERS
  conn_id     - Client connection identifier  
  flow_id     - QOS flow identifier
  cmd_cb      - Command processing callback
  user_data   - Context data (cookie)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int ds_qmh_netplat_qos_release
(
  int32                       conn_id,
  int32                       flow_id,
  platform_cmd_hdlr_type      cmd_cb,
  void                       *user_data
)
{
  dsqmh_netplat_conn_type  *conn_ptr = NULL;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH netplat_qos_release: conn=%d flow=%d",
                 conn_id, flow_id, 0 );

  /* Validate connection ID */
  if( !DSQMH_NETPLAT_CONNID_ISVALID( conn_id ) )
  {
    DSQMH_MSG_ERROR( "Invalid connection ID (%d)",conn_id,0,0 );
    return DSQMH_FAILED;
  }
  conn_ptr = DSQMH_GET_CONN_PTR(conn_id);
  
  /* Send success response for command given this is a no-op */
  if( cmd_cb )
  {
    cmd_cb( DS_NETPLAT_RSP_QOS_RELEASE, DS_NETPLAT_ERR_NONE, user_data );
  }

  /* Wait for Linux NetMgr daemon to post asynchronous event */

  return DSQMH_SUCCESS;
} /* ds_qmh_netplat_qos_release() */



/*===========================================================================
FUNCTION DS_QMH_NETPLAT_IOCTL_ON_DEV

DESCRIPTION
  This function queries the platform for the current global IP address on the
  associated network interface.

PARAMETERS

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
#if 0
static int ds_qmh_netplat_ioctl_on_dev
(
  const char * dev,
  unsigned int req,
  struct ifreq * ifr
)
{
  int rval = DSQMH_FAILED;

  /* Set device name in the ioctl req struct */
  (void)std_strlcpy(ifr->ifr_name, dev, sizeof(ifr->ifr_name)-1);

  /* Issue ioctl on the device */
  if( 0 > ioctl(dsqmh_netplat_state.sockfd, req, ifr) ) {
    DSQMH_MSG_ERROR( "Failed on platform IOCTL for cmd 0x%x",req,0,0 );
    goto error;
  }

  rval = DSQMH_SUCCESS;

 error:
  return rval;
}
#endif

/*===========================================================================
FUNCTION DS_QMH_NETPLAT_GET_IP_ADDRESS

DESCRIPTION
  This function queries the platform for the current global IP address on the
  associated network interface.

PARAMETERS
  conn_id        - Platform connection identifier
  address_ptr    - Pointer to IP address
  preferred_ptr  - Pointer to prefered lifetime pointer (IPV6 family only)
  valid_ptr      - Pointer to valid lifetime pointer (IPV6 family only)

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int ds_qmh_netplat_get_ip_address
(
  int32                       conn_id,
  ip_addr_type               *address_ptr,
  uint32                     *preferred_ptr,
  uint32                     *valid_ptr
)
{
  dsqmh_netplat_conn_type  *conn_ptr = NULL;

#ifdef FEATURE_DSS_LINUX
  ASSERT( address_ptr );

  DSQMH_MSG_MED( "QMH netplat_get_ip_address: conn=%d", conn_id, 0, 0 );
  
  /* Validate connection ID */
  if( !DSQMH_NETPLAT_CONNID_ISVALID( conn_id ) )
  {
    DSQMH_MSG_ERROR( "Invalid connection ID (%d)",conn_id,0,0 );
    return DSQMH_FAILED;
  }
  conn_ptr = DSQMH_GET_CONN_PTR( conn_id );

  /* Return the address infor if valid */
  if( TRUE == conn_ptr->addr_info.valid )
  {
    address_ptr->type = conn_ptr->addr_info.addr.ps_ss_family;

    if( IPV6_ADDR == address_ptr->type )
    {
      memcpy( address_ptr->addr.v6, &conn_ptr->addr_info.addr.ps_ss_align, sizeof(address_ptr->addr.v6) );
      
      /* IPV6 address lifetime parameters*/
      if( preferred_ptr )
      {
        *preferred_ptr = conn_ptr->addr_info.ipv6_lifetime.prefered;
      }
      if( valid_ptr )
      {
        *valid_ptr = conn_ptr->addr_info.ipv6_lifetime.valid;
      }
    }
    else
    {
      memcpy( &address_ptr->addr.v4, &conn_ptr->addr_info.addr.ps_ss_align, sizeof(address_ptr->addr.v4) );
    }
  }
  else
  {
    DSQMH_MSG_ERROR( "Address info invalid for conn %d", conn_id, 0, 0 );
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;
#else
  DSQMH_MSG_ERROR( "Unsupported operation", 0, 0, 0 );
  return DSQMH_FAILED;
#endif /* FEATURE_DSS_LINUX */

} /* ds_qmh_netplat_get_ip_address() */

/*===========================================================================
FUNCTION DS_QMH_NETPLAT_GET_DEVICE_INFO

DESCRIPTION
  This function queries the platform the network interface information
  such as interface name and the index of the interface.

PARAMETERS
  conn_id        - Platform connection identifier
  device_info    - Pointer to device information to be populated
  ps_errno       - ps_errno is set in case of an error return,
                   correlates to values in dserrno.h

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int ds_qmh_netplat_get_device_info
(
  int32                           conn_id,
  ps_iface_ioctl_device_info_type *device_info,
  sint15                          *ps_errno
)
{
  dsqmh_netplat_conn_type  *conn_ptr = NULL;

#ifdef FEATURE_DSS_LINUX
  DSQMH_ASSERT( device_info,"NULL  device_info input param" );
  DSQMH_ASSERT( ps_errno,"NULL ps_errno input param" );

  DSQMH_MSG_MED( "ds_qmh_netplat_get_device_info: conn=%d", conn_id, 0, 0 );

  /* Validate connection ID */
  if( !DSQMH_NETPLAT_CONNID_ISVALID( conn_id ) )
  {
    DSQMH_MSG_ERROR( "Invalid connection ID (%d)",conn_id,0,0 );
    *ps_errno = DS_EINVAL;
    return DSQMH_FAILED;
  }
  conn_ptr = DSQMH_GET_CONN_PTR( conn_id );

  if (conn_ptr->dev_info.info_valid)
  {
    std_strlcpy(device_info->device_name,
                conn_ptr->dev_info.dev_name,
                PS_MAX_DEVICE_NAME_LEN );

    device_info->if_index = conn_ptr->dev_info.if_index;
  }
  else
  {
    DSQMH_MSG_ERROR( "Device information not set by "
                     "lower layers: connection ID (%d)",conn_id,0,0 );
    *ps_errno = DS_EINVAL;
    return DSQMH_FAILED;
  }

  return DSQMH_SUCCESS;
#else
  DSQMH_MSG_ERROR( "Unsupported operation", 0, 0, 0 );
  *ps_errno = DS_EINVAL;
  return DSQMH_FAILED;
#endif /* FEATURE_DSS_LINUX */

}


/*===========================================================================
FUNCTION DS_QMH_NETPLAT_CONFIGURE_IFACE

DESCRIPTION
  This function performs platform-specific opertions necessary to
  configure the PS Iface instance for the given connection.

PARAMETERS

DEPENDENCIES
  None.

RETURN VALUE
  DSQMH_SUCCESS on successful command submission.
  DSQMH_FAILED on error condition.

SIDE EFFECTS
  None.

===========================================================================*/
int ds_qmh_netplat_configure_iface
(
  int32                       conn_id
)
{
  ps_iface_type   *iface_ptr = NULL;
  ip_addr_type     ip_addr;
  ps_ip_addr_type  ps_ip_addr;
  uint32           valid_lifetime;
  uint32           pref_lifetime;
  
  DSQMH_MSG_MED( "QMH netplat_configure_iface: conn=%d", conn_id, 0, 0 );

  iface_ptr = DSQMH_GET_IFACE_PTR( conn_id );

  /* Query the global IP address from platfrom. */
  if( DSQMH_SUCCESS !=
      ds_qmh_netplat_get_ip_address( conn_id, &ip_addr, &pref_lifetime, &valid_lifetime ) )
  {
    DSQMH_MSG_ERROR( "Failed to get IP address for conn %d",conn_id,0,0 );
    return DSQMH_FAILED;
  }

  /* Assign IP address into the PS Iface */
  if( IPV4_ADDR == ip_addr.type )
  {
    if( ps_iface_set_v4_addr( iface_ptr, (ps_ip_addr_type*)&ip_addr ) )
    {
      DSQMH_MSG_ERROR( "Failed to assign IPv4 address %d",conn_id,0,0 );
      return DSQMH_FAILED;
    }
  }
  else
  {
    /* Assign prefix and IID for IPV6 address */
    //TODO Need to get gateway_iid, assume 0 for now
#ifndef FEATURE_DATA_LINUX_LE
    if( 0 != ps_iface_apply_v6_prefix( iface_ptr, 0, ip_addr.addr.v6[0], valid_lifetime, pref_lifetime, 64 ) )
    {
      DSQMH_MSG_ERROR( "Failed to assign IPv6 address prefix %d",conn_id,0,0 );
      return DSQMH_FAILED;
    }
    if( TRUE != ps_iface_set_v6_iid( iface_ptr, ip_addr.addr.v6[1]) )
    {
      DSQMH_MSG_ERROR( "Failed to assign IPv6 address IID %d",conn_id,0,0 );
      return DSQMH_FAILED;
    }
#endif
  }

#ifndef FEATURE_DATA_LINUX_LE
  /* Update the loopback routing table */
  ps_route_lo_update( iface_ptr,(ps_ip_addr_type*)&ip_addr );
  
  /* Report back address assignment */
  memset( &ps_ip_addr, 0x0, sizeof(ps_ip_addr) );
  ps_iface_get_addr( iface_ptr, &ps_ip_addr );
  if( IPV4_ADDR == ps_ip_addr.type )
  {
    DSQMH_MSG_IPV4_ADDR( "Assigned Proxy Iface ", ps_ip_addr.addr.v4.ps_s_addr );
  }
  else if( IPV6_ADDR == ps_ip_addr.type )
  {
    DSQMH_MSG_IPV6_ADDR( "Assigned Proxy Iface ", ps_ip_addr.addr.v6.ps_s6_addr64 );
  }
  else
  {
    DSQMH_MSG_ERROR( "Unsupported IP address type %d",ps_ip_addr.type,0,0 );
  }
#endif

  return DSQMH_SUCCESS;
}


/*===========================================================================
FUNCTION DS_QMH_NETPLAT_DEINIT

DESCRIPTION
  This function de-initializes the network platform layer module.

PARAMETERS
  None.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
void ds_qmh_netplat_deinit( void )
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH netplat_deinit", 0, 0, 0 );

  /* Release Linux NetMgr to stop network interface events */
  if( NETMGR_SUCCESS !=
      netmgr_client_release( dsqmh_netplat_state.netmgr_client_hndl ) )
  {
    DSQMH_MSG_ERROR( "Failed to release with NetMgr",0,0,0 );
  }
  dsqmh_netplat_state.netmgr_client_hndl = DSQMH_INVALID_HANDLE;

  close( dsqmh_netplat_state.sockfd );

  dsqmh_netplat_state.initialized = FALSE;
  
} /* ds_qmh_netplat_deinit */


/*===========================================================================
FUNCTION DS_QMH_NETPLAT_INIT

DESCRIPTION
  This function initializes the network platform layer module.

PARAMETERS
  None.

DEPENDENCIES
  None.

RETURN VALUE
  None.

SIDE EFFECTS
  None.

===========================================================================*/
void ds_qmh_netplat_init( void )
{
  int32   i;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  DSQMH_MSG_MED( "QMH netplat_init", 0, 0, 0 );

  memset( &dsqmh_netplat_state, 0x0, sizeof(dsqmh_netplat_state) );
  
  /* Initialize each connection. */
  for( i=0; i<DSQMH_NETPLAT_MAX_CONN; i++)
  {
    dsqmh_netplat_state.conns[i].conn_id = DSQMH_NETPLAT_INVALID_CONNID;
  }

  /* Open a UDP socket of datagram type to use for issuing IOCTLs */
  if ((dsqmh_netplat_state.sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    DSQMH_MSG_ERROR( "Failed on platform socket open",0,0,0 );
    ASSERT(0);
  }

  /* Register with Linux NetMgr to receive network interface events */
  dsqmh_netplat_state.netmgr_client_hndl = DSQMH_INVALID_HANDLE;
  if( NETMGR_SUCCESS !=
      netmgr_client_register( ds_qmh_netplat_ind_cb,
                              (void*)&dsqmh_netplat_state,
                              &dsqmh_netplat_state.netmgr_client_hndl ) )
  {
    DSQMH_MSG_ERROR( "Failed to register with NetMgr",0,0,0 );
    ASSERT(0);
  }
  
  /* Ensure deregister as client on process exit */
  atexit( ds_qmh_netplat_deinit );
  dsqmh_netplat_state.initialized = TRUE;
  
} /* ds_qmh_netplat_init */

#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */
