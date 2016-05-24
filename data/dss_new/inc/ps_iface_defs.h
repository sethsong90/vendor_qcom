#ifndef PS_IFACE_DEFS_H
#define PS_IFACE_DEFS_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                        P S _ I F A C E _ D E F S . H

DESCRIPTION
  Header containing PS_IFACE names that need to be used in other contexts,
  but that don't required the entire ps_iface header.

Copyright (c) 2002-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                            EDIT HISTORY FOR FILE

  $PVCSPath: L:/src/asw/MM_DATA/vcs/ps_iface_defs.h_v   1.4   07 Feb 2003 20:12:48   ubabbar  $
  $Header: //source/qcom/qct/modem/api/datamodem/main/latest/ps_iface_defs.h#6 $ : //source/qcom/qct/modem/api/datacommon/main/latest/ps_iface_defs.h#41 $ $DateTime: 2011/08/04 11:10:56 $ $Author: anupamad $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/03/11    mct    Added new event for reporting fast dormancy status to apps.
07/15/11    msr    Defined minimum MTU values for v4 and v6 ifaces
06/06/2011  sid    Added iface teardown reason enum to support
                   PPP partial context feature.
05/09/10    asn    Added support for new event for active Ifaces
04/27/11    sy     Added PAP, CHAP failures to PPP call end reason.
12/10/10    ls     Updated ps_iface_net_down_reason_type enum to include
                   some call cmd err reason
10/03/10    az     Additional net down cause code for LTE and eHRPD
09/13/10    ss     Adding support for new hs_status_ind: HSDPA+ and DC_HSDPA+
08/07/10    at     IPV6_ENABLED_DEFAULT value set to TRUE.
07/21/10    gc     Changed some conflicting error codes
07/15/10    ts     Added support for linger and dormancy.
06/30/10    rt     NAT IFACE addition.
05/26/10    gc     Pass PPP failure code to PS
05/14/10    guru   Added defenitions for supporting IP session continuity
05/12/10    jy     New data type ps_iface_bearer_ip_type for supporting Dual-IP
                   bearer EPC handoff.
04/21/10    sa     Added LO_MODE_B_IFACE.
03/15/10    hs     Increased MAX_SYSTEM_IFACES by 2 to support 4 PDNs on LTE
02/17/10    hs     Added definitions to support bearer tech IOCTL on LTE
08/31/09   mga     Merged from eHRPD branch
04/28/09    dm     Added new MIP net down reason error codes
03/26/09    pp     CMI De-featurization.
12/14/08    pp     Common Modem Interface: Public/Private API split.
07/18/08    dm     Fixed compiler warnings
07/01/08    pp     Added SLIP_IFACE, UICC_IFACE.
05/22/08    am     Added boolean force_dereg_cbacks to
                   ps_iface_mcast_event_info_type.
05/15/08    dm     Added support to pass the dormancy info code (reject cause)
                   in case of RAB reestablishment reject to applications
04/03/08    am     BCMCS 2.0 support.
05/21/07    msr/sv Added support for IFACE linger
04/17/07    es     Added support for EXTENDED_IP_CONFIG_EV.
03/27/07    msr    Added API for RF conditions and HDR-1x handdown
01/23/07    msr    EMPA enhancements
12/15/06    mct    Added additional event/timer for privacy extensions.
12/04/06    asn    ENH to propagate SM cause-codes to App
11/16/06    msr    Added FLOW_INFO_CODE_UPDATED_EV
11/02/06    mct    Added enum for total supported IPv6 addresses. Added
                   missing WLAN iface to total number of supported ifaces.
11/1/06     rt     Merged EIDLE_SCI feature changes.
09/18/06    hm     Moved macro PS_IFACE_IS_ID_A_GROUP to ps_iface.h
09/12/06    msr    Removed redundant state field in event_info structure
08/14/06    sv     Merged slotted/Mode attribute ioctl.
08/03/06    sv     Added support for header compression filtering.
07/31/06    msr    Added support for PRIMARY_QOS_MODIFY
06/06/06    ss     Incremented max ifaces to account for DVB-H IPv6 iface
05/18/06    msr    Added STA_IFACE
04/30/06    rt     Added PS_IFACE_MAX_DOMAIN_NAME_SIZE for SIP server
                   domain names.
04/18/06    mct    Added enums for loopbacl optimization and supporting
                   multiple IPv6 iids in RFC 3041.
04/14/06    ss     Added DVBH iface name and DVBH info codes
03/28/06    mpa    Added new BCMCS flow status.
02/13/06    mct    Updated/added some BCMCS info codes.
01/25/06    mct    Renamed some QOS info codes.
12/07/05    vas    Added more failure reasons in rate inertia failure enum
12/05/05    msr    Changed IP fltr client IDs
12/05/05    msr    Split QOS client in to INPUT and OUTPUT clients
11/14/05    rt     Added new cause code enums in
                   ps_iface_net_down_reason_type.
11/08/05    msr    Fixed merge issues related to HDR rate inertia changes
10/18/05    msr    Removed support for FLOW_ACTIVATE_MODIFIED_EV and
                   PHYS_LINK_UP_MODIFIED_EV and changed ipfltr_info structure
10/12/05    mct    Added support for iface flow added/deleted events.
09/13/05    sv     Added support for new HDR rev0 rate inertia ioctl and HDR
                   HPT mode ioctl.
09/01/05    rt     Moved ps_iface_data_bearer_rate definition to
                   ps_iface_defs.h
09/01/05    rt     Added ps_iface_ioctl_bearer_tech_changed_type for
                   TECH_CHANGED_EV
08/26/05    mpa    Added new BCMCS flow statuses.
08/15/05    mct    Added support for QOS_AWARE/UNAWARE_EV and added
                   QOS_CONFIGURING.
08/14/05    rt     Added IFACE_BEARER_TECH_CHANGED_EV.
08/03/05    msr    Added FLOW_MODIFY_ACCEPTED_EV and FLOW_MODIFY_REJECTED_EV
07/25/05    rt     Added ps_iface_net_down_reason_type and modified
                   structure ps_iface_event_info_u_type.
05/03/05    msr    Moved all IP FLTR related definitions to ps_iface_ipfltr.h
04/18/05    mct    Added Multicast support.
04/17/05    msr    Added stuff (states, events etc) related to new ps_flow.
04/16/05    ks     Added new events and states for new PHYS_LINK_NULL state.
03/15/05    tmr    Adding DUN_IFACE to iface definitions
01/10/05    sv     Merged IPSEC changes.
01/10/05    msr    Increased MAX_SYSTEM_IFACES to 15 to include Ethernet.
01/08/05    vp     Removed the extra defines for DEFAULT_V6_INDEX and
                   MAX_IPV6_PREFIXES.
12/23/04    lyr    Added WLAN_IFACE to ps_iface_name_enum_type
11/19/04    msr    Added IFACE_STATE_INVALID, PHYS_LINK_STATE_INVALID states.
11/16/04    ks     Removed IFACE_PHYS_LINK_UP_MODIFIED_EV,
                   IFACE_PHYS_LINK_FLOW_ENABLED_EV and
                   IFACE_PHYS_LINK_FLOW_DISABLED_EV.
10/25/04    msr    Increased MAX_SYSTEM_IFACES to include IP ANY and
                   IPV6 loopback interfaces.
10/27/04    jd     Added rx and tx packet counters to ps_iface structure
08/13/04    mct    Added support for the MT event.
08/12/04    sv     Added support to distinguish IFACE_FLOW_ENABLED event
                   from PHYS_LINK_FLOW_ENABLED event.
08/11/04    mvl    Added default values for IPv6 enabled and failover mode.
08/06/04    mct    Added failover enums so they are accessible by PS task.
07/30/04    mvl    Changed representation of states to be a bit mask as that
                   will simplify cheking state against multiple states.
07/29/04    msr    Added IFACE_ADDR_FAMILY_CHANGED_EV.
07/12/04    mvl    Added some support for IPv6 - including new indication.
06/11/04    vp     Typedef'ed ps_iface_addr_family_type as ip_addr_enum_type
05/17/04    aku    Removed featurization of BCMCS iface with the objevtive of
                   not having any features in the header file.
04/28/04    aku    Added support for the BCMCS iface
04/27/04    usb    ps_iface_ipfltr_id_type definition, incremented max
                   iface count by 1 for 1x v6 iface.
04/19/04    vp     Added ps_iface_ipv6_iid_enum_type
03/18/04    ak     Added enums for phys link asking for dormant info.
02/18/04    usb    Added client id enum for IP filtering clients, updated
                   event info to include client id.
02/04/04    ak     Added in events for phys link and transient states.
12/26/03    usb    Added new ps_iface state and event for CONFIGURING.
10/14/03    mct    Added event enums for coming_up, going_down, and phys_link
                   equivalents.
08/28/03    ss     Changed ps_iface_id_type to uint32. Added macro to
                   determine if an id corresponds to a group
08/15/03    aku    Changes to support phys link separation from ps_iface
07/22/03    usb    New data type, enums and constant definitions, bumped up
                   MAX_SYSTEM_IFACES value.
05/27/03    om     Added IPSEC_IFACE name.
05/13/03    usb    Incremented MAX_SYSTEM_IFACES for near future needs,
                   changed iface name values to bit mask.
05/05/03    aku    Moved MAX_SYSTEM_IFACES defintion from ps_iface.h
02/07/03    usb    Added event IFACE_IPFLTR_UPDATED_EV.
01/31/03    usb    Added LO_IFACE to the iface name enum
12/24/02    aku    Moved ps_iface_event_enum_type to this file from
                   ps_iface.h
11/15/02    mvl    Added physical link statistics.
09/17/02    mvl    created file.
===========================================================================*/


/*===========================================================================

                          INCLUDE FILES FOR MODULE

===========================================================================*/
#include "comdef.h"
#include "dserrno.h"
#include "ps_in.h"

/*===========================================================================

                              TYPE DEFINITIONS

===========================================================================*/
/*---------------------------------------------------------------------------
  MAX_SYSTEM_IFACES - The maximum number of interfaces in the system.
  4 - CDMA SN IFACE (1 x v4 pkt + 1 x v6 pkt + 1 x async + 1 x any)
  1 - CDMA AN IFACE
  3 - UMTS IFACE (3 Primary PDP contexts)
  1 - WLAN IFACE
  3 - SIO IFACE (1 x CDMA, 1 x UMTS, 1 x Ethernet)
  2 - LO IFACE  (1 x v4 pkt + 1 x v6 pkt)
 16 - IPSEC IFACE
  1 - CDMA MCAST IFACE
  1 - MediaFLO IFACE
  1 - DUN (dial-up networking) IFACE
  2 - DVB-H IFACE (1 IPv4 + 1 IPv6)
  1 - STA IFACE
  1 - MBMS IFACE
  1 - UW_FMC IFACE
  1 - LO_MODE_B_IFACE
  --------------------------------------------------------------------------
  The following section covers the IPv4 and IPv6 loopback routing tables.
  --------------------------------------------------------------------------
  When a new interface is added the appropriate loopback #define listed below
  which matches the interface's IP address family should be updated.

  The value of MAX_IPV4_LO_IFACES and MAX_IPV6_LO_IFACES should always be:
  the total number of loopback ifaces for that IP version + 1;

  Logical interfaces ONLY supported if they obtain their own unique IP address
  and do not inherit IP information from an underlying iface. IPSEC ifaces
  are not supported. Do not add these ifaces to the loopback section or
  increment the loopback defines.

  MAX_IPV4_LO_IFACES - The maximum number of ifaces supported for IPv4 loopback.
  1 - CDMA
  3 - UMTS
  1 - WLAN
  3 - SIO
  1 - LO
  1 - DUN
  -------
  10 - Total

  MAX_IPV6_LO_IFACES - The maximum number of ifaces supported for IPv6 loopback.
  1 - CDMA
  3 - UMTS
  1 - LO
  -------
  5 - Total

Check with Satish about IPSEC ifaces..
---------------------------------------------------------------------------*/
#define MAX_SYSTEM_IFACES  49

