/*===========================================================================
                        P S _ L O G G I N G _ D I A G . C

GENERAL DESCRIPTION
  This file contains data path logging request handler functions which are
  diag callback functions registered during powerup.  These callback
  functions are the interface between the DM (diagnostic tool) and the PS.
  Also provides access to the lookup table maintained locally.

EXTERNAL FUNCTIONS
  dpl_init()
    Registers functions with diag.

  dpli_set_ps_iface_ptr()
    Set ps_iface table in the lookup table.

  dpli_get_ps_iface_default_desc()
    Get default description of the ifname from the lookup table.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2004-2011 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary.
===========================================================================*/

/*===========================================================================
                        EDIT HISTORY FOR MODULE

 $Header: //source/qcom/qct/modem/datamodem/interface/utils/rel/11.03/src/ps_logging_diag.c#1 $
 $Author: zhasan $ $DateTime: 2011/06/17 12:02:33 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
03/22/11    kk     Adding description for new RmNet ifaces in DPL list.
10/01/10    sa     Added appropriate log messages before ASSERT(0).
07/09/10    sy     Fixed type-punned compiler warnings.
03/26/09    pp     CMI De-featurization.
21/11/08    am     Using new macros for DPL IID flags.
10/17/08    am     Fixed ANSI C warnings for bit-field usage.
09/04/08    rt     Added IWLAN iface for logging.
09/04/08    ssh    Removed featurization from ps_iface_dpl_lookup_table
09/01/08    ssh    Added desc for RmNet and UICC ifaces to lookup table
07/18/08    dm     Fixed compiler warnings
02/14/07    scb    Fixed Critical LINT errors
02/08/07    scb    Fixed Klocwork High errors
12/25/06    msr    Fixed broken secondary link logging
07/17/06    mp     Fixed logging of zero length DPL packets
06/05/06    mp     Moved DPL link logging control block from ps_iface to
                   ps_phys_link
04/04/06    jd     Cleaned up include files
02/22/06    msr    Using single critical section
02/06/06    msr    Updated for L4 tasklock/crit sections.
08/16/05    msr    Fixed PS_BRANCH_TASKFREE()
08/15/05    msr    Using ps_flow instead of ps_phys_link for flow_logging
05/31/05   aku/jd  Added WLAN iface to ifname list
05/12/05    ks     Fixed Lint errors.
03/14/05    ks     Fixed handling of wildcard for link logging
01/27/05    ssh    Changed include file name dsbyte.h to ps_byte.h
01/12/05    msr    Added code review comments.
01/10/05    msr    Added support for flow and link logging.
11/16/04    ks     Changed the check for duplicate IIDs in set logging
                   request.
11/16/04    msr    Changed the condition which checks if DPL_IID_IFNAME_MAX
                   is greater than MAX_SYSTEM_IFACES.
11/08/04    msr    Added the condition, which checks if DPL_IID_IFNAME_MAX is
                   greater than MAX_SYSTEM_IFACES.
11/2/04    ks/msr  Changed Diag cmd handler code's names.
11/1/04   msr/ks   Generating correct responses when diag tool
                   sends illegal requests
10/31/04  msr/ks   Clean up.
09/10/04  ks/msr   Major cleanup.
08/09/04    vd     created file.
===========================================================================*/


/*===========================================================================
                     INCLUDE FILES FOR MODULE
===========================================================================*/
#include "comdef.h"
#include "target.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#ifdef FEATURE_DATA_PS_DATA_LOGGING

#include "AEEstd.h"
#include "diagcmd.h"
#include "diagpkt.h"

#include "ps_iface.h"
#include "ps_phys_link.h"
#include "ps_byte.h"
#include "ps_logging_defs.h"
#include "ps_iface_logging.h"
#include "ps_logging_diag.h"
#include "ps_loggingi.h"
#include "ps_utils.h"
#include "ps_crit_sect.h"
#include "ds_Utils_DebugMsg.h"


/*===========================================================================

                        FORWARD DECLARATIONS

===========================================================================*/
static PACKED void *ps_logging_diag_reset_logging
(
  PACKED void  * req_pkt,
  uint16         pkt_len
);

static PACKED void *ps_logging_diag_get_supported_ifaces
(
  PACKED void  * req_pkt,
  uint16         pkt_len
);

static PACKED void *ps_logging_diag_get_iface_desc
(
  PACKED void  * req_pkt,
  uint16         pkt_len
);

static PACKED void *ps_logging_diag_set_iface_logging
(
  PACKED void  * req_pkt,
  uint16         pkt_len
);

static PACKED void *ps_logging_diag_get_supported_protocols
(
  PACKED void  * req_pkt,
  uint16         pkt_len
);



/*===========================================================================

                                  CONSTANTS

===========================================================================*/
/*---------------------------------------------------------------------------
  Size of header added to each DIAG request/response
---------------------------------------------------------------------------*/
#define DIAG_HDR_LEN  (sizeof(diagpkt_subsys_header_type))

/*---------------------------------------------------------------------------
  Offset in to DIAG header where sub system command code (SSCC) is found
---------------------------------------------------------------------------*/
#define DIAG_HDR_SSCC_OFFSET  2

/*---------------------------------------------------------------------------
  iid.link_instance values for ps_flows. Only 0 and 255 are supported. If it
  is 0, IID corresponds to default flow and if it is 255, IID corresponds
  to all secondary flows
---------------------------------------------------------------------------*/
#define DEFAULT_FLOW_INST  0x00
#define ALL_SEC_FLOW_INST  0xFF



/*===========================================================================

                                VARIABLES

===========================================================================*/
/*---------------------------------------------------------------------------
 diag command format:

  { 75 = SSIDDISPATCH, XX = SSID = 42 for DPL, YY = SSID_CMD, 0x00 }

   diag handler structure format:
   [ { handler }, { handler }, ..., { last handler } ]

   where { handler } is

   { lower ssid_cmd value limit,
     upper ssid_cmd value limit,
     ssid-specific command handler }
---------------------------------------------------------------------------*/
/*lint -save -e641 enum to int conversion is minor can be ignored */
static const diagpkt_user_table_entry_type ps_logging_diag_tbl[] =
{
  {
   PS_LOGGING_DIAG_RESET_LOGGING,
   PS_LOGGING_DIAG_RESET_LOGGING,
   ps_logging_diag_reset_logging
  },
  {
   PS_LOGGING_DIAG_GET_SUPPORTED_IFACES,
   PS_LOGGING_DIAG_GET_SUPPORTED_IFACES,
   ps_logging_diag_get_supported_ifaces
  },
  {
   PS_LOGGING_DIAG_GET_IFACE_DESC,
   PS_LOGGING_DIAG_GET_IFACE_DESC,
   ps_logging_diag_get_iface_desc
  },
  {
   PS_LOGGING_DIAG_SET_IFACE_LOGGING,
   PS_LOGGING_DIAG_SET_IFACE_LOGGING,
   ps_logging_diag_set_iface_logging
  },
  {
   PS_LOGGING_DIAG_GET_SUPPORTED_PROTOCOLS,
   PS_LOGGING_DIAG_GET_SUPPORTED_PROTOCOLS,
   ps_logging_diag_get_supported_protocols
  }
};

/*---------------------------------------------------------------------------
  PCAP link type values
---------------------------------------------------------------------------*/
typedef enum
{
  DPL_PCAP_INVALID      = 0x00000000,
  DPL_PCAP_IP_RAW       = 0x0000000C,
  DPL_PCAP_IP_CLASSICAL = 0x0000006A,
  DPL_PCAP_PPP          = 0x0F000032,
  DPL_PCAP_PPP_IN_HDLC  = 0x00000032,
  DPL_PCAP_ETHERNET     = 0x00000001,
  DPL_PCAP_WIDTH        = 0x0FFFFFFF  /* Ensure that enum occupies 4 bytes */
} dpl_pcap_linktype_enum_type;

/*---------------------------------------------------------------------------
  Type used to define lookup table
---------------------------------------------------------------------------*/
typedef struct ps_iface_dpl_lookup_info
{
  ps_iface_type         * ps_iface_ptr;
  char                    desc[DPL_IFNAME_DESC_S_LEN];
} ps_iface_dpl_lookup_info_type;

