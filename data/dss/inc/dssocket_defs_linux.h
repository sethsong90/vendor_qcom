/******************************************************************************

                D S S O C K E T _ D E F S _ L I N U X . H

******************************************************************************/

/******************************************************************************

  @file    dssocket_defs_linux.h
  @brief   Misc. DSS definitions 

  DESCRIPTION
  Header file containing misc. DSS definitions.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //linux/pkgs/proprietary/data/main/source/dss/inc/dssocket_defs_linux.h#2 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
05/30/08   vk         Added support for GET_DEVICE_NAME IOCTL
05/22/08   vk         Added definitions for APN override support
09/28/07   vk         Initial version

******************************************************************************/

#ifndef __DSSOCKET_DEFS_LINUX_H__
#define __DSSOCKET_DEFS_LINUX_H__

/*---------------------------------------------------------------------------
                       Network policy data structures.
---------------------------------------------------------------------------*/
#define DSS_UMTS_APN_MAX_LEN  100
#define DSS_STRING_MAX_LEN    127

typedef struct {
    unsigned char length;
    char name[DSS_UMTS_APN_MAX_LEN];
} dss_umts_apn_type;        /* This is added for APN override
                               needed for google RIL          */
typedef struct {
  unsigned char length;
  char value[DSS_STRING_MAX_LEN];
} dss_string_type;

typedef enum
{
  DSS_AUTH_PREF_NOT_SPECIFIED = 0,
  DSS_AUTH_PREF_PAP_CHAP_NOT_ALLOWED,
  DSS_AUTH_PREF_PAP_ONLY_ALLOWED,
  DSS_AUTH_PREF_CHAP_ONLY_ALLOWED,
  DSS_AUTH_PREF_PAP_CHAP_BOTH_ALLOWED,
  DSS_AUTH_PREF_MAX
} dss_auth_pref_type;

typedef enum
{
  DSS_DATA_CALL_ORIGIN_DEFAULT         = 0,
  DSS_DATA_CALL_ORIGIN_EMBEDDED        = DSS_DATA_CALL_ORIGIN_DEFAULT,
  DSS_DATA_CALL_ORIGIN_LAPTOP          = 1,
  DSS_DATA_CALL_ORIGIN_MAX             = 2
}dss_data_call_origin_type;

typedef enum
{
  DSS_IFACE_POLICY_ANY          = 0,
  DSS_IFACE_POLICY_UP_ONLY   = 1,
  DSS_IFACE_POLICY_UP_PREFERRED = 2
} dss_iface_policy_flags_enum_type;

/*---------------------------------------------------------------------------
PS_IFACE_INVALID_ID - Id which does not identify any valid iface
---------------------------------------------------------------------------*/
#define PS_IFACE_INVALID_ID  0

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
  ANY_IFACE_GROUP    = 0x7FFF,

  /* Interface Names - these are bit masks, msb always set to 1 */
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
  LO_IFACE           = 0x8800

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
  Invalid iface_id: does not identify any valid iface
---------------------------------------------------------------------------*/
#define DSS_IFACE_INVALID_ID  (PS_IFACE_INVALID_ID)

/*---------------------------------------------------------------------------
  Definition of iface names
---------------------------------------------------------------------------*/
typedef enum
{
  DSS_IFACE_CDMA_SN      = CDMA_SN_IFACE,
  DSS_IFACE_CDMA_AN      = CDMA_AN_IFACE,
  DSS_IFACE_CDMA_BCAST   = CDMA_BCAST_IFACE,
  DSS_IFACE_FLO          = FLO_IFACE,
  DSS_IFACE_DVBH         = DVBH_IFACE,
  DSS_IFACE_UMTS         = UMTS_IFACE,
  DSS_IFACE_SIO          = SIO_IFACE,
  DSS_IFACE_LO           = LO_IFACE,
  DSS_IFACE_WLAN         = WLAN_IFACE,
  DSS_IFACE_WWAN         = WWAN_GROUP,
  DSS_IFACE_ANY_DEFAULT  = ANY_DEFAULT_GROUP,
  DSS_IFACE_ANY          = ANY_IFACE_GROUP,
  DSS_IFACE_RM           = RM_GROUP
} dss_iface_name_enum_type;

/*---------------------------------------------------------------------------
  opaque interface id
---------------------------------------------------------------------------*/