#define MAX_IPV4_LO_IFACES 11
#define MAX_IPV6_LO_IFACES 6
/*---------------------------------------------------------------------------
  MAX_PHYS_LINKS_PER_IFACE - maximum number of phys links allowed for each
  iface.
---------------------------------------------------------------------------*/
#define MAX_PHYS_LINKS_PER_IFACE  5

/*---------------------------------------------------------------------------
  PS_IFACE_MAX_DOMAIN_NAME_SIZE - Maximum Domain name size.
---------------------------------------------------------------------------*/
#define PS_IFACE_MAX_DOMAIN_NAME_SIZE    256

/*---------------------------------------------------------------------------
  PS_IFACE_MAX_SIP_SERVER_DOMAIN_NAMES - Max supported Sip domain names.
---------------------------------------------------------------------------*/
#define PS_IFACE_MAX_SIP_SERVER_DOMAIN_NAMES    5

/*---------------------------------------------------------------------------
  PS_IFACE_MAX_FIREWALL_RULES - Max supported Firewall rules.
---------------------------------------------------------------------------*/
#define PS_IFACE_MAX_FIREWALL_RULES    15

/*---------------------------------------------------------------------------
PS_IFACE_MAX_SIP_SERVER_ADDRESSES - Max supported Sip addresses.
---------------------------------------------------------------------------*/
#define PS_IFACE_MAX_SIP_SERVER_ADDRESSES       6

/*---------------------------------------------------------------------------
PS_IFACE_MAX_SEARCH_LIST_DOMAIN_NAMES -
                                 Max supported domain name search list size
---------------------------------------------------------------------------*/
#define PS_IFACE_MAX_SEARCH_LIST_DOMAIN_NAMES    6

/*---------------------------------------------------------------------------
PS_IFACE_NUM_DNS_ADDRS -
Currently DS support a maximum of 2 DNS addresses per iface -
Primary and Secondary.
This define shall be irrelevant and its usage altered when the handset
supports more DNS addresses.
---------------------------------------------------------------------------*/
#define PS_IFACE_NUM_DNS_ADDRS               2

/*---------------------------------------------------------------------------
  MAX_IPV6_PREFIXES - the maximum number of IPv6 prefixes we can support
---------------------------------------------------------------------------*/
#define MAX_IPV6_PREFIXES 1
#define DEFAULT_V6_INDEX  0

/*---------------------------------------------------------------------------
  MAX_IPV6_IIDS - the maximum number of IPv6 iids we support per interface,
                  including internal addresses and those from ext. devices
---------------------------------------------------------------------------*/
#define MAX_IPV6_IIDS 10

/*---------------------------------------------------------------------------
  DEFAULT_UNUSED_IPV6_PRIV_ADDR_TIMEOUT - default (2 min, or 120,000 ms)
    The maximum time to wait for an application to bind to the privacy addr
    before deleteing it.
---------------------------------------------------------------------------*/
#define DEFAULT_UNUSED_IPV6_PRIV_ADDR_TIMEOUT 120000

/*---------------------------------------------------------------------------
  MIN_IPV6_VALID_LIFETIME - The minimum time (7200 sec) that the IPv6 valid
    lifetime can be set to.
---------------------------------------------------------------------------*/
#define MIN_IPV6_VALID_LIFETIME 7200

/*---------------------------------------------------------------------------
  MAX_IPV6_ADDRS - the total number of IPv6 addresses supported per interface
---------------------------------------------------------------------------*/
#define MAX_IPV6_ADDRS (MAX_IPV6_PREFIXES * MAX_IPV6_IIDS)

/*---------------------------------------------------------------------------
  PS_IFACE_MAX_MCAST_FLOWS - maximum number of mcast flow requests in a
  MCAST_JOIN_EX / MCAST_LEAVE_EX / MCAST_REGISTER_EX ioctl calls.
---------------------------------------------------------------------------*/
#define PS_IFACE_MAX_MCAST_FLOWS (25)

/*---------------------------------------------------------------------------
  PS_IFACE_MAX_STATIC_NAT_ENTRIES - maximum number of static NAT entry
  get requests in a PS_IFACE_IOCTL_GET_STATIC_NAT_ENTRY
---------------------------------------------------------------------------*/
#define PS_IFACE_MAX_STATIC_NAT_ENTRIES (10)

/*---------------------------------------------------------------------------
  Define minimum MTU values for V4 and V6 ifaces. An iface can never have a
  value lower than these values
---------------------------------------------------------------------------*/
#define PS_IFACE_MIN_V4_MTU   576
#define PS_IFACE_MIN_V6_MTU  1280

/*---------------------------------------------------------------------------
  Default filter id and precedence when qos request is initiated
---------------------------------------------------------------------------*/
#define PS_IFACE_IPFLTR_DEFAULT_ID 256
#define PS_IFACE_IPFLTR_DEFAULT_PRCD 256

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_ID_TYPE
  Opaque handle which uniquely identifies ps_iface
  First 8 bits are index into global_iface_ptr_array,
  Next 16 bits are reserved for future use, currently are set to FFFF
  Lowest 8 bits are phys link instance

  +----------------+----------------+----------------+----------------+
  |    8 bits      |    8 bits      |    8 bits      |    8 bits      |
  +----------------+----------------+----------------+----------------+
  |   Iface Index  |    Reserved for future use      | phys link inst |
  +----------------+----------------+----------------+----------------+
---------------------------------------------------------------------------*/
typedef uint32 ps_iface_id_type;

/*---------------------------------------------------------------------------
PS_IFACE_INVALID_ID - Id which does not identify any valid iface
---------------------------------------------------------------------------*/
#define PS_IFACE_INVALID_ID  0

#define PS_IFACE_PROC_ID_LOCAL 0x0
#define PS_IFACE_PROC_ID_ANY   0x7FFFFFFE
#define PS_IFACE_PROC_ID_NONE  0x7FFFFFFF

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_MT_HANDLE_TYPE
---------------------------------------------------------------------------*/
typedef uint32 ps_iface_mt_handle_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_MCAST_HANDLE_TYPE
---------------------------------------------------------------------------*/
typedef uint32 ps_iface_mcast_handle_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_NAME_ENUM_TYPE

DESCRIPTION
  List of interface names and groups.

  IMPORTANT NOTE: increment MAX_SYSTEM_IFACES (see below) appropriately
  if a new iface name is added to this list.
---------------------------------------------------------------------------*/
typedef enum
{
  /* Group Names - these are bit masks (uses bottom 15 bits), msb is 0 */
  ANY_DEFAULT_GROUP  = 0x0001,
  WWAN_GROUP         = 0x0002,
  RM_GROUP           = 0x0004,
  BCAST_MCAST_GROUP  = 0x0008,
  IFACE_3GPP_GROUP   = 0x0010,
  IFACE_3GPP2_GROUP  = 0x0020,
  IFACE_EPC_GROUP    = 0x0040,
  ANY_IFACE_GROUP    = 0x7FFF,

  IFACE_MASK         = 0x8000,
  CDMA_SN_IFACE      = 0x8001,
  CDMA_AN_IFACE      = 0x8002,
  UMTS_IFACE         = 0x8004,
  SIO_IFACE          = 0x8008,
  CDMA_BCAST_IFACE   = 0x8010,
  WLAN_IFACE         = 0x8020,
  DUN_IFACE          = 0x8040,
  FLO_IFACE          = 0x8080,
  DVBH_IFACE         = 0x8100,
  STA_IFACE          = 0x8200,
  IPSEC_IFACE        = 0x8400,
  LO_MODE_B_IFACE    = 0x8401,
  LO_IFACE           = 0x8800,
  MBMS_IFACE         = 0x8801,
  IWLAN_3GPP_IFACE   = 0x8802,
  IWLAN_3GPP2_IFACE  = 0x8804,
  MIP6_IFACE         = 0x8808,
  SLIP_IFACE         = 0x8810,
  UICC_IFACE         = 0x8820,
  UW_FMC_IFACE       = 0x8840,
  EPC_IFACE          = 0x8880,
  NAT_IFACE          = 0x8881
} ps_iface_name_enum_type;


/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_STATE_ENUM_TYPE

DESCRIPTION
  Enum for states of an interface.  This is a bit mask to ease comparisons
  NOT so that the interface can be in multiple states!  i.e. assignments
  should always be straight assignements.
---------------------------------------------------------------------------*/
typedef enum
{
  IFACE_STATE_INVALID = 0x00,
  IFACE_DISABLED      = 0x01,
  IFACE_DOWN          = 0x02,
  IFACE_COMING_UP     = 0x04,
  IFACE_CONFIGURING   = 0x08,   /* address configuration in progress       */
  IFACE_ROUTEABLE     = 0x10,   /* packet can be routed to this interface  */
  IFACE_UP            = 0x20,   /* data can originate over this interface  */
  IFACE_GOING_DOWN    = 0x40,
  IFACE_LINGERING     = 0x80
} ps_iface_state_enum_type;


/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_HANDOFF_ENUM_TYPE

DESCRIPTION
  Enum for handoff classes of an interface.  This is a bit mask to ease
  comparisons NOT so that the interface can be in multiple classes!
  i.e. assignments should always be straight assignements.
---------------------------------------------------------------------------*/
typedef enum
{
  PS_IFACE_HANDOFF_CLASS_NONE = 0x00,
  PS_IFACE_HANDOFF_CLASS_EPC  = 0x01
} ps_iface_handoff_class_enum_type;


/*---------------------------------------------------------------------------
TYPEDEF FLOW_STATE_TYPE

DESCRIPTION
  Flow state enum - what is the flow doing
---------------------------------------------------------------------------*/
typedef enum
{
  FLOW_STATE_INVALID = 0x00,
  FLOW_NULL          = 0x01,
  FLOW_ACTIVATING    = 0x02,
  FLOW_ACTIVATED     = 0x04,
  FLOW_SUSPENDING    = 0x08,
  FLOW_SUSPENDED     = 0x10,
  FLOW_RESUMING      = 0x20,
  FLOW_GOING_NULL    = 0x40,
  FLOW_CONFIGURING   = 0x80
} ps_flow_state_enum_type;


/*---------------------------------------------------------------------------
TYPEDEF PHYS_LINK_STATE_TYPE

DESCRIPTION
  Physical link state enum - what is the physical link doing
---------------------------------------------------------------------------*/
typedef enum
{
  PHYS_LINK_STATE_INVALID = 0x00,
  PHYS_LINK_DOWN          = 0x01,
  PHYS_LINK_COMING_UP     = 0x02,
  PHYS_LINK_UP            = 0x04,
  PHYS_LINK_GOING_DOWN    = 0x08,
  PHYS_LINK_RESUMING      = 0x10,
  PHYS_LINK_GOING_NULL    = 0x20,
  PHYS_LINK_NULL          = 0x40
} phys_link_state_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_IPV6_ADDR_STATE_ENUM_TYPE

DESCRIPTION
  State of the IPv6 address.
---------------------------------------------------------------------------*/
typedef enum
{
  // No address has been created. An invalid entry.
  IPV6_ADDR_STATE_INVALID    = 0,
  // This address is pending DAD verification
  IPV6_ADDR_STATE_TENTATIVE  = 1,
  // This address has been verified (DAD/local) and is ready to be allocated
  IPV6_ADDR_STATE_UNASSIGNED = 2,
  // This address has been assigned.
  IPV6_ADDR_STATE_VALID      = 3,
  // This address has been deprecated
  IPV6_ADDR_STATE_DEPRECATED = 4
} ps_iface_ipv6_addr_state_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_IPV6_ADDR_TYPE_ENUM_TYPE