/*---------------------------------------------------------------------------
  Lookup table used to store default description of each ps_iface and
  pointer to each ps_iface.
---------------------------------------------------------------------------*/
static
  ps_iface_dpl_lookup_info_type ps_iface_dpl_lookup_table[DPL_IID_IFNAME_MAX] =
{
  /* ifname  ps_iface_ptr   description                                    */
  /* 0  */    {NULL,         "NULL"                      } /* Must not be
                                                             used          */
  /* 1  */    ,{NULL,         "CDMA SN IFACE 1"          }
  /* 2  */    ,{NULL,         "CDMA SN IFACE 2"          }  
  /* 3  */    ,{NULL,         "CDMA Um SN ANY PKT"       }
  /* 4  */    ,{NULL,         "CDMA Um SN ASYNC"         }
  /* 5  */    ,{NULL,         "CDMA An IPv4"             }
  /* 6  */    ,{NULL,         "UMTS PDP CONTEXT 0"       }
  /* 7  */    ,{NULL,         "UMTS PDP CONTEXT 1"       }
  /* 8  */    ,{NULL,         "UMTS PDP CONTEXT 2"       }
  /* 9  */    ,{NULL,         "Rm - SIO UMTS"            }
  /* 10 */    ,{NULL,         "Rm - SIO CDMA"            }
  /* 11 */    ,{NULL,         "CDMA Broadcast"           }
  /* 12 */    ,{NULL,         "IP Security"              }
  /* 13 */    ,{NULL,         "Loopback V4"              }
  /* 14 */    ,{NULL,         "Loopback V6"              }
  /* 15 */    ,{NULL,         "Rm Network 0"             }
  /* 16 */    ,{NULL,         "WLAN Network Infra"       }
  /* 17 */    ,{NULL,         "DVBH IPv4 Iface "         }
  /* 18 */    ,{NULL,         "DVBH IPv6 Iface "         }
  /* 19 */    ,{NULL,         "Rm Network 1"             }
  /* 20 */    ,{NULL,         "Rm Network 2"             }
  /* 21 */    ,{NULL,         "Rm Network 3"             }
  /* 22 */    ,{NULL,         "Rm Network 4"             }
  /* 23 */    ,{NULL,         "UICC"                     }
  /* 24 */    ,{NULL,         "IWLAN 3GPP2"              }
  /* 25 */    ,{NULL,         "IWLAN 3GPP PDP 0"         }
  /* 26 */    ,{NULL,         "IWLAN 3GPP PDP 1"         }
  /* 27 */    ,{NULL,         "IWLAN 3GPP PDP 2"         }
  /* 28 */    ,{NULL,         "STA"                      }
  /* 29 */    ,{NULL,         "UW_FMC"                   }
  /* 30 */    ,{NULL,         "Proxy Iface 0"            }
  /* 31 */    ,{NULL,         "Proxy Iface 1"            }
  /* 32 */    ,{NULL,         "Proxy Iface 2"            }
  /* 33 */    ,{NULL,         "Proxy Iface 3"            }
  /* 34 */    ,{NULL,         "Proxy Iface 4"            }
  /* 35 */    ,{NULL,         "LTE Default V4 Iface"     }
  /* 36 */    ,{NULL,         "LTE Default V6 Iface"     }
  /* 37 */    ,{NULL,         "LTE non default Iface 1"  }
  /* 38 */    ,{NULL,         "LTE non default Iface 2"  }
  /* 39 */    ,{NULL,         "LTE non default Iface 3"  }
  /* 40 */    ,{NULL,         "LTE non default Iface 4"  }
  /* 41 */    ,{NULL,         "LTE non default Iface 5"  }
  /* 42 */    ,{NULL,         "LTE non default Iface 6"  }
  /* 43 */    ,{NULL,         "CDMA SN IFACE 3"          }
  /* 44 */    ,{NULL,         "CDMA SN IFACE 4"          }
  /* 45 */    ,{NULL,         "CDMA SN IFACE 5"          }
  /* 46 */    ,{NULL,         "CDMA SN IFACE 6"          }
  /* 47 */    ,{NULL,         "CDMA SN IFACE 7"          }
  /* 48 */    ,{NULL,         "CDMA SN IFACE 8"          }
  /* 49 */    ,{NULL,         "NAT Iface 1"              }
  /* 50 */    ,{NULL,         "WLAN Network Adhoc"       }
  /* 51 */    ,{NULL,         "WLAN Network SoftAP"      }
  /* 52 */    ,{NULL,         "Rm Network 5"             }
  /* 53 */    ,{NULL,         "Rm Network 6"             }
  /* 54 */    ,{NULL,         "Rm Network 7"             }
  /* 55 */    ,{NULL,         "Rm Network 8"             }
  /* 56 */    ,{NULL,         "Rm Network 9"             }
};


/*===========================================================================

                           LOCAL MACRO DEFINITIONS

===========================================================================*/
/*===========================================================================
MACRO    DPLI_FILL_IID

DESCRIPTION
  Extract IID from request pkt and populate into a variable

PARAMETRS
  iid     : IID to be filled
  req_ptr : Pointer to data

RETURN VALUE
  None

DEPENDENCIES
  NONE

SIDE EFFECTS
  req_ptr will be incremented by four bytes.
===========================================================================*/
#define DPLI_FILL_IID( iid, req_ptr )                                \
  iid.link_instance = *req_ptr++;                                    \
  iid.protocol      = *req_ptr++;                                    \
  iid.ifname        = (dpl_iid_ifname_enum_type) *req_ptr++;         \
  iid.dpl_flags     = *req_ptr++;


/*===========================================================================

                           INTERNAL FUNCTIONS

===========================================================================*/

