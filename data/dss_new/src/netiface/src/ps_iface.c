/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             P S _ I F A C E . C


GENERAL DESCRIPTION
  This is the implementation of the PS interface "member functions".  This
  provides the interface between the IP layer (including IP routing between
  interfaces) and the device specific IP transport mechanism, such as RLC,
  PPP/RLP or 802.3.

INITIALIZATION AND SEQUENCING REQUIREMENTS
  In order to use the functions defined in this module it is important that
  the interface data structure be "created" using the create() function.

Copyright (c) 2002-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_iface.c#5 $
  $Author: smudired $ $DateTime: 2011/07/22 07:59:04 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
06/10/11    kr     Added in_use flag check
05/06/11    rt     Added handling of IFACE_DISABLED case during tear down.
01/07/10    cp     Added ps_iface_get_mtu() API to get the mtu of the base
                   Iface.
12/08/10    asn    Support for Arbitration, code merged from active working branch
08/24/10    asn    Added routine to get logical Iface from phys Iface
05/27/10    ssh    memset ipv6 addr memory after allocation
03/09/10    ash    Drill Down to base iface of logical iface chain for ipv6
                   addrresses.
09/09/09    ss     Critical section released in error cases.
08/16/09    pp     ps_iface_fill_next_hop_addr API introduced.
07/10/09    hm     Splitting PS iface data path and control path.
06/24/09    am     Renamed hton/ntoh macros with ps_ prefix.
04/06/09    pp     Clear metainfo in error scenarios.
04/14/09    sp     ps_iface_tx_cmd: The packet is dropped, if the IFACE state
                   is DOWN.
03/26/09    pp     CMI De-featurization.
02/13/09    mjb    #include "ps_mem.h"
10/29/08    pp     ps_iface_input: graceful packet drop for invalid IP version
                   / No-memory cases.
10/08/08    pp     Metainfo Fixes.
10/03/08    pp     Metainfo optimizations/Updates.
08/18/08    vp     Featurized addr_mgmt support
07/01/08    mct    Updated for MIPv6 laptop support.
09/12/08    pp     Metainfo optimizations.
05/29/08    ssh    Memory leak in ps_ifacei_logical_default_tx_cmd()
04/29/08    am     Updated Arbitration/Policy Manager now supports OMH
                   phase-2.
05/01/08    mct    Updated tx cmd to populate meta info mac addr info for ipv6
04/04/08    ssh    Fixed the associate() APIs. New passive_disassociate() API.
02/25/08    ssh    Fix to ps_ifacei_logical_default_tx_cmd()
11/02/07    ks     Set Address Scope appropriately for multixast pkts for
                   rmnet calls.
09/01/07    ssh    QoS support for logical ifaces
06/26/07    rs     Added code to allocate and get the IGMP v1 Querier
                   present timer (used in multicasting) associated with the
                   interface.
07/02/07    msr    Fixed issues found in linger implementation
05/21/07    msr/sv Added support for IFACE linger
05/07/07    mct    IPv6 Neighbor Discovery
05/02/07    scb    Fixed bug where the ADDR_CHANGE_EV was not being called and
                   hence PPP_RESYNC was not being initiated.
04/26/07    mct    Update prefix lifetime rules and fixed handoff issue.
04/15/07    es     Added dhcp_client_handle in ps_iface_struct and set and
                   get functions for handle.
04/09/07    vp     Addition for mip6_bootstrap_info
02/21/07    scb    Fixed High LINT errors
02/14/07    scb    Fixed Critical LINT errors
02/06/07    scb    Fixed Klockwork Critical errors
12/15/06    mct    Added unused timer and deleted event support.
11/21/06    rt     Added IGMP/MLD support for MBMS.
11/06/06    mct    Supporting IPv4/IPv6 on a single interface for multicast
                   ifaces.
11/06/06    hm     Using a single free_domain_name_list function instead of
                   two different versions for SIP and DNS.
11/04/06    hm     Added support for domain name search list option for DHCPv6
11/02/06    mct    Added support for RFC3041: IPv6 Privacy Extensions.
11/01/06    rt     Changed Free SIP server and domain name fn's to external.
09/18/06    hm     Changed function ps_iface_get_handle to use the new
                   strcture of iface_id given in ps_iface_defs.h
09/05/06    ks     Include network_params_info_type struct in ps_iface
                   instead of only auth_info
08/03/06    sv     Added support for header compression filtering.
06/05/06    mp     Moved DPL link logging control block from ps_iface to
                   ps_phys_link
06/15/06    mct    Added more validation and IPv6 featurization.
06/13/06    rt     Modified set_sip_serv and set_sip_domain functions to
                   expect a pointer to an array.
06/01/06    mct    Added support for retrieving all DNS addresses of an iface.
05/31/06    rt     Modified checks in set_sip_serv and set_sip_domain
                   functions.
05/03/06    msr    Moved ps_flow specific functions and definitions to
                   ps_iface_flow.c
05/01/06    msr    Moved logging QoS state for QOS_MODIFY to ps_flow.c
04/30/06    rt     Added code for SIP server and domain names.
04/25/06    msr    L4/Tasklock code review changes
04/12/06    msr    Logging info_code along with QoS state
02/22/06    msr    Using single critical section
02/21/06    mct    IPv6 compiler fixes and gateway iid fix.
02/13/06    msr    Fixed compilation errors
02/06/06    msr    Updated for L4 tasklock/crit sections.
02/03/06    msr    Hanling wrap around of flow instance correctly
01/09/06    msr    Conforming to new QOS_MODIFY API
12/15/05    msr    Using macros instead of directly looking in to fi_result of
                   meta info
12/09/05    sv     Added support for new data path framework.
12/05/05    msr    Conforming to changes made to support fltr validation in
                   filtering library
11/30/05    msr    Logging QOS_MODIFY as well
11/07/05    rt     Fixed Coverity Errors.
10/12/05    mct    Added support for posting flow added/deleted events.
09/20/05    ks     Incrementing ps_iface tx stats for QMI.
09/09/05    jd     Added ps_iface_auth_set_params() to modify mode
                   independent authentication parameters
09/04/05    msr    Fixed incorrect Tx fltr handling in
                   ps_iface_modify_flow_accepted()
08/29/05    msr    Changed API to fetch Tx fltr spec in requested QOS and
                   pending QOS_MODIFY
08/16/05    msr    Fixed PS_BRANCH_TASKFREE()
08/15/05    mct    Fixed naming causing some issues w/C++ compilers.
08/15/05    mct    Added qos configure support.
08/15/05    msr    Added support for DPL flow logging and API to fetch
                   Tx fltr spec in requested QOS and pending QOS_MODIFY
08/15/05    kr     Added support for stats update
08/10/05    msr    Added support for QOS_MODIFY
05/13/05    ks     fixed an error in ps_iface_set_addr introduced during lint
05/12/05    mct    Lint changes.
05/03/05    msr    Not caching iface's addr family in ps_flow any more and
                   tweaked how cookie is assigned when a flow is created
04/26/05    vp     Check for multicast addr in v6_addr_match.
04/25/05    mct    Logical iface flow must be activated/deactivated on
                   association and disassociation.
04/22/05    ks     Checking for meta_info_ptr->pkt_info_ptr being NULL in
                   ps_iface_tx_cmd.
04/22/05    ks     Checking for meta_info_ptr in ps_iface_tx_cmd
04/20/05    sv     Modified ps iface filtering to use new filter pkt function.
04/19/05    mct    Added ps_ifacei_logical_default_tx_cmd for logical ifaces.
                   Added ps_iface_passive_associate(), restructured
                   ps_iface_active_assoc.
04/19/05    msr    wrapped net hop metainfo generation in ps_iface_input
                   under IS_BROADCAST
04/17/05    msr    Added new functions to handle ps_flow.
04/15/05    vp     Additional functions for getting V6 address, prefix, iid
                   Moved the code for reading IPv6 IID from NV to DS.
03/25/05    ifk    Initialize v6 ttl in ps_iface_create()
03/08/05    jd     Keep track of TX and RX pkt count
02/07/05    ks     Validating target_iface_ptr in ps_iface_set_bridge.
01/27/05    ssh    Changed include file name dsbyte.h to ps_byte.h
01/24/05    ks     Removed the ASSERT from ps_iface_get_phys_link_from_id().
01/12/05    msr    Added core review comments.
01/10/05    ks     Added support for flow logging.
01/10/05    sv     Merged IPSEC changes.
01/10/05    msr    Added support for link logging.
01/10/05    ifk    Added fragment processing for fragments returned by
                   filtering
01/08/05    hba    Created meta-info in ps_iface_input specificaly for
                   FEATURE_DATA_RM_NET when the Um iface is bridged to the RM
                   iface.
01/06/05    msr    Printing a message when ps_iface_tx_cmd() returns failure.
01/07/05    vp     Fixed IP address print statement.
12/27/04    vp     Freed meta info in ps_iface_input for non-bridged iface.
12/14/04    mct    Fixed assignment of gateway IID.
12/09/04    vp     Freed meta_info in ps_iface_input in case of bridged iface.
12/06/04    mct    Fixed IP address print statement.
11/19/04    msr    Using internal ps_iface macros instead of external ones.
10/25/04    msr    Changed the way primary phys link is maintained in
                   ps_iface
11/01/04    ks     Changed the input to tx_cmd() from ptr to ptr of
                   ps_meta_info_type to single pointer.
10/31/04    msr/ks Added code to log packets on transmit and receive side.
                   added initialization of data protocol logging structure in
                   ps_iface_create().
10/30/04    ks     Changed ptr to ptr of ps_meta_info_type to single pointer.
10/14/04   ks/msr  Added support for Data Protocol Logging and IP logging is
                   enabled.
10/13/04    vp     Removed byte order conversion in ps_iface_get_cached_addr()
                   as ipv4_addr is stored in network order in iface.
09/23/04    jd     Ensure ps_iface_input does not forward packets to a bridge
                   interface in configuring state.
                   Support for DHCP IP filters under FEATURE_DATA_RM_NET.
09/09/04    mct    Fixed a merge issue with ps_iface_remove_v6_prefix.
09/07/04    sv     Fixed ps_iface_get_v4_addr to return 0 if iface_ptr is NULL.
08/30/04    usb    Enabled Sockets filtering for all incoming packets.
08/23/04    mct    Featurized some IPV6 functions.
08/13/04    mvl    Added messaging for prefix addition.
08/12/04    sv     Modified phys_link_up/down_cmd to not to flow control the
                   iface.
08/12/04    sv     Remove PHYS_LINK_MASK specific code from ps_iface_enable
                   and disable flow
08/12/04    mvl    Added more messaging using name:instance with name in hex
08/10/04    mvl    Messages now printing hex format names.
08/10/04    ifk    Don't return -1 in apply_v6_prefix() if RA is for prefix
                   already in use.
08/06/04    mct    Fixed bug where IPV6 prefix timers being erased, ASSERT
                   in ps_iface_create, and iface state initialization.
08/03/04    mvl    Made limitations for setting of address family stricter.
08/02/04    mct    Changed macros to internal ps_iface macros and included
                   ps_ifacei_utils.h.
08/02/04    ak     Fixed up some new functions to be _V4 or _V6 instead of
                   _IPV4 or IPV6.  Also added new functions to get DNS addrs.
07/30/04  mvl/ifk  Added real prefix handling support.
07/30/04    vp     Changes due to consolidation of MTU and net_info structure
                   in ps_iface.
07/29/04    msr    Invoking callbacks when IFACE_ADDR_FAMILY_CHANGE_EV occurs.
07/28/04    ak     On setting address family, zero out the other IP address.
07/27/04    ak     Support for setting a flag indicating the ip info is
                   inherited from an underlying associated iface.
07/16/04    ak     Added code to do iface ref cnt's.
07/01/04    ifk    Removed setting of DSM kind field.
06/14/04    mct    Added change for multiple phys links per iface. Now passing
                   iface or'd with physlink flow mask to event cback.
06/11/04    vp     Changes for representation of IP addresses as struct ps_in_addr
                   or struct ps_in6_addr
05/26/04    mct    Added check for infinite valid liftime for V6 prefix. Also
                   fixed v6 compilation error.
05/03/04    vp     Byte ordering change for user defined IPv6 IID.
04/28/04    aku    Enable transmitting of packets in GOING_DOWN state.
04/30/04    usb    Getting peer info from iface_cb instead of calling cback.
04/27/04    usb    Added outgoing pkt filtering for QOS support, Passing
                   ptr instead of ref ptr to metainfo thru the stack.
04/26/04    mct    Fixed featurization error.
04/23/04    aku/rc Return link local address in IFACE_COMING_UP state also
04/23/04    vp     Fixed generate_ipv6_iid to use DS get nv functions.
04/21/04    sv     Added ps_iface_delete_all_prefixes function.
04/21/04    mct    Featurized another V6 function.
04/19/04    vp     Added function ps_iface_generate_ipv6_iid for ipv6 iid
                   generation.
04/09/04    ak     Added old phys_link up/down functions for when needing
                   old interface.
03/29/04    mct    Fixed errors when compiling with V6 turned off.
03/23/04    sv     Added ps_iface_get_linklocal_addr function.
03/18/04    ak     Handle case where ref_cnt_inc() returns an error value.
                   Also added code for dorm_status registration in
                   ref_cnt_inc() parameters.
02/25/04    kvd    send iface_addr_change indication in IFACE_CONFIGURING
                   state also as this would be valid case for MIP calls
02/18/04    usb    Client id support for ps iface IP filtering.
02/14/04    rc     Modified code to use functions instead of macros for
                   getting the ps_iface address (v4 or v6) family.
02/13/04    ak     Moved SDB handler into phys link.  Kept an interface here
                   however.
02/09/04    vp     Fixed the byte ordering in v4_mapped_v6 address in
                   ps_iface_get_cached_addr()
02/04/04    ak     Added in code that helps implement phys link and iface
                   separation.  Also some code to support events on transient
                   states.
02/03/04    aku    Added support for logical ifaces.
01/30/04    usb    Calling down indication if bring_up_cmd() fails.
01/23/04    aku    Added support to return v4 mapped v6 if caller asks for a
                   v6 address using a v4 iface.
01/12/04    aku    Externalized macros to determine IP version of ps_iface.
12/26/03    usb    Handling for new ps_iface state CONFIGURING.
12/07/03    mvl    Added support for apllying v6 prefix and getting v6 addr
11/11/03    aku/rc In ps_iface_create(), set the interface input function to
                   ip6_input instead of ip_input
10/16/03    mct    Fixed cmds to support notifications for transient
                   states(coming_up/going_down and phys_link equivalents).
10/09/03    jd     fully initialize global_iface_ptr_array
09/25/03    ifk    Added IPV6_ADDR case in ps_iface_set_addr()
08/28/03    ss     Updated ps_iface_get_handle() to accept dss_iface_id_type
08/25/03    aku    ps_mem_free() takes pointer-to-pointer as arg.
08/19/03    aku    Added call to ps_phys_link_create() in ps_iface_create().
08/15/03    aku    Added support to separate phys link from ps_iface and
                   moved events and indication functions to ps_ifacei_event.c
07/29/03    ss     Changed ps_iface_sdb_status_handler() to call the upper
                   layer handler through a function pointer
07/23/03    mvl    ps_iface_input() using a function pointer to pass
                   datagrams to the IP protocol.
07/09/03    ss     Added call to dssocki_sdb_status_handler() for SDB notif.
07/07/03    atp    Added ps_iface_sdb_status_handler() as part of SDB
                   enhancements to pass status notification to application.
05/05/03    aku    Replaced ps_iface_ioctl_name_enum_type with
                   dss_iface_ioctl_name_enum_type in ps_iface_get_handle()
05/02/03    usb    Removed instance param from ps_iface_create(), instances
                   are now assigned automatically to the ifaces in
                   sequential order. Resetting DNS info when the iface
                   goes DOWN.
03/19/03    aku    Set the ip address to zero in ps_iface_get_addr() if the
                   type field is being set to IP_ADDR_INVALID.
02/21/03    usb    Fixed bug in ps_iface_get_handle() where it was not
                   returning NULL if no matching iface is found, removed
                   TASKLOCK around call to tx_fn() for bridge iface.
02/13/03    usb    Fixed event buffer allocation to avoid memory leak and
                   dsm item corruption, fixed ipfltr_updated_ind() to
                   calculate new filter count correctly.
02/12/03    aku    Added support for flow control of non system sockets.
02/11/03    usb    changed ps_iface_ipfltr_updated_ind() to take num filters
                   actually added and deleted to calculate cur value.
02/07/03    usb    Added IP filter info to iface_cb, added event
                   notification for IFACE_IPFLTR_UPDATED_EV.
12/24/02    aku    Added ps_iface_get_handle() and support for
                   global_iface_array.
12/06/02    mvl    Fixed comiler warnings.
11/25/02    usb    Added q_type to ps_ifacei_event_buf_type to store the
                   queue on which the event buf is actually registered.
                   Enhanced error handling in event callback functions.
11/19/02    jd     Backed out routeable callbacks only on if state change
11/15/02    mvl    Added physical link statistics.
11/14/02    jd/usb Fixed teardown to return AEE_SUCCESS if no teardown fn reg'd
                   Call iface routeable event callbacks only on iface state
                   change
11/14/02    jd     Allow ps_iface_get_addr in routeable (for MIP)
11/12/02    mvl    Will report IP address changes in ROUTABLE state also.
10/31/02    mvl    Wrapped all relevant functions completely in
                   TASKLOCK()s.  Removed the delete() call.
10/25/02    usb    Flow is only disabled during phys link up cmd if we
                   are coming out of dormancy, not for fresh originations.
                   Also fixed a bug in tx_cmd().
10/08/02    mvl    Setting the kind field of the IP packet in
                   ps_iface_input(), and also forwarding to the bridged
                   interface if bridging is enabled.
10/01/02    mvl    Added gateway address to the v4_net_info.
09/17/02    mvl    Added cookie to iface_cb, calling route_flush when an
                   interface is disabled.  cback_reg() now returns success if
                   a callback is already registered.  Cleaning up the
                   net_info when interface goes down.  More tasklock fixes.
                   Added the bridge processing call.
09/06/02    mvl    fixed some bugs in new memory handling for cback events.
09/04/02    mvl    changed memory handling for event callback buffers.
09/03/02    mvl    Missing handler for bring down commands will not fail, but
                   will return success.  When physical link is in coming_up
                   or going_down state the interface has flow disabled.
                   fixed bug where state was being set to transient value
                   before ensuring that it was possible to do so (i.e. there
                   was a handler for it).  Only posting the IP addr changed
                   event in the up state.  Put TASKLOCK()s around many
                   functions.  Re-ordered functions.
08/30/02    om     Added ACL registration in pf_iface_create().
08/13/02    mvl    updated ps_iface state macros for name change
07/30/02    usb    Various Fixes: DOWN event was being posted in
                   routeable_ind() fn; tear_down_cmd() and
                   phys_link_down_cmd() were changing
                   state after calling the callback; Added an ASSERT if
                   phys_link_down_cmd() is called and iface is DOWN or
                   DISABLED.
07/24/02    mvl    Replaced ASSERT() in ps_iface_input() with MSG_HIGH() as
                   this case is possible under normal operation.
07/16/02    usb    Hack to receive IP packets int the stack until we have
                   the new routing module.
07/15/02    mvl    More IP addressing functionality.
07/11/02    mvl    Moved PS_IFACE_IS_VALID() to header file.  Also added
                   support for global event callbacks.  Updates for new PPP
                   architecture.
07/01/02    mvl    added ps_iface_set_ip_addr()
05/14/02    mvl    initial compile and checkin
03/15/02   na/mvl  created file.
===========================================================================*/
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                                INCLUDE FILES

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
#include "customer.h"
#include "comdef.h"

#ifdef FEATURE_DATA_PS
#include "msg.h"
#include "amssassert.h"

#include "dserrno.h"

#include "ds_flow_control.h"
#include "ps_iface.h"
#include "ps_ifacei.h"
#include "ps_flow.h"
#include "ps_flowi.h"
#include "ps_phys_linki.h"
#include "ps_ifacei_event.h"
#include "ps_ifacei_utils.h"
#include "ps_routei.h"
#include "ps_routei_acl.h"
#include "ps_utils.h"
#include "ps_stat_iface.h"
#include "ps_stat_flow.h"
#include "ps_stat_phys_link.h"
#include "ps_mem.h"

#ifdef FEATURE_DATACOMMON_PS_IFACE_IO
#include "ps_ip.h"
#include "ps_ip4.h"
#ifdef FEATURE_DATA_PS_IPV6
#include "ps_ip6.h"
#endif
#endif


#ifdef FEATURE_DATA_PS_DATA_LOGGING
#include "ps_logging.h"
#include "ps_logging_defs.h"
#endif /* FEATURE_DATA_PS_DATA_LOGGING */

#ifdef FEATURE_DATA_PS_IPV6
#include "ps_iface_addr_v6.h"
#include "ps_ifacei_addr_v6.h"
#ifdef FEATURE_DATA_PS_ADDR_MGMT
#include "ps_iface_addr_mgmt.h"
#endif /* FEATURE_DATA_PS_ADDR_MGMT */
#endif /* FEATURE_DATA_PS_IPV6 */

#ifdef FEATURE_DATA_PS_MCAST
#include "ps_mcasti.h"
#endif /* FEATURE_DATA_PS_MCAST */
#include "ds_Utils_DebugMsg.h"
#include "ps_policy_mgr.h"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                           FWD DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
int ps_iface_get_root_iface
(
  ps_iface_type **logical_iface_arr_ptr,
  uint8          *num_logical_iface,
  ps_iface_type **iface_arr_ptr,
  uint8           num_iface
);

int ps_iface_arbitration_tear_down_cmd
(
  ps_iface_type *this_iface_ptr,
  int16         *ps_errno,
  void          *client_data_ptr
);

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                           LOCAL DATA DECLARATIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*-------------------------------------------------------------------------
  GLOBAL_IFACE_PTR_ARRAY - Array of pointers to all of the the created
    interfaces on the system.  The semantics of this array are that empty
    slots have a value of 0, any other values indicate the slot points to an
    iface.
  GLOBAL_IFACE_PTR_ARRAY_INDEX - the next open slot in the array
---------------------------------------------------------------------------*/
ps_iface_type *global_iface_ptr_array[MAX_SYSTEM_IFACES] = {0,};
static uint8 global_iface_ptr_array_index = 0;

/*---------------------------------------------------------------------------
  Tuning the number of ps iface sip serv address buffers needed by this module
---------------------------------------------------------------------------*/
#define PS_IFACE_SIP_SERV_ADDR_BUF_SIZE  ((sizeof(ps_iface_sip_serv_addr_type) + 3) & ~3)

#ifndef FEATURE_DATA_PS_LOW_MEM_CHIPSET
/*---------------------------------------------------------------------------
  # of SIP Server Addresses. 13 for each iface. 1 1x, 1 WLAN, 3 UMTS
---------------------------------------------------------------------------*/
#define PS_IFACE_SIP_SERV_ADDR_BUF_NUM      (13 * 5)
#define PS_IFACE_SIP_SERV_ADDR_BUF_HIGH_WM  (13 * 5)
#define PS_IFACE_SIP_SERV_ADDR_BUF_LOW_WM   1