DESCRIPTION
  The type of the IPv6 address. Public/Private, etc.
---------------------------------------------------------------------------*/
typedef enum
{
  // This is either a PUBLIC address or unused
  IPV6_ADDR_TYPE_INVALID          = 0,
  // This address is a public address
  IPV6_ADDR_TYPE_PUBLIC           = 1,
  // This address is an in use private shareable address
  IPV6_ADDR_TYPE_PRIV_SHARED      = 2,
  // This address is an in use private unique address
  IPV6_ADDR_TYPE_PRIV_UNIQUE      = 3,
  // This address is in use by an external device
  IPV6_ADDR_TYPE_EXTERNAL         = 4
} ps_iface_ipv6_addr_type_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_EVENT_ENUM_TYPE

DESCRIPTION
  List of all the possible events on an interface that modules can register
  callbacks for.
---------------------------------------------------------------------------*/
typedef enum
{
  IFACE_MIN_EV                   =  0,
  IFACE_PHYS_LINK_DOWN_EV        =  0,
  IFACE_PHYS_LINK_COMING_UP_EV   =  1,
  IFACE_PHYS_LINK_UP_EV          =  2,
  IFACE_PHYS_LINK_GOING_DOWN_EV  =  3,
  IFACE_PHYS_LINK_GONE_EV        =  4,
  IFACE_PHYS_LINK_MAX_EV,

  /* phys_link aliases for the existing events       */
  PHYS_LINK_DOWN_EV          = IFACE_PHYS_LINK_DOWN_EV,
  PHYS_LINK_COMING_UP_EV     = IFACE_PHYS_LINK_COMING_UP_EV,
  PHYS_LINK_UP_EV            = IFACE_PHYS_LINK_UP_EV,
  PHYS_LINK_GOING_DOWN_EV    = IFACE_PHYS_LINK_GOING_DOWN_EV,
  PHYS_LINK_GONE_EV          = IFACE_PHYS_LINK_GONE_EV,
  PHYS_LINK_FLOW_ENABLED_EV  = 5,
  PHYS_LINK_FLOW_DISABLED_EV = 6,
  PHYS_LINK_RESUMING_EV      = 7,
  PHYS_LINK_GOING_NULL_EV    = 8,
  PHYS_LINK_707_DOS_ACK_EV   = 9,

  PHYS_LINK_MAX_EV,
  IFACE_UP_EV                     = 10,
  IFACE_GOING_DOWN_EV             = 11,
  IFACE_ENABLED_EV                = 12,
  IFACE_DISABLED_EV               = 13,
  IFACE_DOWN_EV                   = 14,
  IFACE_COMING_UP_EV              = 15,
  IFACE_CONFIGURING_EV            = 16,
  IFACE_FLOW_ENABLED_EV           = 17,
  IFACE_FLOW_DISABLED_EV          = 18,
  IFACE_ROUTEABLE_EV              = 19,
  IFACE_ADDR_CHANGED_EV           = 20,
  IFACE_MTU_CHANGED_EV            = 21,
  IFACE_DELETE_EV                 = 22,
  IFACE_IPFLTR_UPDATED_EV         = 23,
  IFACE_PRI_PHYS_LINK_CHANGED_EV  = 24,
  IFACE_PREFIX_UPDATE_EV          = 25, /* IPv6 specific event */
  IFACE_VALID_RA_EV               = 26, /* IPv6 specific event */
  IFACE_ADDR_FAMILY_CHANGED_EV    = 27,
  IFACE_MT_REQUEST_EV             = 28,
  IFACE_MCAST_REGISTER_SUCCESS_EV = 29,
  IFACE_MCAST_REGISTER_FAILURE_EV = 30,
  IFACE_MCAST_DEREGISTERED_EV     = 31,
  IFACE_BEARER_TECH_CHANGED_EV    = 32,
  IFACE_QOS_AWARE_SYSTEM_EV       = 33,
  IFACE_QOS_UNAWARE_SYSTEM_EV     = 34,
  IFACE_FLOW_ADDED_EV             = 35,
  IFACE_FLOW_DELETED_EV           = 36,

  IFACE_ENABLE_HDR_REV0_RATE_INERTIA_SUCCESS_EV = 37,
  IFACE_ENABLE_HDR_REV0_RATE_INERTIA_FAILURE_EV = 38,

  IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SUCCESS_EV         = 39,
  IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_FAILURE_EV         = 40,
  IFACE_ENABLE_HDR_SET_EIDLE_SLOTTED_MODE_SESSION_CHANGED_EV = 41,

  IFACE_IPV6_PRIV_ADDR_GENERATED_EV                   = 42,
  IFACE_IPV6_PRIV_ADDR_DEPRECATED_EV                  = 43,
  IFACE_IPV6_PRIV_ADDR_EXPIRED_EV                     = 44,
  IFACE_IPV6_PRIV_ADDR_DELETED_EV                     = 45,
  IFACE_OUTAGE_NOTIFICATION_EV                        = 46,
  IFACE_707_NETWORK_SUPPORTED_QOS_PROFILES_CHANGED_EV = 47,
  IFACE_RF_CONDITIONS_CHANGED_EV                      = 48,
  IFACE_EXTENDED_IP_CONFIG_EV                         = 49,
  IFACE_LINGERING_EV                                  = 50,

  IFACE_MBMS_CONTEXT_ACT_SUCCESS_EV                   = 51,
  IFACE_MBMS_CONTEXT_ACT_FAILURE_EV                   = 52,
  IFACE_MBMS_CONTEXT_DEACT_SUCCESS_EV                 = 53,
  IFACE_MBMS_CONTEXT_DEACT_FAILURE_EV                 = 54,

  IFACE_AUTHENTICATING_EV                             = 55,
  IFACE_APP_PREEMPTED_EV                              = 56,
  IFACE_MCAST_STATUS_EV                               = 57,
  IFACE_IDLE_EV                                       = 58,
  IFACE_ACTIVE_OUT_OF_USE_EV                          = 59,
  IFACE_FAST_DORMANCY_STATUS_EV                       = 60,
  IFACE_ALL_EV                                        = 61,
  IFACE_EVENT_MAX                                     = IFACE_ALL_EV + 1,

  /*-------------------------------------------------------------------------
    To make sure that libraries are not messed up when features are undefined,
    assigning an explicit value to FLOW_MIN_EV. Otherwise library will have
    one value for flow event and regular code will have another value
  -------------------------------------------------------------------------*/
  FLOW_MIN_EV                     = IFACE_EVENT_MAX,
  FLOW_NULL_EV                    = FLOW_MIN_EV,
  FLOW_ACTIVATING_EV              = FLOW_MIN_EV + 1,
  FLOW_ACTIVATED_EV               = FLOW_MIN_EV + 2,
  FLOW_SUSPENDING_EV              = FLOW_MIN_EV + 3,
  FLOW_SUSPENDED_EV               = FLOW_MIN_EV + 4,
  FLOW_RESUMING_EV                = FLOW_MIN_EV + 5,
  FLOW_GOING_NULL_EV              = FLOW_MIN_EV + 6,
  FLOW_CONFIGURING_EV             = FLOW_MIN_EV + 7,
  FLOW_TX_ENABLED_EV              = FLOW_MIN_EV + 8,
  FLOW_TX_DISABLED_EV             = FLOW_MIN_EV + 9,
  FLOW_RX_FLTR_UPDATED_EV         = FLOW_MIN_EV + 10,
  FLOW_MODIFY_ACCEPTED_EV         = FLOW_MIN_EV + 11,
  FLOW_MODIFY_REJECTED_EV         = FLOW_MIN_EV + 12,
  FLOW_PRIMARY_MODIFY_RESULT_EV   = FLOW_MIN_EV + 13,
  FLOW_INFO_CODE_UPDATED_EV       = FLOW_MIN_EV + 14,
  /*---------------------------------------------------------------------------
    Event represents that auxiliary information associated with
    Rx or TX filter spec associated with a flow has been changed by the network
  ---------------------------------------------------------------------------*/
  FLOW_FLTR_AUX_INFO_UPDATED_EV   = FLOW_MIN_EV + 15,
  FLOW_MAX_EV

} ps_iface_event_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_EXTENDED_INFO_CODE_ENUM_TYPE

DESCRIPTION
  Extended reason codes to pass additional information with the
  iface/flow/phys_link events.  If extended information is not available,
  the default value PS_EIC_NOT_SPECIFIED should be used.
---------------------------------------------------------------------------*/
typedef enum
{
  PS_EIC_NOT_SPECIFIED = 0,

  PS_EIC_QOS_INTERNAL_MIN                     = 1,
  PS_EIC_QOS_INVALID_PARAMS                   = PS_EIC_QOS_INTERNAL_MIN + 2,
  PS_EIC_QOS_INTERNAL_INVALID_PARAMS          = PS_EIC_QOS_INVALID_PARAMS,
  PS_EIC_QOS_INTERNAL_CALL_ENDED              = PS_EIC_QOS_INTERNAL_MIN + 3,
  PS_EIC_QOS_INTERNAL_ERROR                   = PS_EIC_QOS_INTERNAL_MIN + 4,
  PS_EIC_QOS_INSUFFICIENT_LOCAL_RESOURCES     = PS_EIC_QOS_INTERNAL_MIN + 5,
  PS_EIC_QOS_TIMED_OUT_OPERATION              = PS_EIC_QOS_INTERNAL_MIN + 6,
  PS_EIC_QOS_INTERNAL_UNKNOWN_CAUSE_CODE      = PS_EIC_QOS_INTERNAL_MIN + 7,
  PS_EIC_QOS_INTERNAL_MODIFY_IN_PROGRESS      = PS_EIC_QOS_INTERNAL_MIN + 8,
  PS_EIC_QOS_INTERNAL_MAX,

  PS_EIC_QOS_NETWORK_MIN                      = 128,
  PS_EIC_QOS_NOT_SUPPORTED                    = PS_EIC_QOS_NETWORK_MIN + 1,
  PS_EIC_QOS_NOT_AVAILABLE                    = PS_EIC_QOS_NETWORK_MIN + 2,
  PS_EIC_QOS_NOT_GUARANTEED                   = PS_EIC_QOS_NETWORK_MIN + 3,
  PS_EIC_QOS_INSUFFICIENT_NET_RESOURCES       = PS_EIC_QOS_NETWORK_MIN + 4,
  PS_EIC_QOS_AWARE_SYSTEM                     = PS_EIC_QOS_NETWORK_MIN + 5,
  PS_EIC_QOS_UNAWARE_SYSTEM                   = PS_EIC_QOS_NETWORK_MIN + 6,
  PS_EIC_QOS_REJECTED_OPERATION               = PS_EIC_QOS_NETWORK_MIN + 7,
  PS_EIC_QOS_WILL_GRANT_WHEN_QOS_RESUMED      = PS_EIC_QOS_NETWORK_MIN + 8,
  PS_EIC_QOS_NETWORK_CALL_ENDED               = PS_EIC_QOS_NETWORK_MIN + 9,
  PS_EIC_QOS_NETWORK_SVC_NOT_AVAILABLE        = PS_EIC_QOS_NETWORK_MIN + 10,
  PS_EIC_QOS_NETWORK_L2_LINK_RELEASED         = PS_EIC_QOS_NETWORK_MIN + 11,
  PS_EIC_QOS_NETWORK_L2_LINK_REESTAB_REJ      = PS_EIC_QOS_NETWORK_MIN + 12,
  PS_EIC_QOS_NETWORK_L2_LINK_REESTAB_IND      = PS_EIC_QOS_NETWORK_MIN + 13,
  PS_EIC_QOS_NETWORK_UNKNOWN_CAUSE_CODE       = PS_EIC_QOS_NETWORK_MIN + 14,
  PS_EIC_QOS_NETWORK_DISJOINT_PROFILE_SET_SUGGESTED
                                              = PS_EIC_QOS_NETWORK_MIN + 15,
  PS_EIC_QOS_NETWORK_NULL_PROFILE_SUGGESTED   = PS_EIC_QOS_NETWORK_MIN + 16,
  PS_EIC_QOS_NETWORK_UE_NOT_AUTHORIZED        = PS_EIC_QOS_NETWORK_MIN + 17,
  PS_EIC_QOS_NETWORK_MAX,

  PS_EIC_NETWORK_MIN                          = 200,
  PS_EIC_NETWORK_NOT_SPECIFIED                = PS_EIC_NETWORK_MIN + 1,
  PS_EIC_NETWORK_BUSY                         = PS_EIC_NETWORK_MIN + 2,
  PS_EIC_NETWORK_MAX

} ps_extended_info_code_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_NET_DOWN_REASON_TYPE