/*===========================================================================
FUNCTION    DPLI_IS_VALID_IID

DESCRIPTION
  Validates the IID

PARAMETRS
  iid    : IID to be validated
  ifname : DPL IID Iface name

RETURN VALUE
  TRUE on success
  FALSE otherwise

DEPENDENCIES
  NONE

SIDE EFFECTS
  NONE
===========================================================================*/
static boolean dpli_is_valid_iid
(
  dpl_iid_type              iid,
  dpl_iid_ifname_enum_type  ifname
)
{
  ps_iface_type      * ps_iface_ptr;
  ps_flow_type       * flow_ptr;
  ps_phys_link_type  * phys_link_ptr;
  uint8                n_links;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Reserved field is not used in current definition and must be zero
  -------------------------------------------------------------------------*/
  if (IS_DPL_IID_RESERVED_NONZERO(iid))
  {
    LOG_MSG_ERROR("Invalid reserved bits in IID", 0, 0, 0);
    return FALSE;
  }

  /*-------------------------------------------------------------------------
    ifname in IID must match with the ifname passed in request pkt. IIDs
    which donot match are ignored by set_iface_logging.
  -------------------------------------------------------------------------*/
  if (ifname != iid.ifname)
  {
    LOG_MSG_ERROR("Invalid interface name in IID", 0, 0, 0);
    return FALSE;
  }

  if ( (ifname < DPL_IID_IFNAME_MIN) ||
       (ifname >= DPL_IID_IFNAME_MAX) )
  {
    LOG_MSG_ERROR("Invalid ifname %d", ifname, 0, 0);
    return FALSE;
  }

  ps_iface_ptr = ps_iface_dpl_lookup_table[ifname].ps_iface_ptr;
  ASSERT(PS_IFACE_IS_VALID(ps_iface_ptr));

  /*-------------------------------------------------------------------------
    Validate IID for network logging if L bit is DPL_IID_L_BIT_NETWORK and
    F bit is DPL_IID_FLOW_BIT_NONFLOW. Validate IID for flow logging if
    L bit is DPL_IID_L_BIT_NETWORK and F bit is DPL_IID_FLOW_BIT_FLOW.
    Validate IID for link logging if L bit is DPL_IID_FLOW_BIT_LINK
  -------------------------------------------------------------------------*/
  if (IS_DPL_IID_L_BIT_NETWORK(iid))
  {
    if (IS_DPL_IID_F_BIT_NONFLOW(iid))
    {
      if (ifname != ps_iface_ptr->dpl_net_cb.tx_dpl_id.ifname)
      {
        LOG_MSG_ERROR("Network logging is not enabled for iface, 0x%p",
                  ps_iface_ptr, 0, 0);
        return FALSE;
      }

      if ( !(DPL_IID_NETPROT_MIN <= iid.protocol &&
             DPL_IID_NETPROT_MAX > iid.protocol))
      {
        LOG_MSG_ERROR("Invalid network protocol in IID", 0, 0, 0);
        return FALSE;
      }

      /*---------------------------------------------------------------------
        For Network Logging, link instance must always be zero. If this
        convention is not followed, post processing tool's filename
        generation is affected.
      ---------------------------------------------------------------------*/
      if (0 != iid.link_instance)
      {
        LOG_MSG_ERROR("Invalid link value in IID", 0, 0, 0);
        return FALSE;
      }
    } /* if network logging */
    else  /* if flow logging */
    {
      if (IS_DPL_IID_DIR_RX(iid))
      {
        LOG_MSG_ERROR("Flow logging is not supported on Rx", 0, 0, 0);
        return FALSE;
      }

      if (iid.link_instance != DEFAULT_FLOW_INST &&
          iid.link_instance != ALL_SEC_FLOW_INST)
      {
        LOG_MSG_ERROR("Invalid flow inst value in IID", 0, 0, 0);
        return FALSE;
      }
      else
      {
        /*-------------------------------------------------------------------
          Default flow is statically allocated, hence no need for
          critical section
        -------------------------------------------------------------------*/
        flow_ptr = PS_IFACE_GET_DEFAULT_FLOW(ps_iface_ptr);
        ASSERT(PS_FLOW_IS_VALID(flow_ptr));

        if (ifname != flow_ptr->dpl_flow_cb.tx_dpl_id.ifname)
        {
          LOG_MSG_ERROR("Flow logging is not enabled for iface, 0x%p",
                    ps_iface_ptr, 0, 0);
          return FALSE;
        }
      }

      if (DPL_IID_NETPROT_IP != iid.protocol)
      {
        LOG_MSG_ERROR("Invalid network protocol in IID", 0, 0, 0);
        return FALSE;
      }
    }  /* if flow logging */
  }  /* if L bit is DPL_IID_L_BIT_NETWORK */
  else  /* if L bit is DPL_IID_L_BIT_LINK */
  {
    if (IS_DPL_IID_F_BIT_FLOW(iid))
    {
      LOG_MSG_ERROR("Flow logging and link logging are set for iface, 0x%p",
                ps_iface_ptr, 0, 0);
      return FALSE;
    }

    /*-----------------------------------------------------------------------
      Make sure that link instance is less than the number of phys links
      associated with this iface
    -----------------------------------------------------------------------*/
    n_links = PS_IFACE_GET_NUM_PHYS_LINKS(ps_iface_ptr);
    if (iid.link_instance >= n_links)
    {
      LOG_MSG_ERROR("Invalid link value in IID", 0, 0, 0);
      return FALSE;
    }

    phys_link_ptr =
      PS_IFACE_GET_PHYS_LINK_BY_INST(ps_iface_ptr, iid.link_instance);
    ASSERT(PS_PHYS_LINK_IS_VALID(phys_link_ptr));

    if (ifname != phys_link_ptr->dpl_link_cb.tx_dpl_id.ifname)
    {
      LOG_MSG_ERROR("Link logging is not enabled for iface, 0x%p",
                ps_iface_ptr, 0, 0);
      return FALSE;
    }

    /*-----------------------------------------------------------------------
      User is not allowed to specify link protocol. So Diag tool must send
      wildcard in protocol field
    -----------------------------------------------------------------------*/
    if (DPL_IID_LINKPROT_WILDCARD != iid.protocol)
    {
      LOG_MSG_ERROR("Invalid link protocol in IID", 0, 0, 0);
      return FALSE;
    }
  }  /* if link logging */

  return TRUE;

} /* dpli_is_valid_iid() */



/*===========================================================================
FUNCTION    DPLI_SET_LOGGING_FLAG

DESCRIPTION
  Set appropriate logging flag(s) and partial logging length for a given IID

PARAMETRS
  iid     : IID, which contains the flag information
  snaplen : Partial log length for the protocol in this IID

RETURN VALUE
  NONE

DEPENDENCIES
  IID must have been validated.

SIDE EFFECTS
  NONE
===========================================================================*/
static void dpli_set_logging_flag
(
  dpl_iid_type  iid,
  uint32        snaplen
)
{
  ps_iface_type      * ps_iface_ptr;
  ps_flow_type       * flow_ptr;
  ps_phys_link_type  * phys_link_ptr;
#ifdef FEATURE_DATA_PS_QOS
  void               * sec_flow_handle;
  void               * new_sec_flow_handle;
#endif /* FEATURE_DATA_PS_QOS */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if ( (iid.ifname < DPL_IID_IFNAME_MIN) ||
       (iid.ifname >= DPL_IID_IFNAME_MAX) )
  {
    LOG_MSG_ERROR("Invalid ifname %d", iid.ifname, 0, 0);
    return;
  }

  ps_iface_ptr = ps_iface_dpl_lookup_table[iid.ifname].ps_iface_ptr;

  if (snaplen > SNAPLEN_MAX)
  {
    snaplen = SNAPLEN_MAX;
  }

  /*-------------------------------------------------------------------------
    Set Network/flow protocol logging flag and snaplen
  -------------------------------------------------------------------------*/
  if (IS_DPL_IID_L_BIT_NETWORK(iid))
  {
    if ( (iid.protocol < DPL_IID_NETPROT_MIN) ||
         (iid.protocol >= DPL_IID_NETPROT_MAX) )
    {
      LOG_MSG_ERROR("Invalid Network protocol logging %d",
      iid.protocol, 0, 0);
      return;
    }

    if (IS_DPL_IID_F_BIT_NONFLOW(iid))  /* if network logging */
    {
      if (IS_DPL_IID_DIR_RX(iid))
      {
        ps_iface_ptr->dpl_net_cb.recv_cb.mask |=
          (0x01 << (iid.protocol - 1));
        ps_iface_ptr->dpl_net_cb.recv_cb.snaplen[iid.protocol] =
          snaplen;
      }
      else
      {
        /*-------------------------------------------------------------------
          No need to enter critical section since these values are updated
          only in DIAG context
        -------------------------------------------------------------------*/
        ps_iface_ptr->dpl_net_cb.tx_cb.mask |=
          (0x01 << (iid.protocol - 1));
        ps_iface_ptr->dpl_net_cb.tx_cb.snaplen[iid.protocol] =
          snaplen;
      }
    }
    else  /* if flow logging */
    {
      if (IS_DPL_IID_DIR_RX(iid))
      {
        LOG_MSG_FATAL_ERROR("Flow logging is not supported on Rx", 0, 0, 0);
      }

      /*---------------------------------------------------------------------
        Populate default ps_flow is instance is DEFAULT_FLOW_INST. Else update
        ps_iface's master copy and all secondary flow's
      ---------------------------------------------------------------------*/
      if (iid.link_instance == DEFAULT_FLOW_INST)
      {
        /*-------------------------------------------------------------------
          No need to enter critical section since default flow is never
          deleted
        -------------------------------------------------------------------*/
        flow_ptr = PS_IFACEI_GET_DEFAULT_FLOW(ps_iface_ptr);

        flow_ptr->dpl_flow_cb.tx_cb.mask |=
          (0x01 << (iid.protocol - 1));
        flow_ptr->dpl_flow_cb.tx_cb.snaplen[iid.protocol] = snaplen;
      }
#ifdef FEATURE_DATA_PS_QOS
      else  /* if all secondary flows */
      {
        ASSERT(iid.link_instance == ALL_SEC_FLOW_INST);

        /*-------------------------------------------------------------------
          No need to enter critical section since these values are updated
          only in DIAG context
        -------------------------------------------------------------------*/
        ps_iface_ptr->dpl_sec_flow_copy_cb.mask |=
          (0x01 << (iid.protocol - 1));
        ps_iface_ptr->dpl_sec_flow_copy_cb.snaplen[iid.protocol] =
          snaplen;

        /*-------------------------------------------------------------------
          Also update all secondary flow's flow logging control block
        -------------------------------------------------------------------*/
        PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

        sec_flow_handle = ps_iface_get_sec_flow_handle(ps_iface_ptr);
        while (sec_flow_handle != NULL &&
               ps_iface_get_sec_flow_by_handle(ps_iface_ptr,
                                               sec_flow_handle,
                                               &flow_ptr,
                                               &new_sec_flow_handle))
        {
          flow_ptr->dpl_flow_cb.tx_cb.mask |=
            (0x01 << (iid.protocol - 1));
          flow_ptr->dpl_flow_cb.tx_cb.snaplen[iid.protocol] =
            snaplen;

          sec_flow_handle = new_sec_flow_handle;
        }

        PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
      }  /* if all secondary flows */
#endif /* FEATURE_DATA_PS_QOS */
    } /* if flow logging */
  }
  /*-------------------------------------------------------------------------
    Set Link protocol logging flag and snaplen
  -------------------------------------------------------------------------*/
  else
  {
    phys_link_ptr =
      PS_IFACE_GET_PHYS_LINK_BY_INST(ps_iface_ptr, iid.link_instance);
    ASSERT(PS_PHYS_LINK_IS_VALID(phys_link_ptr));

    if (IS_DPL_IID_DIR_RX(iid))
    {
      phys_link_ptr->dpl_link_cb.recv_cb.is_logged = TRUE;
      phys_link_ptr->dpl_link_cb.recv_cb.snaplen   = snaplen;
    }
    else
    {
      phys_link_ptr->dpl_link_cb.tx_cb.is_logged = TRUE;
      phys_link_ptr->dpl_link_cb.tx_cb.snaplen   = snaplen;
    }
  }

} /* dpli_set_logging_flag() */