#else
/*---------------------------------------------------------------------------
  # of SIP Server Addresses. 13 for each iface. 1 1x/UMTS, 1 WLAN
---------------------------------------------------------------------------*/
#define PS_IFACE_SIP_SERV_ADDR_BUF_NUM      (13 * 2)
#define PS_IFACE_SIP_SERV_ADDR_BUF_HIGH_WM  (13 * 2)
#define PS_IFACE_SIP_SERV_ADDR_BUF_LOW_WM   1
#endif /* FEATURE_DATA_PS_LOW_MEM_CHIPSET */

/*----------------------------------------------------------------------------
  Allocate memory to hold ps_iface_sip_serv_addr along with ps_mem header
----------------------------------------------------------------------------*/
static int ps_iface_sip_serv_addr_buf_mem[PS_MEM_GET_TOT_SIZE_OPT
                                          (
                                            PS_IFACE_SIP_SERV_ADDR_BUF_NUM,
                                            PS_IFACE_SIP_SERV_ADDR_BUF_SIZE
                                          )];

#ifdef FEATURE_DATA_PS_MEM_DEBUG
/*----------------------------------------------------------------------------
  Array of pointers used to facilitate easy debugging.  The first one points
  to the ps_mem header and the latter points to ps_iface_sip_serv_addr_buf
----------------------------------------------------------------------------*/
static ps_mem_buf_hdr_type         * ps_iface_sip_serv_addr_buf_hdr[PS_IFACE_SIP_SERV_ADDR_BUF_NUM];
static ps_iface_sip_serv_addr_type * ps_iface_sip_serv_addr_buf_ptr[PS_IFACE_SIP_SERV_ADDR_BUF_NUM];
#endif /* FEATURE_DATA_PS_MEM_DEBUG */

/*---------------------------------------------------------------------------
  Tuning the number of ps iface sip serv domain name buffers needed by this
  module
---------------------------------------------------------------------------*/
#define PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_SIZE  ((sizeof(ps_iface_sip_serv_domain_name_type) + 3) & ~3)

#ifndef FEATURE_DATA_PS_LOW_MEM_CHIPSET
/*---------------------------------------------------------------------------
  # of SIP server domain names. 13 for each iface. 1 1x, 1 WLAN, 3 UMTS
---------------------------------------------------------------------------*/
#define PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_NUM       (13 * 5)
#define PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_HIGH_WM   (13 * 5)
#define PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_LOW_WM     1

#else
/*---------------------------------------------------------------------------
  # of SIP server domain names. 13 for each iface. 1 1x/UMTS, 1 WLAN
---------------------------------------------------------------------------*/
#define PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_NUM       (13 * 2)
#define PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_HIGH_WM   (13 * 2)
#define PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_LOW_WM     1
#endif /* FEATURE_DATA_PS_LOW_MEM_CHIPSET */

/*----------------------------------------------------------------------------
  Allocate memory to hold ps_iface_sip_serv_domain_name along with ps_mem header
----------------------------------------------------------------------------*/
static int ps_iface_sip_serv_domain_buf_mem[PS_MEM_GET_TOT_SIZE_OPT
                                            (
                                              PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_NUM,
                                              PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_SIZE
                                            )];

#ifdef FEATURE_DATA_PS_MEM_DEBUG
/*----------------------------------------------------------------------------
  Array of pointers used to facilitate easy debugging.  The first one points
  to the ps_mem header and the latter points to
  ps_iface_sip_serv_domain_name_buf
----------------------------------------------------------------------------*/
static ps_mem_buf_hdr_type                 * ps_iface_sip_serv_domain_name_buf_hdr[PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_NUM];
static ps_iface_sip_serv_domain_name_type  * ps_iface_sip_serv_domain_name_buf_ptr[PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_NUM];
#endif /* FEATURE_DATA_PS_MEM_DEBUG */

/*---------------------------------------------------------------------------
  Tuning the number of ps iface domain name search list node buffers needed
  by this module
---------------------------------------------------------------------------*/
#define PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_SIZE  ((sizeof(ps_iface_domain_name_search_list_node_type) + 3) & ~3)

#ifdef FEATURE_DATA_PS_LOW_MEM_CHIPSET
/*---------------------------------------------------------------------------
  # of domain names search list node buffers. 13 for each iface.
  1 1x/UMTS, 1 WLAN,
---------------------------------------------------------------------------*/
#define PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_NUM       (13 * 2)
#define PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_HIGH_WM   (13 * 2)
#define PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_LOW_WM     1

#else
/*---------------------------------------------------------------------------
  # of domain names search list node buffers. 13 for each iface. 1 1x, 1 WLAN,
  3 UMTS
---------------------------------------------------------------------------*/
#define PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_NUM       (13 * 5)
#define PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_HIGH_WM   (13 * 5)
#define PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_LOW_WM     1
#endif /* FEATURE_DATA_PS_LOW_MEM_CHIPSET */

/*----------------------------------------------------------------------------
  Allocate memory to hold ps_iface_domain_name_search_list_node along with
  ps_mem header
----------------------------------------------------------------------------*/
static int ps_iface_domain_name_search_list_node_buf_mem[PS_MEM_GET_TOT_SIZE_OPT
                                (
                                  PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_NUM,
                                  PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_SIZE
                                )];

#ifdef FEATURE_DATA_PS_MEM_DEBUG
/*----------------------------------------------------------------------------
  Array of pointers used to facilitate easy debugging.  The first one points
  to the ps_mem header and the latter ponts to
  ps_iface_domain_name_search_list_node_buf
----------------------------------------------------------------------------*/
static ps_mem_buf_hdr_type                         * ps_iface_domain_name_search_node_buf_hdr[PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_NUM];
static ps_iface_domain_name_search_list_node_type  * ps_iface_domain_name_search_node_buf_ptr[PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_NUM];
#endif /* FEATURE_DATA_PS_MEM_DEBUG */

#ifdef FEATURE_DATA_PS_MIPV6
/*---------------------------------------------------------------------------
  Tuning the number of ps iface mip6 bootstrap config info buffers needed by
  this module
---------------------------------------------------------------------------*/
#define PS_IFACE_MIP6_BOOTSTRAP_CFG_INFO_BUF_SIZE  ((sizeof(ps_iface_mip6_bootstrap_cfg_info_type) + 3) & ~3)

#define PS_IFACE_MIP6_BOOTSTRAP_CFG_INFO_BUF_NUM        5
#define PS_IFACE_MIP6_BOOTSTRAP_CFG_INFO_BUF_HIGH_WM    5
#define PS_IFACE_MIP6_BOOTSTRAP_CFG_INFO_BUF_LOW_WM     1

/*----------------------------------------------------------------------------
  Allocate memory to hold ps_iface_mip6_bootstrap_cfg along with ps_mem header
----------------------------------------------------------------------------*/
static int ps_iface_mip6_bootstrap_cfg_info_buf_mem[PS_MEM_GET_TOT_SIZE_OPT
                                      (
                                        PS_IFACE_MIP6_BOOTSTRAP_CFG_INFO_BUF_NUM,
                                        PS_IFACE_MIP6_BOOTSTRAP_CFG_INFO_BUF_SIZE
                                      )];

#ifdef FEATURE_DATA_PS_MEM_DEBUG
/*----------------------------------------------------------------------------
  Array of pointers used to facilitate easy debugging.  The first one points
  to the ps_mem header and the latter ponts to ps_iface_mip6_bootstrap_cfg_buf
----------------------------------------------------------------------------*/
static ps_mem_buf_hdr_type                   * ps_iface_mip6_bootstrap_cfg_info_buf_hdr[PS_IFACE_MIP6_BOOTSTRAP_CFG_INFO_BUF_NUM];
static ps_iface_mip6_bootstrap_cfg_info_type * ps_iface_mip6_bootstrap_cfg_info_buf_ptr[PS_IFACE_MIP6_BOOTSTRAP_CFG_INFO_BUF_NUM];
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
#endif /* FEATURE_DATA_PS_MIPV6 */

/*---------------------------------------------------------------------------
  Tuning the number of ps flow buffers needed by this module
---------------------------------------------------------------------------*/
#define PS_FLOW_BUF_SIZE  ((sizeof(ps_flow_type) + 3) & ~3)

#ifndef FEATURE_DATA_PS_LOW_MEM_CHIPSET

#define PS_FLOW_BUF_NUM       PS_FLOW_MAX_PS_FLOWS
#define PS_FLOW_BUF_HIGH_WM   (MAX_SYSTEM_IFACES + 35)
#define PS_FLOW_BUF_LOW_WM    (MAX_SYSTEM_IFACES + 5)

#else

#define PS_FLOW_BUF_NUM       MAX_SYSTEM_IFACES
#define PS_FLOW_BUF_HIGH_WM   MAX_SYSTEM_IFACES
#define PS_FLOW_BUF_LOW_WM    5
#endif /* FEATURE_DATA_PS_LOW_MEM_CHIPSET */

/*----------------------------------------------------------------------------
  Allocate memory to hold ps_flow along with ps_mem header
----------------------------------------------------------------------------*/
static int ps_flow_buf_mem[PS_MEM_GET_TOT_SIZE_OPT(PS_FLOW_BUF_NUM,PS_FLOW_BUF_SIZE)];

#ifdef FEATURE_DATA_PS_MEM_DEBUG
/*----------------------------------------------------------------------------
  Array of pointers used to facilitate easy debugging.  The first one points
  to the ps_mem header and the latter ponts to ps_flow_buf
----------------------------------------------------------------------------*/
static ps_mem_buf_hdr_type * ps_flow_buf_hdr[PS_FLOW_BUF_NUM];
static ps_flow_type        * ps_flow_buf_ptr[PS_FLOW_BUF_NUM];
#endif /* FEATURE_DATA_PS_MEM_DEBUG */

/*===========================================================================

                             INTERNAL MACROS

===========================================================================*/
/*---------------------------------------------------------------------------
  MACRO to check if an interface supports IPv4 address family.
  Note: iface_ptr must be Valid.
---------------------------------------------------------------------------*/
#define ADDR_FAMILY_IS_V4(iface_ptr)            \
 ((iface_ptr)->iface_private.addr_family == IFACE_IPV4_ADDR_FAMILY)

/*===========================================================================

                             INTERNAL FORWARD DECLARATIONS

===========================================================================*/

static void ps_iface_linger_timeout_cb
(
  void* this_iface_ptr
);

/*===========================================================================

                             INTERNAL FUNCTIONS

===========================================================================*/
/*===========================================================================
FUNCTION PS_IFACEI_COPY_ADDR_FAMILY

DESCRIPTION
  Copies the address family of the source to the destination.

PARAMETERS
  dest_iface_prtr: iface to copy addr family to
  src_iface_ptr:   iface to copy addr_family from

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
static void ps_ifacei_copy_addr_family
(
  ps_iface_type *dest_iface_ptr,
  ps_iface_type *src_iface_ptr
)
{
  ps_iface_addr_family_type       addr_family;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(dest_iface_ptr) || !PS_IFACE_IS_VALID(src_iface_ptr))
  {
    LOG_MSG_FATAL_ERROR("Invalid iface_ptr 0x%p or 0x%p",
        dest_iface_ptr, src_iface_ptr,0);
    ASSERT(0);
    return;
  }

  addr_family = ps_iface_get_addr_family(src_iface_ptr);
  if (addr_family == IFACE_INVALID_ADDR_FAMILY)
  {
    LOG_MSG_FATAL_ERROR("Bad addr family in src iface 0x%x:%d",
         src_iface_ptr->name,
         src_iface_ptr->instance,
         0);
    ASSERT(0);
  }
  else
  {
    (void)ps_iface_set_addr_family(dest_iface_ptr, addr_family);
  }

} /* ps_ifacei_copy_addr_family() */

/*===========================================================================
FUNCTION PS_IFACE_LINGER_TIMEOUT_CB()

DESCRIPTION
  The callback function associated with the pstimer for the linger timeout.
  This callback is called when the linger timeout on the interface expires.

DEPENDENCIES
  None.

RETURN VALUE
  None

SIDE EFFECTS
  None.
===========================================================================*/
void
ps_iface_linger_timeout_cb
(
  void* this_iface_ptr
)
{

  int16         ps_errno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID((ps_iface_type *)this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  LOG_MSG_INFO2("Linger timeout on iface 0x%x",this_iface_ptr,0,0);
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  (void) ps_iface_go_null_cmd ((ps_iface_type *)this_iface_ptr,
                               &ps_errno,
                               NULL);
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
}



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                             EXTERNAL FUNCTIONS
             ---------------------------------------------------
                               CONFIGURATION

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
/*===========================================================================
FUNCTION PS_IFACE_INIT()

DESCRIPTION
  Initializes ps_iface module

DEPENDENCIES
  None

PARAMETERS
  None

RETURN VALUE
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_init
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Initialize Pools
  -------------------------------------------------------------------------*/
  if (PS_MEM_POOL_INIT_OPT(PS_MEM_PS_SIP_SERV_ADDR_TYPE,
                           ps_iface_sip_serv_addr_buf_mem,
                           PS_IFACE_SIP_SERV_ADDR_BUF_SIZE,
                           PS_IFACE_SIP_SERV_ADDR_BUF_NUM,
                           PS_IFACE_SIP_SERV_ADDR_BUF_HIGH_WM,
                           PS_IFACE_SIP_SERV_ADDR_BUF_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) ps_iface_sip_serv_addr_buf_hdr,
                           (int *) ps_iface_sip_serv_addr_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    LOG_MSG_FATAL_ERROR("Can't init the module", 0, 0, 0);
  }

  if (PS_MEM_POOL_INIT_OPT(PS_MEM_PS_SIP_SERV_DOMAIN_NAME_TYPE,
                           ps_iface_sip_serv_domain_buf_mem,
                           PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_SIZE,
                           PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_NUM,
                           PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_HIGH_WM,
                           PS_IFACE_SIP_SERV_DOMAIN_NAME_BUF_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) ps_iface_sip_serv_domain_name_buf_hdr,
                           (int *) ps_iface_sip_serv_domain_name_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    LOG_MSG_FATAL_ERROR("Can't init the module", 0, 0, 0);
  }

  if (PS_MEM_POOL_INIT_OPT(PS_MEM_PS_DOMAIN_NAME_SEARCH_LIST_NODE_TYPE,
                           ps_iface_domain_name_search_list_node_buf_mem,
                           PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_SIZE,
                           PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_NUM,
                           PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_HIGH_WM,
                           PS_IFACE_DOMAIN_NAME_SEARCH_LIST_NODE_BUF_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) ps_iface_domain_name_search_node_buf_hdr,
                           (int *) ps_iface_domain_name_search_node_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    LOG_MSG_FATAL_ERROR("Can't init the module", 0, 0, 0);
  }

#ifdef FEATURE_DATA_PS_MIPV6
  if (PS_MEM_POOL_INIT_OPT(PS_MEM_PS_MIP6_BOOTSTRAP_CFG_INFO_TYPE,
                           ps_iface_mip6_bootstrap_cfg_info_buf_mem,
                           PS_IFACE_MIP6_BOOTSTRAP_CFG_INFO_BUF_SIZE,
                           PS_IFACE_MIP6_BOOTSTRAP_CFG_INFO_BUF_NUM,
                           PS_IFACE_MIP6_BOOTSTRAP_CFG_INFO_BUF_HIGH_WM,
                           PS_IFACE_MIP6_BOOTSTRAP_CFG_INFO_BUF_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) ps_iface_mip6_bootstrap_cfg_info_buf_hdr,
                          (int *) ps_iface_mip6_bootstrap_cfg_info_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    LOG_MSG_FATAL_ERROR("Can't init the module", 0, 0, 0);
  }
#endif /* FEATURE_DATA_PS_MIPV6 */

  if (PS_MEM_POOL_INIT_OPT(PS_MEM_PS_FLOW_TYPE,
                           ps_flow_buf_mem,
                           PS_FLOW_BUF_SIZE,
                           PS_FLOW_BUF_NUM,
                           PS_FLOW_BUF_HIGH_WM,
                           PS_FLOW_BUF_LOW_WM,
                           NULL,
#ifdef FEATURE_DATA_PS_MEM_DEBUG
                           (int *) ps_flow_buf_hdr,
                           (int *) ps_flow_buf_ptr
#else
                           NULL,
                           NULL
#endif /* FEATURE_DATA_PS_MEM_DEBUG */
                          ) == -1)
  {
    LOG_MSG_FATAL_ERROR("Can't init the module", 0, 0, 0);
  }
} /* ps_iface_init() */


