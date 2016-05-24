#ifndef DHCP6_CLIENT_H
#define DHCP6_CLIENT_H

/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        D H C P 6 _ C L I E N T . H

GENERAL DESCRIPTION
  Dynamic Host Configuration Protocol for IPV6 client public header file.
  This header file contains the public function to interface with the
  DHCP client.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  - The DHCP6 client is started by calling dhcp6_client_start().  
  - The DHCP6 client is stoped by calling dhcp6_client_stop(). 

Copyright (c) 2006-2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                           EDIT HISTORY FOR MODULE
                           
$Id: //source/qcom/qct/modem/api/datamodem/main/latest/dhcp6_client.h#1 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
05/14/09    pp     Moved Local Heap allocations to Modem Heap.
12/14/08    pp     Common Modem Interface: Public/Private API split.
08/25/08    am     Added DHCP ongoing determination.
03/26/07    es     Added support for unicast addr. Removed IOCTL changes,
                   including dhcp6i_client_status_cb_ex callback.
01/23/07    es     Changes made for MIPv6 support, eg. passing generic DUID 
                   instead of hw_addr, home_network_id_option.
                   Added dhcp6_client_duid_init functions.
02/23/07    rt     Changed dhcp6i_client_status_cb_ex() name to internal fn.
02/01/07    hm     Added IOCTL to get fresh DHCP configuration
06/29/06    es     Initial development work.
===========================================================================*/

#include "ps_iface.h"
#include "ps_in.h"
#include "dhcp_client_common.h"

/* Parameters controlling DHCPv6 client behavior */
typedef struct 
{
  /* Cookie to verify initialization of struct.
   * This is a private internal value. */
  uint32 cookie;
  
  /* client DUID */
  uint8 * client_duid;
  uint32  client_duid_len;
  
  /* The following two params determine if the client includes these 
   * options in sending a Information-Request message. */
  
  /* If set, the client ID option is included in all messages sent 
   * from the client to the server (ie. the Inform message for the 
   * stateless client). Section 18.1.5 of RFC 3315 states that 
   * "The client SHOULD include a Client Identifier option
   * to identify itself to the server. If the client does not include
   * a Client Identifier option, the server will not be able to return
   * any client-specific options to the client, or the server may
   * choose not to respond to the message at all." */
  boolean client_id_option;
  
  /* If set, the reconfigure accept option is included in all allowed 
   * messages from the client to the server (see Section A RFC 3315).
   * Section 22.20, RFC 3315 states:
   * "A client uses the Reconfigure Accept option to announce to the
   * server whether the client is willing to accept Reconfigure
   * messages. The default behavior, in the absence of this option,
   * means unwillingness to accept Reconfigure messages. */
  boolean reconfigure_accept_option;
  
  /* The following five params determine if the client request these 
   * options in the Option Request Option.
   * Suggested value for these options is true. */
  boolean sip_domain_name_list_option;  /* detailed in RFC 3319 */
  boolean sip_ipv6_list_option;         /* detailed in RFC 3319 */ 
  boolean dns_servers_option;           /* detailed in RFC 3646 */ 
  boolean dns_domain_searchlist_option; /* detailed in RFC 3646 */
  boolean info_refresh_time_option;     /* detailed in RFC 4242 */
  
  /* User should set this to TRUE if it is using mechanism defined in
   * 3GPP2 X.S0011-002-D Section 5.5.2 for obtaining MIPv6 bootstrap info,
   * and is expecting bootstrap info, otherwise bootstrap config info may
   * not be set in ps_iface */
  boolean expect_vendor_opts_bootstrap_info;

  /* detailed in draft-ietf-mip6-bootstrapping-integrated-dhc-01.txt 
   * Option code is placed in Option Request Option if set to true.  
   * default value is FALSE */
  boolean mip6_home_network_id_option;

  /* home network info option values to send to DHCPv6 server,
   * zeroed out by default */
  dhcp6_mip6_hnid_option_vals mip6_home_network_id_vals;  

  /* This is not specified by RFC - this is the number of times we 
   * try to send an information request message before we tell the upper
   * layer that we have failed to get provisioning.  
   * Suggest 4. */
  uint32 informationrequest_tries;
  
  /* The following information-request retransmission suggested values
   * are specified in RFC 3315, Section 5.5, and 14. */
  
  /* Max desyncronization delay
   * in seconds, RFC suggested value is 1 sec */
  uint32 informationrequest_max_delay; 

  /* initial retransmit time 
   * in seconds, RFC suggested value is 1 sec */
  uint32 informationrequest_irt;       

  /* max retransmit time
   * in seconds, RFC suggested value is 120 sec */
  uint32 informationrequest_max_rt;    

  /* The following information refresh time suggested values 
   * are specified in RFC 4242, Section 3.1 */
  
  /* default refresh time 
   * in seconds, RFC suggested value is 86400 */
  uint32 info_refresh_time_default;
  
  /* minimum refresh time 
   * in seconds, RFC suggested value is 600 */
  uint32 info_refresh_time_minimum;
  
  /* DHCPv6 server address to send DHCP messages to when 
   * send_multicast flag is TRUE, default value is 
   * ALL_DHCP_RELAY_AGENTS_AND_SERVERS_ADDR multicast address */
  struct ps_in6_addr server_addr;

  /* Full DHCPv6 client will require additional parameters 
   * eg. lease related params, solicit/renew/request/etc
   * retransmission parameters, that will go here. */

} dhcp6_client_params;