/*===========================================================================
FUNCTION    DPLI_IS_LOGGING_FLAG_SET

DESCRIPTION
  Checks if logging flag is already set for this IID.

PARAMETRS
  iid     : IID, which identifies a log point uniquely

RETURN VALUE
  TRUE if logging flag is already set
  FALSE otherwise

DEPENDENCIES
  IID must have been validated

SIDE EFFECTS
  NONE
===========================================================================*/
static boolean dpli_is_logging_flag_set
(
  dpl_iid_type  iid
)
{
  ps_iface_type      * ps_iface_ptr;
  ps_phys_link_type  * phys_link_ptr;
  ps_flow_type       * flow_ptr;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if( (iid.ifname < DPL_IID_IFNAME_MIN) ||
      (iid.ifname >= DPL_IID_IFNAME_MAX))
  {
    LOG_MSG_ERROR("Invalid ifname %d", iid.ifname, 0, 0);
    return FALSE;
  }

  ps_iface_ptr = ps_iface_dpl_lookup_table[iid.ifname].ps_iface_ptr;

  /*-------------------------------------------------------------------------
    Check if Network/flow protocol logging flag is set.
  -------------------------------------------------------------------------*/
  if (IS_DPL_IID_L_BIT_NETWORK(iid))
  {
    if (IS_DPL_IID_F_BIT_NONFLOW(iid)) /* if network logging */
    {
      if (IS_DPL_IID_DIR_RX(iid))
      {
        if (((ps_iface_ptr->dpl_net_cb.recv_cb.mask) &
               (0x01 << (iid.protocol - 1))) != 0)
        {
          return TRUE;
        }
      }
      else
      {
        if (((ps_iface_ptr->dpl_net_cb.tx_cb.mask) &
               (0x01 << (iid.protocol - 1))) != 0)
        {
          return TRUE;
        }
      }
    }
    else /* if flow logging */
    {
      ASSERT(IS_DPL_IID_DIR_TX(iid));

      if (iid.link_instance == DEFAULT_FLOW_INST)
      {
        /*-------------------------------------------------------------------
          No need to enter critical section since default flow is never
          deleted
        -------------------------------------------------------------------*/
        flow_ptr = PS_IFACEI_GET_DEFAULT_FLOW(ps_iface_ptr);

        if (((flow_ptr->dpl_flow_cb.tx_cb.mask) &
               (0x01 << (iid.protocol - 1))) != 0)
        {
          return TRUE;
        }
      }
      else
      {
        ASSERT(iid.link_instance == ALL_SEC_FLOW_INST);

        if (((ps_iface_ptr->dpl_sec_flow_copy_cb.mask) &
               (0x01 << (iid.protocol - 1))) != 0)
        {
          return TRUE;
        }
      }
    }
  }
  /*-------------------------------------------------------------------------
    Check if Link protocol logging flag is set
  -------------------------------------------------------------------------*/
  else
  {
    phys_link_ptr =
      PS_IFACE_GET_PHYS_LINK_BY_INST(ps_iface_ptr, iid.link_instance);
    ASSERT(PS_PHYS_LINK_IS_VALID(phys_link_ptr));

    if (IS_DPL_IID_DIR_RX(iid))
    {
      if (phys_link_ptr->dpl_link_cb.recv_cb.is_logged == TRUE)
      {
        return TRUE;
      }
    }
    else
    {
      if (phys_link_ptr->dpl_link_cb.tx_cb.is_logged == TRUE)
      {
        return TRUE;
      }
    }
  }

  return FALSE;

} /* dpli_is_logging_flag_set() */



/*===========================================================================
FUNCTION    DPLI_RESET_FLAG

DESCRIPTION
  Resets logging flags of a ps_iface

PARAMETERS
  ifname : DPL IID iface name

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
static void dpli_reset_flag
(
  dpl_iid_ifname_enum_type  ifname
)
{
  ps_iface_type      * ps_iface_ptr;
  ps_flow_type       * flow_ptr;
  ps_phys_link_type  * phys_link_ptr;
  uint8                i;
  uint8                n_links;   /* # of traffic channels                 */
#ifdef FEATURE_DATA_PS_QOS
  void               * sec_flow_handle;
  void               * new_sec_flow_handle;
#endif /* FEATURE_DATA_PS_QOS */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /* Sanity Check */
  if(ifname < DPL_IID_IFNAME_MIN || ifname >= DPL_IID_IFNAME_MAX)
  {
    LOG_MSG_ERROR("Invalid ifname %d", ifname, 0, 0);
    ASSERT(0);
    return;
  }

  ps_iface_ptr = ps_iface_dpl_lookup_table[ifname].ps_iface_ptr;
  if (PS_IFACE_IS_VALID(ps_iface_ptr))
  {
    /*-----------------------------------------------------------------------
      Reset ps_iface logging control block. Can't memset the whole structure
      since IID is set in control block when ps_iface is created and never
      set again.
    -----------------------------------------------------------------------*/
    memset((&ps_iface_ptr->dpl_net_cb.tx_cb),
           0,
           sizeof(ps_iface_ptr->dpl_net_cb.tx_cb));

    memset(&(ps_iface_ptr->dpl_net_cb.recv_cb),
           0,
           sizeof(ps_iface_ptr->dpl_net_cb.recv_cb));

    /*-----------------------------------------------------------------------
      Reset ps_flow logging control block in ps_iface, its default ps_flow,
      and all secondary ps_flows associated with it. Can't memset the whole
      structure since IID is set in control block when ps_flow is created
      never set again.
    -----------------------------------------------------------------------*/
    memset(&(ps_iface_ptr->dpl_sec_flow_copy_cb),
           0,
           sizeof(ps_iface_ptr->dpl_sec_flow_copy_cb));

    flow_ptr = PS_IFACEI_GET_DEFAULT_FLOW(ps_iface_ptr);
    memset(&(flow_ptr->dpl_flow_cb.tx_cb),
           0,
           sizeof(flow_ptr->dpl_flow_cb.tx_cb));

#ifdef FEATURE_DATA_PS_QOS
    PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

    sec_flow_handle = ps_iface_get_sec_flow_handle(ps_iface_ptr);
    while (sec_flow_handle != NULL &&
           ps_iface_get_sec_flow_by_handle(ps_iface_ptr,
                                           sec_flow_handle,
                                           &flow_ptr,
                                           &new_sec_flow_handle))
    {
      memset(&(flow_ptr->dpl_flow_cb.tx_cb),
             0,
             sizeof(flow_ptr->dpl_flow_cb.tx_cb));
      sec_flow_handle = new_sec_flow_handle;
    }

    PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);