int32 ps_iface_create_internal
(
  ps_iface_type            * this_iface_ptr,
  ps_iface_name_enum_type    name,
  ps_phys_link_type        * phys_link_array,
  uint8                      num_phys_links
)
{
  static  ps_iface_cookie_type  global_cookie = 1;
  int32                         loop;
  uint8                         instance = 0;
  int16                         ps_errno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  ASSERT(global_iface_ptr_array_index != MAX_SYSTEM_IFACES);

  /*-------------------------------------------------------------------------
    No need to enter critical section as ifaces are created at power up
    and PS knows about them only after they are created and until then
    only DS context accesses them
  -------------------------------------------------------------------------*/
  /*-------------------------------------------------------------------------
    Validate the iface, and make sure that the interface has not already been
    created.
  -------------------------------------------------------------------------*/
  if(this_iface_ptr == NULL ||
     this_iface_ptr->iface_private.this_iface == this_iface_ptr)
  {
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Find the highest instance number used so far for the iface of this type.
    The instance of the newly created iface should be one higher than that.

    Fetching global_iface_ptr_array_index inside a critical section so that
    race conditions do not happen. Otherwise while one thread is reading
    this value, another thread could be updating it. See CR 266766.
  -------------------------------------------------------------------------*/
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  loop = global_iface_ptr_array_index - 1;
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  for(; loop >= 0; loop--)
  {
    /*-----------------------------------------------------------------------
      Since ifaces are added sequentially and they are never deleted, we
      traverse the array of iface bakwards and as soon as we get an iface
      matching "name", its instance number is the highest among all ifaces
      of that type.
    -----------------------------------------------------------------------*/
    if(global_iface_ptr_array[loop]->name == name)
    {
      instance = global_iface_ptr_array[loop]->instance + 1;
      break;
    }
  }

  LOG_MSG_INFO1("Creating iface(0x%p) 0x%x:%d", this_iface_ptr, name, instance);

  /*-------------------------------------------------------------------------
    reset the private info and initialize all of the queue strucutures
  -------------------------------------------------------------------------*/
  memset(&(this_iface_ptr->iface_private),
         0,
         sizeof(this_iface_ptr->iface_private));
  for(loop = 0; loop < IFACE_EVENT_MAX; loop++)
  {
    (void)q_init((this_iface_ptr->iface_private.event_q_array) + loop);
  }

#ifdef FEATURE_DATA_PS_DATA_LOGGING
  /*-------------------------------------------------------------------------
    Reset all DPL specific storage
  -------------------------------------------------------------------------*/
  memset(&(this_iface_ptr->dpl_net_cb),
         0,
         sizeof(this_iface_ptr->dpl_net_cb));

  memset(&(this_iface_ptr->dpl_sec_flow_copy_cb),
         0,
         sizeof(this_iface_ptr->dpl_sec_flow_copy_cb));
#endif /* FEATURE_DATA_PS_DATA_LOGGING */

  /*-------------------------------------------------------------------------
    Initialize the Tx and Rx IP filter queues.
  -------------------------------------------------------------------------*/
  for(loop = 0; loop < IP_FLTR_CLIENT_MAX; loop++)
  {
    (void) q_init(&(this_iface_ptr->iface_private.ipfltr_info[loop]));
  }

  list_init(&this_iface_ptr->iface_private.rx_qos_fltr_node_list);

  /*-------------------------------------------------------------------------
    Insert the iface into the global iface_ptr array.

    Fetching global_iface_ptr_array_index inside a critical section so that
    race conditions do not happen. Otherwise while one thread is reading
    this value, another thread could be updating it. See CR 266766.
  -------------------------------------------------------------------------*/
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  global_iface_ptr_array[global_iface_ptr_array_index] = this_iface_ptr;
  this_iface_ptr->iface_private.iface_index = global_iface_ptr_array_index;
  global_iface_ptr_array_index += 1;
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    Set the this ptr, assign the name and instance, set the tx function to
    the default value, disallow the dormant cleanup and set the peer
    address function to NULL.
  -------------------------------------------------------------------------*/
  this_iface_ptr->iface_private.state      = IFACE_DISABLED;
  this_iface_ptr->name                     = name;
  this_iface_ptr->instance                 = instance;

  /*-------------------------------------------------------------------------
    Set the address family to V4 by default. Do not use the
    set_addr_family() function here, since we do not want to invoke cbacks
    on initial iface creation.
  -------------------------------------------------------------------------*/
  this_iface_ptr->iface_private.addr_family = IPV4_ADDR;

  /*-------------------------------------------------------------------------
    Assign the cookie, and increment the global counter.
  -------------------------------------------------------------------------*/
  this_iface_ptr->iface_private.cookie = global_cookie;
  global_cookie += 1;

#ifdef FEATURE_DATACOMMON_PS_IFACE_IO
  this_iface_ptr->iface_private.ip_input   = ip4_input;
#else
  this_iface_ptr->iface_private.ip_input   = NULL;
#endif /* FEATURE_DATACOMMON_PS_IFACE_IO */

  /*-------------------------------------------------------------------------
    Initialize the net info structure.
    Set the MTU in the net info to the default values.  Set the v6 hop limit
    to default value.
  -------------------------------------------------------------------------*/
  memset( &this_iface_ptr->net_info, 0, sizeof(network_info_type) );
  this_iface_ptr->net_info.mtu = DEFAULT_DATA_UNIT;

#ifdef FEATURE_DATA_PS_IPV6
  this_iface_ptr->net_info.net_ip.v6.curr_hop_limit = IP_DEF_TTL;
#ifdef FEATURE_DATA_PS_ADDR_MGMT
  this_iface_ptr->dad_f_ptr      = ps_iface_addr_mgmt_default_dad_func;
  this_iface_ptr->addr_cb_f_ptr  = ps_iface_addr_mgmt_default_addr_cb_func;
#endif /* FEATURE_DATA_PS_ADDR_MGMT */
#endif /* FEATURE_DATA_PS_IPV6 */

  /*-------------------------------------------------------------------------
    Initialize the link list of SIP server IP addresses and domain names.
  -------------------------------------------------------------------------*/
  memset (&this_iface_ptr->sip_serv_addr_list, 0, sizeof (list_type));
  list_init(&this_iface_ptr->sip_serv_addr_list);

  memset (&this_iface_ptr->sip_serv_domain_name_list, 0, sizeof (list_type));
  list_init(&this_iface_ptr->sip_serv_domain_name_list);

#ifdef FEATURE_DATA_PS_MIPV6
  this_iface_ptr->mip6_bootstrap_info = NULL;
#endif /* FEATURE_DATA_PS_MIPV6 */

#ifdef FEATURE_DATA_PS_MCAST
   this_iface_ptr->iface_private.if_mcast_grps = NULL;
   this_iface_ptr->iface_private.querier_v1_timer = 0;
#endif /* FEATURE_DATA_PS_MCAST */

  this_iface_ptr->iface_private.dhcp_client_handle = NULL;

  /*-------------------------------------------------------------------------
    Initiailze the PROC_ID
  -------------------------------------------------------------------------*/
  PS_IFACEI_RESET_PROC_ID(this_iface_ptr);

  /*-------------------------------------------------------------------------
    Initialize the link list of domain names.
  -------------------------------------------------------------------------*/
  memset (&this_iface_ptr->domain_name_search_list, 0, sizeof(list_type));
  list_init(&this_iface_ptr->domain_name_search_list);

  /*-------------------------------------------------------------------------
    intialize statistics
  -------------------------------------------------------------------------*/
  memset(&this_iface_ptr->stats, 0, sizeof(ps_iface_stats_type));
  memset(&this_iface_ptr->iface_i_stats,
         0,
         sizeof(this_iface_ptr->iface_i_stats));

  /*-------------------------------------------------------------------------
    initialize Last IO timestamp
  -------------------------------------------------------------------------*/
  this_iface_ptr->last_io_time = 0;

  /*-----------------------------------------------------------------------
    Create default flow and initialize its fields. QOS is not supported on
    default flow
  -----------------------------------------------------------------------*/
  (void) ps_flowi_create(FALSE,
                         &this_iface_ptr->iface_private.flow.default_flow_ptr,
                         &ps_errno);
  if (this_iface_ptr->iface_private.flow.default_flow_ptr == NULL)
  {
    LOG_MSG_ERROR("No mem for default flow", 0, 0, 0);
    return -1;
  }

  this_iface_ptr->iface_private.flow.default_flow_ptr->flow_private.iface_ptr = this_iface_ptr;

  PS_FLOWI_SET_CAPABILITY(this_iface_ptr->iface_private.flow.default_flow_ptr,
                          PS_FLOW_CAPABILITY_DEFAULT);
  PS_FLOWI_SET_COOKIE(this_iface_ptr->iface_private.flow.default_flow_ptr,
                      PS_IFACE_DEFAULT_FLOW_INST);

  this_iface_ptr->iface_private.flow.default_flow_ptr->flow_private.this_flow =
    this_iface_ptr->iface_private.flow.default_flow_ptr;
  this_iface_ptr->iface_private.flow.next_cookie = PS_IFACE_MIN_SEC_FLOW_INST;
  list_init(&this_iface_ptr->iface_private.flow.sec_flow_list);

  /*-----------------------------------------------------------------------
    Initialize the DNS cache
  -----------------------------------------------------------------------*/
  memset(&(this_iface_ptr->iface_private.dns_cache_q), 0,
         sizeof(this_iface_ptr->iface_private.dns_cache_q));
  (void) q_init(&(this_iface_ptr->iface_private.dns_cache_q));

  /*-------------------------------------------------------------------------
    Create the phys link(s)
  -------------------------------------------------------------------------*/
  if(phys_link_array != NULL)
  {
    (void)ps_phys_link_create(phys_link_array, num_phys_links);

    /*-----------------------------------------------------------------------
      Setup the phys link information - the primary is always defaulted to
      the first instance (i.e. 0).
    -----------------------------------------------------------------------*/
    this_iface_ptr->iface_private.phys_link.array     = phys_link_array;
    this_iface_ptr->iface_private.phys_link.instances = num_phys_links;
    this_iface_ptr->iface_private.phys_link.primary   = 0;
  }

  /*-------------------------------------------------------------------------
    Set self-referential ptr here as iface is initialized completely. This
    way critical section is not needed during initialization, as any usage of
    iface will fail while it is being initialized if self referential ptr is
    not set
  -------------------------------------------------------------------------*/
  this_iface_ptr->iface_private.this_iface = this_iface_ptr;

  memset (&(this_iface_ptr->event_info_cache),
          0,
          sizeof(iface_event_info_cache_type));
  this_iface_ptr->event_info_cache.ip_addr.type = IPV4_ADDR;
  this_iface_ptr->event_info_cache.state = IFACE_DOWN;

  return instance;
} /* ps_iface_create_internal() */


/*===========================================================================
FUNCTION PS_IFACE_CREATE()

DESCRIPTION
    This will memset()s the private data struct to 0, initializes the
    "this_ptr" to point to the interface control block itself, initializes
    the queues and the TX ptr to the default value.  It will also add the
    interface to the global interface pointer array - this will ASSERT() if
    there are not enough interface slots available.

    NOTE: the memset() sets the iface_state to DISABLED.

PARAMETERS
    this_iface_ptr: ptr to interface control block on which to operate on.
    name:     the name of the interface (like CDMA_SN_IFACE)
    this_iface_outgoing_acl_ptr: pointer to the outgoing acl to include the
                                 default ACL in the creation of the IFACE.
    this_iface_incoming_acl_ptr: pointer to the incoming acl to include the
                                 default ACL in the creation of the IFACE.
    phys_link_array: array of physlinks associated with this interface.
                     - valid for static-binding-physical iface
                     - NULL for logical or dynamic-binding-physical iface
    num_phys_links: the number of physlinks in the array

RETURN VALUE
  Instance number of the newly created iface
 -1 - on error

DEPENDENCIES
  The interface being created MUST NOT be freed before this function returns

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_create
(
  ps_iface_type            * this_iface_ptr,
  ps_iface_name_enum_type    name,
  acl_type                 * this_iface_outgoing_acl_ptr,
  acl_type                 * this_iface_incoming_acl_ptr,
  ps_phys_link_type        * phys_link_array,
  uint8                      num_phys_links
)
{
  int32  instance;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  instance = ps_iface_create_internal( this_iface_ptr,
                                       name,
                                       phys_link_array,
                                       num_phys_links);
  if (-1 == instance)
  {
    return -1;
  }

  /*-------------------------------------------------------------------------
    Set up PS Iface's default input/output functions.
    If the iface being created is a logical iface set default tx cmd to the
    default logical iface tx cmd.
  -------------------------------------------------------------------------*/
  if (phys_link_array == NULL)
  {
#ifdef FEATURE_DATACOMMON_PS_IFACE_IO
    this_iface_ptr->iface_private.tx_cmd = ps_ifacei_logical_default_tx_cmd;
#else
    this_iface_ptr->iface_private.tx_cmd = NULL;
#endif /* FEATURE_DATACOMMON_PS_IFACE_IO */

    this_iface_ptr->iface_private.is_logical = TRUE;
  }
  else
  {
#ifdef FEATURE_DATACOMMON_PS_IFACE_IO
    this_iface_ptr->iface_private.tx_cmd = ps_ifacei_default_tx_cmd;
#else
    this_iface_ptr->iface_private.tx_cmd = NULL;
#endif /* FEATURE_DATACOMMON_PS_IFACE_IO */

    this_iface_ptr->iface_private.is_logical = FALSE;
  }

  /*-------------------------------------------------------------------------
    Add the mode handler's outgoing ACL for this interface and also store it
    in this IFACE.
  -------------------------------------------------------------------------*/
  if(this_iface_outgoing_acl_ptr)
  {
    this_iface_outgoing_acl_ptr->if_ptr = (void*) this_iface_ptr;
    (void)route_acl_add(ROUTE_ACL_DEFAULT_OUTBOUND,
                  this_iface_outgoing_acl_ptr,
                  ROUTE_ACL_ANY_PRIORITY);
    this_iface_ptr->outgoing_acl_ptr = this_iface_outgoing_acl_ptr;
  }

  /*-------------------------------------------------------------------------
    Add the mode handler's incoming ACL for this interface
  -------------------------------------------------------------------------*/
  if(this_iface_incoming_acl_ptr)
  {
    this_iface_incoming_acl_ptr->if_ptr = (void*) this_iface_ptr;
    (void)route_acl_add(ROUTE_ACL_DEFAULT_INBOUND,
                  this_iface_incoming_acl_ptr,
                  ROUTE_ACL_ANY_PRIORITY);
  }

  return instance;
} /* ps_iface_create() */



/*===========================================================================
FUNCTION PS_IFACE_ACTIVE_ASSOCIATE()

DESCRIPTION
  This function is used to (re)associate a logical iface with another iface
  Currently can only associate to a physical iface.

  This will register interest in the underlying phys link (i.e, inc() its
  ref cnt), as well as bring_up() the target physical ps_iface.  If the
  logical_iface was pointing at another ps_iface previously, then that
  ps_iface is torn down.  If that also implies that the logical_iface was
  pointing at a different phys_link, then the old phys_link's ref_Cnt is
  also decremented.

  If the inherit_ip_info boolean is TRUE, then also drills down
  via new assoc_iface to get new addr_family.

  NOTE: This should be called after the logical iface has been created i.e.
  after the call to ps_iface_create()

PARAMETERS
  logical_iface_ptr: Ptr to the logical interface
  ps_iface_data_proc_f_ptr_type: Rx processing function
  this_iface_ptr: Ptr to interface control block to associate with.
  inherit_ip_info: TRUE = IP info inherited from base iface.
  ps_errno: Returns an error code.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  This function MUST be TASKLOCK()ed

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_active_associate
(
  ps_iface_type                  *logical_iface_ptr,
  ps_iface_type                  *new_assoc_iface_ptr,
  boolean                         inherit_ip_info,
  int16                          *ps_errno
)
{
  ps_phys_link_type              *old_phys_link_ptr;
  ps_iface_type                  *old_assoc_iface_ptr;
  int                             ret_val = 0;
  int16                           dummy_errno;
  ps_flow_type                   *old_ps_flow_ptr =  NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if(!PS_IFACE_IS_VALID(logical_iface_ptr))
  {
    LOG_MSG_FATAL_ERROR("Invalid logical_iface_ptr 0x%p", logical_iface_ptr,0,0);
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return(-1);
  }

  LOG_MSG_INFO1("Actively (re)associating with iface", 0, 0, 0);

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  old_assoc_iface_ptr = PS_IFACEI_GET_ASSOC_IFACE(logical_iface_ptr);
  old_ps_flow_ptr     = PS_IFACEI_GET_DEFAULT_FLOW(logical_iface_ptr);
  old_phys_link_ptr   = PS_FLOW_GET_PHYS_LINK(old_ps_flow_ptr);

  if( (ret_val = ps_iface_passive_associate(logical_iface_ptr,
                                            new_assoc_iface_ptr,
                                            inherit_ip_info,
                                            ps_errno)) < 0)
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return(ret_val);
  }

  /*-------------------------------------------------------------------------
    Bring up the new iface.  Save the return value.  Then tear down the
    old iface (if there was an old iface). If old_phys_link_ptr is non-NULL
    then we were once actively associated with the iface. Otherwise it was
    just a passive association.
  -------------------------------------------------------------------------*/
  /* might need to check for if = to current phys link */
  if ( (old_assoc_iface_ptr == new_assoc_iface_ptr) &&
       (old_phys_link_ptr != NULL) )
  {
    LOG_MSG_INFO1("Iface re-associating to same iface",0,0,0);
  }
  else
  {
    ret_val = ps_iface_bring_up_cmd(new_assoc_iface_ptr, ps_errno, NULL);

    if((old_assoc_iface_ptr != NULL) &&
       (old_assoc_iface_ptr != PS_IFACEI_GET_ASSOC_IFACE(logical_iface_ptr)))
    {
      (void)ps_iface_tear_down_cmd(old_assoc_iface_ptr, &dummy_errno, NULL);
    }
  }

  /*-------------------------------------------------------------------------
    If associated physical/logical iface is flow controlled, set the flow
    control mask in the logical iface to indicate that the assocaited iface
    iface is flow controlled.
  -------------------------------------------------------------------------*/
  if(!PS_IFACEI_FLOW_ENABLED(new_assoc_iface_ptr))
  {
    ps_iface_enable_flow(logical_iface_ptr, DS_FLOW_ASSOC_IFACE_MASK);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return(ret_val);

} /* ps_iface_active_associate() */


/*===========================================================================
FUNCTION PS_IFACE_PASSIVE_ASSOCIATE()

DESCRIPTION
  This function is used to (re)associate a logical iface with another iface
  Currently can only associate to a physical iface.

  This function does not explicity express interest in an iface. In other
  words, no ref counts are changed, and no interfaces are brought up as
  a result of this call.

  If the inherit_ip_info boolean is TRUE, then also drills down
  via new assoc_iface to get new addr_family.

  NOTE: This should be called after the logical iface has been created i.e.
  after the call to ps_iface_create()

PARAMETERS
  logical_iface_ptr: Ptr to the logical interface
  ps_iface_data_proc_f_ptr_type: Rx processing function
  this_iface_ptr: Ptr to interface control block to associate with.
  inherit_ip_info: TRUE = IP info inherited from base iface.
  ps_errno: Returns an error code.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  This function MUST be TASKLOCK()ed

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_passive_associate
(
  ps_iface_type                  *logical_iface_ptr,
  ps_iface_type                  *new_assoc_iface_ptr,
  boolean                         inherit_ip_info,
  int16                          *ps_errno
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Make sure input IFACE's are valid.
  -------------------------------------------------------------------------*/
  if (!PS_IFACE_IS_VALID(new_assoc_iface_ptr) ||
      !PS_IFACE_IS_VALID(logical_iface_ptr))
  {
    LOG_MSG_FATAL_ERROR("Invalid iface is passed, new_assoc_iface_ptr 0x%p, "
        "logical_iface_ptr 0x%p",
        new_assoc_iface_ptr, logical_iface_ptr, 0);
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return(-1);
  }


#ifdef MSG_4
  MSG_4(MSG_SSID_DS,
        MSG_LEGACY_MED,
        "(Re)Associate iface 0x%x:%d to iface 0x%x:%d",
        logical_iface_ptr->name,
        logical_iface_ptr->instance,
        new_assoc_iface_ptr->name,
        new_assoc_iface_ptr->instance);
#endif  /* MSG_4 */

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    Assocaite the logical iface to the physical/logical iface.
  -------------------------------------------------------------------------*/
  logical_iface_ptr->iface_private.assoc_iface_ptr = new_assoc_iface_ptr;

  /*-------------------------------------------------------------------------
    If the inherit_ip_info boolean is TRUE, then drill down to get the
    addr family.  First set the private flag to FALSE, so that we can
    copy the addr family.
  -------------------------------------------------------------------------*/
  logical_iface_ptr->iface_private.inherit_ip_info = FALSE;

  if (inherit_ip_info == TRUE)
  {
    ps_ifacei_copy_addr_family(logical_iface_ptr, new_assoc_iface_ptr);
    logical_iface_ptr->iface_private.inherit_ip_info = TRUE;
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_passive_associate() */

/*===========================================================================
FUNCTION PS_IFACE_ACTIVE_DISASSOCIATE()

DESCRIPTION
  This function is used to disassociate a logical iface from the underlying
  ps_iface.

  This will de_register interest in the underlying phys link (i.e, dec() its
  ref cnt), as well as tear_down() the target physical ps_iface.

  NOTE: This should be called after the logical iface has been created i.e.
  after the call to ps_iface_create()

PARAMETERS
  logical_iface_ptr: Ptr to the logical interface
  ps_errno: error code to return to caller.
  client_data_ptr: data, that is passed to the client handler function for
                   underlying tear_down.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  This function MUST be TASKLOCK()ed

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_active_disassociate
(
  ps_iface_type                  *logical_iface_ptr,
  int16                          *ps_errno,
  void                           *client_data_ptr
)
{
  ps_iface_type                  *old_assoc_iface_ptr;
  int                             ret_val;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if(!PS_IFACE_IS_VALID(logical_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid logical iface_ptr 0x%x", logical_iface_ptr, 0, 0);
    *ps_errno = DS_EINVAL;
    ASSERT(0);
    return(-1);
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if (PS_IFACEI_GET_ASSOC_IFACE(logical_iface_ptr) == NULL)
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_INFO1("No associated PS_IFACE to deassociate",0,0,0);
    return(0);
  }

  /*-------------------------------------------------------------------------
    Save the old ps_iface ptr.
  -------------------------------------------------------------------------*/
  old_assoc_iface_ptr = PS_IFACEI_GET_ASSOC_IFACE(logical_iface_ptr);

  if (!PS_IFACE_IS_VALID(old_assoc_iface_ptr))
  {
    LOG_MSG_FATAL_ERROR("Logical iface 0x%x:%d assoc'ed to invalid ps_iface 0x%p",
         logical_iface_ptr->name,
         logical_iface_ptr->instance,
         old_assoc_iface_ptr
       );

    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    ASSERT(0);

    *ps_errno = DS_EINVAL;
    return(-1);
  }

#ifdef MSG_4
  MSG_4( MSG_SSID_DS,
         MSG_LEGACY_MED,
         "De-associating log iface 0x%x:%d from iface 0x%x:%d",
         logical_iface_ptr->name,
         logical_iface_ptr->instance,
         old_assoc_iface_ptr->name,
         old_assoc_iface_ptr->instance
       );
#endif  /* MSG_4 */

  /*-------------------------------------------------------------------------
    Set assoc iface and phys link ptrs to NULL.
  -------------------------------------------------------------------------*/
  logical_iface_ptr->iface_private.assoc_iface_ptr     = NULL;
  logical_iface_ptr->iface_private.inherit_ip_info     = FALSE;

  /*-------------------------------------------------------------------------
    Decrement count on old phys link. Old_phys_link_ptr should not be null,
    since error checks at top of function indicate this logical iface was
    associated with a good physical ps_iface.

    Then tear down ps_iface we were associated with (must be non-null due
    to error checks at top of function).
    ps_flow_go_null_ind will unbind phys_link as well.
  -------------------------------------------------------------------------*/
  ps_flow_go_null_ind( PS_IFACEI_GET_DEFAULT_FLOW(logical_iface_ptr),
                       PS_EIC_NOT_SPECIFIED);

  ret_val = ps_iface_tear_down_cmd(old_assoc_iface_ptr,
                                   ps_errno,
                                   client_data_ptr
                                  );

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return(ret_val);

} /* ps_iface_active_disassociate() */


/*===========================================================================
FUNCTION PS_IFACE_PASSIVE_DISASSOCIATE()

DESCRIPTION
  This function is used to passively disassociate a logical iface from the
  underlying ps_iface.

  NOTE: This should be called after the logical iface has been created i.e.
  after the call to ps_iface_create()

PARAMETERS
  logical_iface_ptr: Ptr to the logical interface
  ps_errno: Returns an error code.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  This function MUST be TASKLOCK()ed

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_passive_disassociate
(
  ps_iface_type                  *logical_iface_ptr,
  int16                          *ps_errno
)
{
  ps_iface_type                  *old_assoc_iface_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed", 0, 0, 0);
    ASSERT(0);
    return(-1);
  }

  /*-------------------------------------------------------------------------
    Make sure input IFACE is valid and is a logical iface.
  -------------------------------------------------------------------------*/
  if (!PS_IFACE_IS_VALID(logical_iface_ptr) ||
      !PS_IFACE_IS_LOGICAL(logical_iface_ptr))
  {
    LOG_MSG_FATAL_ERROR("Invalid iface is passed, logical_iface_ptr 0x%p",
        logical_iface_ptr, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return(-1);
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if (PS_IFACEI_GET_ASSOC_IFACE(logical_iface_ptr) == NULL)
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_INFO1("No associated PS_IFACE to deassociate",0,0,0);
    return(0);
  }

  old_assoc_iface_ptr = PS_IFACEI_GET_ASSOC_IFACE(logical_iface_ptr);

  if (!PS_IFACE_IS_VALID(old_assoc_iface_ptr))
  {
    LOG_MSG_FATAL_ERROR("Logical iface 0x%x:%d assoc'ed to invalid ps_iface 0x%p",
         logical_iface_ptr->name,
         logical_iface_ptr->instance,
         old_assoc_iface_ptr
       );

    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return(-1);
  }

  LOG_MSG_INFO2("Passively disassociating iface 0x%x:%d",
          logical_iface_ptr->name, logical_iface_ptr->instance, 0);

  logical_iface_ptr->iface_private.assoc_iface_ptr = NULL;
  logical_iface_ptr->iface_private.inherit_ip_info = FALSE;

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return(0);

} /* ps_iface_passive_disassociate() */


/*===========================================================================
FUNCTION PS_IFACE_GET_HANDLE()

DESCRIPTION
  This function returns a ps_iface handle based on the interface name
  and instance passed in.  If no such interfac exists, a value of NULL is
  returned.

PARAMETERS
  Pointer to struct containing name and instance information.

RETURN VALUE
  NULL for failure
  ps_iface handle on success

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
ps_iface_type *ps_iface_get_handle
(
  ps_iface_id_type iface_id
)
{
  uint8 iface_index;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Now we dont need to traverse through the global iface array to find the
  iface pointer. The iface_id actually represents an index into the
  global_iface_ptr_array. Only condition we need is to check that iface_id
  is less than the MAX_SYSTEM_IFACES. (or if we can declare the
  global_iface_ptr_array_index as extern, then we can compare with that).
  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -   */
  if( PS_IFACE_INVALID_ID == iface_id  ||
      0x00FFFF00UL != (0x00FFFF00UL & iface_id))
  {
    LOG_MSG_ERROR("Invalid Iface Id %x", iface_id, 0, 0);
    return NULL;
  }

  iface_index = (uint8)((iface_id & 0xFF000000)>>24) ;

  if( iface_index  >= MAX_SYSTEM_IFACES)
  {
    return NULL;
  }

  return global_iface_ptr_array[iface_index];

} /* ps_iface_get_handle() */

