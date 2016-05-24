/*===========================================================================

                             D S S _ P I N G . C

DESCRIPTION
 The Data Services Ping API File. Contains API functions for pinging a
 remote destination.

EXTERNALIZED FUNCTIONS
  dss_ping_init_options()
    Initializes the ping options' structure to default values.
  dss_ping_start()
    Ping the remote destination with the user specified ping options.
  dss_ping_stop()
    Stop pinging. Can be used to stop an ongoing ping session.

Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.

===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $PVCSPath: $
  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/dss_ping.c#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
24/11/08    am     High/Medium lint fixes.
10/24/08    am     Fixed compiler warnings for off-target.
04/29/07   ss      Added ping6 support/fixed issues found in testing
04/03/07   ss/msr  Fixed issues found in testing
01/22/07    ss     Created.
===========================================================================*/

/*===========================================================================

                      INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "target.h"
#include "customer.h"
#ifdef FEATURE_DATA_PS
#ifdef FEATURE_DATA_PS_PING
#ifdef FEATURE_DS_SOCKETS
#include "msg.h"
#include "dss_ping.h"
#include "dssdns.h"
#include "dss_ping_config.h"
#include "dss_ping_comm_mgr.h"
#include "ds_Utils_DebugMsg.h"


/*===========================================================================

                          DATA DECLARATIONS

===========================================================================*/



/*===========================================================================

                      EXTERNAL FUNCTION DEFINITIONS

===========================================================================*/

/*===========================================================================
FUNCTION DSS_PING_INIT_OPTIONS

DESCRIPTION
  This function initializes the ping options (e.g, number of times to ping
  the destination) to default values.

DEPENDENCIES
  None.

RETURN VALUE
  On success, return DSS_SUCCESS. On error, return DSS_ERROR.

SIDE EFFECTS
  None.
===========================================================================*/
int16 dss_ping_init_options
(
  dss_ping_config_type  *ping_configs
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if( ping_configs == NULL )
  {
    LOG_MSG_INFO1("NULL pointer", 0, 0, 0);
    return DSS_ERROR;
  }

  /* Initialize to default values */
  ping_configs->num_pings           = DSS_PING_DEFAULT_TIMES_TO_PING;
  ping_configs->num_data_bytes      = DSS_PING_DEFAULT_PING_DATA_BYTES;
  ping_configs->ttl                 = DSS_PING_DEFAULT_TTL;
  ping_configs->ping_interval_time  = DSS_PING_DEFAULT_PING_RETRANSMIT_TIME;
  ping_configs->ping_response_time_out  =
    DSS_PING_DEFAULT_PING_RESPONSE_TIME_OUT;
  ping_configs->cookie              = DSS_PING_COOKIE;

  return DSS_SUCCESS;
} /* dss_ping_init_options() */