#endif /* FEATURE_DATA_PS_QOS */

    /*-----------------------------------------------------------------------
      Reset primary phys link logging control block. Can't memset the whole
      structure since IID is set in control block when control block is
      created then it is never set again.
    -----------------------------------------------------------------------*/
    n_links = PS_IFACE_GET_NUM_PHYS_LINKS(ps_iface_ptr);
    for (i = 0; i < n_links; i++)
    {
      phys_link_ptr = PS_IFACE_GET_PHYS_LINK_BY_INST(ps_iface_ptr, i);
      ASSERT(PS_PHYS_LINK_IS_VALID(phys_link_ptr));

      memset(&(phys_link_ptr->dpl_link_cb.tx_cb),
             0,
             sizeof(phys_link_ptr->dpl_link_cb.tx_cb));

      memset(&(phys_link_ptr->dpl_link_cb.recv_cb),
             0,
             sizeof(phys_link_ptr->dpl_link_cb.recv_cb));
    } /* for each phys link */
  }
} /* dpli_reset_flag() */



/*===========================================================================
FUNCTION    PS_LOGGING_DIAG_RESET_LOGGING

DESCRIPTION
  Handles the request that comes from DM to reset data logging configuration.

PARAMETERS
  req_pkt : request packet coming from diagnostic tool(DM) to diagnostic link.
  pkt_len : length of the request, mandatory parameter for diag request
            handlers.

RETURN VALUE:
  AEE_SUCCESS : PACKED void* resp_ptr: void* containing diag header.
  FAILURE : NULL if a diag buffer could not be allocated for response
            Empty get_interface-description response packet if request
            or input parameters are invalid.

DEPENDENCIES
  NONE

SIDE EFFECTS
  NONE
===========================================================================*/
static PACKED void *ps_logging_diag_reset_logging
(
  PACKED void  * req_pkt,
  uint16         pkt_len
)
{
  PACKED void               * return_ptr;
  uint8                     * request_ptr;
  dpl_iid_ifname_enum_type    i;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifdef FEATURE_DATA_PS_LOGGING_DEBUG
  LOG_MSG_INFO1("QXDM called PS_LOGGING_DIAG_RESET_LOGGING", 0, 0, 0);
  memdump ((void *) req_pkt, pkt_len);
#endif

  /*-------------------------------------------------------------------------
    Validate the request pkt's length
  -------------------------------------------------------------------------*/
  if (NULL == req_pkt || DIAG_HDR_LEN != pkt_len)
  {
    LOG_MSG_INFO1("Invalid request is sent by QXDM", 0, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_LEN_F, req_pkt, pkt_len));
  }

  /*-------------------------------------------------------------------------
    Validate request pkt's SSCC
  -------------------------------------------------------------------------*/
  request_ptr = (uint8 *) req_pkt + DIAG_HDR_SSCC_OFFSET;
  if (PS_LOGGING_DIAG_RESET_LOGGING != hget16(request_ptr))
  {
    LOG_MSG_INFO1("Invalid request is sent by QXDM", 0, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_CMD_F, req_pkt, pkt_len));
  }

  for (i = DPL_IID_IFNAME_MIN; i < DPL_IID_IFNAME_MAX; i++)
  {
    dpli_reset_flag(i);
  }

  /*-------------------------------------------------------------------------
    Fill response with diagnostic header
  -------------------------------------------------------------------------*/
  return_ptr = diagpkt_subsys_alloc(DIAG_SUBSYS_PS_DATA_LOGGING,
                                    PS_LOGGING_DIAG_RESET_LOGGING,
                                    DIAG_HDR_LEN );
  ASSERT(NULL != return_ptr);

  return (return_ptr);

} /* ps_logging_diag_reset_logging() */



/*===========================================================================
FUNCTION    PS_LOGGING_DIAG_GET_SUPPORTED_IFACES

DESCRIPTION
  Function responds with the list of all supported interafces to DM.

PARAMETERS
  req_pkt : request packet with diag header and ifname
  pkt_len : length of request packet (mandatory argument required by
           diag request handlers.

RETURN VALUE
  AEE_SUCCESS : PACKET void* response pointer with diag header, number of
            supported interfaces, list of supported interfaces.
  FAILURE : NULL if a diag buffer could not be allocated for response
            Empty get_interface-description response packet if request
            or input parameters are invalid.

DEPENDENCIES:
  None

SIDE EFFECTS:
  None
===========================================================================*/
static PACKED void *ps_logging_diag_get_supported_ifaces
(
  PACKED void  * req_pkt,
  uint16         pkt_len
)
{
  PACKED void  * return_ptr;               /* Response message             */
  uint8        * resp_ptr;                 /* Fill response pkt using this */
  uint8        * request_ptr;
  unsigned int   len;                      /* Length of response pkt       */
  uint8          supported_ifaces;         /* # of non-NULL interfaces in
                                              global iface array           */
  uint8          i;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifdef FEATURE_DATA_PS_LOGGING_DEBUG
  LOG_MSG_INFO1("QXDM called PS_LOGGING_DIAG_GET_SUPPORTED_IFACES", 0, 0, 0);
  memdump ((void *) req_pkt, pkt_len);
#endif

  /*-------------------------------------------------------------------------
    Validate the request pkt's length
  -------------------------------------------------------------------------*/
  if (NULL == req_pkt || DIAG_HDR_LEN != pkt_len)
  {
    LOG_MSG_INFO1("Invalid request is sent by QXDM", 0, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_LEN_F, req_pkt, pkt_len));
  }

  /*-------------------------------------------------------------------------
    Validate request pkt's SSCC
  -------------------------------------------------------------------------*/
  request_ptr = (uint8 *) req_pkt + DIAG_HDR_SSCC_OFFSET;
  if (PS_LOGGING_DIAG_GET_SUPPORTED_IFACES != hget16(request_ptr))
  {
    LOG_MSG_INFO1("Invalid request is sent by QXDM", 0, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_CMD_F, req_pkt, pkt_len));
  }
  
  PS_ENTER_CRIT_SECTION(&global_ps_crit_section);

  /*-------------------------------------------------------------------------
    Count the valid number of interfaces
  -------------------------------------------------------------------------*/
  for (i = DPL_IID_IFNAME_MIN, supported_ifaces = 0;
       i < DPL_IID_IFNAME_MAX;
       i++)
  {
    if (NULL != ps_iface_dpl_lookup_table[i].ps_iface_ptr)
    {
      supported_ifaces++;
    }
  }

  /*-------------------------------------------------------------------------
    Memory to be allocated to the response:
    response is:
    DIAG HDR
    # of interfaces
    list of supported interfaces (dpl_ifname_e_type values (1 per interface))
  -------------------------------------------------------------------------*/
  len = DIAG_HDR_LEN + sizeof(supported_ifaces) +
        supported_ifaces * sizeof(dpl_iid_ifname_enum_type);

  /*-------------------------------------------------------------------------
    allocate diagbuf to hold response
  -------------------------------------------------------------------------*/
  return_ptr =  diagpkt_subsys_alloc(DIAG_SUBSYS_PS_DATA_LOGGING,
                                     PS_LOGGING_DIAG_GET_SUPPORTED_IFACES,
                                     len);
  ASSERT(NULL != return_ptr);

  /*-------------------------------------------------------------------------
    Skip Diag header
  -------------------------------------------------------------------------*/
  resp_ptr = (uint8 *) return_ptr + DIAG_HDR_LEN;

  /*-------------------------------------------------------------------------
    Byte copy # of suppoted interfaces
  -------------------------------------------------------------------------*/
  resp_ptr = put8(resp_ptr, supported_ifaces);

  /*-------------------------------------------------------------------------
    Fill ifname of each supported iface
  -------------------------------------------------------------------------*/
  for (i = DPL_IID_IFNAME_MIN; i < DPL_IID_IFNAME_MAX; i++)
  {
    /*-----------------------------------------------------------------------
      Add ifacename value in response. variable i is an index in to
      dpli_ps_iface_lookup_table and is same as dpl_iid_ifname_enum_type.
    -----------------------------------------------------------------------*/
    if (NULL != ps_iface_dpl_lookup_table[i].ps_iface_ptr)
    {
      resp_ptr = put8(resp_ptr, i);
    }
  }

  PS_LEAVE_CRIT_SECTION(&global_ps_crit_section);

  return (return_ptr);

} /* ps_logging_diag_get_supported_ifaces() */