DESCRIPTION
  Enum type to determine the network going down reason.
  The 26 onwards values are taken from the 3GPP document and mapped
  corresponding to the values defined in the doc.
  3GPP TS 24.008 version 3.5.0 Release 1999
  The values from 500 onwards do NOT map to the 3GPP spec.
  The net down reasons have the following format:

  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  |          Type                 |             Reason            |
  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  The upper two bytes represent the type of error.
  The type is 0 for all legacy net down reason such as CDMA, GSM, HDR, EAP
  and IPSEC
  The type is 1 for all MIP error codes.
  The type is 6 for all 3GPP error codes.
  The type is 7 for all PPP error codes.
  The type is 8 for all eHRPD error codes.
  The type is 9 for all IPv6 error codes
  The lower two bytes represent the actual net down reason.
---------------------------------------------------------------------------*/
#define TYPE_REASON_VAL(type, reason) ((0xFFFF0000 & (type << 16)) | (0x0000FFFF & reason))
typedef enum
{
  PS_NET_DOWN_REASON_NOT_SPECIFIED               = 0, /* Reason not known */
  PS_NET_DOWN_REASON_OPERATOR_DETERMINED_BARRING = TYPE_REASON_VAL(6,8),
  PS_NET_DOWN_REASON_LLC_SNDCP_FAILURE           = TYPE_REASON_VAL(6,25),
  PS_NET_DOWN_REASON_INSUFFICIENT_RESOURCES   = TYPE_REASON_VAL(6,26),
  PS_NET_DOWN_REASON_UNKNOWN_APN              = TYPE_REASON_VAL(6,27),
  PS_NET_DOWN_REASON_UNKNOWN_PDP              = TYPE_REASON_VAL(6,28),
  PS_NET_DOWN_REASON_AUTH_FAILED              = TYPE_REASON_VAL(6,29),
  PS_NET_DOWN_REASON_GGSN_REJECT              = TYPE_REASON_VAL(6,30),
  PS_NET_DOWN_REASON_ACTIVATION_REJECT        = TYPE_REASON_VAL(6,31),
  PS_NET_DOWN_REASON_OPTION_NOT_SUPPORTED     = TYPE_REASON_VAL(6,32),
  PS_NET_DOWN_REASON_OPTION_UNSUBSCRIBED      = TYPE_REASON_VAL(6,33),
  PS_NET_DOWN_REASON_OPTION_TEMP_OOO          = TYPE_REASON_VAL(6,34),
  PS_NET_DOWN_REASON_NSAPI_ALREADY_USED       = TYPE_REASON_VAL(6,35),
  PS_NET_DOWN_REASON_REGULAR_DEACTIVATION     = TYPE_REASON_VAL(6,36),
  PS_NET_DOWN_REASON_QOS_NOT_ACCEPTED         = TYPE_REASON_VAL(6,37),
  PS_NET_DOWN_REASON_NETWORK_FAILURE          = TYPE_REASON_VAL(6,38),
  PS_NET_DOWN_REASON_UMTS_REATTACH_REQ        = TYPE_REASON_VAL(6,39),
  PS_NET_DOWN_REASON_FEATURE_NOT_SUPPORTED    = TYPE_REASON_VAL(6,40),
  PS_NET_DOWN_REASON_TFT_SEMANTIC_ERROR       = TYPE_REASON_VAL(6,41),
  PS_NET_DOWN_REASON_TFT_SYNTAX_ERROR         = TYPE_REASON_VAL(6,42),
  PS_NET_DOWN_REASON_UNKNOWN_PDP_CONTEXT      = TYPE_REASON_VAL(6,43),
  PS_NET_DOWN_REASON_FILTER_SEMANTIC_ERROR    = TYPE_REASON_VAL(6,44),
  PS_NET_DOWN_REASON_FILTER_SYNTAX_ERROR      = TYPE_REASON_VAL(6,45),
  PS_NET_DOWN_REASON_PDP_WITHOUT_ACTIVE_TFT   = TYPE_REASON_VAL(6,46),
  PS_NET_DOWN_REASON_IP_V4_ONLY_ALLOWED       = TYPE_REASON_VAL(6,50),
  PS_NET_DOWN_REASON_IP_V6_ONLY_ALLOWED       = TYPE_REASON_VAL(6,51),
  PS_NET_DOWN_REASON_SINGLE_ADDR_BEARER_ONLY  = TYPE_REASON_VAL(6,52),
  PS_NET_DOWN_REASON_ESM_INFO_NOT_RECEIVED    = TYPE_REASON_VAL(6,53),
  PS_NET_DOWN_REASON_PDN_CONN_DOES_NOT_EXIST  = TYPE_REASON_VAL(6,54),
  PS_NET_DOWN_REASON_MULTI_CONN_TO_SAME_PDN_NOT_ALLOWED = TYPE_REASON_VAL(6,55),
  PS_NET_DOWN_REASON_INVALID_TRANSACTION_ID       = TYPE_REASON_VAL(6,81),
  PS_NET_DOWN_REASON_MESSAGE_INCORRECT_SEMANTIC   = TYPE_REASON_VAL(6,95),
  PS_NET_DOWN_REASON_INVALID_MANDATORY_INFO       = TYPE_REASON_VAL(6,96),
  PS_NET_DOWN_REASON_MESSAGE_TYPE_UNSUPPORTED     = TYPE_REASON_VAL(6,97),
  PS_NET_DOWN_REASON_MSG_TYPE_NONCOMPATIBLE_STATE = TYPE_REASON_VAL(6,98),
  PS_NET_DOWN_REASON_UNKNOWN_INFO_ELEMENT         = TYPE_REASON_VAL(6,99),
  PS_NET_DOWN_REASON_CONDITIONAL_IE_ERROR         = TYPE_REASON_VAL(6,100),
  PS_NET_DOWN_REASON_MSG_AND_PROTOCOL_STATE_UNCOMPATIBLE = TYPE_REASON_VAL(6,101),
  PS_NET_DOWN_REASON_PROTOCOL_ERROR           = TYPE_REASON_VAL(6,111),
  PS_NET_DOWN_REASON_APN_TYPE_CONFLICT       = TYPE_REASON_VAL(6,112),

  PS_NET_DOWN_REASON_INTERNAL_MIN                 = TYPE_REASON_VAL(2,200),
  PS_NET_DOWN_REASON_INTERNAL_ERROR               = TYPE_REASON_VAL(2,201),
  PS_NET_DOWN_REASON_INTERNAL_CALL_ENDED          = TYPE_REASON_VAL(2,202),
  PS_NET_DOWN_REASON_INTERNAL_UNKNOWN_CAUSE_CODE  = TYPE_REASON_VAL(2,203),
  PS_NET_DOWN_REASON_UNKNOWN_CAUSE_CODE           = TYPE_REASON_VAL(2,204),
  PS_NET_DOWN_REASON_CLOSE_IN_PROGRESS            = TYPE_REASON_VAL(2,205),
  PS_NET_DOWN_REASON_NW_INITIATED_TERMINATION     = TYPE_REASON_VAL(2,206),
  PS_NET_DOWN_REASON_APP_PREEMPTED                = TYPE_REASON_VAL(2,207),
  PS_NET_DOWN_REASON_INTERNAL_MAX,

/* To map CDMA specific call-end reasons from CM */
  PS_NET_DOWN_REASON_CDMA_LOCK                 = TYPE_REASON_VAL(3,500),
  PS_NET_DOWN_REASON_INTERCEPT                 = TYPE_REASON_VAL(3,501),
  PS_NET_DOWN_REASON_REORDER                   = TYPE_REASON_VAL(3,502),
  PS_NET_DOWN_REASON_REL_SO_REJ                = TYPE_REASON_VAL(3,503),
  PS_NET_DOWN_REASON_INCOM_CALL                = TYPE_REASON_VAL(3,504),
  PS_NET_DOWN_REASON_ALERT_STOP                = TYPE_REASON_VAL(3,505),
  PS_NET_DOWN_REASON_ACTIVATION                = TYPE_REASON_VAL(3,506),
  PS_NET_DOWN_REASON_MAX_ACCESS_PROBE          = TYPE_REASON_VAL(3,507),
  PS_NET_DOWN_REASON_CCS_NOT_SUPPORTED_BY_BS   = TYPE_REASON_VAL(3,508),
  PS_NET_DOWN_REASON_NO_RESPONSE_FROM_BS       = TYPE_REASON_VAL(3,509),
  PS_NET_DOWN_REASON_REJECTED_BY_BS            = TYPE_REASON_VAL(3,510),
  PS_NET_DOWN_REASON_INCOMPATIBLE              = TYPE_REASON_VAL(3,511),
  PS_NET_DOWN_REASON_ALREADY_IN_TC             = TYPE_REASON_VAL(3,512),
  PS_NET_DOWN_REASON_USER_CALL_ORIG_DURING_GPS = TYPE_REASON_VAL(3,513),
  PS_NET_DOWN_REASON_USER_CALL_ORIG_DURING_SMS = TYPE_REASON_VAL(3,514),
  PS_NET_DOWN_REASON_NO_CDMA_SRV               = TYPE_REASON_VAL(3,515),
  PS_NET_DOWN_REASON_MC_ABORT                  = TYPE_REASON_VAL(3,516),
  PS_NET_DOWN_REASON_PSIST_NG                  = TYPE_REASON_VAL(3,517),
  PS_NET_DOWN_REASON_UIM_NOT_PRESENT           = TYPE_REASON_VAL(3,518),
  PS_NET_DOWN_REASON_RETRY_ORDER               = TYPE_REASON_VAL(3,519),
  PS_NET_DOWN_REASON_ACCESS_BLOCK              = TYPE_REASON_VAL(3,520),
  PS_NET_DOWN_REASON_ACCESS_BLOCK_ALL          = TYPE_REASON_VAL(3,521),
  PS_NET_DOWN_REASON_IS707B_MAX_ACC            = TYPE_REASON_VAL(3,522),

/* To map GSM/WCDMA specific call-end reasons from CM */
  PS_NET_DOWN_REASON_CONF_FAILED               = TYPE_REASON_VAL(3,1000),
  PS_NET_DOWN_REASON_INCOM_REJ                 = TYPE_REASON_VAL(3,1001),
  PS_NET_DOWN_REASON_NO_GW_SRV                 = TYPE_REASON_VAL(3,1002),
  PS_NET_DOWN_REASON_NO_GPRS_CONTEXT           = TYPE_REASON_VAL(3,1003),
  PS_NET_DOWN_REASON_ILLEGAL_MS                = TYPE_REASON_VAL(3,1004),
  PS_NET_DOWN_REASON_ILLEGAL_ME                = TYPE_REASON_VAL(3,1005),
  PS_NET_DOWN_REASON_GPRS_SERVICES_AND_NON_GPRS_SERVICES_NOT_ALLOWED = TYPE_REASON_VAL(3,1006),
  PS_NET_DOWN_REASON_GPRS_SERVICES_NOT_ALLOWED                       = TYPE_REASON_VAL(3,1007),
  PS_NET_DOWN_REASON_MS_IDENTITY_CANNOT_BE_DERIVED_BY_THE_NETWORK    = TYPE_REASON_VAL(3,1008),
  PS_NET_DOWN_REASON_IMPLICITLY_DETACHED                             = TYPE_REASON_VAL(3,1009),
  PS_NET_DOWN_REASON_PLMN_NOT_ALLOWED                                = TYPE_REASON_VAL(3,1010),
  PS_NET_DOWN_REASON_LA_NOT_ALLOWED                                  = TYPE_REASON_VAL(3,1011),
  PS_NET_DOWN_REASON_GPRS_SERVICES_NOT_ALLOWED_IN_THIS_PLMN          = TYPE_REASON_VAL(3,1012),
  PS_NET_DOWN_REASON_PDP_DUPLICATE                                   = TYPE_REASON_VAL(3,1013),
  PS_NET_DOWN_REASON_UE_RAT_CHANGE                                   = TYPE_REASON_VAL(3,1014),
  PS_NET_DOWN_REASON_CONGESTION                                      = TYPE_REASON_VAL(3,1015),
  PS_NET_DOWN_REASON_NO_PDP_CONTEXT_ACTIVATED                        = TYPE_REASON_VAL(3,1016),
  PS_NET_DOWN_REASON_ACCESS_CLASS_DSAC_REJECTION                     = TYPE_REASON_VAL(3,1017),

/* To map HDR specific call-end reasons from CM */
  PS_NET_DOWN_REASON_CD_GEN_OR_BUSY            = TYPE_REASON_VAL(3,1500),
  PS_NET_DOWN_REASON_CD_BILL_OR_AUTH           = TYPE_REASON_VAL(3,1501),
  PS_NET_DOWN_REASON_CHG_HDR                   = TYPE_REASON_VAL(3,1502),
  PS_NET_DOWN_REASON_EXIT_HDR                  = TYPE_REASON_VAL(3,1503),
  PS_NET_DOWN_REASON_HDR_NO_SESSION            = TYPE_REASON_VAL(3,1504),
  PS_NET_DOWN_REASON_HDR_ORIG_DURING_GPS_FIX   = TYPE_REASON_VAL(3,1505),
  PS_NET_DOWN_REASON_HDR_CS_TIMEOUT            = TYPE_REASON_VAL(3,1506),
  PS_NET_DOWN_REASON_HDR_RELEASED_BY_CM        = TYPE_REASON_VAL(3,1507),
  PS_NET_DOWN_REASON_COLLOC_ACQ_FAIL           = TYPE_REASON_VAL(3,1508),
  PS_NET_DOWN_REASON_OTASP_COMMIT_IN_PROG      = TYPE_REASON_VAL(3,1509),
  PS_NET_DOWN_REASON_NO_HYBR_HDR_SRV           = TYPE_REASON_VAL(3,1510),
  PS_NET_DOWN_REASON_HDR_NO_LOCK_GRANTED       = TYPE_REASON_VAL(3,1511),
  PS_NET_DOWN_REASON_HOLD_OTHER_IN_PROG        = TYPE_REASON_VAL(3,1512),
  PS_NET_DOWN_REASON_HDR_FADE                  = TYPE_REASON_VAL(3,1513),
  PS_NET_DOWN_REASON_HDR_ACC_FAIL              = TYPE_REASON_VAL(3,1514),

/* To map technology-agnostic call-end reasons from CM */
  PS_NET_DOWN_REASON_CLIENT_END               = TYPE_REASON_VAL(3,2000),
  PS_NET_DOWN_REASON_NO_SRV                   = TYPE_REASON_VAL(3,2001),
  PS_NET_DOWN_REASON_FADE                     = TYPE_REASON_VAL(3,2002),
  PS_NET_DOWN_REASON_REL_NORMAL               = TYPE_REASON_VAL(3,2003),
  PS_NET_DOWN_REASON_ACC_IN_PROG              = TYPE_REASON_VAL(3,2004),
  PS_NET_DOWN_REASON_ACC_FAIL                 = TYPE_REASON_VAL(3,2005),
  PS_NET_DOWN_REASON_REDIR_OR_HANDOFF         = TYPE_REASON_VAL(3,2006),

/* To map some call cmd err reasons from CM */
  PS_NET_DOWN_REASON_OFFLINE                  = TYPE_REASON_VAL(3,2500),
  PS_NET_DOWN_REASON_EMERGENCY_MODE           = TYPE_REASON_VAL(3,2501),
  PS_NET_DOWN_REASON_PHONE_IN_USE             = TYPE_REASON_VAL(3,2502),
  PS_NET_DOWN_REASON_INVALID_MODE             = TYPE_REASON_VAL(3,2503),
  PS_NET_DOWN_REASON_INVALID_SIM_STATE        = TYPE_REASON_VAL(3,2504),
  PS_NET_DOWN_REASON_NO_COLLOC_HDR            = TYPE_REASON_VAL(3,2505),
  PS_NET_DOWN_REASON_CALL_CONTROL_REJECTED    = TYPE_REASON_VAL(3,2506),

  /* EAP error codes. Start from 5000 */
  PS_NET_DOWN_REASON_EAP_MIN = TYPE_REASON_VAL(4,5000), /* Do not use. Used for bounds check. */
  PS_NET_DOWN_REASON_EAP_CLIENT_ERR_UNABLE_TO_PROCESS,
  PS_NET_DOWN_REASON_EAP_CLIENT_ERR_UNSUPPORTED_VERS,
  PS_NET_DOWN_REASON_EAP_CLIENT_ERR_INSUFFICIENT_CHALLANGES,
  PS_NET_DOWN_REASON_EAP_CLIENT_ERR_RAND_NOT_FRESH,
  PS_NET_DOWN_REASON_EAP_NOTIFICATION_GENERAL_FAILURE_AFTER_AUTH, /*EAP 0*/
  PS_NET_DOWN_REASON_EAP_NOTIFICATION_GENERAL_FAILURE_BEFORE_AUTH, /*EAP 16384*/
  PS_NET_DOWN_REASON_EAP_NOTIFICATION_TEMP_DENIED_ACCESS, /*EAP 1026*/
  PS_NET_DOWN_REASON_EAP_NOTIFICATION_USER_NOT_SUBSCRIBED, /*EAP 1031*/
  PS_NET_DOWN_REASON_EAP_SUCCESS, /*EAP 32768*/
  PS_NET_DOWN_REASON_EAP_NOTIFICATION_REALM_UNAVAILABLE,
  PS_NET_DOWN_REASON_EAP_NOTIFICATION_USER_NAME_UNAVAILABLE,
  PS_NET_DOWN_REASON_EAP_NOTIFICATION_CALL_BARRED,
  PS_NET_DOWN_REASON_EAP_MAX, /* Do not use. Used for bounds check. */

  /* IPSEC Error Codes. Start from 5100*/
  PS_NET_DOWN_REASON_IPSEC_MIN = TYPE_REASON_VAL(5,5100), /* Do not use. Used for bounds check. */
  PS_NET_DOWN_REASON_IPSEC_GW_UNREACHABLE,
  PS_NET_DOWN_REASON_IPSEC_AUTH_FAILED,
  PS_NET_DOWN_REASON_IPSEC_CERT_INVALID,
  PS_NET_DOWN_REASON_IPSEC_INTERNAL_ERROR,
  PS_NET_DOWN_REASON_IPSEC_MAX, /* Do not use. Used for bounds check. */

  /* MIP Error codes. */
  PS_NET_DOWN_REASON_MIP_FA_ERR_REASON_UNSPECIFIED              = TYPE_REASON_VAL(1,64),
  PS_NET_DOWN_REASON_MIP_FA_ERR_ADMINISTRATIVELY_PROHIBITED     = TYPE_REASON_VAL(1,65),
  PS_NET_DOWN_REASON_MIP_FA_ERR_INSUFFICIENT_RESOURCES          = TYPE_REASON_VAL(1,66),
  PS_NET_DOWN_REASON_MIP_FA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE
                                                                = TYPE_REASON_VAL(1,67),
  PS_NET_DOWN_REASON_MIP_FA_ERR_HA_AUTHENTICATION_FAILURE       = TYPE_REASON_VAL(1,68),
  PS_NET_DOWN_REASON_MIP_FA_ERR_REQUESTED_LIFETIME_TOO_LONG     = TYPE_REASON_VAL(1,69),
  PS_NET_DOWN_REASON_MIP_FA_ERR_MALFORMED_REQUEST               = TYPE_REASON_VAL(1,70),
  PS_NET_DOWN_REASON_MIP_FA_ERR_MALFORMED_REPLY                 = TYPE_REASON_VAL(1,71),
  PS_NET_DOWN_REASON_MIP_FA_ERR_ENCAPSULATION_UNAVAILABLE       = TYPE_REASON_VAL(1,72),
  PS_NET_DOWN_REASON_MIP_FA_ERR_VJHC_UNAVAILABLE                = TYPE_REASON_VAL(1,73),
  PS_NET_DOWN_REASON_MIP_FA_ERR_UNKNOWN_CHALLENGE               = TYPE_REASON_VAL(1,104),
  PS_NET_DOWN_REASON_MIP_FA_ERR_MISSING_CHALLENGE               = TYPE_REASON_VAL(1,105),
  PS_NET_DOWN_REASON_MIP_FA_ERR_STALE_CHALLENGE                 = TYPE_REASON_VAL(1,106),
  PS_NET_DOWN_REASON_MIP_FA_ERR_REVERSE_TUNNEL_UNAVAILABLE      = TYPE_REASON_VAL(1,74),
  PS_NET_DOWN_REASON_MIP_FA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET
                                                                = TYPE_REASON_VAL(1,75),
  PS_NET_DOWN_REASON_MIP_FA_ERR_DELIVERY_STYLE_NOT_SUPPORTED    = TYPE_REASON_VAL(1,79),
  PS_NET_DOWN_REASON_MIP_FA_ERR_MISSING_NAI                     = TYPE_REASON_VAL(1,97),
  PS_NET_DOWN_REASON_MIP_FA_ERR_MISSING_HA                      = TYPE_REASON_VAL(1,98),
  PS_NET_DOWN_REASON_MIP_FA_ERR_MISSING_HOME_ADDR               = TYPE_REASON_VAL(1,99),

  PS_NET_DOWN_REASON_MIP_HA_ERR_REASON_UNSPECIFIED              = TYPE_REASON_VAL(1,128),
  PS_NET_DOWN_REASON_MIP_HA_ERR_ADMINISTRATIVELY_PROHIBITED     = TYPE_REASON_VAL(1,129),
  PS_NET_DOWN_REASON_MIP_HA_ERR_INSUFFICIENT_RESOURCES          = TYPE_REASON_VAL(1,130),
  PS_NET_DOWN_REASON_MIP_HA_ERR_MOBILE_NODE_AUTHENTICATION_FAILURE
                                                                = TYPE_REASON_VAL(1,131),
  PS_NET_DOWN_REASON_MIP_HA_ERR_FA_AUTHENTICATION_FAILURE       = TYPE_REASON_VAL(1,132),
  PS_NET_DOWN_REASON_MIP_HA_ERR_REGISTRATION_ID_MISMATCH        = TYPE_REASON_VAL(1,133),
  PS_NET_DOWN_REASON_MIP_HA_ERR_MALFORMED_REQUEST               = TYPE_REASON_VAL(1,134),
  PS_NET_DOWN_REASON_MIP_HA_ERR_UNKNOWN_HA_ADDR                 = TYPE_REASON_VAL(1,136),
  PS_NET_DOWN_REASON_MIP_HA_ERR_REVERSE_TUNNEL_UNAVAILABLE      = TYPE_REASON_VAL(1,137),
  PS_NET_DOWN_REASON_MIP_HA_ERR_REVERSE_TUNNEL_IS_MANDATORY_AND_T_BIT_NOT_SET
                                                                = TYPE_REASON_VAL(1,138),
  PS_NET_DOWN_REASON_MIP_HA_ERR_ENCAPSULATION_UNAVAILABLE       = TYPE_REASON_VAL(1,139),
  PS_NET_DOWN_REASON_MIP_ERR_REASON_UNKNOWN                     = TYPE_REASON_VAL(1,65535),

  /*PPP error codes */
  PS_NET_DOWN_REASON_PPP_ERR_TIMEOUT                            = TYPE_REASON_VAL(7,1),
  PS_NET_DOWN_REASON_PPP_ERR_AUTH_FAILURE                       = TYPE_REASON_VAL(7,2),
  PS_NET_DOWN_REASON_PPP_ERR_OPTION_MISMATCH                    = TYPE_REASON_VAL(7,3),
  PS_NET_DOWN_REASON_PPP_ERR_PAP_FAILURE                        = TYPE_REASON_VAL(7,31),
  PS_NET_DOWN_REASON_PPP_ERR_CHAP_FAILURE                       = TYPE_REASON_VAL(7,32),
  PS_NET_DOWN_REASON_PPP_ERR_REASON_UNKNOWN                     = TYPE_REASON_VAL(7,65535),

  /* eHRPD error codes */
  PS_NET_DOWN_REASON_EHRPD_ERR_SUBS_LIMITED_TO_V4               = TYPE_REASON_VAL(8,1),
  PS_NET_DOWN_REASON_EHRPD_ERR_SUBS_LIMITED_TO_V6               = TYPE_REASON_VAL(8,2),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_TIMEOUT                    = TYPE_REASON_VAL(8,4),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_FAILURE                    = TYPE_REASON_VAL(8,5),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_3GPP2I_GEN_ERROR           = TYPE_REASON_VAL(8,6),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_3GPP2I_UNAUTH_APN          = TYPE_REASON_VAL(8,7),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_3GPP2I_PDN_LIMIT_EXCEED    = TYPE_REASON_VAL(8,8),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_3GPP2I_NO_PDN_GW           = TYPE_REASON_VAL(8,9),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_3GPP2I_PDN_GW_UNREACH      = TYPE_REASON_VAL(8,10),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_3GPP2I_PDN_GW_REJ          = TYPE_REASON_VAL(8,11),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_3GPP2I_INSUFF_PARAM        = TYPE_REASON_VAL(8,12),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_3GPP2I_RESOURCE_UNAVAIL    = TYPE_REASON_VAL(8,13),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_3GPP2I_ADMIN_PROHIBIT      = TYPE_REASON_VAL(8,14),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_3GPP2I_PDN_ID_IN_USE       = TYPE_REASON_VAL(8,15),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_3GPP2I_SUBSCR_LIMITATION   = TYPE_REASON_VAL(8,16),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_3GPP2I_PDN_EXISTS_FOR_THIS_APN = TYPE_REASON_VAL(8,17),
  PS_NET_DOWN_REASON_EHRPD_ERR_VSNCP_3GPP2I_ERROR_MAX,

  /* IPv6 error codes */
  PS_NET_DOWN_REASON_IPV6_ERR_PREFIX_UNAVAILABLE                = TYPE_REASON_VAL(9,1),
  PS_NET_DOWN_REASON_IPV6_ERR_MAX,

  PS_NET_DOWN_REASON_MAX             /* DO NOT USE. Used for bounds check */
} ps_iface_net_down_reason_type;