/*===========================================================================
FUNCTION PS_IFACE_SET_BRIDGE()

DESCRIPTION
  This function will set the bridge pointer and inform the address server
  the bridging mode.  If the bridged_iface_ptr is NULL this will "un-bridge"
  the interface.

PARAMETERS
  this_iface_ptr:    ptr to interface control block on which to operate on.
  bridged_iface_ptr: ptr to interface to bridge to.

RETURN VALUE
  None

DEPENDENCIES
  This should only ever be called by a client from a SINGLE task context.

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_set_bridge
(
  ps_iface_type  * this_iface_ptr,
  ps_iface_type  * target_iface_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if (PS_IFACE_IS_VALID(target_iface_ptr))
  {
    MSG_5(MSG_SSID_DS,
          MSG_LEGACY_MED,
          "bridging iface 0x%x:%d in state %d to iface 0x%x:%d",
          this_iface_ptr->name,
          this_iface_ptr->instance,
          PS_IFACEI_GET_STATE(this_iface_ptr),
          target_iface_ptr->name,
          target_iface_ptr->instance);
  }
  else if (target_iface_ptr == NULL)
  {
    LOG_MSG_INFO2("bridging 0x%p(%d) to 0x%p",
            this_iface_ptr,
            PS_IFACEI_GET_STATE(this_iface_ptr),
            target_iface_ptr);
  }
  else
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    ASSERT(0);
    LOG_MSG_INFO2("Invalid target_iface_ptr 0x%p is passed", target_iface_ptr, 0, 0);
    return;
  }

  /*-------------------------------------------------------------------------
    Set the bridge pointer to the value passed in, and call the bridge
    processing function if there is one.
  -------------------------------------------------------------------------*/
  this_iface_ptr->iface_private.tx_bridge_ptr = target_iface_ptr;

  if (this_iface_ptr->bridge_proc_f_ptr != NULL)
  {
    (void) this_iface_ptr->bridge_proc_f_ptr(this_iface_ptr,
                                             target_iface_ptr,
                                             this_iface_ptr->client_data_ptr);
  }

  if(target_iface_ptr != NULL &&
     target_iface_ptr->bridge_target_proc_f_ptr != NULL)
  {
    (void) target_iface_ptr->bridge_target_proc_f_ptr
                                          (target_iface_ptr,
                                           this_iface_ptr,
                                           target_iface_ptr->client_data_ptr);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_set_bridge() */


/*===========================================================================
FUNCTION PS_IFACE_SET_TX_FUNCTION()

DESCRIPTION
  This function is used to set the TX function.  Unsetting the function is
  done by passing in NULL for the tx_cmd parameter.

PARAMETERS
  this_iface_ptr:  interface on which to operate
  tx_cmd:          Tx command being set
  tx_cmd_info:     Info to pass to registered cmd handler

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_set_tx_function
(
  ps_iface_type             *this_iface_ptr,
  ps_iface_tx_cmd_f_ptr_type tx_cmd,
  void                      *tx_cmd_info
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  this_iface_ptr->iface_private.tx_cmd = tx_cmd;

#ifdef FEATURE_DATACOMMON_PS_IFACE_IO
  /*-------------------------------------------------------------------------
    if the tx_cmd parameter is NULL then register the default command
  -------------------------------------------------------------------------*/
  if(tx_cmd == NULL)
  {
    this_iface_ptr->iface_private.tx_cmd = ps_ifacei_default_tx_cmd;
  }
#endif

  /*-------------------------------------------------------------------------
    Set the command info to whatever was passed in.
  -------------------------------------------------------------------------*/
  this_iface_ptr->iface_private.tx_cmd_info = tx_cmd_info;
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_set_tx_function() */



/*===========================================================================
FUNCTION PS_IFACE_SET_RX_FUNCTION()

DESCRIPTION
  This function is used to set the RX function.

PARAMETERS
  this_iface_ptr:  interface on which to operate
  rx_cmd:          Rx command being set

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_set_rx_function
(
  ps_iface_type             *this_iface_ptr,
  ps_iface_data_proc_f_ptr_type rx_cmd
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  this_iface_ptr->iface_private.rx_proc_f_ptr = rx_cmd;
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_set_rx_function() */



/*===========================================================================
FUNCTION PS_IFACE_ENABLE_FLOW()

DESCRIPTION
  Interface user enables flow on the interface.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  flow_mask: bit mask that identifies the caller.

  THIS FUNCTION SHOULD NOT BE CALLED IN AN ISR.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_enable_flow
(
  ps_iface_type *this_iface_ptr,
  uint32         flow_mask
)
{
  ps_iface_event_info_u_type event_info;
  ds3g_flow_e_type              flow_type = DS_FLOW_ENABLE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  memset(&event_info, 0, sizeof(event_info));

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    store the previous flow mask in the event info variable, and remove the
    client mask from the tx_flow_mask.
  -------------------------------------------------------------------------*/
  event_info.flow_mask = ps_iface_get_flow_mask(this_iface_ptr);

  DS_FLOW_CTRL_SET_MASK(flow_type,
                        this_iface_ptr->iface_private.tx_flow_mask,
                        flow_mask);

  /*-------------------------------------------------------------------------
    only call the callback on the transition to the flow being enabled
  -------------------------------------------------------------------------*/
  if(PS_IFACEI_FLOW_ENABLED(this_iface_ptr) &&
     event_info.flow_mask != ALL_FLOWS_ENABLED)
  {
    LOG_MSG_INFO2_6("client 0x%x enabling flow on iface 0x%x:%d -> mask 0x%x",
                    flow_mask,
                    this_iface_ptr->name,
                    this_iface_ptr->instance,
                    this_iface_ptr->iface_private.tx_flow_mask, 0, 0);

    ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                  NULL,
                                  IFACE_FLOW_ENABLED_EV,
                                  event_info);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
} /* ps_iface_enable_flow() */



/*===========================================================================
FUNCTION PS_IFACE_DISABLE_FLOW()

DESCRIPTION
  client disables flow on the interface.

  THIS FUNCTION SHOULD NOT BE CALLED IN AN ISR.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  flow_mask: bit mask that identifies the caller.

RETURN VALUE
  None

DEPENDENCIES

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_disable_flow
(
  ps_iface_type *this_iface_ptr,
  uint32         flow_mask
)
{
  ps_iface_event_info_u_type event_info;
  ds3g_flow_e_type              flow_type = DS_FLOW_DISABLE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  memset(&event_info, 0, sizeof(event_info));

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    store the previous flow mask in the event info variable, and insert the
    client mask into the tx_flow_mask.
  -------------------------------------------------------------------------*/
  event_info.flow_mask = ps_iface_get_flow_mask(this_iface_ptr);

  DS_FLOW_CTRL_SET_MASK(flow_type,
                        this_iface_ptr->iface_private.tx_flow_mask,
                        flow_mask);

  /*-------------------------------------------------------------------------
    only call the callback on the transition to the flow being disabled
  -------------------------------------------------------------------------*/
  if(event_info.flow_mask == ALL_FLOWS_ENABLED &&
     this_iface_ptr->iface_private.tx_flow_mask != ALL_FLOWS_ENABLED)
  {
    LOG_MSG_INFO2_6("client 0x%x disabling flow on iface 0x%x:%d -> mask 0x%x",
                    flow_mask,
                    this_iface_ptr->name,
                    this_iface_ptr->instance,
                    this_iface_ptr->iface_private.tx_flow_mask, 0, 0);

    ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                  NULL,
                                  IFACE_FLOW_DISABLED_EV,
                                  event_info);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
} /* ps_iface_disable_flow() */



/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                       IP ADDRESS INTERFACE FUNCTIONS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*===========================================================================
FUNCTION PS_IFACE_SET_V4_ADDR()

DESCRIPTION
  This function is used to set the local address - this should only be called
  by the addr_event() function - DO NOT TELL the address server - the caller
  is responsible for that.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  ip_addr: V4 address to set.

RETURN VALUE
  0 if sucessful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_set_v4_addr
(
  ps_iface_type          * this_iface_ptr,
  const ps_ip_addr_type  * ip_addr_ptr
)
{
  ps_iface_event_info_u_type  event_info;
  uint32                      old_v4_addr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr) || ip_addr_ptr == NULL)
  {
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if (PS_IFACEI_IP_INFO_IS_INHERITED(this_iface_ptr))
  {
    LOG_MSG_FATAL_ERROR("cannot set an ip address on a logical iface which inherits ip info",
        0, 0, 0);
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  switch (ip_addr_ptr->type)
  {
    case IPV4_ADDR:
      /*---------------------------------------------------------------------
        Assign the the address to the interface
      ---------------------------------------------------------------------*/
      MSG_6(MSG_SSID_DS,
            MSG_LEGACY_MED,
            "Setting iface 0x%x:%d IP Address to %d.%d.%d.%d",
            this_iface_ptr->name,
            this_iface_ptr->instance,
            (unsigned char)(ip_addr_ptr->addr.v4.ps_s_addr),
            (unsigned char)(ip_addr_ptr->addr.v4.ps_s_addr >> 8),
            (unsigned char)(ip_addr_ptr->addr.v4.ps_s_addr >> 16),
            (unsigned char)(ip_addr_ptr->addr.v4.ps_s_addr >> 24));

      old_v4_addr = this_iface_ptr->iface_private.ipv4_addr;
      this_iface_ptr->iface_private.ipv4_addr = ip_addr_ptr->addr.v4.ps_s_addr;

      /*-----------------------------------------------------------------------
        Iface has an IP address in UP and ROUTEABLE state. Post
        IFACE_ADDR_CHANGED_EV if address is changed.

        Iface will also have an IP address in CONFIGURING state if PPP resync
        happened or if IP reconfiguration is triggered. Iface will not have
        an IP address during the initial configuration though. So post
        IFACE_ADDR_CHANGED_EV only if old address is not 0 and if address is
        changed.
      -----------------------------------------------------------------------*/
      if ((PS_IFACEI_GET_STATE(this_iface_ptr) == IFACE_UP ||
           PS_IFACEI_GET_STATE(this_iface_ptr) == IFACE_ROUTEABLE ||
           (PS_IFACEI_GET_STATE(this_iface_ptr) == IFACE_CONFIGURING &&
            old_v4_addr != 0)) &&
          old_v4_addr != ip_addr_ptr->addr.v4.ps_s_addr)
      {
        event_info.ip_addr.type              = IPV4_ADDR;
        event_info.ip_addr.addr.v4.ps_s_addr = old_v4_addr;

        ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                      NULL,
                                      IFACE_ADDR_CHANGED_EV,
                                      event_info);
      }

      break;

    case IP_ADDR_INVALID:
    case IPV6_ADDR:
      /*---------------------------------------------------------------------
        These are not a supported funtion for this interface - though it could
        be modified to set the IID... ut there is a macro for that.
      ---------------------------------------------------------------------*/
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;

    default:
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      ASSERT(0);
      return -1;
  } /* switch(address type) */

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_set_v4_addr() */



/*===========================================================================
FUNCTION PS_IFACE_GET_ADDR()

DESCRIPTION
  This function is used to get the local address of the passed in interface.
  If on interface type is specified in the ip_addr_ptr, either v4 or v6 will
  be returned depending on which one is set (v4 will always be returned if it
  is set).

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  ip_addr_ptr:    value return - the address will be will be stored here

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_get_addr
(
  ps_iface_type *this_iface_ptr,
  ps_ip_addr_type  *ip_addr_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(this_iface_ptr) || ip_addr_ptr == NULL)
  {
    ASSERT(0);
    return;
  }

  /*-------------------------------------------------------------------------
    The address is only valid if the state of the interface is UP - in all
    other cases return invalid address.
  -------------------------------------------------------------------------*/
  switch(this_iface_ptr->iface_private.state)
  {
    case IFACE_DISABLED:
    case IFACE_DOWN:
    case IFACE_COMING_UP:
      /*---------------------------------------------------------------------
        Return the invalid address, the address is invalid in this state
      ---------------------------------------------------------------------*/
      memset(ip_addr_ptr, 0, sizeof(ps_ip_addr_type));
      ip_addr_ptr->type = IP_ADDR_INVALID;
      break;

    case IFACE_CONFIGURING:
      /*---------------------------------------------------------------------
        Note that the address could be 0 in this state but its still valid
      ---------------------------------------------------------------------*/
    case IFACE_ROUTEABLE:
    case IFACE_UP:
      /*---------------------------------------------------------------------
        Returning a valid IP address in the GOING_DOWN state enables
        applications to transmit data in the window between iface
        transitioning to GOING_DOWN state and the link layer being CLOSED.
        The link layer discards packets once the link is CLOSED.
      ---------------------------------------------------------------------*/
    case IFACE_GOING_DOWN:
    case IFACE_LINGERING:
      /*---------------------------------------------------------------------
        Get the address.
      ---------------------------------------------------------------------*/
      ps_iface_get_cached_addr(this_iface_ptr, ip_addr_ptr);
      break;

  default:
    ASSERT(0);
  } /* switch(iface state) */

} /* ps_iface_get_addr() */


/*===========================================================================
FUNCTION PS_IFACE_GET_IPV6_ADDR()

DESCRIPTION
  This function will return IPv6 address for the interface to be used by
  transport layers. If the interface is a v6 interface and IPv6 is enabled
  then this function returns the source address of the interface with
  appropriate scope based on the passed destination address.
  Otherwise it just returns a v4 mapped v6 address by making a call to
  ps_iface_get_addr().

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  dest_ip_ptr:    Destination IPv6 address.
  ip_addr_ptr:    value return - the address will be  will be stored here

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_get_ipv6_addr
(
  ps_iface_type            *this_iface_ptr,
  const struct ps_in6_addr *dest_ip_ptr,
  ps_ip_addr_type          *ip_addr_ptr
)
{
  /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(this_iface_ptr == NULL || dest_ip_ptr == NULL || ip_addr_ptr == NULL)
  {
    LOG_MSG_ERROR("NULL paraemter in ps_iface_get_ipv6_addr()",0,0,0);
    ASSERT(0);
    return;
  }

  /*-----------------------------------------------------------------------
    Check is the destination address is of link-local or global scope.
    If the destination address is of link-local scope, get the link-local
    address otherwise get the global address.
  -----------------------------------------------------------------------*/
  ip_addr_ptr->type = IPV6_ADDR;

#ifdef FEATURE_DATA_PS_IPV6
  if (PS_IN6_IS_ADDR_MC_LINKLOCAL(dest_ip_ptr) ||
                             PS_IN6_IS_ADDR_LINKLOCAL(dest_ip_ptr->ps_s6_addr32))
  {
    ps_iface_get_linklocal_addr(this_iface_ptr, ip_addr_ptr);
  }
  else
#endif /* FEATURE_DATA_PS_IPV6 */
  {
    ps_iface_get_addr(this_iface_ptr, ip_addr_ptr);
  }

  return;
}/* ps_iface_get_ipv6_addr() */


/*===========================================================================
FUNCTION PS_IFACE_GET_CACHED_ADDR()

DESCRIPTION
  This function will return the address that is stored in the requested IP
  address field of the interface regardless of the state.

  Since the IP address can either be stored in this IFACE or an associated
  IFACE, find the IP address, based on the inherit_ip_info boolean.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  ip_addr_ptr:    value return - the address will be  will be stored here

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_get_cached_addr
(
  ps_iface_type *this_iface_ptr,
  ps_ip_addr_type  *ip_addr_ptr
)
{
  ps_iface_type       * base_iface_ptr;  /* iface with ip addr stored locally*/
  boolean               v4_mapped_v6 = FALSE;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr) || ip_addr_ptr == NULL)
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    return;
  }

  /*-------------------------------------------------------------------------
    Determine the type of address being requested - and set the address type
    in the response appropriately.
  -------------------------------------------------------------------------*/
  switch(ip_addr_ptr->type)
  {
    case IP_ANY_ADDR:
      if(ps_iface_addr_family_is_v4(base_iface_ptr))
      {
        ip_addr_ptr->type = IPV4_ADDR;
      }
      else
      {
        ip_addr_ptr->type = IPV6_ADDR;
      }
      break;

    case IPV6_ADDR:
      /*---------------------------------------------------------------------
        If caller has asked for v6 address on a v4 iface, return a v4 mapped
        v6 address.
      ---------------------------------------------------------------------*/
      if(ps_iface_addr_family_is_v4(base_iface_ptr))
      {
        v4_mapped_v6 = TRUE;
      }
      break;

    case IPV4_ADDR:
      if(!ps_iface_addr_family_is_v4(base_iface_ptr))
      {
        ip_addr_ptr->type = IP_ADDR_INVALID;
      }
      break;

    default:
      ASSERT(0);

  } /* switch(requested address type) */

  /*-------------------------------------------------------------------------
    Based on the response type fill in the address field apporpriately
  -------------------------------------------------------------------------*/
  switch(ip_addr_ptr->type)
  {
    case IPV4_ADDR:
      ip_addr_ptr->addr.v4.ps_s_addr = base_iface_ptr->iface_private.ipv4_addr;
      break;

    case IPV6_ADDR:
      /*-----------------------------------------------------------------------
        Map v4 to v6 address if necessary.
      -----------------------------------------------------------------------*/
      if(v4_mapped_v6)
      {
        PS_IN6_GET_V4_MAPPED_V6_FROM_V4(ip_addr_ptr->addr.v6.ps_s6_addr,
                                     base_iface_ptr->iface_private.ipv4_addr);
      }
#ifdef FEATURE_DATA_PS_IPV6
      else
      {
        if( (base_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX] ==
             NULL) ||
            ((base_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->prefix ==
             0) &&
            (base_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->iid ==
             0)))
        {
          LOG_MSG_INFO1("No valid IPv6 address exists on iface %p",
                   base_iface_ptr, 0, 0);

          memset(ip_addr_ptr, 0, sizeof(ps_ip_addr_type));
          ip_addr_ptr->type = IP_ADDR_INVALID;
          PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
          return;
        }
        memcpy
        (
          ip_addr_ptr->addr.v6.ps_s6_addr64,
          &(base_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->prefix),
          sizeof(uint64)
        );
        memcpy
        (
          ip_addr_ptr->addr.v6.ps_s6_addr64 + 1,
          &(base_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->iid),
          sizeof(uint64)
        );
      }
#else
      else
      {
        memset(ip_addr_ptr, 0, sizeof(ps_ip_addr_type));
        ip_addr_ptr->type = IP_ADDR_INVALID;
      }
#endif /* FEATURE_DATA_PS_IPV6 */
      break;

    case IP_ADDR_INVALID:
      memset(ip_addr_ptr, 0, sizeof(ps_ip_addr_type));
      ip_addr_ptr->type = IP_ADDR_INVALID;
      break;

    default:
      ASSERT(0);
  } /* switch(addr type) */

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

} /* ps_iface_get_cached_addr() */



/*===========================================================================
FUNCTION PS_IFACE_GET_V4_ADDR

DESCRIPTION
  This function returns the IPV4 address of an iface.  If the iface is
  NULL or IPV6, then it returns NULL.

PARAMETERS
  this_iface_ptr: Target iface ptr

RETURN VALUE
  An IPV4 address if the iface is valid and in the IPV4 family.  Else
  returns NULL.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
uint32 ps_iface_get_v4_addr
(
  ps_iface_type       *this_iface_ptr
)
{
  ps_ip_addr_type            ip_addr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if( PS_IFACE_IS_VALID(this_iface_ptr) )
  {
    /*-----------------------------------------------------------------------
      Set the ip_addr struct to all zero's.  This includes the field which
      holds the IP address.
    -----------------------------------------------------------------------*/
    ip_addr.type = IPV4_ADDR;

    ps_iface_get_cached_addr(this_iface_ptr, &ip_addr);

    if (ip_addr.type == IP_ADDR_INVALID)
    {
      return(0);
    }
    else
    {
      return(ip_addr.addr.v4.ps_s_addr);
    }
  }
  return 0x00000000;
} /* ps_iface_get_v4_addr() */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_FAMILY_IS_ANY()

DESCRIPTION
  This function determines if the specified ps_iface is an ANY iface. This
  means that the interface supports receiving both IPv4 and IPv6 packets.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.

RETURN VALUE
  true if any iface, false otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_addr_family_is_any
(
  const ps_iface_type        *this_iface_ptr
)
{
 /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return(FALSE);
  }

  return (ps_iface_get_addr_family((ps_iface_type*)this_iface_ptr) ==
                                                      IFACE_ANY_ADDR_FAMILY);
} /* ps_iface_addr_family_is_any() */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_FAMILY_IS_V4()

DESCRIPTION
  This function determines if the specified ps_iface is an IPv4 iface.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.

RETURN VALUE
  true if v4 iface, false otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_addr_family_is_v4
(
  const ps_iface_type        *this_iface_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return(FALSE);
  }

  return (ps_iface_get_addr_family((ps_iface_type *)this_iface_ptr) ==
                                                      IFACE_IPV4_ADDR_FAMILY);
} /* ps_iface_addr_family_is_v4 */

/*===========================================================================
FUNCTION PS_IFACE_ADDR_FAMILY_IS_V6()

DESCRIPTION
  This function determines if the specified ps_iface is an IPv6 iface.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.

RETURN VALUE
  true if v6 iface, false otherwise

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_addr_family_is_v6
(
  const ps_iface_type        *this_iface_ptr
)
{
 /*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return(FALSE);
  }

  return (ps_iface_get_addr_family((ps_iface_type *)this_iface_ptr) ==
                                                      IFACE_IPV6_ADDR_FAMILY);
} /* ps_iface_addr_family_is_v6() */


/*===========================================================================
FUNCTION PS_IFACE_SET_ADDR_FAMILY()

DESCRIPTION
  This function will configure the interface to support the requested IP
  address family.

  This can be called on a logical iface which inherits its IP info.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  addr_family:    the address family to set this interface up for.

RETURN VALUE
  0 on success
 -1 on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_set_addr_family
(
  ps_iface_type             *this_iface_ptr,
  ps_iface_addr_family_type  addr_family
)
{
  ps_iface_event_info_u_type    event_info;
#ifdef FEATURE_DATA_PS_IPV6
  uint8                         i;
  ps_ifacei_v6_addr_type     *v6_addr_ptr;
#endif /* FEATURE_DATA_PS_IPV6 */

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if ( PS_IFACEI_IP_INFO_IS_INHERITED(this_iface_ptr) )
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_FATAL_ERROR("Can't set addr family when ip info is inherited",0,0,0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    The address family can only be changed if we are not in Up, Routable, or
    going down.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_GET_STATE(this_iface_ptr) &
      (IFACE_ROUTEABLE | IFACE_UP | IFACE_GOING_DOWN))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  event_info.addr_family_info.old_addr_family =
    ps_iface_get_addr_family(this_iface_ptr);

  /*-------------------------------------------------------------------------
    Set the IP input function pointer to the correct IP input function - or
    fail if the address family value is not recognized.  Zero out the other
    IP address for consistency.
  -------------------------------------------------------------------------*/
  switch(addr_family)
  {
  case IFACE_IPV4_ADDR_FAMILY:

#ifdef FEATURE_DATACOMMON_PS_IFACE_IO
    this_iface_ptr->iface_private.ip_input = ip4_input;
#endif

    this_iface_ptr->iface_private.addr_family = IFACE_IPV4_ADDR_FAMILY;

#ifdef FEATURE_DATA_PS_IPV6
    if(this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX] != NULL)
    {
      for(i = 0; i < MAX_IPV6_ADDRS; i++)
      {
        v6_addr_ptr = this_iface_ptr->iface_private.ipv6_addrs[i];
        if(v6_addr_ptr == NULL)
        {
          break;
        }
        PS_TIMER_FREE_HANDLE(v6_addr_ptr->pref_lifetimer_handle);
        PS_TIMER_FREE_HANDLE(v6_addr_ptr->valid_lifetimer_handle);
        PS_MEM_FREE(this_iface_ptr->iface_private.ipv6_addrs[i]);
      }
    }
#endif /* FEATURE_DATA_PS_IPV6 */
    break;

#ifdef FEATURE_DATA_PS_IPV6
  case IFACE_IPV6_ADDR_FAMILY:
    /*------------------------------------------------------------------------
      Allocate the initial public address. All privacy addresses are allocated
      dynamically when requested.
    ------------------------------------------------------------------------*/
    if(this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX] == NULL)
    {
      this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX] =
        (ps_ifacei_v6_addr_type *) ps_mem_get_buf(PS_MEM_IPV6_ADDR_TYPE);

      if(NULL == this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX])
      {
        PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
        LOG_MSG_ERROR("No IPv6 address items available!",0,0,0);
        return -1;
      }
    }
    memset(this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX],
           0,
           sizeof(ps_ifacei_v6_addr_type));
#ifdef FEATURE_DATACOMMON_PS_IFACE_IO
    this_iface_ptr->iface_private.ip_input = ip6_input;
#endif

    this_iface_ptr->iface_private.addr_family = IFACE_IPV6_ADDR_FAMILY;
    this_iface_ptr->iface_private.ipv4_addr = 0;
    break;