/*==========================================================================
FUNCTION DHCP6_CLIENT_DUID_LLT_INIT()

Description:
  This function will initialize a buffer with a DUID-LLT given the proper 
  field values. 
  
Arguments: 
  duid_buf    - A pointer to a buffer with sufficient size to store the 
                duid of the client interface.
  duid_len    - Size of buffer provided. If size is insufficient, 
                function returns -1.
  hw_type     - An enum indicating the hardware type.
  time        - A 32-bit time value.
  ll_addr     - A variable length link-layer address.
  ll_addr_len - Length of ll_addr passed in.
  
Return value:
  Returns -1 if failed, valid size of DUID in bytes if ok.

===========================================================================*/
int
dhcp6_client_duid_llt_init
(
  void                      * duid_buf,
  uint32                      duid_buf_len,
  dhcp_client_hw_type_enum    hw_type,
  uint32                      time,
  void                      * ll_addr,
  uint32                      ll_addr_len
);

/*==========================================================================
FUNCTION DHCP6_CLIENT_DUID_EN_INIT()

Description:
  This function will initialize a buffer with a DUID-EN given the proper 
  field values. 
  
Arguments: 
  duid_buf       - A pointer to a buffer with sufficient size to store the 
                   duid of the client interface.
  duid_len       - Size of buffer provided. If size is insufficient, 
                   function returns -1.
  enterprise_num - A 32-bit vendor enterprise number.
  identifier     - A variable length link-layer address.
  
Return value:
  Returns -1 if failed, valid size of DUID in bytes if ok.

===========================================================================*/
int
dhcp6_client_duid_en_init
(
  void                      * duid_buf,
  uint32                      duid_buf_len,
  uint32                      enterprise_num,
  void                      * identifier,
  uint32                      identifier_len
);

/*==========================================================================
FUNCTION DHCP6_CLIENT_DUID_LL_INIT()

Description:
  This function will initialize a buffer with a DUID-LL given the proper 
  field values. 
  
Arguments: 
  duid_buf    - A pointer to a buffer with sufficient size to store the 
                duid of the client interface.
  duid_len    - Size of buffer provided. If size is insufficient, 
                function returns -1.
  hw_type     - An enum indicating the hardware type.
  ll_addr     - A variable length link-layer address.
  ll_addr_len - Length of ll_addr passed in.
  
Return value:
  Returns -1 if failed, valid size of DUID in bytes if ok.

===========================================================================*/
int
dhcp6_client_duid_ll_init
(
  void                      * duid_buf,
  uint32                      duid_buf_len,
  dhcp_client_hw_type_enum    hw_type,
  void                      * ll_addr,
  uint32                      ll_addr_len
);

/*==========================================================================
FUNCTION DHCP6_CLIENT_PARAMS_INIT()

Description:
  This function will initialize a dhcp6_client_params struct with
  suggested default values. 
  
Arguments: 
  params      - The handle to the params struct to fill with default
                values.
  duid        - A pointer to a buffer containing the duid of the
                client interface.
  duid_len    - The length of the duid passed in duid.

Return value:
  None.

Dependencies:
  The 'params' handle should not be NULL and space already allocated
  for the struct.
===========================================================================*/
int 
dhcp6_client_params_init
(
  dhcp6_client_params * params, 
  void * duid,
  uint32 duid_len
);

