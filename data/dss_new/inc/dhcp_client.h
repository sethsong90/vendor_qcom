#ifndef DHCP_CLIENT_H
#define DHCP_CLIENT_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        D H C P _ C L I E N T . H

GENERAL DESCRIPTION
  DMSS Dynamic Host Configuration Protocol client public header file.
  This header file contains the public function to interface with the
  DHCP client.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  - The DHCP client is started by calling dhcp_client_start().
  - The DHCP client is stoped by calling dhcp_client_stop().

Copyright (c) 2004-2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                        EDIT HISTORY FOR MODULE

$Id: //source/qcom/qct/modem/api/datamodem/main/latest/dhcp_client.h#1 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/07/09    kk     Added support for client identifier option(opt type=61).
05/14/09    pp     Moved Local Heap allocations to Modem Heap.
03/26/09    pp     CMI De-featurization.
12/14/08    pp     Common Modem Interface: Public/Private API split.
08/25/08    am     Added DHCP ongoing determination.
01/29/08    am     Added Rapid Commit Option handling.
04/15/07    es     Removed DHCP IOCTL callback functions, added params_init
                   function to initialize client parameters.  Added unicast
                   address and param request booleans to params struct.
02/01/07    hm     Added IOCTL to get fresh DHCP configuration.
08/12/04    clp    Initial development work done.
===========================================================================*/
#include "ps_iface.h"
#include "dhcp_client_common.h"

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
The DHCP client needs to know about media connects and disconnects.
This will be done with start and stop calls.  Similarly, the DHCP
client needs to inform the network stack of when the current address
is not valid any longer.  This will done through the Configuration
shim layer.  In addition, the DHCP client layer will need
initialization before starting (for memory pool initialization, etc).
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*==========================================================================
FUNCTION DHCP_CLIENT_PARAMS_INIT()

Description:
  This function will initialize a dhcp_client_params struct with
  suggested default values.

Arguments:
  params          - The handle to the params struct to fill with default
                    values.
  hw_addr         - The hw_addr identifying the DHCP client.
  hw_addr         - The length of the hw_addr passed in 'hw_addr'.

Return value:
  Returns -1 if failed, 0 if OK.

Dependencies:
  The 'params' handle should not be NULL and space already allocated
  for the struct.
===========================================================================*/

/* The Params struct contains parameters which determine the behavior of
   the DHCP client */
typedef struct
{
  /* Cookie to verify initialization of struct.
   * This is a private internal value. */
  uint32 cookie;

  /* Default value is DHCP_CLIENT_HWTYPE_ETHERNET */
  dhcp_client_hw_type_enum  hw_type;
  uint32 hw_addr_len;
  uint8  *hw_addr;

  /* The following params determine if the client request these
   * options in the Parameter Request List Option.
   * If the options are not requested, they are subsequently
   * not set in the ps_iface even if the DHCP server responds
   * with the relevant configuration info.
   * Suggested value for these options is TRUE. */
  boolean domain_name_server_option; /* RFC 2132, 3.8 */
  boolean subnet_mask_option;        /* RFC 2132, 3.3 */
  boolean router_option;             /* RFC 2132, 3.5 */
  boolean interface_mtu_option;      /* RFC 2132, 5.1 */
  boolean domain_name_option;        /* RFC 2132, 3.17 */
  boolean sip_server_option;         /* RFC 3361 */
  boolean rapid_commit_option;       /* RFC 4039 */

  /* Number of times to try at various states */
        /* The number of tries to request at startup before starting
         * over with discover. Suggest 2
         */
  uint32 reboot_tries;
        /* The number of tries to send the discover before starting
         * over in the init state.  This is not well defined in the
         * RFC, but allows us to count the number of times through the
         * init state to indicate a failure to get a lease.  Suggest 4
         */
  uint32 selecting_tries;
        /* The number of tries to send the request message in the
         * requesting state.  Suggest 4
         */
  uint32 requesting_tries;

  /* Number of times before "failing" */
        /* This is not specified by RFC, but this is the number of
         * times we end up in the init state without successfully
         * getting a lease before we tell the upper layer that we have
         * failed to get a lease.  The upper layer may have us
         * continue (our default), or to abort.  Suggest 3
         */
  uint32 init_tries;
        /* This is also not specified by RFCm by this is the number of
         * timers we try to send an inform message before we tell the
         * upper layer that we have failed to get provisioning.  We
         * will stop trying on failure, per RFC 2131, section 4.4.3.
         * Suggest 4
         */
  uint32 inform_tries;

  /* Times to wait at the beginning of various states */
        /* The amount of time to wait before starting the init state
         * after a decline. RFC suggests a minimum of 10 (seconds)
         */
  uint32 decline_restart_time;
        /* This sets the maximum random time (between 1 second and
         * this value) to wait before sending the request in the init
         * state.  The RFC suggests 10 (seconds)
         */
  uint32 init_restart_time;
        /* This is the time to allow the ARP layer to verify the
         * address that we got in the discover.  The RFC gives no
         * guidence.  Suggest 2 (seconds)
         */
  uint32 verify_time;

  /* Base times for exponential back off.  These are the times that
   * are doubled on each failure to get the expected result to retry,
   * as specified by RFC 2131, section 4.1
   */
        /* This sets the initial time between requests in the reboot
         * state.  The RFC suggests 4 (seconds).
         */
  uint32 reboot_base_time;
        /* This sets the initial time between discover messages in the
         * select state.  The RFC gives minimal guidence. Suggest 4
         * (seconds)
         */
  uint32 select_base_time;
        /* This sets the initial time between request messages in the
         * requesting state.  Suggest 4 (seconds)
         */
  uint32 request_base_time;
        /* This sets the initial time between inform messages in the
         * inform state.  Suggest 4 (Seconds)
         */
  uint32 inform_base_time;

  /* Address DHCPv4 that client tries to send DHCP messages to
   * when send_broadcast is true. Default is DHCP_CLIENT_BROADCAST_ADDR */
  uint32 server_addr;

  /* Default value is DHCP_CLIENT_ID_OPTTYPE_NONE, meaning that the
   * client identifier option is not used.
   * For information about the other enum values, check the enum definition
   * in dhcp_client_common.h */
  dhcp_client_id_option_type_enum client_id_type;

  /* The length of the custom client identifier, if it has been set.
   * For DHCP_CLIENT_ID_OPTTYPE_NONE and DHCP_CLIENT_ID_OPTTYPE_HWADDR,
   * this value should be set to zero.
   * This value is also bound by DHCP_CLIENT_CLIENT_ID_MIN_LEN and 
   * DHCP_CLIENT_CLIENT_ID_MAX_LEN. */
  uint32 client_id_len;

  /* Buffer that the dhcp clients should use to pass the custom client
   * identifier value, if necessary.
   * This SHOULD be the pointer to a static buffer allocated by the
   * client. */
  uint8 *client_id;

} dhcp_client_params;