#endif /* FEATURE_DATA_PS_IPV6 */

  case IFACE_ANY_ADDR_FAMILY:
    /*-----------------------------------------------------------------------
      Currently only multicast ifaces are supported for receiving both
      IPv4 and IPv6 data on the same interface.
    -----------------------------------------------------------------------*/
    if(this_iface_ptr->group_mask == BCAST_MCAST_GROUP)
    {
#ifdef FEATURE_DATACOMMON_PS_IFACE_IO
      this_iface_ptr->iface_private.ip_input = ip_any_input;
#endif

      this_iface_ptr->iface_private.addr_family = IFACE_ANY_ADDR_FAMILY;

      /*---------------------------------------------------------------------
        Clear both IPv4 and IPv6 addresses.
      ---------------------------------------------------------------------*/
      this_iface_ptr->iface_private.ipv4_addr = 0;
#ifdef FEATURE_DATA_PS_IPV6
      if(this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX] != NULL)
      {
        for(i = 0; i < MAX_IPV6_ADDRS; i++)
        {
          v6_addr_ptr = this_iface_ptr->iface_private.ipv6_addrs[i];
          if(v6_addr_ptr == NULL)
          {
            break;
          }
          PS_TIMER_FREE_HANDLE(v6_addr_ptr->pref_lifetimer_handle);
          PS_TIMER_FREE_HANDLE(v6_addr_ptr->valid_lifetimer_handle);
          PS_MEM_FREE(this_iface_ptr->iface_private.ipv6_addrs[i]);
        }
      }
#endif /* FEATURE_DATA_PS_IPV6 */
    }
    else
    {
      LOG_MSG_ERROR("Interface group: %x is not BCAST_MCAST",
                 this_iface_ptr->group_mask,0,0);
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;
    }
    break;

  default:
    LOG_MSG_ERROR("IPv%d is not supported", addr_family, 0, 0);
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;

  } /* switch(addr_family) */


  /*---------------------------------------------------------------------------
    If the address family is changed, call the address family changed callbacks
  ---------------------------------------------------------------------------*/
  if (event_info.addr_family_info.old_addr_family != addr_family)
  {
     event_info.addr_family_info.new_addr_family = addr_family;
     ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                   NULL,
                                   IFACE_ADDR_FAMILY_CHANGED_EV,
                                   event_info);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_set_addr_family() */



/*===========================================================================
FUNCTION PS_IFACE_GET_ADDR_FAMILY()

DESCRIPTION
  This function retrieves the address family an interface is supporting.
  Based on inherit_ip_info boolean, will find addr_family either
  in this IFACE or in an underlying associated iface.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.

RETURN VALUE
  The address family on the curent interface
  IFACE_INVALID_ADDR_FAMILY on failure.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
ps_iface_addr_family_type  ps_iface_get_addr_family
(
  ps_iface_type          *this_iface_ptr
)
{
  ps_iface_type    *base_iface_ptr;  /* iface with ip addr stored locally  */
  ps_iface_addr_family_type  ret_val = IFACE_INVALID_ADDR_FAMILY;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    return IFACE_INVALID_ADDR_FAMILY;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

 /*------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    return IFACE_INVALID_ADDR_FAMILY;
  }

  ret_val = base_iface_ptr->iface_private.addr_family;

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return ret_val;

} /* ps_iface_get_addr_family() */


/*===========================================================================
FUNCTION PS_IFACE_GET_PEER_ADDR()

DESCRIPTION
  This function returns the address of the gateway node on this interface.
  It will return the address of the interfaces peer (e.g. the
  GGSN, or the PDSN) if this is a point-to-point link.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  ip_addr_ptr:    value return - the address will be will be stored here

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  ip_addr_ptr->type will be set to IP_ADDR_INVALID if the function is called
  in an invalid state or if the gateway address is not available.
  For V6 ifaces, gateway address will be computed using link local prefix.
===========================================================================*/
void ps_iface_get_peer_addr
(
  const ps_iface_type *this_iface_ptr,
  ps_ip_addr_type     *ip_addr_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if (!PS_IFACE_IS_VALID(this_iface_ptr) || ip_addr_ptr == NULL)
  {
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    The address is only valid if the state of the interface is UP - in all
    other cases return invalid address.
  -------------------------------------------------------------------------*/
  switch(this_iface_ptr->iface_private.state)
  {
    case IFACE_DISABLED:
    case IFACE_DOWN:
    case IFACE_COMING_UP:

      /*---------------------------------------------------------------------
        Return the invalid address, the address is invalid in this state
      ---------------------------------------------------------------------*/
      ip_addr_ptr->type = IP_ADDR_INVALID;
      break;

    case IFACE_ROUTEABLE:
    case IFACE_UP:
    case IFACE_GOING_DOWN:

    /*-----------------------------------------------------------------------
      Following state occurs even during the data session, hence is valid
    -----------------------------------------------------------------------*/
    case IFACE_CONFIGURING:
      break;

  default:
    ASSERT(0);
  } /* switch(iface state) */

  if(ip_addr_ptr->type == IP_ANY_ADDR)
  {
    ip_addr_ptr->type =
      (ADDR_FAMILY_IS_V4(this_iface_ptr)) ? IPV4_ADDR : IPV6_ADDR;
  }

  switch(ip_addr_ptr->type)
  {
  case IPV4_ADDR:
    /*-----------------------------------------------------------------------
      return the v4 gateway address if available
    -----------------------------------------------------------------------*/
    if(this_iface_ptr->v4_net_info.gateway.ps_s_addr)
    {
      ip_addr_ptr->addr.v4.ps_s_addr =
                               this_iface_ptr->v4_net_info.gateway.ps_s_addr;
    }
    else
    {
      ip_addr_ptr->type = IP_ADDR_INVALID;
    }
    break;

  case IPV6_ADDR:
    /*-----------------------------------------------------------------------
      return the link local IPv6 address, if gateway iid is available
    -----------------------------------------------------------------------*/
#ifdef FEATURE_DATA_PS_IPV6
    if((this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX] != NULL) &&
       (this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->gateway_iid))
    {
      ip_addr_ptr->addr.v6.ps_s6_addr32[0] = ps_htonl(0xFE800000UL);
      ip_addr_ptr->addr.v6.ps_s6_addr32[1] = 0;
      ip_addr_ptr->addr.v6.ps_s6_addr64[1] =
       this_iface_ptr->iface_private.ipv6_addrs[DEFAULT_V6_INDEX]->gateway_iid;
    }
    else
#endif /* FEATURE_DATA_PS_IPV6 */
    {
      ip_addr_ptr->type = IP_ADDR_INVALID;
    }
    break;

  case IP_ADDR_INVALID:
    break;

  default:
    ASSERT(0);
  } /* switch(requested address type) */

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  if(ip_addr_ptr->type == IP_ADDR_INVALID)
  {
    memset(ip_addr_ptr, 0, sizeof(ps_ip_addr_type));
    ip_addr_ptr->type = IP_ADDR_INVALID;
  }
} /* ps_iface_get_peer_addr() */


/*===========================================================================
FUNCTION PS_IFACE_SET_V4_DNS_ADDRS

DESCRIPTION
  This function sets the primary and secondary DNS addr's on the
  IPV4 iface.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  prim_dns_ptr:  input primary dns address
  sec_dns_ptr:   input secondary dns address

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_set_v4_dns_addrs
(
  ps_iface_type               *this_iface_ptr,
  const ps_ip_addr_type       *prim_dns_ptr,
  const ps_ip_addr_type       *sec_dns_ptr
)
{
  ps_iface_type    *base_iface_ptr;  /* iface with ip addr stored locally  */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (prim_dns_ptr == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed, prim_dns_ptr, 0x%p",
              prim_dns_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", this_iface_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if (ps_iface_get_addr_family(this_iface_ptr) != IPV4_ADDR)
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_INFO1("Cannot set v4 DNS address on a non v4 interface",0,0,0);
    return -1;
  }

  /*------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Ensure that the addr family is consistent through the logica/assoc
    iface chain.
  -------------------------------------------------------------------------*/
  ASSERT(ps_iface_get_addr_family(base_iface_ptr) == IPV4_ADDR);

  base_iface_ptr->v4_net_info.primary_dns = prim_dns_ptr->addr.v4;
  if(sec_dns_ptr != NULL)
  {
    base_iface_ptr->v4_net_info.secondary_dns = sec_dns_ptr->addr.v4;
  }
  else
  {
    base_iface_ptr->v4_net_info.secondary_dns.ps_s_addr = 0;
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_set_v4_dns_addrs() */


/*===========================================================================
FUNCTION PS_IFACE_GET_V4_DNS_ADDRS

DESCRIPTION
  This function returns the primary and secondary DNS addr's on the
  IPV4 iface.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  prim_dns_ptr:  storage for primary dns address
  sec_dns_ptr:  storage for secondary dns address

RETURN VALUE
  None.  However, if the addr family is not IPV4, then the input
  parameters are stored with zero.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_get_v4_dns_addrs
(
  ps_iface_type         *this_iface_ptr,
  ip_addr_type          *prim_dns_ptr,
  ip_addr_type          *sec_dns_ptr
)
{
  ps_iface_type    *base_iface_ptr;  /* iface with ip addr stored locally  */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (prim_dns_ptr == NULL || sec_dns_ptr == NULL)
  {
    LOG_MSG_ERROR("NULL parameters are passed, prim_dns_ptr, 0x%p, "
              "sec_dns_ptr, 0x%p",
              prim_dns_ptr, sec_dns_ptr, 0);
    ASSERT(0);
    return;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", this_iface_ptr, 0, 0);
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if (ps_iface_get_addr_family(this_iface_ptr) != IPV4_ADDR)
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    prim_dns_ptr->addr.v4 = 0;
    sec_dns_ptr->addr.v4  = 0;
    prim_dns_ptr->type    = IP_ADDR_INVALID;
    sec_dns_ptr->type     = IP_ADDR_INVALID;
    return;
  }

  /*------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    return;
  }

  /*-------------------------------------------------------------------------
    Ensure that the addr family is consistent through the logica/assoc
    iface chain.
  -------------------------------------------------------------------------*/
  ASSERT(ps_iface_get_addr_family(base_iface_ptr) == IPV4_ADDR);

  prim_dns_ptr->addr.v4 = base_iface_ptr->v4_net_info.primary_dns.ps_s_addr;
  sec_dns_ptr->addr.v4  = base_iface_ptr->v4_net_info.secondary_dns.ps_s_addr;

  /*-------------------------------------------------------------------------
    Set appropriate ip address types.
  -------------------------------------------------------------------------*/
  if(prim_dns_ptr->addr.v4 == 0)
  {
    prim_dns_ptr->type    = IP_ADDR_INVALID;
  }
  else
  {
    prim_dns_ptr->type    = IPV4_ADDR;
  }

  if(sec_dns_ptr->addr.v4 == 0)
  {
    sec_dns_ptr->type     = IP_ADDR_INVALID;
  }
  else
  {
    sec_dns_ptr->type     = IPV4_ADDR;
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
} /* ps_iface_get_v4_dns_addrs() */


/*===========================================================================
FUNCTION PS_IFACE_GET_ALL_DNS_ADDRS

DESCRIPTION
  This function returns all the DNS addresses stored on the interface.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  dns_addrs_ptr:  The ptr to the DNS addrs
  num_dns_addrs_ptr:  The number of DNS addresses space is allocated for.
                      This is an output param as well and will be set to the
                      # of valid DNS addresses actually returned.

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_get_all_dns_addrs
(
  ps_iface_type         *this_iface_ptr,
  ip_addr_type          *dns_addrs_ptr,
  uint8                 *num_dns_addrs_ptr
)
{
  ip_addr_type      prim_dns_addr, sec_dns_addr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if(!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", this_iface_ptr, 0, 0);
    ASSERT(0);
    return;
  }

  if( (dns_addrs_ptr == NULL) || (num_dns_addrs_ptr == NULL) )
  {
    LOG_MSG_ERROR("NULL param, dns_addrs_ptr: 0x%p or num_dns_addrs_ptr: 0x%p",
              dns_addrs_ptr, num_dns_addrs_ptr, 0);
    ASSERT(0);
    return;
  }

  /*-------------------------------------------------------------------------
    This function to be changed once more than 2 DNS addresses are supported.
  -------------------------------------------------------------------------*/
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  if(ps_iface_addr_family_is_v4(this_iface_ptr))
  {
    ps_iface_get_v4_dns_addrs(this_iface_ptr, &prim_dns_addr, &sec_dns_addr);
  }
#ifdef FEATURE_DATA_PS_IPV6
  else if(ps_iface_addr_family_is_v6(this_iface_ptr))
  {
    ps_iface_get_v6_dns_addrs(this_iface_ptr, &prim_dns_addr, &sec_dns_addr);
  }
#endif /* FEATURE_DATA_PS_IPV6 */
  else
  {
   PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
   ASSERT(0);
   return;
  }

  /*-------------------------------------------------------------------------
    This to be changed once multiple DNS addresses are supported.
  -------------------------------------------------------------------------*/
  if(*num_dns_addrs_ptr >= 2)
  {
    memcpy(&dns_addrs_ptr[0], &prim_dns_addr, sizeof(ip_addr_type) );
    memcpy(&dns_addrs_ptr[1], &sec_dns_addr, sizeof(ip_addr_type) );
    *num_dns_addrs_ptr = 2;
  }
  else
  {
    memcpy(&dns_addrs_ptr[0], &prim_dns_addr, sizeof(ip_addr_type) );
    *num_dns_addrs_ptr = 1;
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
} /* ps_iface_get_all_dns_addrs() */

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

                                  COMMANDS

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/

/*===========================================================================
FUNCTION PS_IFACE_BRING_UP_CMD_EX()

DESCRIPTION
  This function is used to bring up an iface.  Will call the registered
  callback if the interface is in the down state.  If it is in the up state,
  it returns success.  If the interface is in the coming state or down it
  return DS_EWOULDBLOCK.  Finally if it is in the going down state, it
  returns DS_ECLOSEINPROGRESS.

  The operations goes like the following:
    - For a non-active iface,
      1. increment client ref count
      2. bring up iface
        - internally, call bring up cback is called only if the iface is in
          down state.
    - For an active iface
      1. increment client ref count
      2. bring up iface
        - internally, bring up cback is called only if the iface is in down
          state, e.g., the iface has not been brought up by the interface
          controller or by an app.
      3. check the ref count and call in_use_cback for the 1st iface client

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  app_priority :  priority of the app requesting for iface to be up.
  ps_errno: value to return error codes
  client_data_ptr: data, that is passed to the client handler function.

RETURN VALUE
  0: if interface is UP on return, operation succeeded
 -1: if interface is not UP on return
     operation result is in ps_errno:
     DS_EWOULDBLOCK:  interface is COMING UP
     else:            interface is DOWN and bring up failed

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_bring_up_cmd_ex
(
  ps_iface_type  * this_iface_ptr,
  int32            app_priority,
  int16          * ps_errno,
  void           * client_data_ptr
)
{
  int  ret_val = -1;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL ps_errno", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  if (app_priority <= PS_POLICY_MGR_PRIORITY_INVALID ||
      app_priority > PS_POLICY_MGR_PRIORITY_MAX)
  {
    LOG_MSG_ERROR("Invalid App priority", 0, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  MSG_4(MSG_SSID_DS,
        MSG_LEGACY_HIGH,
        "Bringing up iface 0x%x:%d, old priority 0x%x, req app priority %d",
        this_iface_ptr->name,
        this_iface_ptr->instance,
        PS_IFACEI_GET_PRIORITY_MASK(this_iface_ptr),
        app_priority);

  PS_IFACEI_INC_REF_CNT( this_iface_ptr, app_priority);

  MSG_5(MSG_SSID_DS,
        MSG_LEGACY_HIGH,
        "Bringing up iface 0x%x:%d, state %d, ref_cnt %d, new priority 0x%x",
        this_iface_ptr->name,
        this_iface_ptr->instance,
        PS_IFACEI_GET_STATE(this_iface_ptr),
        PS_IFACE_GET_REF_CNT(this_iface_ptr),
        PS_IFACEI_GET_PRIORITY_MASK(this_iface_ptr));

  ret_val = ps_iface_active_bring_up_cmd_ex( this_iface_ptr,
                                             app_priority,
                                             ps_errno,
                                             client_data_ptr );

  /*-----------------------------------------------------------------------
   Check status and check if action for active iface is needed
  ------------------------------------------------------------------------*/
  if ( ret_val == -1 && *ps_errno == DS_EOPNOTSUPP )
  {
    PS_IFACEI_DEC_REF_CNT( this_iface_ptr, app_priority);
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid status from ps_iface_active_bring_up_cmd", 0, 0, 0);
    return -1;
  }


  if ( PS_IFACE_GET_REF_CNT(this_iface_ptr) == 1 &&
       PS_IFACE_GET_IS_ACTIVE(this_iface_ptr) == TRUE )
  {
    if ( this_iface_ptr->in_use_cmd_f_ptr != NULL )
    {
      (void) this_iface_ptr->in_use_cmd_f_ptr(this_iface_ptr, client_data_ptr);
    }
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return ret_val;

} /* ps_iface_bring_up_cmd_ex() */


/*===========================================================================
FUNCTION PS_IFACE_ACTIVE_BRING_UP_CMD()

DESCRIPTION
  This function is used to bring up an iface.  This function is not required
  to be called for an application. It does not modify the ref_count. It simply
  tries to bring up an iface. Will call the registered
  callback if the interface is in the down state.  If it is in the up state,
  it returns success.  If the interface is in the coming state or down it
  returne ewouldblock.  Finally if it is in the going down state, it returns
  closeinprogress.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  ps_errno:       value to return error codes
  client_data_ptr: data, that is passed to the client handler function.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_active_bring_up_cmd
(
  ps_iface_type *this_iface_ptr,
  int16         *ps_errno,
  void          *client_data_ptr
)
{

  LOG_MSG_INFO2( "Active bring up cmd", 0, 0, 0 );
  return ps_iface_active_bring_up_cmd_ex( this_iface_ptr,
                                          PS_POLICY_MGR_LEGACY_PRIORITY,
                                          ps_errno,
                                          client_data_ptr );

}  /* ps_iface_active_bring_up_cmd */

/*===========================================================================
FUNCTION PS_IFACE_ACTIVE_BRING_UP_CMD_EX()

DESCRIPTION
  This function is used to bring up an iface.  This function is not required
  to be called for an application. It does not modify the ref_count. It simply
  tries to bring up an iface. Will call the registered
  callback if the interface is in the down state.  If it is in the up state,
  it returns success.  If the interface is in the coming state or down it
  returne ewouldblock.  Finally if it is in the going down state, it returns
  closeinprogress.

PARAMETERS
  this_iface_ptr:  ptr to interface control block on which to operate on.
  app_priority:    application priority
  ps_errno:        value to return error codes
  client_data_ptr: data, that is passed to the client handler function.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_active_bring_up_cmd_ex
(
  ps_iface_type *this_iface_ptr,
  int32          app_priority,
  int16         *ps_errno,
  void          *client_data_ptr
)
{
  ps_flow_type               * ps_flow_ptr = NULL;
  ps_iface_event_info_u_type   event_info;
  ps_timer_error_type          timer_ret_val;
  int                          ret_val = -1;
  int                          result  =  0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY( "Iface 0x%p, app_prio %d", this_iface_ptr, app_priority, 0);

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL ps_errno", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  MSG_4(MSG_SSID_DS,
        MSG_LEGACY_HIGH,
        "Active bringing up iface 0x%x:%d, state %d, curr priority 0x%x",
        this_iface_ptr->name,
        this_iface_ptr->instance,
        PS_IFACEI_GET_STATE(this_iface_ptr),
        PS_IFACEI_GET_PRIORITY_MASK(this_iface_ptr));

  /*-------------------------------------------------------------------------
    take action based on the current state.
  -------------------------------------------------------------------------*/
  switch(PS_IFACEI_GET_STATE(this_iface_ptr))
  {
  case IFACE_UP:
  case IFACE_ROUTEABLE:
    ret_val = 0;
    break;

  case IFACE_LINGERING:
    /*-----------------------------------------------------------------------
      If the IFACE is lingering cancel the timer and post UP indication.
    -----------------------------------------------------------------------*/
    if( this_iface_ptr->iface_private.iface_linger_timer != 0 &&
        ps_timer_is_running(this_iface_ptr->iface_private.iface_linger_timer))
    {
      timer_ret_val=
        ps_timer_cancel(this_iface_ptr->iface_private.iface_linger_timer);
      if(timer_ret_val != PS_TIMER_SUCCESS)
      {
        LOG_MSG_ERROR("Error canceling timer 0x%x",
          this_iface_ptr->iface_private.iface_linger_timer, 0, 0);
        ASSERT(0);
      }

      this_iface_ptr->last_io_time = (uint32)msclock();

      ps_iface_up_ind(this_iface_ptr);
      ret_val = 0;
    }
    else
    {
      *ps_errno = DS_EINVAL;
      ret_val = -1;
      ASSERT (0);
      LOG_MSG_ERROR("Invalid linger timer handle ",0,0,0);
    }
    break;

  case IFACE_DOWN:

    this_iface_ptr->last_io_time = (uint32)msclock();

    //TODO should we have default values for DNS?  If so then copy them to the
    // net info DNS slots here here.
    /*-----------------------------------------------------------------------
      Call down indication in case of any error to reset all iface_cb
      variables appropriately.
    -----------------------------------------------------------------------*/
    if( ( this_iface_ptr->bring_up_cmd_f_ptr    != NULL ) ||
        ( this_iface_ptr->bring_up_cmd_f_ptr_ex != NULL ) )
    {
      /*---------------------------------------------------------------------
        Can't bind default flow to primary phys link for logical ifaces since
        they don't have phys links. Binding is done explictly when logical
        iface associates with a physical iface

        Also, no need to do similar bind for an iface supporting dynamic phys
        link binding since in this case, the binding should be taken care of
        explicitly by the iface controller.
      ---------------------------------------------------------------------*/
      if (this_iface_ptr->iface_private.phys_link.array != NULL )
      {
        ps_flow_ptr = PS_IFACEI_GET_DEFAULT_FLOW(this_iface_ptr);
        (void) ps_flowi_bind_phys_link(ps_flow_ptr,
                                       PS_IFACEI_GET_PHYS_LINK(this_iface_ptr));
      }

      event_info.state = this_iface_ptr->iface_private.state;
      this_iface_ptr->iface_private.state = IFACE_COMING_UP;

      ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                    NULL,
                                    IFACE_COMING_UP_EV,
                                    event_info);

      /* Check if ex ptr is defined, use that */
      if ( this_iface_ptr->bring_up_cmd_f_ptr_ex != NULL )
      {
        result = this_iface_ptr->bring_up_cmd_f_ptr_ex( this_iface_ptr,
                                                        app_priority,
                                                        client_data_ptr );
      }
      else if ( this_iface_ptr->bring_up_cmd_f_ptr != NULL )
      {
        result = this_iface_ptr->bring_up_cmd_f_ptr( this_iface_ptr,
                                                     client_data_ptr );
      }

      if( result < 0 )
      {
        if (DS_ENETCLOSEINPROGRESS ==
              this_iface_ptr->iface_private.ps_errno_for_bring_up_cmd_f_ptr_failure)
        {
          *ps_errno = DS_ENETCLOSEINPROGRESS;
        }
        else
        {
          *ps_errno = DS_ENETNONET;

          if (ps_flow_ptr != NULL)
          {
            ps_flowi_unbind_phys_link(ps_flow_ptr);
          }

          ps_iface_down_ind(this_iface_ptr); /* will dec phys link ref cnt */
        }
      }
      else
      {
        *ps_errno = DS_EWOULDBLOCK;
      }
    }
    else
    {
      LOG_MSG_ERROR("Invalid bring up cmd fptr and fptr_ex, ret DS_EOPNOTSUPP",0,0,0);
      *ps_errno = DS_EOPNOTSUPP;
      ps_iface_down_ind(this_iface_ptr);
    }
    ret_val = -1;
    break;

  case IFACE_COMING_UP:
  case IFACE_CONFIGURING:
    *ps_errno = DS_EWOULDBLOCK;
    ret_val = -1;
    break;

  case IFACE_GOING_DOWN:
    /*-----------------------------------------------------------------------
      The iface ref_cnt will go to zero when the IFACE is down().  Do not
      decrement on this error.
    -----------------------------------------------------------------------*/
    *ps_errno = DS_ENETCLOSEINPROGRESS;
    ret_val = -1;
    break;

  default:
    ASSERT(0);
    MSG_4(MSG_SSID_DS,
          MSG_LEGACY_HIGH,
          "ps iface 0x%x:%d active bring up failure, ref cnt %d, priority 0x%x",
          this_iface_ptr->name,
          this_iface_ptr->instance,
          PS_IFACE_GET_REF_CNT(this_iface_ptr),
          PS_IFACEI_GET_PRIORITY_MASK(this_iface_ptr));
    *ps_errno = DS_EOPNOTSUPP;
    ret_val = -1;
    break;

  } /* switch(state) */

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return ret_val;

} /* ps_iface_active_bring_up_cmd_ex */

