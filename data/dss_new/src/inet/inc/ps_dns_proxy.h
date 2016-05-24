#ifndef PS_DNS_PROXY_H
#define PS_DNS_PROXY_H
/*===========================================================================

                P S _ D N S _ P R O X Y . H

DESCRIPTION
   This is DNS Proxy header file.


  Copyright (c) 2010 by Qualcomm Technologies, Inc.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

  $Header: //source/qcom/qct/modem/datamodem/protocols/inet/rel/11.03/inc/ps_dns_proxy.h#1 $ $DateTime: 2011/06/17 12:02:33 $ $Author: zhasan $

===========================================================================*/

/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/

#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#include "ps_iface.h"
#include "dsm.h"
#include "dsm_queue.h"
#include "dserrno.h"

/*===========================================================================

                            DATA DECLARATIONS

===========================================================================*/

/* Total number of DNS queries that can be handled at the same time. */
#define NUM_DNS_PROXY_ENTRIES    15 


/* The server handle structure */
typedef struct 
{
  /* The socket handle for the socket we are getting DNS requests over. */
  sint15 rm_sockfd;

  /* The socket handle for the socket we are sending DNS requests over. */
  sint15 um_sockfd;

  /* NAT iface pointer */
  ps_iface_type  *nat_iface_ptr;

  /* This watermark holds any items that can't be written out right
   * away.  The items will be dequeued and sent when the write event
   * callback is called.
   */
  dsm_watermark_type rm_write_wm;
  dsm_watermark_type um_write_wm;

  /* The size of the watermark DNE, apperently in bytes */
#define DNS_PROXY_SEND_WM_SZ (2000)

  /* This is the queue head needed by the above watermark */
  q_type  rm_write_wm_q;
  q_type  um_write_wm_q;
} ps_dns_proxy_info_type;

typedef struct
{
  int16     sockfd;
  uint32    sock_event_mask;
  void     *user_data_ptr;
} ps_dns_proxy_sock_event_cmd_info_type;

typedef struct
{
  ps_iface_type  *nat_iface_ptr;
} ps_dns_proxy_enable_cmd_info_type;

typedef struct
{
  ps_iface_type  *nat_iface_ptr;
} ps_dns_proxy_disable_cmd_info_type;

typedef struct
{
  ps_iface_type  *nat_iface_ptr;
  ps_iface_event_enum_type   event;
} ps_dns_proxy_um_iface_ev_cmd_info_type;

typedef struct  ps_dns_proxy_query_entry_s
{
  uint32    src_addr;
  uint16    src_port;
  uint16    orig_query_id;
  uint16    mapped_query_id;
  uint32    time_stamp;
} ps_dns_proxy_query_entry_type;

/*===========================================================================

                        FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION DNS_PROXYI_ENABLE()

DESCRIPTION
  This function enables the DNS Proxy. It takes two iface pointers as input.
  The RM iface pointer is the iface where DNS proxy will receive requests.
  The UM iface pointer is the iface where DNS request will be forwarded to
  the actual DNS server that is configured on the UM iface.

  As of now only one DNS proxy can be run i.e. DNS proxy can enabled on
  only on one interface.

DEPENDENCIES

PARAMETERS
  nat_iface_ptr: Pointer to the iface where DNS proxy will run.
  ds_errno:     Error, if any.

RETURN VALUE
  Success  0
  Fail    -1
  

SIDE EFFECTS
  None
===========================================================================*/
int32
dns_proxyi_enable
(
 ps_iface_type               *nat_iface_ptr,
 sint15                      *ds_errno 
);

/*===========================================================================
FUNCTION DNS_PROXYI_DISABLE()

DESCRIPTION
  This function disables the DNS Proxy. Since we support only one DNS proxy
  in the system, the input nat_iface_ptr to this function should be the same
  as used when the DNS Proxy was enabled.

DEPENDENCIES

PARAMETERS
  nat_iface_ptr: The iface on which DNS proxy is running.
  ds_errno: Error number, if any.

RETURN VALUE
  Success  0
  Fail    -1
  

SIDE EFFECTS
  None
===========================================================================*/
int32
dns_proxyi_disable
(
 ps_iface_type               *nat_iface_ptr,
 sint15                      *ds_errno 
);


#endif /* FEATURE_DATA_PS */
#endif /* PS_DNS_PROXY_H */