typedef enum
{
  PS_IFACE_MCAST_IC_NOT_SPECIFIED             = 0,

  /* BCMCS info codes #'s 1-1000 */
  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_CANCELLED = 100,

  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_UNABLE_TO_MONITOR = 300,
  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_REQUESTED = 301,
  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_TIMEOUT = 302,
  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_LOST = 303,
  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_SYS_UNAVAILABLE = 304,

  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_AN_REJECT_NOT_AVAILABLE = 400,
  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_AN_REJECT_NOT_TRANSMITTED = 401,
  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_AN_REJECT_INVALID_AUTH_SIG = 402,

  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_UNAVAILABLE = 500,
  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_NO_MAPPING = 501,
  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_ID_NOT_FOUND_FOR_GIVEN_MULTICAST_IP = 502,

  PS_IFACE_MCAST_BCMCS_MAX_FLOWS_REACHED = 503,
  PS_IFACE_MCAST_BCMCS_MAX_DEPRECATED_INFO_CODE = 504, /* Backward compatibility */

  /* BCMCS 2.0: Info codes (range 600 onwards) */
  /* Some of the above info codes are re-written as 2p0 info codes */
  PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_CANCELLED = 600,
  PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_UNABLE_TO_MONITOR = 601,
  PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_TIMEOUT = 602,
  PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_LOST = 603,
  PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_SYS_UNAVAILABLE = 604,
  PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_UNAVAILABLE = 605,

  PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_AN_REJECT_NOT_AVAILABLE = 606,
  PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_AN_REJECT_NOT_TRANSMITTED = 607,
  PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_AN_REJECT_INVALID_AUTH_SIG = 608,

  PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_NO_MAPPING = 609,
  PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_ID_NOT_FOUND_FOR_GIVEN_MULTICAST_IP = 610,
  PS_IFACE_MCAST_BCMCS2p0_FLOW_STATUS_REQUESTED = 611,
  PS_IFACE_MCAST_BCMCS2p0_MAX_FLOWS_REACHED = 612,

  /* BCMCS 2.0 New info codes */
  PS_IFACE_MCAST_BCMCS_JOIN_REQ_IN_PROGRESS = 613,
  PS_IFACE_MCAST_BCMCS_FLOW_REQUEST_SENT = 614,
  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_MAX_MONITORED_FLOWS = 615,
  PS_IFACE_MCAST_BCMCS_FLOW_STATUS_MONITORED = 616,
  PS_IFACE_MCAST_BCMCS_REGISTRATION_SUCCESS = 617,
  PS_IFACE_MCAST_BCMCS_REGISTRATION_NOT_ALLOWED = 618,
  PS_IFACE_MCAST_BCMCS_REGISTRATION_FAILED = 619,
  PS_IFACE_MCAST_BCMCS_FLOW_DEREGISTERED = 620,

  /* MediaFLO info codes #'s 1051-1100*/
  PS_IFACE_MCAST_FLO_IP_OR_PORT_NOT_SUPPORTED = 1051,
  PS_IFACE_MCAST_FLO_NO_AUTHORIZATION         = 1052,
  PS_IFACE_MCAST_FLO_NO_SYSTEM_COVERAGE       = 1053,
  PS_IFACE_MCAST_FLO_MAX_FLOW_REACHED         = 1054,

/* DVB-H info codes, #1101-1150 */
  PS_IFACE_MCAST_DVBH_IP_OR_PORT_NOT_FOUND    = 1101,
  PS_IFACE_MCAST_DVBH_SYSTEM_UNAVAILABLE      = 1102,
  PS_IFACE_MCAST_DVBH_BAD_REQUEST             = 1103,
  PS_IFACE_MCAST_DVBH_REQUEST_CONFLICT        = 1104,
  PS_IFACE_MCAST_DVBH_DUP_REQUEST             = 1105,
  PS_IFACE_MCAST_DVBH_MAX_FLOWS_REACHED       = 1106,

/* MBMS  info codes, #1151-1200 */
  PS_IFACE_MCAST_MBMS_SYSTEM_UNAVAILABLE = 1151

  /* Any additional info code */
} ps_iface_mcast_info_code_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_HDR_MODE_FAILURE_REASON_CODE_ENUM_TYPE

