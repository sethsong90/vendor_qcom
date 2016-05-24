#ifndef DSS_NETPOLICY_H
#define DSS_NETPOLICY_H
/*===========================================================================

         D A TA   S E R V I C E S   N E T W O R K   P O L I C Y  I N F O
                       H E A D E R  F I L E

DESCRIPTION

 The Network policy info Header File contains shared variables
 and enums, as well as declarations for functions related to network polciy.


 -----------------------------------------------------------------------------
 Copyright (c) 2003-2010 Qualcomm Technologies, Inc. 
 All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
 -----------------------------------------------------------------------------

===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $Header: //linux/pkgs/proprietary/data/main/source/dss/inc/dss_netpolicy.h#2 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
05/28/08    vk     Added support for APN override
02/05/07    sv     Added IM_CN flag to dss_netpolicy.
03/30/04    vp     Merged changes from June04Dev_05.00.00 PS branch.
11/12/03    sv     Added is_routeable flag to dss_netpolicy.
10/28/03    sv     Added version macro for dss_get_iface_id_by_policy.
10/20/03    sv     Added dss_get_iface_id_by_policy function.
09/11/03    sv     Renamed private to dss_netpolicy_private.
08/28/03    ss     Moved declarations of iface name and id to dssocket_defs.h
07/17/03    sv     Moved netpolicy info data structures to new header file.

===========================================================================*/

/*===========================================================================

                      INCLUDE FILES FOR MODULE

===========================================================================*/
#if 0
#include "comdef.h"
#include "customer.h"
#endif

#ifdef FEATURE_DATA_PS
#if 0
#include "ps_iface_defs.h"
#include "dssocket_defs.h"
#endif
/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
#if 0
typedef enum
{
  DSS_IFACE_ID = 0,
  DSS_IFACE_NAME = 1
} dss_iface_id_kind_enum_type;
#endif

/*---------------------------------------------------------------------------
  dss_iface_id_type will be treated as opaque and typedef'ed to int
  16 bits for iface name, 8 bits for instance and 8 for phys link
  the user will get the iface_id by supplying iface and instance to a
  function that will return dss_iface_id_type. an attempt to get the
  iface_id for "ANY" iface will return error.
---------------------------------------------------------------------------*/

typedef struct
{
  dss_iface_id_kind_enum_type kind;
  union
  {
    dss_iface_id_type id;
    dss_iface_name_enum_type name;
  } info;
} dss_iface_type;

/*---------------------------------------------------------------------------
                       Network policy data structures.
---------------------------------------------------------------------------*/

#if 0
typedef enum
{
  DSS_IFACE_POLICY_ANY          = 0,
  DSS_IFACE_POLICY_UP_ONLY   = 1,
  DSS_IFACE_POLICY_UP_PREFERRED = 2
} dss_iface_policy_flags_enum_type;
#endif

/* Definitions to maintain backwards compatibility */
#define DSS_IFACE_POLICY_SPECIFIED  DSS_IFACE_POLICY_ANY
#define DSS_IFACE_POLICY_UP_SPEC    DSS_IFACE_POLICY_UP_PREFERRED


typedef struct
{
  dss_iface_policy_flags_enum_type policy_flag; /* Desired policy behavior */
  dss_iface_type                   iface;                 /* Desired IFACE */
  boolean                          ipsec_disabled;  /* Is IPSEC disabled ? */
  int                              family;   /* ipv4 or ipv6 or don't care */
  boolean                          is_routeable; /* Is interface routeable?*/
#if defined (FEATURE_DS_MOBILE_IP) && defined (FEATURE_DATA_PS_MIP_CCOA)
  boolean sip_iface_reqd;                   /* Physical iface required     */
#endif
  dss_string_type username;     /* username - added for RIL         */
  dss_string_type password;     /* password - added for RIL         */
  dss_auth_pref_type auth_pref; /* none/pap/chap/both               */
  dss_data_call_origin_type   data_call_origin; /*default, laptop, embedded*/
  struct
  {
    int pdp_profile_num;
    boolean im_cn_flag;                /* IM-CN flag for IMS               */
    dss_umts_apn_type apn;             /* APN - added for Google RIL       */
  } umts;                              /* UMTS specific policy information */

  struct
  {
    int data_session_profile_id;
  } cdma;                 /* CDMA specific data session policy information */

  struct
  {
    int cookie;
  } dss_netpolicy_private;         /* Private cookie for internal purposes */
} dss_net_policy_info_type;


/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================
FUNCTION DSS_GET_IFACE_ID_BY_POLICY()

DESCRIPTION
  This function return the interface id based on the specified network policy.

DEPENDENCIES
  None.

PARAMETERS
  dss_net_policy_info_type  net_policy_info: specifies the network policy for
  determining the interface.

RETURN VALUE
  iface_id: If a valid iface could be obtained based on policy
   On error, return DSS_IFACE_INVALID_ID and places the error condition value
   in *dss_errno.

  dss_errno Values
  ----------------
  DS_EFAULT      Netpolicy structure is not initialized.
  DS_ENOROUTE    No interface can be determined from the network policy.

SIDE EFFECTS
  None.
===========================================================================*/
#define DSS_GET_IFACE_ID_BY_POLICY_VERS (1)
dss_iface_id_type
dss_get_iface_id_by_policy
(
  dss_net_policy_info_type  net_policy_info,        /* Network policy info */
  sint15                  * dss_errno             /* error condition value */
);

#endif  /* FEATURE_DATA_PS */

#endif /* DSS_NETPOLICY_H */