typedef unsigned long int dss_iface_id_type;

typedef enum
{
  DSS_IFACE_ID = 0,
  DSS_IFACE_NAME = 1
} dss_iface_id_kind_enum_type;

typedef enum
{
  DSS_IFACE_IOCTL_MIN_EV = 0,
  DSS_IFACE_IOCTL_REG_EVENT_MIN = 0,
  DSS_IFACE_IOCTL_DOWN_EV = DSS_IFACE_IOCTL_REG_EVENT_MIN,
  DSS_IFACE_IOCTL_UP_EV,
  DSS_IFACE_IOCTL_COMING_UP_EV,
  DSS_IFACE_IOCTL_GOING_DOWN_EV,
  DSS_IFACE_IOCTL_PHYS_LINK_DOWN_EV,
  DSS_IFACE_IOCTL_PHYS_LINK_UP_EV,
  DSS_IFACE_IOCTL_PHYS_LINK_COMING_UP_EV,
  DSS_IFACE_IOCTL_PHYS_LINK_GOING_DOWN_EV,
  DSS_IFACE_IOCTL_REG_EVENT_MAX = DSS_IFACE_IOCTL_PHYS_LINK_GOING_DOWN_EV,
  DSS_IFACE_IOCTL_EVENT_MAX,
  DSS_IFACE_IOCTL_EVENT_FORCE_32_BIT = 0x7FFFFFFF /* Force 32bit enum type */
} dss_iface_ioctl_event_enum_type;

/*---------------------------------------------------------------------------
  DSS_IFACE_IOCTL_DOWN_EV and DSS_IFACE_IOCTL_UP_EV parameter type
  - ps iface state
---------------------------------------------------------------------------*/
typedef ps_iface_state_enum_type dss_iface_ioctl_event_info_type;

/*---------------------------------------------------------------------------
  DSS_IFACE_IOCTL_PHYS_LINK_DOWN_EV and DSS_IFACE_IOCTL_PHYS_LINK_UP_EV
  parameter type
  - physical link state
---------------------------------------------------------------------------*/
typedef phys_link_state_type dss_iface_ioctl_phys_link_event_info_type;

typedef union
{
  dss_iface_ioctl_event_info_type              iface_state_info;
  dss_iface_ioctl_phys_link_event_info_type    phys_link_state_info;
} dss_iface_ioctl_event_info_union_type;
/*~ CASE DSS_IFACE_IOCTL_DOWN_EV dss_iface_ioctl_event_info_union_type.iface_state_info */
/*~ CASE DSS_IFACE_IOCTL_UP_EV dss_iface_ioctl_event_info_union_type.iface_state_info */
/*~ CASE DSS_IFACE_IOCTL_COMING_UP_EV dss_iface_ioctl_event_info_union_type.iface_state_info */
/*~ CASE DSS_IFACE_IOCTL_GOING_DOWN_EV dss_iface_ioctl_event_info_union_type.iface_state_info */
/*~ CASE DSS_IFACE_IOCTL_PHYS_LINK_DOWN_EV dss_iface_ioctl_event_info_union_type.phys_link_state_info */
/*~ CASE DSS_IFACE_IOCTL_PHYS_LINK_UP_EV dss_iface_ioctl_event_info_union_type.phys_link_state_info */
/*~ CASE DSS_IFACE_IOCTL_PHYS_LINK_COMING_UP_EV dss_iface_ioctl_event_info_union_type.phys_link_state_info */
/*~ CASE DSS_IFACE_IOCTL_PHYS_LINK_GOING_DOWN_EV dss_iface_ioctl_event_info_union_type.phys_link_state_info */

/*---------------------------------------------------------------------------
  Typedef for event callback function
  (DSS_IFACE_IOCTL_REG_EVENT_CB/ DSS_IFACE_IOCTL_DEREG_EVENT_CB)
---------------------------------------------------------------------------*/
typedef void (*dss_iface_ioctl_event_cb)
(
  dss_iface_ioctl_event_enum_type          event,
  dss_iface_ioctl_event_info_union_type    event_info,
  void                                     *user_data,
  signed short int                         dss_nethandle,
  dss_iface_id_type                        iface_id
);