/*===========================================================================
FUNCTION PS_IFACE_BRING_UP_CMD()

DESCRIPTION
  This function is used to bring up an iface.  Will call the registered
  callback if the interface is in the down state.  If it is in the up state,
  it returns success.  If the interface is in the coming state or down it
  return DS_EWOULDBLOCK.  Finally if it is in the going down state, it
  returns DS_ECLOSEINPROGRESS.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  ps_errno:       value to return error codes
  client_data_ptr: data, that is passed to the client handler function.

RETURN VALUE
  0: if interface is UP on return, operation succeeded
 -1: if interface is not UP on return
     operation result is in ps_errno:
     DS_EWOULDBLOCK:  interface is COMING UP
     else:            interface is DOWN and bring up failed

DEPENDENCIES
  Uses PS_POLICY_MGR_LEGACY_PRIORITY as app priority for bringing up iface

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_bring_up_cmd
(
  ps_iface_type *this_iface_ptr,
  int16         *ps_errno,
  void          *client_data_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  return ps_iface_bring_up_cmd_ex(this_iface_ptr,
                                  PS_POLICY_MGR_LEGACY_PRIORITY,
                                  ps_errno,
                                  client_data_ptr);
} /* ps_iface_bring_up_cmd() */


/*===========================================================================
FUNCTION PS_IFACE_TEAR_DOWN_CMD_EX()

DESCRIPTION
  This function is used to stop an iface.  If it is in the down state,
  it returns success.  If the interface is in the coming up or going down
  state it return DS_EWOULDBLOCK.

  The operations goes like the following:
    - For a non-active iface,
      1. decrement client ref count
      2. if ref count is down to 0, actually tear down iface
    - For an active iface
      1. decrement client ref count
      2. if ref count is down to 0, do not actually tear down iface. Instead,
         call out_use_cback for the last app using the iface (iface controller
         should explicitly tear down active ifaces).

PARAMETERS
  this_iface_prtr: ptr to interface control block on which to operate on.
  app_priority : priority of the app for which iface is torn down.
  ps_errno: error code to return to caller.
  client_data_ptr: data, that is passed to the client handler function.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_tear_down_cmd_ex
(
  ps_iface_type  * this_iface_ptr,
  int32            app_priority,
  int16          * ps_errno,
  void           * client_data_ptr
)
{
  int  ret_val = -1;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL ps_errno", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  if (app_priority <= PS_POLICY_MGR_PRIORITY_INVALID ||
      app_priority > PS_POLICY_MGR_PRIORITY_MAX)
  {
    LOG_MSG_ERROR("Invalid App priority", 0, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  MSG_4(MSG_SSID_DS,
        MSG_LEGACY_HIGH,
        "Tearing down iface 0x%x:%d, old priority 0x%x, req app priority %d",
        this_iface_ptr->name,
        this_iface_ptr->instance,
        PS_IFACEI_GET_PRIORITY_MASK(this_iface_ptr),
        app_priority);

  PS_IFACEI_DEC_REF_CNT( this_iface_ptr, app_priority);

  MSG_5(MSG_SSID_DS,
        MSG_LEGACY_HIGH,
        "Tearing down iface 0x%x:%d, state %d, ref_cnt %d, new priority 0x%x",
        this_iface_ptr->name,
        this_iface_ptr->instance,
        PS_IFACEI_GET_STATE(this_iface_ptr),
        PS_IFACE_GET_REF_CNT(this_iface_ptr),
        PS_IFACEI_GET_PRIORITY_MASK(this_iface_ptr));

  if(PS_IFACE_GET_REF_CNT(this_iface_ptr) == 0)
  {
    /*-----------------------------------------------------------------------
      take action based on the current state only when ref_cnt = 0 (meaning
      no clients interested in this IFACE).

      - For a non-active iface, actually tear down iface
      - For an active iface, do not actually tear down iface. Instead, call
        out_use_cback. (Interface handler should tear down the active iface
        explicitly.
    -----------------------------------------------------------------------*/
    if ( PS_IFACE_GET_IS_ACTIVE(this_iface_ptr) == FALSE )
    {
      ret_val = ps_iface_active_tear_down_cmd_ex ( this_iface_ptr,
                                                   app_priority,
                                                   ps_errno,
                                                   client_data_ptr );
    }
    else
    {
      PS_IFACEI_RESET_IN_USE(this_iface_ptr);
      PS_IFACEI_RESET_LOOKED_UP(this_iface_ptr);
      PS_IFACEI_RESET_PROC_ID(this_iface_ptr);
      if ( this_iface_ptr->out_use_cmd_f_ptr != NULL )
      {
        (void) this_iface_ptr->out_use_cmd_f_ptr ( this_iface_ptr,
                                                   client_data_ptr );
        ret_val = 0;
      }
    }
  }
  else
  {
    /*-----------------------------------------------------------------------
      Return AEE_SUCCESS, but don't bring down IFACE, as there are still more
      clients interested in this iface.
    -----------------------------------------------------------------------*/
    LOG_MSG_INFO2("iface 0x%x:%d still in use, ref cnt %d",
            this_iface_ptr->name,
            this_iface_ptr->instance,
            PS_IFACE_GET_REF_CNT(this_iface_ptr));
    ret_val = 0;
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return ret_val;

} /* ps_iface_tear_down_cmd_ex() */


/*===========================================================================
FUNCTION PS_IFACE_ACTIVE_TEAR_DOWN_CMD_EX()

DESCRIPTION
  This function is used to stop an iface. It does not modify the ref count. It
  maybe called in cases other than the last app using the interface is done.

PARAMETERS
  this_iface_prtr: ptr to interface control block on which to operate on.
  ps_errno: error code to return to caller.
  client_data_ptr: data, that is passed to the client handler function.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_active_tear_down_cmd_ex
(
  ps_iface_type *this_iface_ptr,
  int32          app_priority,
  int16         *ps_errno,
  void          *client_data_ptr
)
{
  ps_iface_event_info_u_type  event_info;
  ps_timer_error_type         timer_ret_val;
  int                         ret_val = -1;
  int32                       delta = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL ps_errno", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  MSG_5(MSG_SSID_DS,
        MSG_LEGACY_HIGH,
        "Active tear down iface 0x%x:%d, state %d, priority- curr 0x%x, app %d",
        this_iface_ptr->name,
        this_iface_ptr->instance,
        PS_IFACEI_GET_STATE(this_iface_ptr),
        PS_IFACEI_GET_PRIORITY_MASK(this_iface_ptr),
        app_priority);

    switch(this_iface_ptr->iface_private.state)
    {
      case IFACE_DISABLED:
      case IFACE_DOWN:
        ret_val = 0;
        break;

      case IFACE_ROUTEABLE:
      case IFACE_UP:
        if (this_iface_ptr->iface_private.linger_timeout_val != 0 &&
            this_iface_ptr->linger_cmd_f_ptr != NULL)
        {
          /*---------------------------------------------------------------
            Calculate linger timeout using last IO
          -----------------------------------------------------------------*/
          delta = ((uint32)msclock()) - this_iface_ptr->last_io_time;

          this_iface_ptr->last_io_time = 0;

          if( delta > 0 &&
              this_iface_ptr->iface_private.linger_timeout_val > (uint32)delta)
          {
            event_info.state = this_iface_ptr->iface_private.state;

            this_iface_ptr->iface_private.state = IFACE_LINGERING;

            ps_ifacei_invoke_event_cbacks( this_iface_ptr,
                                            NULL,
                                            IFACE_LINGERING_EV,
                                            event_info);

            (void) this_iface_ptr->linger_cmd_f_ptr( this_iface_ptr,
                                                     client_data_ptr);

            timer_ret_val =
              ps_timer_start
              (
                this_iface_ptr->iface_private.iface_linger_timer,
                this_iface_ptr->iface_private.linger_timeout_val -
                  (uint32)delta
              );

            if(timer_ret_val != PS_TIMER_SUCCESS)
            {
              LOG_MSG_ERROR("Error startin timer 0x%x",
                this_iface_ptr->iface_private.iface_linger_timer, 0, 0);
              ASSERT(0);
            }

            ret_val = 0;
            break;
          }
          /*---------------------------------------------------------------
            else - immediately tear down the iface - goto fall through
          -----------------------------------------------------------------*/
        }
        /*lint -fallthrough*/

      case IFACE_CONFIGURING:
      case IFACE_COMING_UP:
      if( (this_iface_ptr->tear_down_cmd_f_ptr    == NULL) &&
          (this_iface_ptr->tear_down_cmd_f_ptr_ex == NULL) )
        {
          LOG_MSG_ERROR("No tear down command",0,0,0);
          ret_val = -1;
          *ps_errno = DS_EOPNOTSUPP;
          break;
        }
        event_info.state = this_iface_ptr->iface_private.state;
        this_iface_ptr->iface_private.state = IFACE_GOING_DOWN;
        ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                      NULL,
                                      IFACE_GOING_DOWN_EV,
                                      event_info);

      /* Use ex fptr if present, fallback fptr*/
      if ( this_iface_ptr->tear_down_cmd_f_ptr_ex != NULL )
      {
        (void) this_iface_ptr->tear_down_cmd_f_ptr_ex( this_iface_ptr,
                                                       app_priority,
                                                       client_data_ptr);
      }
      else if ( this_iface_ptr->tear_down_cmd_f_ptr != NULL )
      {
        (void) this_iface_ptr->tear_down_cmd_f_ptr(this_iface_ptr,
                                                   client_data_ptr);
      }

        *ps_errno = DS_EWOULDBLOCK;
        ret_val = -1;
        break;

      case IFACE_GOING_DOWN:
        *ps_errno = DS_EWOULDBLOCK;
        ret_val = -1;
        break;

      /*-----------------------------------------------------------------------
        Iface won't be in LINGERING state unless the last client called
        dsnet_stop() and after that tear_down_cmd() can't be called. Otherwise
        ref cnt will get messed up if tear_down_cmd() is called multiple times
        for the same client
      -----------------------------------------------------------------------*/
      case IFACE_LINGERING:
      default:
        ASSERT(0);
        *ps_errno = DS_EOPNOTSUPP;
        ret_val = -1;
        break;

    } /* switch(state) */

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return ret_val;

}  /* ps_iface_active_tear_down_cmd_ex */

/*===========================================================================
FUNCTION PS_IFACE_ACTIVE_TEAR_DOWN_CMD()

DESCRIPTION
  This function is used to stop an iface. It does not modify the ref count. It
  maybe called in cases other than the last app using the interface is done.

PARAMETERS
  this_iface_prtr: ptr to interface control block on which to operate on.
  ps_errno:        error code to return to caller.
  client_data_ptr: data, that is passed to the client handler function.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_active_tear_down_cmd
(
  ps_iface_type *this_iface_ptr,
  int16         *ps_errno,
  void          *client_data_ptr
)
{
  LOG_MSG_INFO2( "Iface active tear down cmd", 0, 0, 0 );
  return ps_iface_active_tear_down_cmd_ex( this_iface_ptr,
                                           PS_POLICY_MGR_LEGACY_PRIORITY,
                                           ps_errno,
                                           client_data_ptr );
} /* ps_iface_active_tear_down_cmd_ex() */

/*===========================================================================

FUNCTION PS_IFACE_TEAR_DOWN_CMD()

DESCRIPTION
  This function is used to stop an iface.  It

PARAMETERS
  this_iface_prtr: ptr to interface control block on which to operate on.
  app_priority:    application priority
  ps_errno: error code to return to caller.
  client_data_ptr: data, that is passed to the client handler function.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  Uses PS_POLICY_MGR_LEGACY_PRIORITY as app priority for tearing down iface

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_tear_down_cmd
(
  ps_iface_type *this_iface_ptr,
  int16         *ps_errno,
  void          *client_data_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_INFO2( "Iface tear down cmd", 0, 0, 0 );
  return ps_iface_tear_down_cmd_ex(this_iface_ptr,
                                   PS_POLICY_MGR_LEGACY_PRIORITY,
                                   ps_errno,
                                   client_data_ptr);
} /* ps_iface_tear_down_cmd() */

/*===========================================================================

FUNCTION PS_IFACE_ARBITRATION_TEAR_DOWN_CMD()

DESCRIPTION
  This function is used to stop an iface. It DOES reset ref count. It is
  called to teardown an Iface when arbitration is in progress. It does
  exactly what ps_iface_tear_down_cmd_ex() does but also resets the ref cnt.

PARAMETERS
  this_iface_prtr: ptr to interface control block on which to operate on.
  app_priority:    application priority
  ps_errno:        error code to return to caller.
  client_data_ptr: data, that is passed to the client handler function.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  Uses PS_POLICY_MGR_LEGACY_PRIORITY as app priority for tearing down iface

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_arbitration_tear_down_cmd
(
  ps_iface_type *this_iface_ptr,
  int16         *ps_errno,
  void          *client_data_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  int  ret_val = -1;
  int app_priority = PS_POLICY_MGR_LEGACY_PRIORITY;

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL ps_errno", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  if (app_priority <= PS_POLICY_MGR_PRIORITY_INVALID ||
      app_priority > PS_POLICY_MGR_PRIORITY_MAX)
  {
    LOG_MSG_ERROR("Invalid App priority", 0, 0, 0);
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  MSG_4(MSG_SSID_DS,
        MSG_LEGACY_HIGH,
        "Tearing down iface 0x%x:%d, old priority 0x%x, req app priority %d",
        this_iface_ptr->name,
        this_iface_ptr->instance,
        PS_IFACEI_GET_PRIORITY_MASK(this_iface_ptr),
        app_priority);

  /*-------------------------------------------------------------------------
    The ref cnt is forced to zero.
  -------------------------------------------------------------------------*/
  PS_IFACE_RESET_REF_CNT(this_iface_ptr);

  MSG_5(MSG_SSID_DS,
        MSG_LEGACY_HIGH,
        "Tearing down iface 0x%x:%d, state %d, ref_cnt %d, new priority 0x%x",
        this_iface_ptr->name,
        this_iface_ptr->instance,
        PS_IFACEI_GET_STATE(this_iface_ptr),
        PS_IFACE_GET_REF_CNT(this_iface_ptr),
        PS_IFACEI_GET_PRIORITY_MASK(this_iface_ptr));

  if(PS_IFACE_GET_REF_CNT(this_iface_ptr) == 0)
  {
    /*-----------------------------------------------------------------------
      take action based on the current state only when ref_cnt = 0 (meaning
      no clients interested in this IFACE).

      - For a non-active iface, actually tear down iface
      - For an active iface, do not actually tear down iface. Instead, call
        out_use_cback. (Interface handler should tear down the active iface
        explicitly.
    -----------------------------------------------------------------------*/
    if ( PS_IFACE_GET_IS_ACTIVE(this_iface_ptr) == FALSE )
    {
      ret_val = ps_iface_active_tear_down_cmd_ex ( this_iface_ptr,
                                                   app_priority,
                                                   ps_errno,
                                                   client_data_ptr );
    }
    else
    {
      PS_IFACEI_RESET_IN_USE(this_iface_ptr);
      PS_IFACEI_RESET_LOOKED_UP(this_iface_ptr);
      PS_IFACEI_RESET_PROC_ID(this_iface_ptr);
      if ( this_iface_ptr->out_use_cmd_f_ptr != NULL )
      {
        (void) this_iface_ptr->out_use_cmd_f_ptr ( this_iface_ptr,
                                                   client_data_ptr );
        ret_val = 0;
      }
    }
  }
  else
  {
    /*-----------------------------------------------------------------------
      Return SUCCESS, but don't bring down IFACE, as there are still more
      clients interested in this iface.
    -----------------------------------------------------------------------*/
    LOG_MSG_INFO2("iface 0x%x:%d still in use, ref cnt %d",
            this_iface_ptr->name,
            this_iface_ptr->instance,
            PS_IFACE_GET_REF_CNT(this_iface_ptr));
    ret_val = 0;
  }


  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return ret_val;

} /* ps_iface_arbitration_tear_down_cmd() */



/*===========================================================================
FUNCTION PS_IFACE_GO_NULL_CMD()

DESCRIPTION
  This function is used to stop an iface.

PARAMETERS
  this_iface_prtr: ptr to interface control block on which to operate on.
  ps_errno: error code to return to caller.
  client_data_ptr: data, that is passed to the client handler function.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_go_null_cmd
(
  ps_iface_type *this_iface_ptr,
  int16         *ps_errno,
  void          *client_data_ptr
)
{
  ps_iface_event_info_u_type event_info;
  int32                       timer_ret_val = 0;
  int ret_val = -1;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  LOG_MSG_INFO2("GO NULL if 0x%x:%d state %d",
          this_iface_ptr->name,
          this_iface_ptr->instance,
          this_iface_ptr->iface_private.state);

  /*-------------------------------------------------------------------------
    The ref cnt is forced to zero.
  -------------------------------------------------------------------------*/
  PS_IFACE_RESET_REF_CNT(this_iface_ptr);

  switch(this_iface_ptr->iface_private.state)
  {
    case IFACE_DOWN:
      ret_val = 0;
      break;

    case IFACE_ROUTEABLE:
    case IFACE_UP:
    case IFACE_CONFIGURING:
    case IFACE_COMING_UP:
    case IFACE_LINGERING:

      if(this_iface_ptr->iface_private.iface_linger_timer != 0 &&
         ps_timer_is_running(this_iface_ptr->iface_private.iface_linger_timer))
      {
        timer_ret_val=
          ps_timer_cancel(this_iface_ptr->iface_private.iface_linger_timer);
        if(timer_ret_val != PS_TIMER_SUCCESS)
        {
          LOG_MSG_ERROR("Error canceling timer 0x%x",
            this_iface_ptr->iface_private.iface_linger_timer, 0, 0);
          ASSERT(0);
        }
      }

      if( (this_iface_ptr->tear_down_cmd_f_ptr    == NULL) &&
          (this_iface_ptr->tear_down_cmd_f_ptr_ex == NULL) )
      {
        LOG_MSG_ERROR("No tear down command",0,0,0);
        ret_val = -1;
        *ps_errno = DS_EOPNOTSUPP;
        break;
      }
      event_info.state = this_iface_ptr->iface_private.state;
      this_iface_ptr->iface_private.state = IFACE_GOING_DOWN;
      ps_ifacei_invoke_event_cbacks(this_iface_ptr,
                                    NULL,
                                    IFACE_GOING_DOWN_EV,
                                    event_info);

      /* call registered tear down handlers */
      if ( this_iface_ptr->tear_down_cmd_f_ptr_ex != NULL )
      {
        (void) this_iface_ptr->tear_down_cmd_f_ptr_ex( this_iface_ptr,
                                                       PS_POLICY_MGR_LEGACY_PRIORITY,
                                                 client_data_ptr);
      }
      else if ( this_iface_ptr->tear_down_cmd_f_ptr != NULL )
      {
        (void) this_iface_ptr->tear_down_cmd_f_ptr(this_iface_ptr, client_data_ptr);
      }

      *ps_errno = DS_EWOULDBLOCK;
      ret_val = -1;
      break;

    case IFACE_GOING_DOWN:
      *ps_errno = DS_EWOULDBLOCK;
      ret_val = -1;
      break;

    default:
      ASSERT(0);
      *ps_errno = DS_EOPNOTSUPP;
      ret_val = -1;
      break;

    } /* switch(state) */

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return ret_val;

} /* ps_iface_go_null_cmd() */




/*===========================================================================
FUNCTION PS_IFACE_PHYS_LINK_UP_CMD()

DESCRIPTION
  This function is used to bring up the physical link on an iface, but it
  will only do so if the interface is not in the down or disabled state.

PARAMETERS
  this_iface_prtr: ptr to interface control block on which to operate on.
  ps_errno: error code to return to caller.
  client_data_ptr: data, that is passed to the client handler function.

RETURN VALUE
  0: on success
 -1: on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_phys_link_up_cmd
(
  const ps_iface_type *this_iface_ptr,
  int16               *ps_errno,
  void                *client_data_ptr
)
{
  ps_phys_link_type  *phys_link_ptr;
  int                 ret_val = -1;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If the interface is down or disabled this is not a meaningful command
  -------------------------------------------------------------------------*/
  if(PS_IFACEI_GET_STATE(this_iface_ptr) == IFACE_DOWN ||
     PS_IFACEI_GET_STATE(this_iface_ptr) == IFACE_DISABLED)
  {
    LOG_MSG_INFO1("iface 0x%x:%d in invalid state %d",
             this_iface_ptr->name,
             this_iface_ptr->instance,
             PS_IFACEI_GET_STATE(this_iface_ptr));
    *ps_errno = DS_EINVAL; // MVL: enotallowed; needs to be added?
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Call the underlying phys link's "up" command.
  -------------------------------------------------------------------------*/
  phys_link_ptr = PS_IFACEI_GET_PHYS_LINK(this_iface_ptr);

  ret_val = ps_phys_link_up_cmd(phys_link_ptr,
                                ps_errno,
                                client_data_ptr);


  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return ret_val;

} /* ps_iface_phys_link_up_cmd() */



/*===========================================================================
FUNCTION PS_IFACE_PHYS_LINK_DOWN_CMD()

DESCRIPTION
  This function is used to tear down the physical link of an iface, but it
  only does so if the interface is not down or disabled.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  ps_errno: error code to return to caller.
  client_data_ptr: data, that is passed to the client handler function.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_phys_link_down_cmd
(
  const ps_iface_type *this_iface_ptr,
  int16               *ps_errno,
  void                *client_data_ptr
)
{
  int ret_val = -1;
  ps_phys_link_type         *phys_link_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("NULL parameter is passed", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    *ps_errno = DS_EINVAL;
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    If the interface is down or disabled this is not a meaningful command
  -------------------------------------------------------------------------*/
  if(PS_IFACEI_GET_STATE(this_iface_ptr) == IFACE_DOWN ||
     PS_IFACEI_GET_STATE(this_iface_ptr) == IFACE_DISABLED)
  {
    LOG_MSG_ERROR("iface 0x%x:%d in invalid state %d to bring down",
              this_iface_ptr->name,
              this_iface_ptr->instance,
              PS_IFACEI_GET_STATE(this_iface_ptr));
    *ps_errno = DS_EINVAL; // MVL: enotallowed; needs to be added?
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    ASSERT(0);
    return -1;
  }

  /*-------------------------------------------------------------------------
    Call the underlying phys link's "down" command.
  -------------------------------------------------------------------------*/
  phys_link_ptr = PS_IFACEI_GET_PHYS_LINK(this_iface_ptr);

  ret_val = ps_phys_link_down_cmd(
                                   phys_link_ptr,
                                   ps_errno,
                                   client_data_ptr
                                 );

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return ret_val;

} /* ps_iface_phys_link_down_cmd() */