/*===========================================================================
FUNCTION DHCP6_CLIENT_START()

Description:
  This function will configure the DHCP Client for the given
  interface, and returns a handle to the DHCP client object. 

  This function will be serialized through the PS task. 

Arguements:
  dss_iface_id_type - The interface descriptor of the interface to configure.
  dhcp6_client_params params - The parameters that are to be used.
  dhcp_client_status_cb_type status_cb - 
    This function will be called to indicate callback status, such as
    configuration data being provisioned.
    See below for details on the enum.
  void * userdata - A cookie to be used when the status_cb() is called.
      
Return value:
  void * - A handle to the new DHCP client. NULL on error.

Dependencies:
  This must be called before stop, renew or release.
  The dhcp6_client_init() must have been called before this function, 
  and the dhcp6_client_params_init() must have been called to 
  initialize the params struct.

  This function returns the handle.  dhcp6_client_stop must be called
  in all cases (except a NULL handle) since the actual initialization
  is serialized through the PS task. 
===========================================================================*/

void * 
dhcp6_client_start
( 
  dss_iface_id_type iface,
  dhcp6_client_params * params,
  dhcp_client_status_cb_type status_cb,
  void * userdata
);

/*===========================================================================
FUNCTION DHCP6_CLIENT_RENEW() ** NOT IMPLEMENTED **

Description:
  This function is legacy API from the DHCPv4 client, used to configure an 
  instance of the DHCP client for provisioning the PS network stacks for 
  operation.

  This function is not implemented for the stateless DHCPv6 client, and will
  generate an error message. 

Arguements:
  void * dhcp6_client_handle - The handle from dhcp6_client_start() of the
    client to release.

Return value:
  none.

Dependencies:
  none.
===========================================================================*/
void
dhcp6_client_renew
(
  void * dhcp6_client_handle
);

/*===========================================================================
FUNCTION DHCP6_CLIENT_RELEASE() ** NOT IMPLEMENTED **

Description:
  This function is legacy API from the DHCPv4 client, used to cause the DHCP 
  client to release a lease. 

  This function is not implemented for the stateless DHCPv6 client, and will
  generate an error message. 

Arguements:
  void * dhcp6_client_handle - The handle from dhcp6_client_start() of the
    client to release.

Return value:
  None.

Dependencies:
  None.
===========================================================================*/
void 
dhcp6_client_release
(
  void * dhcp6_client_handle 
);

/*===========================================================================
FUNCTION DHCP6_CLIENT_INFORM()

Description:
  This function will cause the DHCPv6 client to try to get provisions
  via an INFORMATIONREQUEST message without getting a lease.

  This function will be serialized through the PS task. 

Arguements:
  void * dhcp6_client_handle - The handle from dhcp6_client_start() of the
    client to release.
  
Return value:
  None.

Dependencies:
  The handle must have been allocated from a dhcp6_client_start()
    function. 
===========================================================================*/
void 
dhcp6_client_inform
(
  void * dhcp6_client_handle  
);

/*===========================================================================
FUNCTION DHCP6_CLIENT_STOP()

Description:
  This function will stop a DHCP client.  This will cause the dhcp to
  close the DHCP client handle and free the associated memory. 

  This function will be serialized through the PS task. 

Arguements:
  dhcp6_client_handle - The handle from dhcp6_client_start() of the
    client to close.

Return value:
  None.

Dependencies:
  The handle must have been allocated from a dhcp6_client_start()
    function. 
===========================================================================*/
void 
dhcp6_client_stop
(
  void ** dhcp6_client_handle 
);

/*=========================================================================
FUNCTION DHCP6_CLIENT_IS_DHCP_IN_PROGRESS

Description:
  This function returns whether DHCP6 client is currently running.

Arguements:
  void * dhcp6_client_handle - handle to DHCP Client core.

Return value:
  TRUE if client is running, FALSE otherwise.

Dependencies:
  The handle must have been allocated by dhcp6_client_start().
=========================================================================*/
boolean
dhcp6_client_is_dhcp_in_progress
(
  void * dhcp6_client_handle
);

#endif /* DHCP6_CLIENT_H */