/*===========================================================================
FUNCTION    PS_LOGGING_DIAG_GET_IFACE_DESC

DESCRIPTION
  given the interface name the function responds with the description of the
  interface and the links corresponding to that interface.

PARAMETERS
  req_pkt : request packet with diag header and ifname
  pkt_len : length of request packet (mandatory argument required by
           diag request handlers.

RETURN VALUE
  AEE_SUCCESS : PACKET void* response pointer with diag header, number of
            supported protocols, list of supported protocols.
  FAILURE : NULL if a diag buffer could not be allocated for response
            Empty get_interface-description response packet if request
            or input parameters are invalid.

DEPENDENCIES
  NONE

SIDE EFFECTS
  NONE
===========================================================================*/
static PACKED void *ps_logging_diag_get_iface_desc
(
  PACKED void  * req_pkt,
  uint16         pkt_len
)
{
  PACKED void             * return_ptr;    /* Response message             */
  uint8                   * resp_ptr;      /* Fill response pkt using this */
  uint8                   * request_ptr;   /* Read from req_pkt using this */
  const char              * iface_desc;    /* Interface Description        */
  ps_iface_type           * ps_iface_ptr;
  ps_phys_link_type       * phys_link_ptr;
  const char              * desc;          /* Place holder to store desc   */
  int32                     ret_val;
  int32                     len;           /* Length of response packet    */
  uint32                    iface_desc_len;
  uint8                     n_links;       /* # of traffic channels        */
  uint8                     i;
  dpl_iid_ifname_enum_type  ifname;        /* IID Interface name           */

#define NUM_FLOWS  1                /* Only default ps_flow's desc is sent */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifdef FEATURE_DATA_PS_LOGGING_DEBUG
  LOG_MSG_INFO1("QXDM called PS_LOGGING_DIAG_GET_IFACE_DESC", 0, 0, 0);
  memdump ((void *) req_pkt, pkt_len);
#endif

  /*-------------------------------------------------------------------------
    Validate the request pkt's length
  -------------------------------------------------------------------------*/
  if (NULL == req_pkt ||
      (DIAG_HDR_LEN + sizeof(dpl_iid_ifname_enum_type)) != pkt_len)
  {
    LOG_MSG_INFO1("Invalid request is sent by QXDM", 0, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_LEN_F, req_pkt, pkt_len));
  }

  /*-------------------------------------------------------------------------
    Validate request pkt's SSCC
  -------------------------------------------------------------------------*/
  request_ptr = (uint8 *) req_pkt + DIAG_HDR_SSCC_OFFSET;
  if (PS_LOGGING_DIAG_GET_IFACE_DESC != hget16(request_ptr))
  {
    LOG_MSG_INFO1("Invalid request is sent by QXDM", 0, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_CMD_F, req_pkt, pkt_len));
  }

  /*-------------------------------------------------------------------------
    Skip Diag header of request pkt
  -------------------------------------------------------------------------*/
  request_ptr = (uint8 *) req_pkt + DIAG_HDR_LEN;

  /*-------------------------------------------------------------------------
    Extract interface name and validate it.
  -------------------------------------------------------------------------*/
  ifname = (dpl_iid_ifname_enum_type) get8(request_ptr);

  if ( !(DPL_IID_IFNAME_MIN <= ifname && DPL_IID_IFNAME_MAX > ifname))
  {
    LOG_MSG_INFO1("Invalid IID ifname (%d) received in request", ifname, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_PARM_F, req_pkt, pkt_len));
  }

  ps_iface_ptr = ps_iface_dpl_lookup_table[ifname].ps_iface_ptr;
  if (NULL == ps_iface_ptr)
  {
    LOG_MSG_INFO1("Logging is not supported on ifname (%d)", ifname, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_PARM_F, req_pkt, pkt_len));
  }

  /*-------------------------------------------------------------------------
    Response packet includes
      - Diag header
      - IID ifname
      - IID ifname description
      - # of flows
      - Description of each flows
      - # of phys_links
      - Description of each phys link
  -------------------------------------------------------------------------*/
  n_links        = PS_IFACE_GET_NUM_PHYS_LINKS(ps_iface_ptr);
  iface_desc     = ps_iface_dpl_get_iface_desc(ps_iface_ptr);
  iface_desc_len = strlen(iface_desc) + 1;    /* to account for \0 as well */
  len = DIAG_HDR_LEN + sizeof(dpl_iid_ifname_enum_type) + iface_desc_len +
        sizeof(n_links);

  /*-------------------------------------------------------------------------
    Handle # of flows and default ps_flow's description
  -------------------------------------------------------------------------*/
  len += (int32)sizeof(uint8) + (int32)strlen("Default flow") + 1;

  for (i = 0; i < n_links; i++)
  {
    /* Account for '\0' at the end of string as well                      */
    phys_link_ptr = PS_IFACE_GET_PHYS_LINK_BY_INST(ps_iface_ptr, i);
    ASSERT(PS_PHYS_LINK_IS_VALID(phys_link_ptr));
    len += (int32)strlen(ps_iface_dpl_get_phys_link_desc(phys_link_ptr)) + 1;
  }

  /*-------------------------------------------------------------------------
    Allocate response buffer and fill first four bytes with diag header
  -------------------------------------------------------------------------*/
  return_ptr = diagpkt_subsys_alloc(DIAG_SUBSYS_PS_DATA_LOGGING,
                                    PS_LOGGING_DIAG_GET_IFACE_DESC,
                                    (uint32)len);
  ASSERT(NULL != return_ptr);

  /*-------------------------------------------------------------------------
    Skip diag header
  -------------------------------------------------------------------------*/
  resp_ptr = (uint8 *) return_ptr + DIAG_HDR_LEN;

  /*-------------------------------------------------------------------------
    Insert iface name in response
  -------------------------------------------------------------------------*/
  resp_ptr = put8(resp_ptr, ifname);

  /*-------------------------------------------------------------------------
    Fill response with iface description
  -------------------------------------------------------------------------*/
  ret_val =  std_strlcpy((char *) resp_ptr,
                         iface_desc,
                         len - (resp_ptr - (uint8 *) return_ptr));
  resp_ptr += iface_desc_len;

  /*-------------------------------------------------------------------------
    Byte copy number of traffic channels
  -------------------------------------------------------------------------*/
  resp_ptr = put8(resp_ptr, n_links);

  /*-------------------------------------------------------------------------
    Add link description in response
  -------------------------------------------------------------------------*/
  for (i = 0; i < n_links; i++)
  {
    phys_link_ptr = PS_IFACE_GET_PHYS_LINK_BY_INST(ps_iface_ptr, i);
    ASSERT(PS_PHYS_LINK_IS_VALID(phys_link_ptr));
    desc = ps_iface_dpl_get_phys_link_desc(phys_link_ptr);
    ret_val =  std_strlcpy((char *) resp_ptr,
                           desc,
                           len - (resp_ptr - (uint8 *) return_ptr));
    resp_ptr += ret_val + 1;
  }

  /*-------------------------------------------------------------------------
    Byte copy number of flows
  -------------------------------------------------------------------------*/
  resp_ptr = put8(resp_ptr, NUM_FLOWS);

  /*-------------------------------------------------------------------------
    Add default ps_flow's description in response
  -------------------------------------------------------------------------*/
  ret_val =  std_strlcpy((char *) resp_ptr,
                         "Default flow",
                         len - (resp_ptr - (uint8 *) return_ptr));
  resp_ptr += ret_val + 1;

  ASSERT(len == (resp_ptr - (uint8 *) return_ptr));
  return (return_ptr);

} /* ps_logging_diag_get_iface_desc() */