DESCRIPTION
  Enum type for HDR mode failure
---------------------------------------------------------------------------*/
typedef enum
{
  PS_HDR_REV0_RATE_INERTIA_REQUEST_REJECTED = 0, /* Request to change rate
                                                    params rejected */
  PS_HDR_REV0_RATE_INERTIA_REQUEST_FAILED_TX,    /* Failed transmission */
  PS_HDR_REV0_RATE_INERTIA_NOT_SUPPORTED,        /* RMAC subtype RMAC1 */
  PS_HDR_REV0_RATE_INERTIA_NO_NET,               /* Not idle or connected */
  PS_HDR_REV0_RATE_INERTIA_FAILURE_REASON_MAX    /* DO NOT USE. Used for
                                                    bounds check */
} ps_hdr_rev0_rate_inertia_failure_code_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_HDR_MODE_FAILURE_REASON_CODE_ENUM_TYPE

DESCRIPTION
  Enum type for HDR mode failure
---------------------------------------------------------------------------*/
typedef enum
{
  PS_HDR_SLOTTED_MODE_REQUEST_REJECTED = 0, /* Request to change slot
                                               cycle index rejected        */
  PS_HDR_SLOTTED_MODE_REQUEST_FAILED_TX,    /* Failed transmission         */
  PS_HDR_SLOTTED_MODE_NOT_SUPPORTED,        /* Slotted mode not supported  */
  PS_HDR_SLOTTED_MODE_NO_NET,               /* Not idle or connected       */
  PS_HDR_SLOTTED_MODE_FAILURE_REASON_MAX    /* DO NOT USE. Used for
                                               bounds check                */
} ps_hdr_slotted_mode_failure_code_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_ADDR_FAMILY_TYPE

DESCRIPTION
  The type that is used to define the IP type that an interface supports.
---------------------------------------------------------------------------*/
typedef ip_addr_enum_type ps_iface_addr_family_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_BEARER_TECHNOLOGY_TYPE

DESCRIPTION
  The type that is used to define the bearer technology type.
---------------------------------------------------------------------------*/
typedef enum
{
  PS_IFACE_NETWORK_MIN = 0,
  PS_IFACE_NETWORK_CDMA,
  PS_IFACE_NETWORK_UMTS,
  PS_IFACE_NETWORK_3GPP = PS_IFACE_NETWORK_UMTS,         /* Alias for UMTS */
  PS_IFACE_NETWORK_WLAN,
  PS_IFACE_NETWORK_MAX
} ps_iface_network_type;