/*===========================================================================
FUNCTION PS_IFACE_SET_NET_PARAMS()

DESCRIPTION
  This function sets the authentication parameters to be used next time the
  interface is brought up.
  The stored parameters will be cleared when the ps_iface goes DOWN.

PARAMETERS
  iface_ptr               : iface where filters are installed
  alg_pref                : allowable authentication algorithms
  auth_creds_ptr          : pointer to memory holding username and password
  mode_spec_auth_info_ptr : pointer to additional authentication info
                            in a format understood by the interface owner

RETURN VALUE
   0 : on success
  -1 : on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_set_net_params
(
  ps_iface_type                   * iface_ptr,
  const network_params_info_type  * net_info_cfg_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  /*-------------------------------------------------------------------------
    Store PS Iface authentication information
  -------------------------------------------------------------------------*/
  if( !PS_IFACE_IS_VALID(iface_ptr) )
  {
    LOG_MSG_ERROR("Invalid parameters are passed, iface 0x%p", iface_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  memcpy( &(iface_ptr->net_cfg_params),
          net_info_cfg_ptr,
          sizeof(network_params_info_type) );

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  return 0;
} /* ps_iface_set_net_params() */



/*===========================================================================
FUNCTION PS_IFACE_SET_SIP_SERV_ADDR

DESCRIPTION
  This function sets the SIP server addresses on an iface. This function
  overwrites the previously stored SIP addresses, if any. Additionally if
  this funciton is called with count value of 0 the SIP server list
  is cleared.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  sip_serv_addr_ptr_array: pointer to the SIP server addr array.
  count : count of the SIP server addresses in the array.
  ps_errno : return error code.

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  This function overwrites the previously stored SIP addresses, if any.
===========================================================================*/
int ps_iface_set_sip_serv_addr
(
  ps_iface_type      *this_iface_ptr,
  ps_ip_addr_type    *sip_serv_addr_ptr_array,
  uint32              count,
  errno_enum_type    *ps_errno
)
{
  ps_iface_type                 *base_iface_ptr;
  ps_iface_sip_serv_addr_type   *sip_serv_addr;
  uint32                         i;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL || (sip_serv_addr_ptr_array == NULL && count != 0))
  {
    LOG_MSG_FATAL_ERROR("NULL parameters are passed, ps_errno, 0x%p, "
        "sip_serv_addr_ptr_array, 0x%p",
        ps_errno, sip_serv_addr_ptr_array, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", this_iface_ptr, 0, 0);
    *ps_errno = E_BAD_ADDRESS;
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    *ps_errno = E_BAD_ADDRESS;
    return -1;
  }

  /*-------------------------------------------------------------------------
    Free the previously stored list.
  -------------------------------------------------------------------------*/
  ps_iface_free_sip_addr_list(&base_iface_ptr->sip_serv_addr_list);

  /*-------------------------------------------------------------------------
    Traverse the array and store the SIP server addresses in the link list.
  -------------------------------------------------------------------------*/
  for (i = 0; i < count; i++)
  {
    sip_serv_addr = (ps_iface_sip_serv_addr_type *)
                       ps_mem_get_buf(PS_MEM_PS_SIP_SERV_ADDR_TYPE);
    if (sip_serv_addr == NULL)
    {
      ps_iface_free_sip_addr_list(&base_iface_ptr->sip_serv_addr_list);
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      LOG_MSG_ERROR("Couldn't create a buffer for SIP Server", 0, 0, 0);
      *ps_errno = E_NO_MEMORY;
      return -1;
    }

    if (ps_iface_get_addr_family(base_iface_ptr) !=
          sip_serv_addr_ptr_array[i].type)
    {
      ps_iface_free_sip_addr_list(&base_iface_ptr->sip_serv_addr_list);
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      LOG_MSG_ERROR("Incorrect IP family type in SIP address",0,0,0);
      *ps_errno = E_VERSION_MISMATCH;
      return -1;
    }

    memcpy(&sip_serv_addr->sip_addr,
           &sip_serv_addr_ptr_array[i],
           sizeof(ps_ip_addr_type));

    list_push_back(&base_iface_ptr->sip_serv_addr_list, &sip_serv_addr->link);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_set_sip_serv_addr() */



/*===========================================================================
FUNCTION PS_IFACE_GET_SIP_SERV_ADDR

DESCRIPTION
  This function returns the list of sip server addresses stored in the iface.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  sip_serv_addr_ptr_array: pointer to the SIP server addresses array.
  count : as input value it contains the count of the SIP server
          addresses asked for and as a return value it contains the
          actual number of addresses returned.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_get_sip_serv_addr
(
  ps_iface_type      *this_iface_ptr,
  ip_addr_type       *sip_serv_addr_ptr_array,
  uint32             *count
)
{
  ps_iface_type                 *base_iface_ptr;
  ps_iface_sip_serv_addr_type   *curr_sip_serv_addr;
  uint32                         temp_count = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (count == NULL || (sip_serv_addr_ptr_array == NULL && *count != 0))
  {
    LOG_MSG_ERROR("NULL parameters are passed, count, 0x%p, "
              "sip_serv_addr_ptr_array, 0x%p",
              count, sip_serv_addr_ptr_array, 0);
    ASSERT(0);
    return;
  }

  if (*count == 0)
  {
    LOG_MSG_INFO1("Count value of 0 passed", 0, 0, 0);
    return;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", this_iface_ptr, 0, 0);
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    return;
  }

  /*-------------------------------------------------------------------------
    Traverse the list and copy the SIP server addresses in the array.
  -------------------------------------------------------------------------*/
  curr_sip_serv_addr = list_peek_front(&base_iface_ptr->sip_serv_addr_list);
  while (temp_count < *count && curr_sip_serv_addr != NULL)
  {
    sip_serv_addr_ptr_array[temp_count].type =
      curr_sip_serv_addr->sip_addr.type;

    if (curr_sip_serv_addr->sip_addr.type == IPV4_ADDR)
    {
      sip_serv_addr_ptr_array[temp_count].addr.v4 =
        curr_sip_serv_addr->sip_addr.addr.v4.ps_s_addr;
    }
    else
    {
      sip_serv_addr_ptr_array[temp_count].addr.v6[0] =
        curr_sip_serv_addr->sip_addr.addr.v6.ps_s6_addr64[0];
      sip_serv_addr_ptr_array[temp_count].addr.v6[1] =
        curr_sip_serv_addr->sip_addr.addr.v6.ps_s6_addr64[1];
    }

    curr_sip_serv_addr = list_peek_next(&base_iface_ptr->sip_serv_addr_list,
                                        &curr_sip_serv_addr->link);
    temp_count++;
  }

  *count = temp_count;

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return;

} /* ps_iface_get_sip_serv_addr() */

/*===========================================================================
FUNCTION PS_IFACE_SET_SIP_DOMAIN_NAMES

DESCRIPTION
  This function sets the SIP server domain names on an iface. This function
  overwrites the previously stored names, if any. If this function is
  called with a count value of 0 the SIP domain name list is cleared.

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  sip_serv_domain_ptr_array: pointer to the SIP domain name array.
  count : count of the SIP server domain names in the array.
  ps_errno : return error code.

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  It is expected that the caller of this function would pass the domain
  names in the form of NULL terminated string.

SIDE EFFECTS
  This function overwrites the previously stored names, if any.
===========================================================================*/
int ps_iface_set_sip_domain_names
(
  ps_iface_type              *this_iface_ptr,
  ps_iface_domain_name_type  *sip_serv_domain_ptr_array,
  uint32                      count,
  errno_enum_type            *ps_errno
)
{
  ps_iface_type                        *base_iface_ptr;
  ps_iface_sip_serv_domain_name_type   *sip_serv_domain_name;
  uint32                                i;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL || (sip_serv_domain_ptr_array == NULL && count != 0))
  {
    LOG_MSG_ERROR("NULL parameters are passed, ps_errno, 0x%p, "
              "sip_serv_domain_ptr_array, 0x%p",
              ps_errno, sip_serv_domain_ptr_array, 0);
    ASSERT(0);
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", this_iface_ptr, 0, 0);
    *ps_errno = E_BAD_ADDRESS;
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    *ps_errno = E_BAD_ADDRESS;
    return -1;
  }

  /*-------------------------------------------------------------------------
    Free the previously stored list.
  -------------------------------------------------------------------------*/
  ps_iface_free_domain_name_list
  (
    &base_iface_ptr->sip_serv_domain_name_list
  );

  /*-------------------------------------------------------------------------
    Traverse the array and store the SIP serv domain names in the link list.
  -------------------------------------------------------------------------*/
  for (i = 0; i < count; i++)
  {
    sip_serv_domain_name =
      (ps_iface_sip_serv_domain_name_type *)
         ps_mem_get_buf(PS_MEM_PS_SIP_SERV_DOMAIN_NAME_TYPE);

    if (sip_serv_domain_name == NULL)
    {
      ps_iface_free_domain_name_list
      (
        &base_iface_ptr->sip_serv_domain_name_list
      );
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      LOG_MSG_ERROR("Couldn't create a buffer for SIP Domain", 0, 0, 0);
      *ps_errno = E_NO_MEMORY;
      return -1;
    }

    memcpy(&sip_serv_domain_name->name,
           &sip_serv_domain_ptr_array[i],
           sizeof(ps_iface_domain_name_type));

    list_push_back(&base_iface_ptr->sip_serv_domain_name_list,
                   &sip_serv_domain_name->link);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_set_sip_domain_names() */



/*===========================================================================
FUNCTION PS_IFACE_GET_SIP_DOMAIN_NAMES

DESCRIPTION

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  sip_serv_domain_ptr_array: pointer to an array of SIP domain names.
  count : count of the SIP server domain names in the array.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_get_sip_domain_names
(
  ps_iface_type              *this_iface_ptr,
  ps_iface_domain_name_type  *sip_serv_domain_ptr_array,
  uint32                     *count
)
{
  ps_iface_type                        *base_iface_ptr;
  ps_iface_sip_serv_domain_name_type   *curr_sip_serv_domain_name;
  uint32                                temp_count = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (count == NULL || (sip_serv_domain_ptr_array == NULL && *count != 0))
  {
    LOG_MSG_ERROR("NULL parameters are passed, count, 0x%p, "
              "sip_serv_domain_ptr_array, 0x%p",
              count, sip_serv_domain_ptr_array, 0);
    ASSERT(0);
    return;
  }

  if (*count == 0)
  {
    LOG_MSG_INFO1("Count value of 0 passed", 0, 0, 0);
    return;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", this_iface_ptr, 0, 0);
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    return;
  }

  /*-------------------------------------------------------------------------
    Traverse the list and store the SIP server addresses in the input array.
  -------------------------------------------------------------------------*/
  curr_sip_serv_domain_name =
    list_peek_front(&base_iface_ptr->sip_serv_domain_name_list);
  while (temp_count < *count && curr_sip_serv_domain_name != NULL)
  {
    memcpy(&sip_serv_domain_ptr_array[temp_count],
           &curr_sip_serv_domain_name->name,
           sizeof(ps_iface_domain_name_type));

    curr_sip_serv_domain_name =
      list_peek_next(&base_iface_ptr->sip_serv_domain_name_list,
                     &curr_sip_serv_domain_name->link);
    temp_count++;
  }

  *count = temp_count;

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return;

} /* ps_iface_get_sip_domain_names() */



/*===========================================================================
FUNCTION PS_IFACE_SET_DOMAIN_NAME_SEARCH_LIST

DESCRIPTION
  This function sets the domain name search list

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  domain_name_ptr_array: pointer to the domain name array.
  count : count of domain names in the domain name array.
  ps_errno : return error code.

RETURN VALUE
  DSS_SUCCESS if successful
  DSS_ERROR if fails

DEPENDENCIES

SIDE EFFECTS
  This function overwrites the previously stored names, if any.
===========================================================================*/
int ps_iface_set_domain_name_search_list
(
  ps_iface_type              *this_iface_ptr,
  ps_iface_domain_name_type  *domain_name_ptr_array,
  uint32                      count,
  errno_enum_type            *ps_errno
)
{
  ps_iface_type                         *base_iface_ptr;
  ps_iface_domain_name_search_list_node_type   *domain_name_search_list_node;
  uint32                                 i;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR("ps_errno is null",0,0,0);
    return -1;
  }

  if(domain_name_ptr_array == NULL && count != 0)
  {
    LOG_MSG_ERROR("NULL parameters are passed, ps_errno, 0x%p, "
              "domain_name_ptr_array, 0x%p",
              ps_errno, domain_name_ptr_array, 0);
    *ps_errno = E_INVALID_ARG;
    return -1;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", this_iface_ptr, 0, 0);
    *ps_errno = E_BAD_ADDRESS;
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    *ps_errno = E_BAD_ADDRESS;
    return -1;
  }

  /*-------------------------------------------------------------------------
    Free the previously stored list.
  -------------------------------------------------------------------------*/
  ps_iface_free_domain_name_list
  (
    &base_iface_ptr->domain_name_search_list
  );

  /*-------------------------------------------------------------------------
    Traverse the array and store the domain names in the link list.
  -------------------------------------------------------------------------*/
  for (i = 0; i < count; i++)
  {
    domain_name_search_list_node =
      (ps_iface_domain_name_search_list_node_type *)
         ps_mem_get_buf(PS_MEM_PS_DOMAIN_NAME_SEARCH_LIST_NODE_TYPE);

    if (domain_name_search_list_node == NULL)
    {
      ps_iface_free_domain_name_list
      (
        &base_iface_ptr->domain_name_search_list
      );
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      LOG_MSG_ERROR("Couldn't create a buffer for domain name list node",
          0, 0, 0);
      *ps_errno = E_NO_MEMORY;
      return -1;
    }

    memcpy(&domain_name_search_list_node->name,
           domain_name_ptr_array+i,
           sizeof(ps_iface_domain_name_type));

    list_push_back(&base_iface_ptr->domain_name_search_list,
                   &domain_name_search_list_node->link);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_set_domain_name_search_list() */



/*===========================================================================
FUNCTION PS_IFACE_GET_DOMAIN_NAME_SEARCH_LIST

DESCRIPTION

PARAMETERS
  this_iface_ptr: ptr to interface control block on which to operate on.
  domain_name_ptr_array: pointer to an array of domain names.
  count : count of the domain names in the array.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_get_domain_name_search_list
(
  ps_iface_type              *this_iface_ptr,
  ps_iface_domain_name_type  *domain_name_ptr_array,
  uint32                     *count
)
{
  ps_iface_type                        *base_iface_ptr;
  ps_iface_domain_name_search_list_node_type  *curr_domain_name;
  uint32                                temp_count = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (count == NULL || (domain_name_ptr_array == NULL && *count != 0))
  {
    LOG_MSG_ERROR("NULL parameters are passed, count, 0x%p, "
              "domain_name_ptr_array, 0x%p",
              count, domain_name_ptr_array, 0);
    ASSERT(0);
    return;
  }

  if (*count == 0)
  {
    LOG_MSG_INFO1("Count value of 0 passed", 0, 0, 0);
    return;
  }

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface, 0x%p, is passed", this_iface_ptr, 0, 0);
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    return;
  }

  /*-------------------------------------------------------------------------
    Traverse the list and store the domain names in the input array.
  -------------------------------------------------------------------------*/
  curr_domain_name =
    list_peek_front(&base_iface_ptr->domain_name_search_list);
  while (temp_count < *count && curr_domain_name != NULL)
  {
    memcpy(domain_name_ptr_array + temp_count,
           &curr_domain_name->name,
           sizeof(ps_iface_domain_name_type));

    curr_domain_name =
      list_peek_next(&base_iface_ptr->domain_name_search_list,
                     &curr_domain_name->link);
    temp_count++;
  }

  *count = temp_count;

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return;

} /* ps_iface_get_domain_name_search_list() */

/*===========================================================================
FUNCTION PS_IFACE_FREE_SIP_ADDR_LIST

DESCRIPTION
  This function frees the SIP server address list.

PARAMETERS
  sip_addr_list : pointer to the link list containing the SIP addresses.

RETURN VALUE

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_free_sip_addr_list
(
  list_type         *sip_addr_list
)
{
  ps_iface_sip_serv_addr_type   *curr_sip_serv_addr;
  ps_iface_sip_serv_addr_type   *next_sip_serv_addr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
   Free the list.
  -------------------------------------------------------------------------*/
  curr_sip_serv_addr = list_peek_front(sip_addr_list);
  while (curr_sip_serv_addr != NULL)
  {
    next_sip_serv_addr = list_peek_next(sip_addr_list,
                                        &curr_sip_serv_addr->link);

    list_pop_item(sip_addr_list, &(curr_sip_serv_addr->link));
    PS_MEM_FREE(curr_sip_serv_addr);

    curr_sip_serv_addr = next_sip_serv_addr;
  } /* while SIP servers in the list exist */

  return;
} /*ps_iface_free_sip_addr_list*/



/*===========================================================================
FUNCTION PS_IFACE_FREE_DOMAIN_NAME_LIST

DESCRIPTION
  This function frees the domain names list. Can be used for freeing
  SIP serv domain name list or DNS domain name search list.

PARAMETERS
  domain_name_list : pointer to the link list containing the domain names.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void ps_iface_free_domain_name_list
(
  list_type         *domain_name_list
)
{
  ps_iface_domain_name_node_type   *curr_domain_name;
  ps_iface_domain_name_node_type   *next_domain_name;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
   Free the list.
  -------------------------------------------------------------------------*/
  curr_domain_name = list_peek_front(domain_name_list);
  while (curr_domain_name != NULL)
  {
    next_domain_name =
      list_peek_next(domain_name_list,
                    &curr_domain_name->link);

    list_pop_item(domain_name_list, &(curr_domain_name->link));
    PS_MEM_FREE(curr_domain_name);

    curr_domain_name = next_domain_name;
  } /* while domain names in the list exist */

  return;
} /* ps_iface_free_domain_name_list() */

/*===========================================================================
FUNCTION PS_IFACE_SET_DHCP_CLIENT_HANDLE()

DESCRIPTION
  This function is used to set the dhcp_client_handle in ps_iface. NULL is
  passed to reset the handle.

PARAMETERS
  this_iface_ptr : ptr to interface control block on which to operate on.
  dhcp_client_handle : handle to set

RETURN VALUE
  0 if sucessful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_set_dhcp_client_handle
(
  ps_iface_type  *this_iface_ptr,
  void           *dhcp_client_handle
)
{
  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return -1;
  }
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  this_iface_ptr->iface_private.dhcp_client_handle = dhcp_client_handle;
  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;
} /* ps_iface_set_dhcp6_client_handle */

/*===========================================================================
FUNCTION PS_IFACE_GET_DHCP_CLIENT_HANDLE()

DESCRIPTION
  This function is used to get the dhcp_client_handle in ps_iface.

PARAMETERS
  this_iface_ptr : ptr to interface control block on which to operate on.

RETURN VALUE
  The dhcp client handle, or NULL if the iface is invalid.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void * ps_iface_get_dhcp_client_handle
(
  ps_iface_type  *this_iface_ptr
)
{
  void * dhcp_client_handle = NULL;
  ps_iface_type * base_iface_ptr;

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    ASSERT(0);
    return NULL;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*------------------------------------------------------------------------
    If iface has inherit IP info true then get the base iface of the logical
    chain, else do nothing.
  -------------------------------------------------------------------------*/
  base_iface_ptr = PS_IFACE_GET_BASE_IFACE(this_iface_ptr);

  /*------------------------------------------------------------------------
    if PS_IFACE_GET_BASE_IFACE returned iface_ptr with inhertied flag set,
    then it indicates error.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IP_INFO_IS_INHERITED(base_iface_ptr))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Invalid associated iface 0x%p", base_iface_ptr, 0, 0);
    return NULL;
  }

  dhcp_client_handle = base_iface_ptr->iface_private.dhcp_client_handle;

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  return dhcp_client_handle;
} /* ps_iface_get_dhcp_client_handle */


