#ifndef DSQMH_NETPLAT_H
#define DSQMH_NETPLAT_H
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

  $Header: //source/qcom/qct/modem/datacommon/qmiifacectls/rel/09.02.01/inc/ds_qmh_netplat.h#3 $

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

#include "ps_iface_ioctl.h"
#ifdef FEATURE_DSS_LINUX
#include <netinet/in.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/if.h>
#include "netmgr.h"
#endif

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/

typedef enum {
  DS_NETPLAT_RSP_INVALID,
  DS_NETPLAT_RSP_BRINGUP,
  DS_NETPLAT_RSP_TEARDOWN,
  DS_NETPLAT_RSP_QOS_REQUEST,
  DS_NETPLAT_RSP_QOS_RELEASE,
  DS_NETPLAT_RSP_MAX
} ds_netplat_rsp_msg_type;

typedef enum {
  DS_NETPLAT_IND_INVALID,
  DS_NETPLAT_IND_PLATFORM_UP,
  DS_NETPLAT_IND_PLATFORM_DOWN,
  DS_NETPLAT_IND_QOS_REQUEST,
  DS_NETPLAT_IND_QOS_RELEASE,
  DS_NETPLAT_IND_MAX
} ds_netplat_ind_msg_type;


typedef enum {
  DS_NETPLAT_ERR_NONE,
  DS_NETPLAT_ERR_FAILED,
  DS_NETPLAT_ERR_MAX
} ds_netplat_err_type;

typedef struct ds_netplat_addrinfo_s {
  boolean                      valid;
  unsigned int                 flags;
  struct ps_sockaddr_storage   addr;
  struct {
    unsigned int               prefered;  /* IPV6 only */
    unsigned int               valid;     /* IPV6 only */
  } ipv6_lifetime;
} ds_netplat_addrinfo_type;

typedef struct
{
  int32                     conn_id;
  int32                     flow_id;
  void                     *info_ptr;
} ds_netplat_info_type;


typedef void (*platform_cmd_hdlr_type)
(
  ds_netplat_rsp_msg_type   rsp_id,
  ds_netplat_err_type       err_id,
  void                     *user_data  
); 

typedef void (*platform_ind_hdlr_type)
(
  ds_netplat_ind_msg_type   ind_id,
  ds_netplat_info_type     *ind_info,
  void                     *user_data  
); 


#ifdef FEATURE_DSS_LINUX

#define DSQMH_NETPLAT_DEVICE_NAME_LEN NETMGR_IF_NAME_MAX_LEN

typedef struct 
{
  int   info_valid;
  char  dev_name[ DSQMH_NETPLAT_DEVICE_NAME_LEN ];
  int   if_index;
}dsqmh_netplat_device_info_type;
#endif /*FEATURE_DSS_LINUX*/

typedef struct dsqmh_netplat_conn_s {
  int32                     conn_id;     
  platform_ind_hdlr_type    ind_hdlr;
  void                     *ind_hdlr_user_data;
#ifdef FEATURE_DSS_LINUX
  netmgr_client_hdl_t       handle;
  ds_netplat_addrinfo_type  addr_info;
  dsqmh_netplat_device_info_type  dev_info;
#endif /* FEATURE_DSS_LINUX */
} dsqmh_netplat_conn_type;



/*===========================================================================
                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION DS_QMH_NETPLAT_OPEN

DESCRIPTION
  This function ...

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
);


/*===========================================================================
FUNCTION DS_QMH_NETPLAT_CLOSE

DESCRIPTION
  This function ...

PARAMETERS

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
);


/*===========================================================================
FUNCTION DS_QMH_NETPLAT_BRINGUP

DESCRIPTION
  This function ...

PARAMETERS

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
);


/*===========================================================================
FUNCTION DS_QMH_NETPLAT_TEARDOWN

DESCRIPTION
  This function ...

PARAMETERS

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
);


/*===========================================================================
FUNCTION DS_QMH_NETPLAT_QOS_REQUEST

DESCRIPTION
  This function ...

PARAMETERS

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
);

/*===========================================================================
FUNCTION DS_QMH_NETPLAT_QOS_RELEASE

DESCRIPTION
  This function ...

PARAMETERS

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
);


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
);

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
);

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
);

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
void ds_qmh_netplat_init( void );

#endif /* FEATURE_DATA_PS && FEATURE_QMI_CLIENT */

#endif    /* DSQMH_NETPLAT_H */