typedef struct
{
  /*-------------------------------------------------------------------------
    rat_mask specifies the type of technology
  -------------------------------------------------------------------------*/
  uint32  rat_mask;
  #define PS_IFACE_CDMA_1X               0x01
  #define PS_IFACE_CDMA_EVDO_REV0        0x02
  #define PS_IFACE_CDMA_EVDO_REVA        0x04
  #define PS_IFACE_CDMA_EVDO_REVB        0x08
  #define PS_IFACE_CDMA_NULL_BEARER      0x8000

  /*-------------------------------------------------------------------------
    so_mask specifies the service option or type of application
  -------------------------------------------------------------------------*/
  uint32  so_mask;

  /*-------------------------------------------------------------------------
    Following are 1x specific so_masks
  -------------------------------------------------------------------------*/
  #define PS_IFACE_CDMA_1X_IS95                0x01
  #define PS_IFACE_CDMA_1X_IS2000              0x02
  #define PS_IFACE_CDMA_1X_IS2000_REL_A        0x04

  /*-------------------------------------------------------------------------
    Following are specific so_masks for Packet Applications on HDR

    The following table gives which of the following so_masks are supported
    by which technology
          +-------------------+----------------------------------+
          | Rev0 supports     |    PS_IFACE_CDMA_EVDO_DPA        |
          +-------------------+----------------------------------+
          | RevA supports     |    PS_IFACE_CDMA_EVDO_DPA        |
          |                   |    PS_IFACE_CDMA_EVDO_MFPA       |
          |                   |    PS_IFACE_CDMA_EVDO_EMPA       |
          |                   |    PS_IFACE_CDMA_EVDO_EMPA_EHRPD |
          +-------------------+----------------------------------+
          | RevB supports     |    PS_IFACE_CDMA_EVDO_DPA        |
          |                   |    PS_IFACE_CDMA_EVDO_MFPA       |
          |                   |    PS_IFACE_CDMA_EVDO_EMPA       |
          |                   |    PS_IFACE_CDMA_EVDO_EMPA_EHRPD |
          |                   |    PS_IFACE_CDMA_EVDO_MMPA       |
          |                   |    PS_IFACE_CDMA_EVDO_MMPA_EHRPD |
          +-------------------+----------------------------------+
  -------------------------------------------------------------------------*/
  #define PS_IFACE_CDMA_EVDO_DPA          0x01
  #define PS_IFACE_CDMA_EVDO_MFPA         0x02
  #define PS_IFACE_CDMA_EVDO_EMPA         0x04
  #define PS_IFACE_CDMA_EVDO_EMPA_EHRPD   0x08
  #define PS_IFACE_CDMA_EVDO_MMPA         0x10
  #define PS_IFACE_CDMA_EVDO_MMPA_EHRPD   0x20

  /*-------------------------------------------------------------------------
    DEPRECATE: As packet Applications are not dependent on Rev0/RevA/RevB
               Use the ones defined.
    Following are DoRA specific so_masks
  -------------------------------------------------------------------------*/
  #define PS_IFACE_CDMA_EVDO_REVA_DPA          PS_IFACE_CDMA_EVDO_DPA
  #define PS_IFACE_CDMA_EVDO_REVA_MFPA         PS_IFACE_CDMA_EVDO_MFPA
  #define PS_IFACE_CDMA_EVDO_REVA_EMPA         PS_IFACE_CDMA_EVDO_EMPA
  #define PS_IFACE_CDMA_EVDO_REVA_EMPA_EHRPD   PS_IFACE_CDMA_EVDO_EMPA_EHRPD

} ps_iface_network_cdma_type;

typedef struct
{
  uint32  rat_mask;
  /*-------------------------------------------------------------------------
    ATTENTION: Following definitions are DEPRECATED. Add any new deifnitions
    to the "PS_IFACE_3GPP" section below
  -------------------------------------------------------------------------*/

  #define PS_IFACE_UMTS_WCDMA             0x01
  #define PS_IFACE_UMTS_GPRS              0x02
  #define PS_IFACE_UMTS_HSDPA             0x04
  #define PS_IFACE_UMTS_HSUPA             0x08
  #define PS_IFACE_UMTS_EDGE              0x10

  /*-------------------------------------------------------------------------
    Aliases to all the above deprecated definitions.
  -------------------------------------------------------------------------*/
  #define PS_IFACE_3GPP_WCDMA             PS_IFACE_UMTS_WCDMA
  #define PS_IFACE_3GPP_GPRS              PS_IFACE_UMTS_GPRS
  #define PS_IFACE_3GPP_HSDPA             PS_IFACE_UMTS_HSDPA
  #define PS_IFACE_3GPP_HSUPA             PS_IFACE_UMTS_HSUPA
  #define PS_IFACE_3GPP_EDGE              PS_IFACE_UMTS_EDGE
  #define PS_IFACE_3GPP_LTE               0x20
  #define PS_IFACE_3GPP_HSDPAPLUS         0x40 /* HSDPA+ hs_status */
  #define PS_IFACE_3GPP_DC_HSDPAPLUS      0x80 /* Dual Carrier HSDPA+
                                                         hs_status */
  #define PS_IFACE_3GPP_NULL_BEARER       0x8000
} ps_iface_network_umts_type;

typedef struct
{
  ps_iface_network_type  current_network;
  union
  {
    ps_iface_network_cdma_type      cdma_type;
    ps_iface_network_umts_type      umts_type;
  }data;
} ps_iface_bearer_technology_type;

/*---------------------------------------------------------------------------
  Serving mode change information.
---------------------------------------------------------------------------*/
typedef struct
{
  ps_iface_bearer_technology_type      old_bearer_tech;
  ps_iface_bearer_technology_type      new_bearer_tech;
} ps_iface_ioctl_bearer_tech_changed_type;

/*---------------------------------------------------------------------------
  Extended IP config event data
---------------------------------------------------------------------------*/
typedef boolean ps_iface_ioctl_extended_ip_config_type;

/*---------------------------------------------------------------------------
  The bearer data rate information. A value of -1 would indicate
  an unknown value.
  max Tx/Rx - As defined by the technology.
  avg Tx/Rx - Rate at which data was transferred over the last sampling
              period (currently 1 second)
  current Tx/Rx - Estimate of the rate that can be supported at this time.
---------------------------------------------------------------------------*/

typedef struct
{
  int32 max_tx_bearer_data_rate;      /*Max Tx bearer data rate*/
  int32 max_rx_bearer_data_rate;      /*Max Rx bearer data rate*/
  int32 avg_tx_bearer_data_rate;      /*Average Tx bearer data rate*/
  int32 avg_rx_bearer_data_rate;      /*Average Rx bearer data rate*/
  int32 current_tx_bearer_data_rate;  /*Current Tx bearer data rate*/
  int32 current_rx_bearer_data_rate;  /*Current Rx bearer data rate*/
} ps_iface_data_bearer_rate;

/*---------------------------------------------------------------------------
  Outage notification information
---------------------------------------------------------------------------*/
typedef struct
{
  uint32  time_to_outage;      /* milliseconds in which outage will start  */
  uint32  duration;            /* milliseconds for which outage will last  */
} ps_iface_outage_notification_event_info_type;

/*---------------------------------------------------------------------------
  Enum to indicate the DOS ack status code. The pkt mgr receives the status
  from CM and maps it to this enum. Changes to CM/Pkt mgr may necessitate
  modifying this enum definition.
---------------------------------------------------------------------------*/
typedef enum ps_phys_link_dos_ack_status_enum_type
{
  PS_PHYS_LINK_DOS_ACK_NONE = -1,           /* completed successfully??SURE?? */
  PS_PHYS_LINK_DOS_ACK_OK = 0,                   /* completed successfully */
  PS_PHYS_LINK_DOS_ACK_HOLD_ORIG_RETRY_TIMEOUT = 1,  /* hold orig retry
                                                        timeout            */
  PS_PHYS_LINK_DOS_ACK_HOLD_ORIG = 2,   /* cannot process because hold orig
                                           is true                         */
  PS_PHYS_LINK_DOS_ACK_NO_SRV = 3,              /* no service              */
  PS_PHYS_LINK_DOS_ACK_ABORT = 4,               /* abort                   */
  PS_PHYS_LINK_DOS_ACK_NOT_ALLOWED_IN_AMPS = 5, /* cannot send in analog mode*/
  PS_PHYS_LINK_DOS_ACK_NOT_ALLOWED_IN_HDR = 6,  /* cannot send in HDR call */
  PS_PHYS_LINK_DOS_ACK_L2_ACK_FAILURE = 7,      /* failure receiving L2 ack */
  PS_PHYS_LINK_DOS_ACK_OUT_OF_RESOURCES = 8,    /* e.g., out of memory buffer*/
  PS_PHYS_LINK_DOS_ACK_ACCESS_TOO_LARGE = 9,    /* msg too large to be sent
                                                   over acc                */
  PS_PHYS_LINK_DOS_ACK_DTC_TOO_LARGE = 10,      /* msg too large to be sent
                                                   over DTC                */
  PS_PHYS_LINK_DOS_ACK_OTHER = 11,              /* any status response other
                                                   than above              */
  PS_PHYS_LINK_DOS_ACK_ACCT_BLOCK = 12,         /* Access blocked based on
                                                   service option          */
  PS_PHYS_LINK_DOS_ACK_L3_ACK_FAILURE = 13,    /* failure receiving L3 ack */
  PS_PHYS_LINK_DOS_ACK_FORCE_32_BIT = 0x7FFFFFFF
} ps_phys_link_dos_ack_status_enum_type;

/*---------------------------------------------------------------------------
  The structure containing information about the sdb ack status. This info
  is returned to the socket application in the sdb ack callback if the sdb
  ack callback socket option is set.
---------------------------------------------------------------------------*/
typedef struct
{
  uint32                                 overflow;
  ps_phys_link_dos_ack_status_enum_type  status;
} ps_phys_link_dos_ack_status_info_type;

/*---------------------------------------------------------------------------
  This structure defines a type to notify of RF condition in a cell
---------------------------------------------------------------------------*/
typedef enum
{
  PS_IFACE_RF_CONDITIONS_MIN       = 0,
  PS_IFACE_RF_CONDITIONS_INVALID   = PS_IFACE_RF_CONDITIONS_MIN,
  PS_IFACE_RF_CONDITIONS_BAD       = 1,
  PS_IFACE_RF_CONDITIONS_GOOD      = 2,
  PS_IFACE_RF_CONDITIONS_DONT_CARE = 3,
  PS_IFACE_RF_CONDITIONS_MAX
} ps_iface_rf_conditions_enum_type;

typedef struct
{
  ps_iface_bearer_technology_type   bearer_tech;
  ps_iface_rf_conditions_enum_type  rf_conditions;
} ps_iface_rf_conditions_info_type;

/*---------------------------------------------------------------------------
  Domain name type. Used for SIP/DNS server domain names.
---------------------------------------------------------------------------*/
typedef struct
{
  char domain_name[PS_IFACE_MAX_DOMAIN_NAME_SIZE];
} ps_iface_domain_name_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_PREFIX_UPDATE_ENUM_TYPE

DESCRIPTION
  Contains the enums describing the action for the accompanying prefix.
---------------------------------------------------------------------------*/

typedef enum
{
  PREFIX_ADDED      = 0, /* Added a prefix          */
  PREFIX_REMOVED    = 1, /* Removed a prefix        */
  PREFIX_DEPRECATED = 2, /* State of prefix changed */
  PREFIX_UPDATED    = 3  /* Lifetimes of prefix have been updated */
} ps_iface_prefix_update_enum_type;

/*===========================================================================
                            FORWARD DECLARATIONS
===========================================================================*/
typedef struct ps_iface_s      ps_iface_type;
typedef struct ps_flow_s       ps_flow_type;
typedef struct ps_phys_link_s  ps_phys_link_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_IPFLTR_CLIENT_ID_ENUM_TYPE

DESCRIPTION
  Client Ids for the users of filtering library.
  NOTE : INPUT filter clients must have a client ID between 0 and
         IP_FLTR_CLIENT_INPUT_MAX
---------------------------------------------------------------------------*/
typedef enum
{
  IP_FLTR_CLIENT_SOCKETS      = 0,  /* Incoming pkt filtering for sockets  */
  IP_FLTR_CLIENT_QOS_INPUT    = 1,  /* Rx Pkt classification fltrs for QOS */
  IP_FLTR_CLIENT_IPSEC_INPUT  = 2,  /* IPSEC filters for input processing  */
  IP_FLTR_CLIENT_FIREWALL_INPUT  = 3,  /* Firewall filters for input
                                                                processing */
  IP_FLTR_CLIENT_INPUT_MAX,
  IP_FLTR_CLIENT_QOS_OUTPUT   = 4,  /* Tx Pkt classification fltrs for QOS */
  IP_FLTR_CLIENT_IPSEC_OUTPUT = 5,  /* IPSEC filters for output processing */
  IP_FLTR_CLIENT_HEADER_COMP  = 6,  /* Header compression protocol         */
  IP_FLTR_CLIENT_MAX
} ps_iface_ipfltr_client_id_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_DORM_ACTION_ENUM_TYPE