typedef enum
{
  PS_IFACE_IOCTL_MIN                      = 0,

  /*-------------------------------------------------------------------------

                              COMMON IOCTLS

    The following operations are common to all interfaces.
  -------------------------------------------------------------------------*/
  PS_IFACE_IOCTL_GET_IPV4_ADDR            = 0, /* Get IPV4 addr of iface   */
  PS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR   = 4, /* Get Primary DNS (IPV4) addr
                                                                  of iface */
  PS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR   = 8, /* Get Secondary DNS (IPV4)
                                                             addr of iface */
  PS_IFACE_IOCTL_GET_IPV4_GATEWAY_ADDR    = 16, /* Get gateway (IPV4)
                                                             addr of iface */
  PS_IFACE_IOCTL_GET_STATE                = 18,/* Get state of iface       */
  PS_IFACE_IOCTL_REG_EVENT_CB             = 20,/* Register callback for
                                                  events. Note that app can
                                                  have only one callback per
                                                  interface for each event */
  PS_IFACE_IOCTL_DEREG_EVENT_CB           = 21,/* Deregister event callback*/
  PS_IFACE_IOCTL_GET_IFACE_NAME           = 28, /*Get the iface name       */
} ps_iface_ioctl_type;

typedef enum
{
  PS_PHYS_LINK_IOCTL_GET_STATE            = 0x80000018, /* Get physical link
                                                           state               */
  PS_PHYS_LINK_IOCTL_GO_ACTIVE            = 0x20000013,  /* Physlink goes Active 
                                                           from Dormant        */
  PS_PHYS_LINK_IOCTL_GO_DORMANT           = 0x20000014  /*Physlink goes dormant*/
} ps_phys_link_ioctl_type;

typedef enum
{
  /*-------------------------------------------------------------------------

                              COMMON IOCTLS

    The following operations are common to all interfaces.
  -------------------------------------------------------------------------*/
  DSS_IFACE_IOCTL_MIN                     = PS_IFACE_IOCTL_MIN,
  /* Get IPV4 addr of iface */
  DSS_IFACE_IOCTL_GET_IPV4_ADDR           = PS_IFACE_IOCTL_GET_IPV4_ADDR,
  /* Get Primary DNS (IPV4) addr of iface */
  DSS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR  =
                                       PS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR,
  /* Get Secondary DNS (IPV4) addr of iface */
  DSS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR  =
                                       PS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR,

  /* Get gateway (IPV4) addr of iface */
  DSS_IFACE_IOCTL_GET_IPV4_GATEWAY_ADDR   = PS_IFACE_IOCTL_GET_IPV4_GATEWAY_ADDR,

  /* Get state of iface */
  DSS_IFACE_IOCTL_GET_STATE               = PS_IFACE_IOCTL_GET_STATE,

  /* Physlink goes dormant*/
  DSS_IFACE_IOCTL_GO_DORMANT              = PS_PHYS_LINK_IOCTL_GO_DORMANT,          

   /* Go active from dormant */
  DSS_IFACE_IOCTL_GO_ACTIVE               = PS_PHYS_LINK_IOCTL_GO_ACTIVE,  

  /* Get physical link state */
  DSS_IFACE_IOCTL_GET_PHYS_LINK_STATE     = PS_PHYS_LINK_IOCTL_GET_STATE,
  /* Register callback for events. Note that app can have only one callback
     per interface for each event */
  DSS_IFACE_IOCTL_REG_EVENT_CB            = PS_IFACE_IOCTL_REG_EVENT_CB,
  /* Deregister event callback */
  DSS_IFACE_IOCTL_DEREG_EVENT_CB          = PS_IFACE_IOCTL_DEREG_EVENT_CB,
  /* Get the iface name */
  DSS_IFACE_IOCTL_GET_IFACE_NAME          = PS_IFACE_IOCTL_GET_IFACE_NAME,

  /* Following are LINUX platform specific IOCTLs */

  /* Bind socket to iface */
  DSS_IFACE_IOCTL_BIND_SOCK_TO_IFACE,

  /* Get device name for iface */
  DSS_IFACE_IOCTL_GET_DEVICE_NAME,
  /*Dormancy indications ON/OFF*/
  DSS_IFACE_IOCTL_DORMANCY_INDICATIONS_ON,

  DSS_IFACE_IOCTL_DORMANCY_INDICATIONS_OFF,
  
  DSS_IFACE_IOCTL_GET_MTU,

  DSS_IFACE_IOCTL_GET_CURRENT_DATA_BEARER,
  
  DSS_IFACE_IOCTL_MAX
} dss_iface_ioctl_type;