/*===========================================================================
FUNCTION    PS_LOGGING_DIAG_SET_IFACE_LOGGING

DESCRIPTION
  This function is used to set the logging flags and partial logging length
  and constructs a response with ifname, number of iids, iid and link type
  fields.

PARAMETRS
  req_pkt : Request packet from diag
  pkt_len : length of request packet (mandatory argument required by
            diag request handlers.

RETURN VALUE
  PACKED void * response pointer with
  -diag header
  -interface name
  -# of iids
  -iid
  -partial log length

DEPENDENCIES
  NONE

SIDE EFFECTS
  NONE
===========================================================================*/
static PACKED void *ps_logging_diag_set_iface_logging
(
  PACKED void  * req_pkt,
  uint16         pkt_len
)
{
  PACKED void  * return_ptr;         /* Response message                   */
  uint8        * resp_ptr;           /* Fill response pkt using this       */
  uint8        * request_ptr;        /* Read from req_pkt using this       */
  uint8        * req_iid_ptr;        /* Points to where IIDs start in req  */
  dpl_iid_type   iid = {0};          /* IID passed in request pkt          */
  uint32         snaplen;            /* Partial log length                 */
  uint16         len;                /* Length of respone packet           */
  uint16         expected_len;       /* Expected length of request pkt     */
  uint8          n_iids;             /* # of IIDs passed in request        */
  uint8          resp_iid_cnt;       /* # of valid IIDs sent back to DM    */
  uint8          i;
  uint8          j;

  dpl_iid_ifname_enum_type    ifname;  /* IID interface name               */
  dpl_pcap_linktype_enum_type link_type;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifdef FEATURE_DATA_PS_LOGGING_DEBUG
  LOG_MSG_INFO1("QXDM called PS_LOGGING_DIAG_SET_IFACE_LOGGING", 0, 0, 0);
  memdump ((void *) req_pkt, pkt_len);
#endif

  /*-------------------------------------------------------------------------
    Make sure that request pkt is of minimum required length
  -------------------------------------------------------------------------*/
  if (NULL == req_pkt ||
      (DIAG_HDR_LEN + sizeof(ifname) + sizeof(n_iids)) > pkt_len)
  {
    LOG_MSG_INFO1("Invalid request is sent by QXDM", 0, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_LEN_F, req_pkt, pkt_len));
  }

  /*-------------------------------------------------------------------------
    Validate request pkt's SSCC
  -------------------------------------------------------------------------*/
  request_ptr = (uint8 *) req_pkt + DIAG_HDR_SSCC_OFFSET;
  if (PS_LOGGING_DIAG_SET_IFACE_LOGGING != hget16(request_ptr))
  {
    LOG_MSG_INFO1("Invalid request is sent by QXDM", 0, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_CMD_F, req_pkt, pkt_len));
  }

  /*-------------------------------------------------------------------------
    Skip Diag header of request pkt
  -------------------------------------------------------------------------*/
  request_ptr = (uint8 *) req_pkt + DIAG_HDR_LEN;

  /*-------------------------------------------------------------------------
    Extract interface name and # of iids
  -------------------------------------------------------------------------*/
  ifname = (dpl_iid_ifname_enum_type) get8(request_ptr++);

  /*-------------------------------------------------------------------------
    Validate range
  -------------------------------------------------------------------------*/
  if (!(DPL_IID_IFNAME_MIN <= ifname && DPL_IID_IFNAME_MAX > ifname))
  {
    LOG_MSG_INFO1("ifname (%d) is not in valid range", ifname, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_PARM_F, req_pkt, pkt_len));
  }

  /*-------------------------------------------------------------------------
    Check if logging is enabled on this ifname
  -------------------------------------------------------------------------*/
  if (NULL == ps_iface_dpl_lookup_table[ifname].ps_iface_ptr)
  {
    LOG_MSG_INFO1("Logging is not supported on ifname (%d)", ifname, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_PARM_F, req_pkt, pkt_len));
  }

  n_iids = get8(request_ptr++);

  /*-------------------------------------------------------------------------
    Validate the request pkt's length
  -------------------------------------------------------------------------*/
  expected_len = DIAG_HDR_LEN + sizeof(ifname) + sizeof(n_iids) +
                 (n_iids * (sizeof(iid) + sizeof(snaplen)));
  if (expected_len != pkt_len)
  {
    LOG_MSG_INFO1("Malformed request is sent by QXDM", 0, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_LEN_F, req_pkt, pkt_len));
  }

  /*-------------------------------------------------------------------------
    Set interface logging request is valid, so clear the previous logging
    settings before reloading with contents of this request.
  -------------------------------------------------------------------------*/
  dpli_reset_flag(ifname);

  /*-------------------------------------------------------------------------
    Find the # of valid IIDs. If L bit is set we send IID for each
    link protocol
  -------------------------------------------------------------------------*/
  req_iid_ptr = request_ptr;
  for (i = 0, resp_iid_cnt = 0; i < n_iids; i++)
  {
    DPLI_FILL_IID(iid, request_ptr);

    if (TRUE == dpli_is_valid_iid(iid, ifname))
    {
      if (TRUE == dpli_is_logging_flag_set(iid))
      {
        /*-------------------------------------------------------------------
          As this is a duplicate iid, make it invalid in the request by
          setting ifname to zero (ifname 0 is reserved hence unused) so that
          it is accounted for only once in the response and also do not set
          logging flags.

          NOTE: Using "request_ptr - 2" as DPLI_FILL_IID would have
          incremented request_ptr by 4 bytes.
        -------------------------------------------------------------------*/
       (void) put8(request_ptr - 2, 0);
      }
      else
      {
        /*-------------------------------------------------------------------
          As this is NOT a duplicate iid, set the logging flags
        -------------------------------------------------------------------*/
        snaplen = hget32(request_ptr);
        dpli_set_logging_flag(iid, snaplen);

        if (IS_DPL_IID_L_BIT_NETWORK(iid))
        {
          resp_iid_cnt++;
        }
        else
        {
          resp_iid_cnt += (uint8)DPL_IID_LINKPROT_MAX - (uint8)DPL_IID_LINKPROT_MIN;
        }
      }
    }
    else
    {
      /*---------------------------------------------------------------------
        Seting ifname to zero in the request even if the IID is invalid
        so that the IID can be discarded from response just by checking
        if iid.ifname == 0
      ---------------------------------------------------------------------*/
      (void) put8(request_ptr - 2, 0);
    }

    /*-----------------------------------------------------------------------
      IIDs are spaced apart by 8 bytes but as DPLI_FILL_IID increments
      request_ptr by four bytes, we need to skip only 4 bytes to get to next
      IID.
    -----------------------------------------------------------------------*/
    request_ptr += sizeof(snaplen);
  }

  ASSERT(pkt_len == (request_ptr - (uint8 *) req_pkt));

  request_ptr = req_iid_ptr;

  /*-------------------------------------------------------------------------
    Calculate reponse length. For IIDs with L bit set, we enumerate all
    possible IIDs by setting protocol field to various link protocols.
    IIDs and corresponding link types are sent to QXDM.
  -------------------------------------------------------------------------*/
  len = DIAG_HDR_LEN + sizeof(ifname) + sizeof(n_iids) +
        resp_iid_cnt * (sizeof(iid) + sizeof(link_type));

  /*-------------------------------------------------------------------------
    Allocate diag buffer and fill in diag header
  -------------------------------------------------------------------------*/
  return_ptr = diagpkt_subsys_alloc(DIAG_SUBSYS_PS_DATA_LOGGING,
                                    PS_LOGGING_DIAG_SET_IFACE_LOGGING,
                                    len);
  ASSERT(NULL != return_ptr);

  /*-------------------------------------------------------------------------
    Use temporary variable to fill response and skip Diag header
  -------------------------------------------------------------------------*/
  resp_ptr = (uint8 *) return_ptr + DIAG_HDR_LEN;

  /*-------------------------------------------------------------------------
    Copy ifname and resp_iid_cnt into response
  -------------------------------------------------------------------------*/
  resp_ptr = put8(resp_ptr, ifname);
  resp_ptr = put8(resp_ptr, resp_iid_cnt);

  for (i = 0; i < n_iids; i++)
  {
    /*-----------------------------------------------------------------------
      Get next iid and skip partial length
    -----------------------------------------------------------------------*/
    DPLI_FILL_IID(iid, request_ptr);
    request_ptr += sizeof(snaplen);

    /*-----------------------------------------------------------------------
      Build the response
    -----------------------------------------------------------------------*/
    if (0 != iid.ifname)
    {
      if (IS_DPL_IID_L_BIT_LINK(iid))
      {
        for (j = DPL_IID_LINKPROT_MIN; j < DPL_IID_LINKPROT_MAX; j++)
        {
          switch (j)
          {
            case DPL_IID_LINKPROT_ETHERNET:
              link_type = DPL_PCAP_ETHERNET;
              break;

            /*---------------------------------------------------------------
              Even though RoHC/IPHC doesn't have PPP on top, 7Es are inserted
              by AMSS so that pcap file can be generated
            ---------------------------------------------------------------*/
            case DPL_IID_LINKPROT_PPP_IN_HDLC:
            case DPL_IID_LINKPROT_ROHC_COMP_IP:
            case DPL_IID_LINKPROT_IPHC_COMP_IP:
              link_type = DPL_PCAP_PPP_IN_HDLC;
              break;

            case DPL_IID_LINKPROT_WILDCARD:
              link_type = DPL_PCAP_INVALID;
              break;

            default:
              link_type = DPL_PCAP_INVALID;
              LOG_MSG_FATAL_ERROR("Invalid DPL link protocol (%d)!", j, 0, 0);
              break;
          }

          iid.protocol = (dpl_iid_linkprot_enum_type) j;
          resp_ptr = put32(resp_ptr, *((uint32 *)((void *)&iid)));
          resp_ptr = hput32(resp_ptr, link_type);

        } /* for all link protocols */
      }
      else
      {
        switch (iid.protocol)
        {
          case DPL_IID_NETPROT_IP:
            link_type = DPL_PCAP_IP_RAW;
            break;

          case DPL_IID_NETPROT_HDLC_UNFRAMED:
            link_type = DPL_PCAP_PPP;
            break;

          default:
            link_type = DPL_PCAP_INVALID;
            LOG_MSG_FATAL_ERROR("Invalid DPL Network protocol ", 0, 0, 0);
            ASSERT(0);
            break;
        }

        resp_ptr = put32(resp_ptr, *((uint32 *)((void *) &iid)));
        resp_ptr = hput32(resp_ptr, link_type);
      }
    }
  } /* for all iids of this iface */

  ASSERT(pkt_len == (request_ptr - (uint8 *) req_pkt));
  ASSERT(len == (resp_ptr - (uint8 *) return_ptr));

  return (return_ptr);

} /* ps_logging_diag_set_iface_logging() */