/*===========================================================================
FUNCTION DSS_PING_START()

DESCRIPTION
 This function is used to ping the specified IP address. It is non-blocking.
 The user is notified of the ping results through the user-specified
 callback function.

DEPENDENCIES
  The ping options are specified through the dss_ping_config_type structure.
  So, before invoking the dss_ping() API, the user MUST initialize the
  dss_ping_config_type structure by calling dss_ping_init_options().
  Once the structure has been initialized with default ping options, the user
  can overwrite these default options with her own specific options.

RETURN VALUE
  On success, return a valid ping handle. On error, return DSS_ERROR and place
  the error condition in errno.


SIDE EFFECTS
  None.
===========================================================================*/
dss_ping_handle dss_ping_start
(
  dss_net_policy_info_type                *net_policy_ptr,
  char                                    *dest_addr_ptr,
  dss_ping_ip_addr_enum_type              dest_addr_type,
  dss_ping_config_type                    *app_ping_options,
  dss_ping_callback_fn_type               app_callback_fn,
  dss_ping_sess_summary_callback_fn_type  app_sess_summary_callback_fn,
  void                                    *app_user_data,
  int16                                   *dss_errno
)
{
  dss_ping_handle                         ping_handle;
  int16                                   addr_family;
  int32                                   ret_val;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2("Ping req recvd,dest addr = %s", dest_addr_ptr, 0, 0);

  if( dss_errno == NULL )
  {
    LOG_MSG_INFO1("errno is NULL", 0, 0, 0);
    return(DSS_ERROR);
  }
  *dss_errno = DSS_SUCCESS;

  if( dest_addr_ptr == NULL)
  {
    LOG_MSG_INFO1("destination addr is NULL", 0, 0, 0);
    *dss_errno = DS_EADDRREQ;
    return(DSS_ERROR);
  }

  if( dest_addr_type != DSS_PING_IPV4_ADDR && dest_addr_type != DSS_PING_IPV6_ADDR )
  {
    LOG_MSG_INFO1("Unsupported address type", 0, 0, 0);
    *dss_errno = DS_EFAULT;
    return(DSS_ERROR);
  }

  if( app_ping_options == NULL )
  {
    LOG_MSG_INFO1("ping_options is NULL", 0, 0, 0);
    *dss_errno = DS_EFAULT;
    return(DSS_ERROR);
  }

  if( app_callback_fn == NULL || app_sess_summary_callback_fn == NULL )
  {
    LOG_MSG_INFO1("ping callback func is NULL", 0, 0, 0);
    *dss_errno = DS_EFAULT;
    return(DSS_ERROR);
  }

  /*-------------------------------------------------------------------------
    Verify that ping options structure is initialized, and that all
    ping options are within valid range.
  -------------------------------------------------------------------------*/
  if( app_ping_options->cookie != DSS_PING_COOKIE )
  {
    LOG_MSG_INFO1("Ping options not initialized",0,0,0);
    *dss_errno = DS_EFAULT;
    return DSS_ERROR;
  }

  if( app_ping_options->num_data_bytes < DSS_PING_MIN_PING_DATA_BYTES ||
      app_ping_options->num_data_bytes > DSS_PING_MAX_PING_DATA_BYTES )
  {
    LOG_MSG_INFO1("Invalid pkt size,min=%d, max=%d", DSS_PING_MIN_PING_DATA_BYTES, DSS_PING_MAX_PING_DATA_BYTES, 0);
    *dss_errno = DS_EMSGSIZE;
    return(DSS_ERROR);
  }

  /*-------------------------------------------------------------------------
    Create a ping session
  -------------------------------------------------------------------------*/
  if( (ping_handle =
       dss_ping_comm_mgr_create_ping_session( net_policy_ptr,
                                              app_ping_options,
                                              app_callback_fn,
                                              app_sess_summary_callback_fn,
                                              app_user_data,
                                              dss_errno ) ) ==
       DSS_PING_INVALID_HANDLE )
  {
    LOG_MSG_ERROR("Unable to create ping session", 0, 0, 0);
    return DSS_ERROR;
  }

  /*-------------------------------------------------------------------------
    Perform DNS lookup 
  -------------------------------------------------------------------------*/
  addr_family = 
    (dest_addr_type == DSS_PING_IPV4_ADDR) ? DSS_AF_INET : DSS_AF_INET6;
  
  ret_val = dss_ping_comm_mgr_perform_dns_lookup (ping_handle, 
                                                  dest_addr_ptr, 
                                                  addr_family, 
                                                  dss_errno);
  if (DSS_SUCCESS != ret_val)
  {
    LOG_MSG_ERROR ("DNS lookup failed err %d", *dss_errno, 0, 0);
    (void) dss_ping_comm_mgr_abort_ping (ping_handle, 
                                         NULL,
                                         DSS_SUCCESS);
    return DSS_ERROR;
  }

  return ping_handle;

} /* dss_ping_start() */


/*===========================================================================
FUNCTION DSS_PING_STOP()

DESCRIPTION
 Stop pinging and close the ping session.

RETURN VALUE
  DSS_SUCCESS: if a correct ping_handle was specified.
  DSS_ERROR: if an incorrect ping_handle was specified.

SIDE EFFECTS
  None.
===========================================================================*/
int16 dss_ping_stop
(
  dss_ping_handle  ping_handle
)
{
  dss_ping_session_close_reason_type  reason;
  int16                               dss_errno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if( ping_handle == DSS_PING_INVALID_HANDLE )
  {
    LOG_MSG_ERROR("Invalid ping handle", 0, 0, 0);
    return DSS_ERROR;
  }

  LOG_MSG_INFO1("Ping sess user abort", 0, 0, 0);
  reason = DSS_PING_USER_REQ;
  dss_errno  = DSS_SUCCESS;
  return dss_ping_comm_mgr_abort_ping(ping_handle, &reason, dss_errno);
} /* dss_ping_stop() */


#endif  /* FEATURE_DS_SOCKETS */
#endif /* FEATURE_DATA_PS_PING */
#endif /* FEATURE_DATA_PS */