/*---------------------------------------------------------------------------
TYPEDEF IP_ADDR_ENUM_TYPE

DESCRIPTION
  An enum that defines all of the address types supported - used to
  discriminate the union below.

  NOTE: The values are chosen to easy debugging.
---------------------------------------------------------------------------*/
typedef enum
{
  IP_ANY_ADDR     = 0,
  IPV4_ADDR       = 4,
  IPV6_ADDR       = 6,
  IP_ADDR_INVALID           = 255,
  IFACE_ANY_ADDR_FAMILY     = IP_ANY_ADDR,
  IFACE_IPV4_ADDR_FAMILY    = IPV4_ADDR,
  IFACE_IPV6_ADDR_FAMILY    = IPV6_ADDR,
  IFACE_UNSPEC_ADDR_FAMILY  = 8,
  IFACE_INVALID_ADDR_FAMILY = IP_ADDR_INVALID
} ip_addr_enum_type;

/*---------------------------------------------------------------------------
TYPEDEF IP_ADDR_TYPE

DESCRIPTION
  structure which is a discriminated union that defines the IP addresses that
  we support.
---------------------------------------------------------------------------*/
typedef struct ip_address
{
  ip_addr_enum_type type;

  union
  {
    unsigned long v4;
    unsigned long long v6[2];
  } addr;

/*~ FIELD ip_address.addr DISC ip_address.type */
/*~ CASE IPV4_ADDR ip_address.addr.v4 */
/*~ CASE IPV6_ADDR ip_address.addr.v6 */
} ip_addr_type;

/*---------------------------------------------------------------------------
TYPEDEF DNS_INFO

DESCRIPTION
  structure defines the dns addresses that we support.
---------------------------------------------------------------------------*/
typedef struct dns_address
{
   ip_addr_type      dns_primary;
   ip_addr_type      dns_secondary;
} dns_info;

typedef enum {
  DATA_BEARER_TECH_UNKNOWN = 0,
  DATA_BEARER_TECH_GPRS = 1,
  DATA_BEARER_TECH_EDGE = 2,
  DATA_BEARER_TECH_UMTS = 3,
  DATA_BEARER_TECH_IS95A = 4,
  DATA_BEARER_TECH_IS95B = 5,
  DATA_BEARERO_TECH_1xRTT =  6,
  DATA_BEARER_TECH_EVDO_0 = 7,
  DATA_BEARER_TECH_EVDO_A = 8,
  DATA_BEARER_TECH_HSDPA = 9,
  DATA_BEARER_TECH_HSUPA = 10,
  DATA_BEARER_TECH_HSPA = 11,
  DATA_BEARER_TECH_EVDO_B = 12,
  DATA_BEARER_TECH_EHRPD = 13,
  DATA_BEARER_TECH_LTE = 14
} dss_iface_ioctl_data_bearer_tech_type;

/*---------------------------------------------------------------------------
  DSS_IFACE_IOCTL_GET_IPV4_ADDR parameter type
  - Iface IPV4 address
---------------------------------------------------------------------------*/
typedef ip_addr_type dss_iface_ioctl_ipv4_addr_type;

/*---------------------------------------------------------------------------
  DSS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR parameter type
  - Iface IPV4 Primary DNS address
---------------------------------------------------------------------------*/
typedef ip_addr_type dss_iface_ioctl_ipv4_prim_dns_addr_type;

/*---------------------------------------------------------------------------
  DSS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR parameter type
  - Iface IPV4 Secondary DNS address
---------------------------------------------------------------------------*/
typedef ip_addr_type dss_iface_ioctl_ipv4_seco_dns_addr_type;

/*---------------------------------------------------------------------------
  DSS_IFACE_IOCTL_GET_IPV4_GATEWAY_ADDR parameter type
  - Iface IPV4 Gateway address
---------------------------------------------------------------------------*/
typedef ip_addr_type dss_iface_ioctl_ipv4_gateway_addr_type;