int
dhcp_client_params_init
(
  dhcp_client_params    * params,
  void                  * hw_addr,
  uint32                  hw_addr_len
);

/*===========================================================================
FUNCTION DHCP_CLIENT_START()

Description:
  This function will configure the DHCP Client for the given
  interface, and returns a handle to the DHCP client object.

  This function will be serialized through the PS task.

Arguements:
  dss_iface_id_type - The interface descriptor of the interface to configure.
  dhcp_client_params params - The parameters that are to be used.
  void (*status_cb)(
    void * userdata,
    dhcp_client_status_enum status ) -
    This function will be called to indicate the obtaining or loss of an IP
    address.  See below for details on the enum.
  void * userdata - A cookie to be used when the status_cb() is called.


Return value:
    if ( NULL != info->storage )
    {
      dhcp_server_stop(&info->storage);
    }
  void * - A handle to the new DHCP client. NULL on error.

Dependencies:
  This must be called before stop, renew or release.
  The dhcp_client_init() must have been called before this function.

  This function returns the handle.  dhcp_client_stop must be called
  in all cases (except a NULL handle) since the actual initializatoin
  is serialized through the PS task.
===========================================================================*/
void *
dhcp_client_start
(
  dss_iface_id_type iface,
  dhcp_client_params * params,
  dhcp_client_status_cb_type status_cb,
  void * userdata
);

/*===========================================================================
FUNCTION DHCP_CLIENT_RENEW()

Description:
  This function starts the DHCP client.  In particular, this function
  will configure an instance of the DHCP client for provisioning the
  PS network stacks for operation.

  The actual provisioning will occur at some point in the future once
  the DHCP client has found and negotiated with a server.

  This function will be serialized through the PS task.

Arguements:
  void * dhcp_client_handle - The handle from dhcp_client_start() of the
    client to release.

Return value:
  none.

Dependencies:
  The dhcp_client_init() function must have been called previously.

  The interface passed in must in such a state that it is able to
  recieve UDP packets and pass them up to the application (DHCP
  client) before an IP address is provisioned based apon the hardware
  address.
===========================================================================*/
void
dhcp_client_renew
(
  void * dhcp_client_handle
);

/*===========================================================================
FUNCTION DHCP_CLIENT_RELEASE()

Description:
  This function will cause the DHCP client to release a lease.

  This function may not be needed and may or may not be
  implemented.

  This function will be serialized through the PS task.

Arguements:
  void * dhcp_client_handle - The handle from dhcp_client_start() of the
    client to release.

Return value:
  None.

Dependencies:
  The handle must have been allocated from a dhcp_client_start()
    function.
===========================================================================*/
void
dhcp_client_release
(
  void * dhcp_client_handle
);

/*===========================================================================
FUNCTION DHCP_CLIENT_INFORM()

Description:
  This function will cause the DHCP client to try to get provisions
  via an INFORM message without getting a lease.

  This function will be serialized through the PS task.

Arguements:
  void * dhcp_client_handle - The handle from dhcp_client_start() of the
    client to release.
  uint32 addr - The IP address we have been provisioned with, in
    network byte order.

Return value:
  None.

Dependencies:
  The handle must have been allocated from a dhcp_client_start()
    function.
===========================================================================*/
void
dhcp_client_inform
(
  void * dhcp_client_handle,
  uint32 addr
);

/*===========================================================================
FUNCTION DHCP_CLIENT_STOP()

Description:
  This function will stop a DHCP client.  This will cause the dhcp to
  close the DHCP client handle and free the associated memory.

  This function will be serialized through the PS task.

Arguements:
  dhcp_client_handle - The handle from dhcp_client_start() of the
    client to close.

Return value:
  None.

Dependencies:
  The handle must have been allocated from a dhcp_client_start()
    function.
===========================================================================*/
void
dhcp_client_stop
(
  void ** dhcp_client_handle
);

/*===========================================================================
FUNCTION DHCP_CLIENT_IS_DHCP_IN_PROGRESS

Description:
  This function returns whether DHCP client is currently running.

Arguements:
  void * handle - handle to DHCP Client core.

Return value:
  TRUE if client is running, FALSE otherwise.

Dependencies:
  The handle must have been allocated by dhcp_clienti_core_start().
===========================================================================*/
boolean
dhcp_client_is_dhcp_in_progress
(
  void * dhcp_client_handle
);

#endif /* DHCP_CLIENT_H */
