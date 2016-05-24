#ifndef DHCP_SERVER_MGR_H
#define DHCP_SERVER_MGR_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                           D H C P _ S E R V E R _ M G R . H

GENERAL DESCRIPTION
  AMSS Dynamic Host Configuration Protocol public header file.
  This header file contains the public function to interface with the
  DHCP server for SoftAp functionality only.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  dhcp_server_mgr_start()
    Serializes the start of a DHCP server through the PS task.
  dhcp_server_mgr_stop()
    Serializes the stop of a DHCP server through the PS task.
  dhcp_server_get_conn_info() 
    Returns the info about the devices which are currently connected.
  dhcp_server_release_conn()
    Updates the lease info associated with the client which is disconnected. 
  dhcp_server_init_params()
    Returns the default DHCP server configuration values.

Copyright (c) 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

when        who    what, where, why
--------    ---    ----------------------------------------------------------
07/02/10    cp     Moved the declarations and definitions to 
                   dhcp_server_defs.h. Removed ARP cache callback mechanism.
03/18/10    cp     Initial development work done.  Added comments.
===========================================================================*/

/*===========================================================================

                     INCLUDE FILES FOR MODULE

===========================================================================*/
#include "ps_iface.h"
#include "dhcp_server_defs.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                           EXTERNALIZED FUNTIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*===========================================================================
FUNCTION DHCP_SERVER_MGR_START()

DESCRIPTION
  This function serializes the start of a DHCP server through the PS
  task.  This will cause the DHCP server to be configured for the
  client with the provided configuration parameters and the resulting 
  handle to be returned in a callback also specified with user data.

DEPENDENCIES
  The client interface must be up.


PARAMETERS
  client      - The interface for which the DHCP is serving addresses.
  config_vals - DHCP Configuration Parameters
  done_cb     - callback function that will be called when the DHCP has started
                This may be called before this function has returned. 
                The handle will be NULL on failure. Non-null on success. 
                The userdata will also be passed to this function.
  msg_cb      - Callback invoked on various DHCP messages being rx'ed/tx'ed
  userdata    - This is user data that will be passed to done_cb.

RETURN VALUE
  boolean - TRUE if message successfully sent to PS task: Expect a
            callback.  FALSE if message not set to PS task: Do not
            expect a callback.

SIDE EFFECTS
  Causes a message to be sent to the PS task to start the DHCP server
===========================================================================*/
boolean
dhcp_server_mgr_start
( 
  ps_iface_type * client, 
  dhcp_server_config_params_s  *config_vals,
  void (*done_cb)(void * userdata, void * handle),
  dhcp_msg_cback_type msg_cb,
  void * userdata
);

/*===========================================================================
FUNCTION DHCP_SERVER_MGR_STOP()

DESCRIPTION
  This function serializes the stop of a DHCP server through the PS
  task.  This will cause the DHCP server specified to be freed.  The
  callback specifed will be called back with the userdata when this is
  done. The clients handle_ptr will be set to NULL before this
  function returns.

DEPENDENCIES
  The handle must have been allocated by a call to dhcp_server_mgr_start.

PARAMETERS
  handle_ptr - The handle to free. 
  done_cb()  - The function to call when done freeing this handle.
               Function will not be called if NULL.
  userdata   - User specifed data to be passed back when the
               callback is called.

RETURN VALUE
  boolean - TRUE if message successfully sent to PS task: Expect a
            callback.  FALSE if message not set to PS task: Do not
            expect a callback.

SIDE EFFECTS
  Causes a message to be sent to the PS task to stop the DHCP server
  specifed by the handle passed in.
===========================================================================*/
boolean
dhcp_server_mgr_stop
(
  void ** handle_ptr,
  void (*done_cb)(void * userdata, boolean ok),
  void * userdata
);

/*===========================================================================
FUNCTION DHCP_SERVER_GET_CONN_INFO()

DESCRIPTION
  This function returns the info about the devices which are 
  currently connected.

Dependencies
  The handle must have being allocated by a call to 
  dhcp_server_mgr_start. Memory must have been allocated for 
  the required number of conn_devs elements.

Parameters
  handle              - DHCP server handle.
  conn_devs           - Contains information about the 
                        connected devices.
  num_conn_devs       - contains the required number of connected devices 
                        information.

Return Value
  int32:    >= 0  least of the actual connected devices and num_conn_devs.
            < 0   operation failed.

SIDE EFFECTS
  If enough memory is not provided for the conn_devs, it may result in 
  access violation.
===========================================================================*/
int32
dhcp_server_get_conn_info
(
  void *handle,
  dhcp_server_conn_devices_info_s * conn_devs,
  uint32 num_conn_devs
);

/*===========================================================================
FUNCTION DHCP_SERVER_RELEASE_CONN

DESCRIPTION
  This function updates the lease info associated with the 
  client being disconnected by WLAN. 

Dependencies
  The handle must have being allocated by a call to 
  dhcp_server_mgr_start. 

Parameters
  handle    - DHCP server handle.
  dev_info  - Contains information about the connected devices.

Return Value
  None

SIDE EFFECTS
  None
===========================================================================*/
void
dhcp_server_release_conn
(
  void *handle,
  dhcp_server_conn_devices_info_s *dev_info
);

/*===========================================================================
FUNCTION DHCP_SERVER_INIT_PARAMS

DESCRIPTION
  This function is used to get the default DHCP server 
  configuration values.

Dependencies
  Memory must have been allocated for the config_vals.

Parameters
  config_vals - Contains the default DHCP configuration values

Return Value
  int32:    >= 0  ' operation success
            < 0   ' operation failed.

SIDE EFFECTS
  None
===========================================================================*/
int32
dhcp_server_init_params
(
  dhcp_server_config_params_s  *config_vals
);
#endif /* DHCP_SERVER_MGR_H */