/*---------------------------------------------------------------------------
  DSS_IFACE_IOCTL_GET_STATE parameter type
  - Iface State
---------------------------------------------------------------------------*/
typedef ps_iface_state_enum_type dss_iface_ioctl_state_type;

/*---------------------------------------------------------------------------
  DSS_IFACE_IOCTL_PHYS_LINK_STATE parameter type
  - Phys link State
---------------------------------------------------------------------------*/
typedef phys_link_state_type dss_iface_ioctl_phys_link_state_type;

/*---------------------------------------------------------------------------
  DSS_IFACE_IOCTL_REG_EVENT_CB/DSS_IFACE_IOCTL_DEREG_EVENT_CB parameter type
  - registering/deregistering for event callbacks
---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
  Typedef for struct used with DSS_IFACE_IOCTL_REG_EVENT_CB and
  DSS_IFACE_IOCTL_DEREG_EVENT_CB. Note that the application must specify a
  valid APP_ID when registering/deregistering for event callbacks.
---------------------------------------------------------------------------*/
typedef struct
{
  dss_iface_ioctl_event_cb        event_cb;
  dss_iface_ioctl_event_enum_type event;
  void                            *user_data_ptr;
  sint15                          app_id;
#define dss_nethandle       app_id
} dss_iface_ioctl_ev_cb_type;

/*---------------------------------------------------------------------------
  DSS_IFACE_IOCTL_GET_IFACE_NAME parameter type
  - Iface name
---------------------------------------------------------------------------*/
typedef ps_iface_name_enum_type    dss_iface_ioctl_iface_name_type;

#define DS_MAX_UDS_PATH_LEN 128

typedef struct dss_iface_ioctl_bind_sock_to_iface_s {
    char                uds_path[DS_MAX_UDS_PATH_LEN];
} dss_iface_ioctl_bind_sock_to_iface_type;

#define DSS_MAX_DEVICE_NAME_LEN 128

typedef struct dss_iface_ioctl_get_device_name_s {
    char                device_name[DSS_MAX_DEVICE_NAME_LEN];
} dss_iface_ioctl_device_name_type;

#define DSS_GET_IFACE_ID_VERS (3)
dss_iface_id_type
dss_get_iface_id
(
  signed short int dss_nethandle
);

/*===========================================================================
FUNCTION DSS_IFACE_IOCTL()

DESCRIPTION
  This function dstermines the ps_iface_ptr associated with the passed in
  identifier (App ID). It then calls ps_iface_ioctl().

DEPENDENCIES
  None.

PARAMETERS

  dss_iface_id_type         - Interface ID on which the specified operations
                              is to be performed

  dss_iface_ioctl_type      - The operation name

  void*                     - Pointer to operation specific structure

  sint15*                   - Error code returned in case of failure (Error
                              values are those defined in dserrno.h)

                              DS_EBADF - Returned by dss_iface_ioctl() if the
                              specified id_ptr is invalid (i.e. id_ptr does
                              not map to a valid ps_iface_ptr).

                              DS_EINVAL - Returned by dss_iface_ioctl() when
                              the specified IOCTL does not belong to the
                              common set of IOCTLs and there is no IOCTL mode
                              handler registered for the specified interface.

                              DS_EOPNOTSUPP - Returned by the lower level
                              IOCTL mode handler when specified IOCTL is not
                              supported by the interface. For instance, this
                              would be returned by interfaces that do not
                              support a certain "iface specific common IOCTL"
                              (i.e. these are common IOCTLs, but the
                              implementation is mode specific, for example,
                              GO_DORMANT).

                              DS_EFAULT - This error code is returned if the
                              specified arguments for the IOCTL are incorrect
                              or if dss_iface_ioctl() or a mode handler
                              encounters an error while executing the IOCTL..
                              For instance, if the 1X interface cannot
                              "GO_DORMANT" it would return this error.

                              DS_NOMEMORY - This error code is returned if we
                              run out of buffers during execution.

RETURN VALUE
  0 - on success
  -1 - on failure

SIDE EFFECTS
  None.

===========================================================================*/
int dss_iface_ioctl
(
  dss_iface_id_type        iface_id,
  dss_iface_ioctl_type     ioctl_name,
  void                     *argval_ptr,
  signed short int         *dss_errno
);

#endif /* __DSSOCKET_DEFS_LINUX_H__ */