/*===========================================================================
FUNCTION PS_IFACE_ENABLE_IFACE_LINGER()

DESCRIPTION
  This function enables linger on the interface.

PARAMETERS
  this_iface_ptr     : ptr to interface control block on which to operate on.

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_iface_enable_iface_linger
(
  ps_iface_type  * this_iface_ptr
)
{

  ps_timer_handle_type         timer_handle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID( this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", this_iface_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

 /*-------------------------------------------------------------------------
    Allocate the iface linger timer
  -------------------------------------------------------------------------*/
  timer_handle = ps_timer_alloc( ps_iface_linger_timeout_cb, this_iface_ptr);
  if (timer_handle == PS_TIMER_INVALID_HANDLE)
  {
    LOG_MSG_ERROR("Failed to allocate linger timer, iface 0x%p",
               this_iface_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  this_iface_ptr->iface_private.iface_linger_timer = timer_handle;

  return 0;

} /* ps_iface_enable_iface_linger() */

/*===========================================================================
FUNCTION PS_IFACE_SET_LINGER_TIMEOUT_VAL()

DESCRIPTION
  This function sets the value for linger timer

PARAMETERS
  this_iface_ptr     : ptr to interface control block on which to operate on.
  linger_timeout_val : timeout value

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_iface_set_linger_timeout_val
(
  ps_iface_type  * this_iface_ptr,
  const uint32     linger_timeout_val
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID( this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", this_iface_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  if (this_iface_ptr->iface_private.iface_linger_timer !=
        PS_TIMER_INVALID_HANDLE)
  {
    this_iface_ptr->iface_private.linger_timeout_val = linger_timeout_val;
  }
  else
  {
    LOG_MSG_ERROR("IFACE linger is not enabled 0x%p", this_iface_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  return 0;

} /* ps_iface_set_linger_timeout_val() */



/*===========================================================================
FUNCTION PS_IFACE_GET_LINGER_TIMEOUT_VAL()

DESCRIPTION
  This function fetches the value for linger timer

PARAMETERS
  this_iface_ptr : ptr to interface control block on which to operate on.

RETURN VALUE
  linger timeout value if successful
  0 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
uint32 ps_iface_get_linger_timeout_val
(
  ps_iface_type  * this_iface_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID( this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", this_iface_ptr, 0, 0);
    ASSERT(0);
    return 0;
  }

  return this_iface_ptr->iface_private.linger_timeout_val;

} /* ps_iface_get_linger_timeout_val() */

/*===========================================================================
FUNCTION PS_IFACE_INC_SOCK_REF_CNT()

DESCRIPTION
  This function increments the interface sockets reference count

PARAMETERS
  iface_ptr: pointer to the interface in question.

RETURN VALUE
  None
===========================================================================*/
void ps_ifacei_inc_sock_ref_cnt
(
  ps_iface_type *iface_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (PS_IFACE_IS_VALID(iface_ptr))
  {
    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    iface_ptr->iface_private.sock_ref_cnt++;

    PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  }
}/* ps_ifacei_inc_sock_ref_cnt() */

/*===========================================================================
FUNCTION PS_IFACEI_DEC_SOCK_REF_CNT()

DESCRIPTION
  This function decrements the interface sockets reference count
  In case it reaches zero value IFACE_IDLE_EV event is fired

PARAMETERS
  iface_ptr: pointer to the interface in question.

RETURN VALUE
  None
===========================================================================*/
void ps_ifacei_dec_sock_ref_cnt
(
  ps_iface_type * iface_ptr
)
{
  ps_iface_event_info_u_type event_info;

/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  memset((void* )(&event_info), 0, sizeof(ps_iface_event_info_u_type));

  if (!PS_IFACE_IS_VALID( iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", iface_ptr, 0, 0);
    ASSERT(0);
    return;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  if(iface_ptr->iface_private.sock_ref_cnt > 0)
  {
    iface_ptr->iface_private.sock_ref_cnt--;

    if(0 == iface_ptr->iface_private.sock_ref_cnt)
    {
      ps_ifacei_invoke_event_cbacks(iface_ptr,
                                    NULL,
                                    IFACE_IDLE_EV,
                                    event_info);
    }
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
}/* ps_ifacei_dec_sock_ref_cnt() */

#ifdef FEATURE_DATA_PS_MCAST
/*===========================================================================
FUNCTION PS_IFACE_MCAST_GET_GROUP_LIST

DESCRIPTION
  This function gets pointer to pointer to first group in the list of
  multicast groups associated with the given interface.

PARAMETERS
  this_iface_ptr : ptr to interface control block on which to operate on.

RETURN VALUE
  double pointer to first group in the list on success and NULL on failure

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
struct ps_mcast_cb ** ps_iface_mcast_get_group_list
(
  ps_iface_type  * this_iface_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", this_iface_ptr, 0, 0);
    ASSERT(0);
    return NULL;
  }

  return (&(this_iface_ptr->iface_private.if_mcast_grps));
} /* ps_iface_mcast_get_group_list() */


/*===========================================================================
FUNCTION PS_IFACE_MCAST_GET_QUERIER_V1_TIMER

DESCRIPTION
  This function gets the current value of the querier V1 timer associated
  with the interface.

PARAMETERS
  this_iface_ptr : ptr to interface control block on which to operate on.

RETURN VALUE
  Value of the querier V1 timer

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
ps_timer_handle_type ps_iface_mcast_get_querier_v1_timer
(
  ps_iface_type  * this_iface_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", this_iface_ptr, 0, 0);
    ASSERT(0);
    return 0;
  }

  return (this_iface_ptr->iface_private.querier_v1_timer);
} /* ps_iface_mcast_get_querier_v1_timer() */


/*===========================================================================
FUNCTION PS_IFACE_ENABLE_MULTICAST

DESCRIPTION
  This function allocates the querier V1 timer associated
  with the interface.

PARAMETERS
  this_iface_ptr : ptr to interface control block on which to operate on.

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_enable_multicast
(
  ps_iface_type  * this_iface_ptr
)
{
  ps_timer_handle_type timer_handle;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", this_iface_ptr, 0, 0);
    ASSERT(0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    Allocating the timer for the interface.
  -------------------------------------------------------------------------*/
  timer_handle = ps_timer_alloc(ps_mcast_querier_v1_cb, NULL);
  if (timer_handle == PS_TIMER_INVALID_HANDLE)
  {
    LOG_MSG_ERROR("Error allocating timer", 0, 0, 0);
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return -1;
  }
  else
  {
    this_iface_ptr->iface_private.querier_v1_timer = timer_handle;
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;
} /* ps_iface_enable_multicast() */


/*===========================================================================
FUNCTION PS_IFACE_DISABLE_MULTICAST

DESCRIPTION
  This function cancels and frees the querier V1 timer associated with the
  interface. This function also leaves all the multicast groups that the
  interface is a member of.

PARAMETERS
  this_iface_ptr : ptr to interface control block on which to operate on.

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_disable_multicast
(
  ps_iface_type  * this_iface_ptr
)
{
  int32                timer_ret_val = 0;
  struct ps_mcast_cb   *cur_grp_ptr;
  ps_ip_addr_type       mcast_grp;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(this_iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", this_iface_ptr, 0, 0);
    return -1;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);
  if (ps_timer_is_running(this_iface_ptr->iface_private.querier_v1_timer))
  {
    timer_ret_val =
      ps_timer_cancel(this_iface_ptr->iface_private.querier_v1_timer);
    if (timer_ret_val != PS_TIMER_SUCCESS)
    {
      LOG_MSG_ERROR("Error cancelling the timer", 0, 0, 0);
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;
    }
  }

  if (this_iface_ptr->iface_private.querier_v1_timer != 0)
  {
    timer_ret_val = ps_timer_free(this_iface_ptr->iface_private.
                                  querier_v1_timer);
    if (timer_ret_val != PS_TIMER_SUCCESS)
    {
      LOG_MSG_ERROR("Error freeing the timer", 0, 0, 0);
      PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      return -1;
    }
  }

  cur_grp_ptr = this_iface_ptr->iface_private.if_mcast_grps;
  while (cur_grp_ptr != NULL)
  {
    cur_grp_ptr->membership_count = 1;
    mcast_grp = cur_grp_ptr->mcast_grp;
    cur_grp_ptr = cur_grp_ptr->next_mcast_grp;
    ps_mcast_delete_group(this_iface_ptr, mcast_grp);
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return 0;

} /* ps_iface_disable_multicast() */
#endif /* FEATURE_DATA_PS_MCAST */

#ifdef FEATURE_DATA_RM_NET
/*===========================================================================
  FUNCTION PS_IFACE_FILL_NEXT_HOP_ADDR()

DESCRIPTION
  This function fills next hop address from either Pkt info or from iface
  pointer. Mainly used by Broadcast scenarios[Currently RmNet!]

  This function is introduced to avoid duplication in IFACE, IP Frag layers

DEPENDENCIES
  A valid Metainfo must be passed in which contain pkt info.
  A valid iface pointer must be passed in used in non-mcast scenarios.

PARAMETERS
  iface_ptr   - Iface pointer
  mi_ref_ptr  - Reference to meta info block
  errno       - Error value returned by the function

RETURN VALUE
  0 for successful processing of the pkt, -1 in case of error

  Currently this function is used only for RmNet cases - can be opened for
  all - if required!
===========================================================================*/
int ps_iface_fill_next_hop_addr
(
  ps_iface_type                        *iface_ptr,
  ps_tx_meta_info_type                 *tx_meta_info_ptr,
  errno_enum_type                      *ps_errno
)
{
  ip_pkt_info_type   *ip_pkt_info_ptr = NULL;
  ps_ip_addr_type     next_hop_addr;

  /* sanity checks! */
  if ((tx_meta_info_ptr == NULL) ||
       (PS_TX_META_GET_RT_META_INFO_PTR(tx_meta_info_ptr) == NULL) ||
       (iface_ptr == NULL))
  {
    LOG_MSG_ERROR("fill_next_hop_addr: sanity failed!", 0, 0, 0);
    *ps_errno = E_INVALID_ARG;
    return -1;
  }

  ip_pkt_info_ptr = &PS_TX_META_GET_PKT_INFO(tx_meta_info_ptr);

  /* Fill next hop addr */
  switch(ip_pkt_info_ptr->ip_vsn)
  {
    case IP_V4:
      next_hop_addr.type    = IPV4_ADDR;
#ifdef FEATURE_DATA_QMI_MCAST
      if(PS_IN_IS_ADDR_MULTICAST(ip_pkt_info_ptr->ip_hdr.v4.dest.ps_s_addr))
      {
        PS_TX_META_SET_IP_ADDR_SCOPE( tx_meta_info_ptr, IP_ADDR_MULTICAST);
        next_hop_addr.addr.v4 = ip_pkt_info_ptr->ip_hdr.v4.dest;
      }
      else
      {
        PS_TX_META_SET_IP_ADDR_SCOPE( tx_meta_info_ptr, IP_ADDR_UNICAST);
        next_hop_addr.addr.v4 = iface_ptr->v4_net_info.gateway;
      }
#else
      next_hop_addr.addr.v4 = iface_ptr->v4_net_info.gateway;
      PS_TX_META_SET_IP_ADDR_SCOPE( tx_meta_info_ptr, IP_ADDR_UNICAST);
#endif /* FEATURE_DATA_QMI_MCAST */
      break;

    case IP_V6:
      next_hop_addr.type    = IPV6_ADDR;
      /*-------------------------------------------------------------------
        If multicast set the next hop address appropriately.
      -------------------------------------------------------------------*/
      if(PS_IN6_IS_ADDR_MULTICAST(&ip_pkt_info_ptr->ip_hdr.v6.hdr_body.base_hdr.
                                     dst_addr))
      {
        next_hop_addr.addr.v6 =
                       ip_pkt_info_ptr->ip_hdr.v6.hdr_body.base_hdr.dst_addr;
        PS_TX_META_SET_IP_ADDR_SCOPE( tx_meta_info_ptr, IP_ADDR_MULTICAST);
      }
      else
      {
        PS_TX_META_SET_IP_ADDR_SCOPE( tx_meta_info_ptr, IP_ADDR_UNICAST);
        /*---------------------------------------------------------------
          Same subnet - next hop is host itself
        ---------------------------------------------------------------*/
        next_hop_addr.addr.v6 =
            ip_pkt_info_ptr->ip_hdr.v6.hdr_body.base_hdr.dst_addr;
      }
      break;

      default:
      LOG_MSG_ERROR("Invalid IP type %d", ip_pkt_info_ptr->ip_vsn, 0, 0);
      /* This case doesn't arise(handled inside ps_ip_pkt_info_generate)
      ...But, just to satisfy Lint! */
      *ps_errno = E_VERSION_MISMATCH;
      return -1;
    }
    PS_TX_META_SET_NEXT_HOP_ADDR( tx_meta_info_ptr, next_hop_addr);
    return 0;
  } /* ps_iface_fill_next_hop_addr */
#endif /* FEATURE_DATA_RM_NET */

/*===========================================================================
FUNCTION PS_IFACE_ENABLE_FIREWALL

DESCRIPTION
  This function enables the firewall on the interface.

PARAMETERS
  iface_ptr : ptr to interface control block on which to operate on.
  is_allowed: To allow/deny firewall rules

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_enable_firewall
(
  ps_iface_type            *iface_ptr,
  boolean                  is_allowed
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", iface_ptr, 0, 0);
    ASSERT(0);
    return FALSE;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  iface_ptr->iface_private.is_firewall_enabled = TRUE;
  iface_ptr->iface_private.is_pkt_allowed = is_allowed;

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return TRUE;
} /* ps_iface_enable_firewall() */

/*===========================================================================
FUNCTION PS_IFACE_ENABLE_FIREWALL

DESCRIPTION
  This function disables the firewall on the interface.

PARAMETERS
  iface_ptr : ptr to interface control block on which to operate on.

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_disable_firewall
(
  ps_iface_type            *iface_ptr
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", iface_ptr, 0, 0);
    ASSERT(0);
    return FALSE;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  iface_ptr->iface_private.is_firewall_enabled = FALSE;

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return TRUE;
} /* ps_iface_disable_firewall() */


/*===========================================================================
FUNCTION PS_IFACE_DELETE_FIREWALL_RULE

DESCRIPTION
  This function deletes the firewall rule on the interface.

PARAMETERS
  iface_ptr : ptr to interface control block on which to operate on.
  handle : Handle to the firewall rule

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_delete_firewall_rule
(
  ps_iface_type            *iface_ptr,
  uint32                   handle
)
{
  int16     ps_errno;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", iface_ptr, 0, 0);
    ASSERT(0);
    return FALSE;
  }

  if(-1 == ps_iface_ipfltr_delete(iface_ptr,
                                  IP_FLTR_CLIENT_FIREWALL_INPUT,
                                  handle,
                                  &ps_errno))
  {
    LOG_MSG_ERROR("Error deleting firewall rule", 0, 0, 0);
    return FALSE;
  }

  return TRUE;
} /* ps_iface_add_firewall_rule() */

/*===========================================================================
FUNCTION PS_IFACE_GET_FIREWALL_RULE

DESCRIPTION
  This function retrieves the firewall rule on the interface based on
  the handle provided

PARAMETERS
  iface_ptr : ptr to interface control block on which to operate on.
  fltr_spec : Filter spec
  handle : Handle to the firewall rule

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_get_firewall_rule
(
  ps_iface_type            *iface_ptr,
  ip_filter_type           *fltr_spec,
  uint32                   handle
)
{
  ps_iface_ipfilteri_type      * fltr_buf;
  q_type                       * fltr_q_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", iface_ptr, 0, 0);
    ASSERT(0);
    return FALSE;
  }

  if (NULL == fltr_spec)
  {
    LOG_MSG_ERROR("NULL fltr spec", 0, 0, 0);
    return FALSE;
  }

  if (PS_IFACE_IPFLTR_INVALID_HANDLE == handle)
  {
    LOG_MSG_ERROR("Invalid handle passed", 0, 0, 0);
    fltr_spec = NULL;
    return FALSE;
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    Make sure that the passed handle belongs to ps_mem pool from which
    fltrs are allocated
  -------------------------------------------------------------------------*/
  fltr_buf = (ps_iface_ipfilteri_type *) handle;

  if (!ps_mem_is_valid(fltr_buf, PS_MEM_PS_IFACE_IPFLTER_TYPE))
  {
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    LOG_MSG_ERROR("Buffer (0x%X) doesn't belong to filter's mem pool",
              handle, 0, 0);
    ASSERT(0);
    PS_BRANCH_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return FALSE;
  }

  fltr_q_ptr =
    &(iface_ptr->iface_private.ipfltr_info[IP_FLTR_CLIENT_FIREWALL_INPUT]);

  while (fltr_buf != NULL)
  {
    if (fltr_buf->fi_handle == handle)
    {
      memcpy((void *)fltr_spec, (const void *)&fltr_buf->filter.fi_default, sizeof (ip_filter_type));
    }
    fltr_buf = q_next(fltr_q_ptr, &(fltr_buf->link));
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  return TRUE;
} /* ps_iface_get_firewall_rule() */

/*===========================================================================
FUNCTION PS_IFACE_GET_FIREWALL_TABLE

DESCRIPTION
  This function retrieves all the firewall rule on the interface

PARAMETERS
  iface_ptr : ptr to interface control block on which to operate on.
  fltr_spec : Filter spec
  num_fltrs: Number of filters to be retrived
  avail_num_fltrs : Actual no of filters installed
  handle : Handles to the firewall rules


RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean ps_iface_get_firewall_table
(
  ps_iface_type            *iface_ptr,
  ip_filter_type           *fltr_spec,
  uint8                    num_fltrs,
  uint8                    *avail_num_fltrs,
  uint32                   *handle
)
{
  ps_iface_ipfilteri_type  * filter_buf_ptr;
  ps_iface_ipfilteri_type  * next_filter_buf_ptr;
  q_type                   * ipfltr_q_ptr;
  uint8                    index = 0;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (!PS_IFACE_IS_VALID(iface_ptr))
  {
    LOG_MSG_ERROR("Invalid iface 0x%p", iface_ptr, 0, 0);
    ASSERT(0);
    return FALSE;
  }

  if (num_fltrs > 0)
  {
    ASSERT(fltr_spec != NULL);
  }

  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  ipfltr_q_ptr = &(iface_ptr->iface_private.ipfltr_info[IP_FLTR_CLIENT_FIREWALL_INPUT]);

  filter_buf_ptr = q_check(ipfltr_q_ptr);

  *avail_num_fltrs = (uint8)q_cnt(ipfltr_q_ptr);

  while (filter_buf_ptr != NULL && num_fltrs > 0)
  {
    memcpy((void *)&(fltr_spec[index]),
           (const void *)&(filter_buf_ptr->filter.fi_default), sizeof (ip_filter_type));
    handle[index] = filter_buf_ptr->fi_handle;
    index++;
    next_filter_buf_ptr = q_next(ipfltr_q_ptr, &(filter_buf_ptr->link));
    filter_buf_ptr = next_filter_buf_ptr;
    num_fltrs--;
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  return TRUE;
} /* ps_iface_get_firewall_table() */

/*===========================================================================
FUNCTION PS_IFACE_GET_ROOT_IFACE

DESCRIPTION
  Find the root/logical Iface for a given array of Ifaces

PARAMETERS
  logical_iface_arr_ptr : output array of Ifaces
  num_logical_iface     : num of Ifaces returned
  iface_arr_ptr         : input array of Ifaces
  num_iface             : num of input Ifaces

RETURN VALUE
  0 if successful
 -1 if fails

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int ps_iface_get_root_iface
(
  ps_iface_type **logical_iface_arr_ptr,
  uint8          *num_logical_iface,
  ps_iface_type **iface_arr_ptr,
  uint8           num_iface
)
{

  ps_iface_type *phys_if_ptr, *dwnstream_assoc_if_ptr, *tmp_if_ptr;
  ps_iface_type *logical_if_ptr = NULL;
  uint32         i = 0;
  int32          loop = 0;
  uint8          num = 0;
  int            ret = -1; /* set if any logical Iface is found */

  ASSERT(global_iface_ptr_array_index <= MAX_SYSTEM_IFACES);

  /* For every Iface provided, find corresponding logical Iface */
  for ( i = 0; i < num_iface; i++ )
  {
    phys_if_ptr    = iface_arr_ptr[i];
    logical_if_ptr = NULL;

    /* Iface sanity */
    if( phys_if_ptr == NULL ||
        phys_if_ptr->iface_private.this_iface != phys_if_ptr )
    {
      ASSERT(0);
      return -1;
    }

    LOG_MSG_INFO2( "find logical for phys Iface [0x%x:%x]",
                   phys_if_ptr->name, phys_if_ptr->instance, 0 );

    /* loop thru to find max depth of */
    for(loop = global_iface_ptr_array_index - 1; loop >= 0; loop--)
    {
      tmp_if_ptr = global_iface_ptr_array[loop];
      dwnstream_assoc_if_ptr = PS_IFACE_GET_ASSOC_IFACE( tmp_if_ptr );

      if ( ( dwnstream_assoc_if_ptr != NULL ) &&
           ( dwnstream_assoc_if_ptr == phys_if_ptr ) )
      {
        logical_if_ptr = tmp_if_ptr;
        LOG_MSG_INFO2( "found logical for phys Iface [0x%x:%x], arr idx [%d]",
                       phys_if_ptr->name, phys_if_ptr->instance, i );
        ret = 0;
      }
    }/* loop thru all entries in global iface array */
    logical_iface_arr_ptr[i] = logical_if_ptr;
    num++;
  } /* loop thru all phys Ifaces */

  *num_logical_iface = num;
  return ret;
}/* ps_iface_get_root_iface() */


/*===========================================================================
FUNCTION PS_IFACE_GET_APP_PRIORITY

DESCRIPTION
  Returns the first non-zero app priority field stored in an iface.

PARAMETERS
  iface_ptr - Pointer to ps iface control block.

RETURN VALUE
  app_priority stored in physical ifaces. In case of error, -1 is returned.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
int32 ps_iface_get_app_priority
(
  ps_iface_type    *iface_ptr
)
{
  int32  index;
  int32  app_priority = PS_POLICY_MGR_PRIORITY_INVALID;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  LOG_MSG_FUNCTION_ENTRY("ps_iface_get_app_priority: Iface 0x%p",
                         iface_ptr, 0, 0);

  do
  {
    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    if (!PS_IFACE_IS_VALID(iface_ptr))
    {
      LOG_MSG_ERROR("ps_iface_get_app_priority: Invalid iface 0x%x",
                    iface_ptr, 0, 0);
      break;
    }

    for (index = PS_POLICY_MGR_IFACE_PRIORITY_MAX - 1; index >= 0; index--)
    {
      if (0 != iface_ptr->iface_private.ref_cnt[index])
      {
        app_priority = index;
        break;
      }
    }

    if (0 > index)
    {
      LOG_MSG_INFO1("ps_iface_get_app_priority: ref cnt is 0 for iface 0x%x:%d",
                    iface_ptr->name, iface_ptr->instance, 0);
      break;
    }

    LOG_MSG_FUNCTION_EXIT("ps_iface_get_app_priority: App priority for "
                            "iface 0x%x:%d is %d",
                          iface_ptr->name, iface_ptr->instance, app_priority);

    PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
    return app_priority;

  } while (0);

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  LOG_MSG_FUNCTION_EXIT("ps_iface_get_app_priority: Fail, returning -1 for "
                          "iface 0x%p", iface_ptr, 0, 0);
  return -1;

} /* ps_iface_get_app_priority() */

/*===========================================================================
FUNCTION PS_IFACE_GET_MTU()

DESCRIPTION
  This functions gets the MTU of the iface

PARAMETERS
  iface_ptr : Target iface ptr

RETURN VALUE
  mtu : MTU of the Base Iface.
  0   : if Iface is invalid

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
uint16 ps_iface_get_mtu
(
  ps_iface_type        *iface_ptr
)
{
  uint16 mtu = 0;
  ps_iface_type *base_iface_ptr = NULL;

  if ( PS_IFACE_IS_VALID(iface_ptr) )
  {

    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    /*------------------------------------------------------------------------
      If iface has inherit IP info true then get the base iface of the logical
      chain, else do nothing.
    ------------------------------------------------------------------------*/
    base_iface_ptr = PS_IFACE_GET_BASE_IFACE(iface_ptr);

    /* Get the mtu value */
    mtu = base_iface_ptr->net_info.mtu;

    PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  }
  return mtu;
} /* ps_iface_get_mtu() */

void ps_iface_set_mtu
(
  ps_iface_type  * this_iface_ptr,
  uint16           new_mtu
)
{
  uint16  mtu_to_set;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  do
  {
    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    if (!PS_IFACE_IS_VALID(this_iface_ptr))
    {
      LOG_MSG_ERROR("ps_iface_set_mtu(): Invalid iface 0x%x",
                    this_iface_ptr, 0, 0);
      break;
    }

    /*-----------------------------------------------------------------------
      Iface's address family can be V4 or V6 or UNSPEC. if it is V4 or V6
      compare against MIN_MTU for V4 and V6 respectively. If it is UNSPEC,
      iface supports both V4 and V6 and since MTU must satify constrains for
      both V4 and V6 and since IN_MTU for V6 is higher than MIN_MTU for V4,
      compare it again MIN_V6_MTU
    -----------------------------------------------------------------------*/
    if (ps_iface_addr_family_is_v4(this_iface_ptr))
    {
      mtu_to_set = MAX(new_mtu, PS_IFACE_MIN_V4_MTU);
    }
    else
    {
      mtu_to_set = MAX(new_mtu, PS_IFACE_MIN_V6_MTU);
    }

    this_iface_ptr->net_info.mtu = mtu_to_set;
    LOG_MSG_INFO1_6("ps_iface_set_mtu(): Set MTU to %d on iface 0x%x:%d, "
                      "client passed %d as MTU",
                    mtu_to_set,
                    this_iface_ptr->name,
                    this_iface_ptr->instance,
                    new_mtu, 0, 0);
  } while (0);

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
  return;

} /* ps_iface_set_mtu() */

#endif /* FEATURE_DATA_PS */