DESCRIPTION
  List of all the possible dormancy actions that phys link may want to do
  and may ask iface if it's okay to do that action.
---------------------------------------------------------------------------*/
typedef enum
{
  IFACE_DORM_ACTION_MIN = 0,      /* DO NOT USE.  Used for bounds checking */
  IFACE_DORM_ACTION_OK_TO_ORIG,   /* is it okay to originate from dormancy */
  IFACE_DORM_ACTION_OK_TO_GO_DORM,/* is it ok to go dormant                */
  IFACE_DORM_ACTION_MAX           /* DO NOT USE.  Used for bounds checking */
} ps_iface_dorm_action_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_DORM_REASON_ENUM_TYPE

DESCRIPTION
  Contains reasons phys link wants to originate from dormancy.
---------------------------------------------------------------------------*/
typedef enum
{
  IFACE_DORM_REASON_MIN = 0,      /* DO NOT USE.  Used for bounds checking */
  IFACE_DORM_REASON_DATA,         /* has data to send                      */
  IFACE_DORM_REASON_PZ_S_NID,     /* PZID/SID/NID changed                  */
  IFACE_DORM_REASON_NETWORK,      /* network changed                       */
  IFACE_DORM_REASON_MAX           /* DO NOT USE.  Used for bounds checking */
} ps_iface_dorm_reason_enum_type;

typedef enum
{
  PS_FAST_DORMANCY_STATUS_SUCCESS       = 1,
  PS_FAST_DORMANCY_STATUS_FAILURE_RETRY = 2
} ps_fast_dormancy_status_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_PREFIX_UPDATE_TYPE

DESCRIPTION
  This contains all of the prefix information related to the update.

MEMBERS:
  prefix: the prefix that changed.
  n_prefixes: the number of prefixes
  kind: added, changed, state change (i.e. preferred -> deprecated)
---------------------------------------------------------------------------*/
typedef struct
{
  struct ps_in6_addr prefix;
  ps_iface_prefix_update_enum_type kind;
  uint8 prefix_len;
} ps_iface_prefix_update_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_PREFIX_INFO_TYPE

DESCRIPTION
  This contains all of the prefix information.

MEMBERS:
  prefix:       the prefix that changed
  prefix_state: the state of the prefix
  prefix_len:   the length of the prefix
---------------------------------------------------------------------------*/
typedef struct
{
  struct ps_in6_addr                 prefix;
  ps_iface_ipv6_addr_state_enum_type prefix_state;
  uint8                              prefix_len;
} ps_iface_prefix_info_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_PRIV_IPV6_ADDR_INFO_TYPE

DESCRIPTION
  This contains the private IPv6 address and type.

MEMBERS:
  ip_addr: the IPv6 privacy address.
  is_unique: whether the address is private shared or private unique
---------------------------------------------------------------------------*/
typedef struct
{
  ps_ip_addr_type   ip_addr;
  boolean           is_unique;
} ps_iface_priv_ipv6_addr_info_type;

typedef struct
{
  ps_iface_mcast_handle_type             handle;
  ps_iface_mcast_info_code_enum_type     info_code;
  boolean                                force_dereg_cbacks;
} ps_iface_mcast_event_info_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_EVENT_INFO_U_TYPE

DESCRIPTION
  Data structure that is used to pass event specific information into the
  callback functions that are registered for the events.

MEMBERS
  state: This will be set when the interface state change(*) callback is
    called and will contain the previous state the interface was in.
    (*) This applies to the IFACE_DOWN_EV and IFACE_UP_EV events.
  link_state: This will be set when the physical link state change(*)
    callback is called and will contain the previous state the link was in.
    (*) this applies to the IFACE_PHYS_LINK_DOWN_EV and IFACE_PHYS_LINK_UP_EV
  flow_mask: This will be set when the flow state(*) is changed, that is,
    flow is enabled, or disabled.  It will contain the previous flow_mask.
    (*) This applies to the IFACE_FLOW_ENABLED_EV and IFACE_FLOW_DISABLED_EV
  ip_addr: This will be set when the IP address of the iface changes.
    Will be set to the previous IP address.
    (*) This applies to IFACE_ADDR_CHANGED_EV.
  ipfltr_info: This will be set when the number of filters installed in the
    iface for a particular client changes. "cnt" will be set to the previous
    number of filters in the iface for this "client".
    (*) This applies to IFACE_IPFLTR_UPDATED_EV.
  prefix_info: the information related to the prefix update.  Applies to
    IFACE_PREFIX_UPDATE_EV.
  phys_link_event_info: same as link_state but also contains extended
    information code to describing the cause of the state change.
  pri_phys_link_ptr: indicates previous primary phys link, applies to
  IFACE_PRI_PHYS_LINK_CHANGED_EV.
 addr_family_info: This will be set when address family of iface changes.
    (*) This applies to IFACE_ADDR_FAMILY_CHANGED_EV.
 iface_down_info:  This will be set to indicate the motivation for the
    PS_iface going down during the down indication.
bearer_tech_changed_info: This would be set when the bearer technology
  changes.
  (*) This applies to IFACE_BEARER_TECH_CHANGED_EV.

---------------------------------------------------------------------------*/
typedef union
{
  struct
  {
    ps_iface_ipfltr_client_id_enum_type  client;
    int                                  curr_fltr_cnt;
  } ipfltr_info;

  struct
  {
    ps_flow_type  * flow_ptr;
    uint8           prev_cnt;
    uint8           new_cnt;
  } rx_fltr_info;

  struct
  {
    ps_flow_state_enum_type          state;
    ps_extended_info_code_enum_type  info_code;
  } flow_event_info;

  struct
  {
    phys_link_state_type             state;
    ps_extended_info_code_enum_type  info_code;
  } phys_link_event_info;

  struct
  {
    boolean  is_modify_succeeded;
  } primary_qos_modify_result_info;

  struct
  {
    ps_phys_link_type                * pri_phys_link_ptr;
    ps_extended_info_code_enum_type    info_code;
  } pri_changed_info;

  struct
  {
    ps_iface_addr_family_type  old_addr_family;
    ps_iface_addr_family_type  new_addr_family;
  } addr_family_info;

  ps_iface_ioctl_bearer_tech_changed_type  bearer_tech_changed_info;

  struct
  {
    int32                                  handle;
    ps_phys_link_dos_ack_status_info_type  status_info;
  } dos_ack_info;

  /*---------------------------------------------------------------------------
    Event info associated with IFACE_FLOW_ADDED_EV/IFACE_FLOW_DELETED_EV.
    Indicates the ps flow which got added/deleted
  ---------------------------------------------------------------------------*/
  ps_flow_type                          * flow_ptr;
  ps_iface_priv_ipv6_addr_info_type       priv_ipv6_addr;
  ps_iface_prefix_update_type             prefix_info;
  ps_ip_addr_type                         ip_addr;
  uint32                                  flow_mask;
  ps_iface_mt_handle_type                 mt_handle;
  ps_iface_mcast_event_info_type          mcast_info;
  ps_iface_state_enum_type                state;
  ps_extended_info_code_enum_type         qos_aware_info_code;
  ps_iface_rf_conditions_info_type        rf_conditions_change_info;

  ps_iface_outage_notification_event_info_type  outage_notification_info;
  ps_hdr_rev0_rate_inertia_failure_code_enum_type
                                    hdr_rev0_rate_inertia_failure_code;
  ps_iface_ioctl_extended_ip_config_type  extended_ip_config_succeeded;

  struct
  {
    ps_hdr_slotted_mode_failure_code_enum_type  hdr_slotted_mode_failure_code;
    uint8                                       sm_current_sci;
  } slotted_mode_info;

  struct
  {
    ps_iface_state_enum_type       state;
    ps_iface_net_down_reason_type  netdown_reason;
  } iface_down_info;

  struct
  {
    ps_fast_dormancy_status_enum_type  dorm_status;
  } fast_dorm_status;
  
  uint16                      ra_lifetime;
  uint8                       n_filters;
} ps_iface_event_info_u_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_STATS_TYPE
---------------------------------------------------------------------------*/
typedef struct
{
  /*-------------------------------------------------------------------------
    IP packets received/transmitted
  -------------------------------------------------------------------------*/
  uint32 rx_pkts;
  uint32 tx_pkts;

  /*-------------------------------------------------------------------------
    physical link statistics - bytes RX/TX by physical layer (e.g. RLP/RLC)
  -------------------------------------------------------------------------*/
  struct
  {
    uint32 rx_bytes;
    uint32 tx_bytes;
  } phys_link;

} ps_iface_stats_type;

/*---------------------------------------------------------------------------
TYPEDEF IPV6_IID_ENUM_TYPE

DESCRIPTION
  Enum type to determine whether the interface ID for an IPv6 interface is
  a random or user supplied.
---------------------------------------------------------------------------*/
typedef enum
{
  IPV6_RANDOM_IID = 0,
  IPV6_USER_IID   = 1
} ps_iface_ipv6_iid_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_IP_VER_FAILOVER_E_TYPE

DESCRIPTION
  Mapping of the NV item for failover behavior.
---------------------------------------------------------------------------*/
typedef enum
{
  IPV4_ONLY                             = 0,
  IPV6_ONLY                             = 1,
  IPV6_PREFERRED                        = 2,
  IPV4_PREFERRED                        = 3,
  IPV6_DESIRED                          = 4,
  IPV4_DESIRED                          = 5
} ps_iface_ip_ver_failover_e_type;

/*---------------------------------------------------------------------------
TYPEDEF PS_IFACE_BEARER_IP_TYPE

DESCRIPTION
  This enum defines the supported bearer on a given ps_iface
---------------------------------------------------------------------------*/
typedef enum
{
  PS_IFACE_BEARER_IP_V4_ONLY = 0,
  PS_IFACE_BEARER_IP_V6_ONLY,
  PS_IFACE_BEARER_IP_SINGLE_BEARER,
  PS_IFACE_BEARER_IP_DUAL_BEARER
} ps_iface_bearer_ip_type;

/*-------------------------------------------------------------------------
  These structures are used to notify eHRPD device manager about
  iface teardown reason.
  Device manager uses this information to differentiate whether to use
  IRAT partial context timer or standard partial context timer.
-------------------------------------------------------------------------*/
typedef enum
{
  /*-------------------------------------------------------------------------
    This value indicates that iface is torn down because of handoff as
    opposed to normal tear down.
  -------------------------------------------------------------------------*/
  PS_IFACE_TEAR_DOWN_REASON_NONE    = 0,
  PS_IFACE_TEAR_DOWN_REASON_HANDOFF = 1
} ps_iface_tear_down_reason_enum_type;

typedef struct
{
  /*-------------------------------------------------------------------------
    Iface teardown reason.
  -------------------------------------------------------------------------*/
  ps_iface_tear_down_reason_enum_type tear_down_reason;
} ps_iface_tear_down_info_type;

/*---------------------------------------------------------------------------
  Defines of the default values for IPV6 enabled and the Failover
  configuration NV items should they not be set.
---------------------------------------------------------------------------*/
#define IPV6_ENABLED_DEFAULT TRUE
#define IP_VER_FAILOVER_DEFAULT IPV6_DESIRED

/* Def from ds_flow_control.h
   - defined to avoid API level churn from ModemData
     (See DS_FLOW_IS_ENABLED)
 */
#define ALL_FLOWS_ENABLED     0x00000000  /* All flows enabled flag  */

#endif /* PS_IFACE_DEFS_H */