/*===========================================================================
FUNCTION    PS_LOGGING_DIAG_GET_SUPPORTED_PROTOCOLS

DESCRIPTION
  Function responds with the list of all supported network level and above
  protocols to DM.

PARAMETERS
  req_pkt : request packet with diag header and ifname
  pkt_len : length of request packet (mandatory argument required by
            diag request handlers.

RETURN VALUE
  SUCCESS : PACKET void* response pointer with diag header, number of
            supported protocols, list of supported protocols.
  FAILURE : NULL if a diag buffer could not be allocated for response
            Empty get_interface-description response packet if request
            or input parameters are invalid.

DEPENDENCIES:
  None

SIDE EFFECTS:
  None
===========================================================================*/
static PACKED void *ps_logging_diag_get_supported_protocols
(
  PACKED void  * req_pkt,
  uint16         pkt_len
)
{
  PACKED void  * return_ptr;               /* Response message             */
  uint8        * resp_ptr;                 /* Fill response pkt using this */
  uint8        * request_ptr;
  unsigned int   len;                      /* Length of response pkt       */
  uint8          i;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

#ifdef FEATURE_DATA_PS_LOGGING_DEBUG
  LOG_MSG_INFO1("QXDM called PS_LOGGING_DIAG_GET_SUPPORTED_PROTOCOLS", 0, 0, 0);
  memdump ((void *) req_pkt, pkt_len);
#endif

  /*-------------------------------------------------------------------------
    Validate the request pkt's length
  -------------------------------------------------------------------------*/
  if (NULL == req_pkt || DIAG_HDR_LEN != pkt_len)
  {
    LOG_MSG_INFO1("Invalid request is sent by QXDM", 0, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_LEN_F, req_pkt, pkt_len));
  }

  /*-------------------------------------------------------------------------
    Validate request pkt's SSCC
  -------------------------------------------------------------------------*/
  request_ptr = (uint8 *) req_pkt + DIAG_HDR_SSCC_OFFSET;
  if (PS_LOGGING_DIAG_GET_SUPPORTED_PROTOCOLS != hget16(request_ptr))
  {
    LOG_MSG_INFO1("Invalid request is sent by QXDM", 0, 0, 0);
    return (diagpkt_err_rsp(DIAG_BAD_CMD_F, req_pkt, pkt_len));
  }

  /*-------------------------------------------------------------------------
    Memory to be allocated to the response:
    response is:
      - DIAG HDR
      - # of suported protocols - 1 byte
      - list of supported protocols (enum values)
  -------------------------------------------------------------------------*/
  len = DIAG_HDR_LEN + sizeof(uint8) +
        ((uint8)DPL_IID_NETPROT_MAX - (uint8)DPL_IID_NETPROT_MIN) *
          sizeof(dpl_iid_netprot_enum_type);

  /*-------------------------------------------------------------------------
    Allocate diagbuf to hold response
  -------------------------------------------------------------------------*/
  return_ptr =  diagpkt_subsys_alloc(DIAG_SUBSYS_PS_DATA_LOGGING,
                                     PS_LOGGING_DIAG_GET_SUPPORTED_PROTOCOLS,
                                     len);
  ASSERT(NULL != return_ptr);

  /*-------------------------------------------------------------------------
    Skip Diag header
  -------------------------------------------------------------------------*/
  resp_ptr = (uint8 *) return_ptr + DIAG_HDR_LEN;

  /*-------------------------------------------------------------------------
    Byte copy # of suppoted protocols
  -------------------------------------------------------------------------*/
  resp_ptr = put8(resp_ptr, (uint8)DPL_IID_NETPROT_MAX - (uint8)DPL_IID_NETPROT_MIN);

  /*-------------------------------------------------------------------------
    Fill enum value of each network protocol
  -------------------------------------------------------------------------*/
  for (i = DPL_IID_NETPROT_MIN; i < DPL_IID_NETPROT_MAX; i++)
  {
    resp_ptr = put8(resp_ptr, i);
  }

  return (return_ptr);

} /* ps_logging_diag_get_supported_protocols() */



/*===========================================================================

                                  EXTERNAL FUNCTIONS

===========================================================================*/
/*===========================================================================
FUNCTION    DPL_INIT

DESCRIPTION
  Registers functions with DIAG.

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void dpl_init
(
  void
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
#if !defined(IMAGE_APPS_PROC)
  LOG_MSG_INFO2("DPL_INIT() is called", 0, 0, 0);

  DIAGPKT_DISPATCH_TABLE_REGISTER(DIAG_SUBSYS_PS_DATA_LOGGING,
                                  ps_logging_diag_tbl);
#endif /* !IMAGE_APPS_PROC */
} /* dpl_init() */



/*===========================================================================
FUNCTION    DPLI_SET_PS_IFACE_PTR

DESCRIPTION
  Set ps_iface_ptr in the lookup table

PARAMETERS
  ps_iface_ptr : pointer to ps_iface structure
  ifname       : IID iface ID

RETURN VALUE
  None

DEPENDENCIES
  ps_iface_ptr and ifname must have been validated

SIDE EFFECTS
  None
===========================================================================*/
void dpli_set_ps_iface_ptr
(
  ps_iface_type           * ps_iface_ptr,
  dpl_iid_ifname_enum_type  ifname
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if ( (ifname < DPL_IID_IFNAME_MIN) ||
       (ifname >= DPL_IID_IFNAME_MAX) )
  {
    LOG_MSG_ERROR("ifname out of range %d", ifname, 0, 0);
    return;
  }
  ps_iface_dpl_lookup_table[ifname].ps_iface_ptr = ps_iface_ptr;

} /* dpli_set_ps_iface_ptr() */



/*===========================================================================
FUNCTION    DPLI_GET_PS_IFACE_DEFAULT_DESC

DESCRIPTION
  Get default description for this ifname from the lookup table

PARAMETERS
  ifname : IID ifname

RETURN VALUE
  Description of the iface corresponding to this ifname

DEPENDENCIES
  ifname must have been validated

SIDE EFFECTS
  None
===========================================================================*/
const char *dpli_get_ps_iface_default_desc
(
  dpl_iid_ifname_enum_type  ifname
)
{
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
  if ( (ifname < DPL_IID_IFNAME_MIN) ||
       (ifname >= DPL_IID_IFNAME_MAX) )
  {
    LOG_MSG_ERROR("ifname out of range %d", ifname, 0, 0);
    return NULL;
  }
  return  ps_iface_dpl_lookup_table[ifname].desc;

} /* dpli_get_ps_iface_default_desc() */
/*lint -restore Restore Lint Warning 641: Converting enum '{...}' to int*/

#endif /* FEATURE_DATA_PS_DATA_LOGGING */

#endif /* FEATURE_DATA_PS */
