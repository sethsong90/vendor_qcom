/*===========================================================================

                      P S _ I F A C E _ I O C T L. C

DESCRIPTION
  This file contains functions used by various modules to access
  network interface.

EXTERNALIZED FUNCTIONS

Copyright (c) 2002-2009 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary
===========================================================================*/

/*===========================================================================

                      EDIT HISTORY FOR FILE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  $PVCSPath: L:/src/asw/MM_DATA/vcs/ps_iface_ioctl.c_v   1.8   13 Feb 2003 14:15:20   ubabbar  $
  $Header: //source/qcom/qct/modem/datamodem/interface/netiface/rel/11.03/src/ps_iface_ioctl.c#2 $ $DateTime: 2011/07/06 05:28:44 $ $Author: vmordoho $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
08/20/08    hm     Moved many DSS ioctls from dss_iface_ioctl to here.
08/15/05    mct    Updated ps_iface_ioctl to be used only as pipe for mode
                   handler specific operations.
07/28/05    rt     Added new ioctl's DSS_IFACE_IOCTL_GET_IFACE_NAME and
                   DSS_IFACE_IOCTL_GET_IP_FAMILY.
05/12/05    ks     fixed lint errors
04/16/05    ks     Added support for new ioctl, DSS_IFACE_GET_HW_ADDR
11/19/04    msr    Using internal ps_iface macros instead of external ones.
10/30/04    msr    Moved handling of DSS_IFACE_IOCTL_GET_ALL_IFACES to
                   dss_iface_ioctl().
11/02/04    vp     Removed the handling of DSS_IFACE_GET_PHYS_LINK_STATE and
                   placed in dss_iface_ioctl.c.
10/13/04    vp     Removed the byte order conversion of v4 addresses.
08/02/04    ak     When getting DNS addrs, call functions in ps_iface to
                   retrieve.
07/30/04    vp     Changes due to consolidation of MTU and net_info structure
                   in ps_iface.
07/11/04    aku    Moved processing of REG and DEREV iface events to
                   dss_iface_ioctl.c
06/14/04    mct    Added IOCTL support for getting V6 primary and secondary DNS
                   addresses and for retrieving the v6 and generic ip address.
06/11/04    vp     Changes for representation of IP addresses as struct ps_in_addr
                   or struct ps_in6_addr in ps_ip_addr_type.
04/30/04    mct    Fixed some lint errors.
02/03/04    aku    Added support for Logical ifaces.
10/03/03    jd     enabled IP filtering ioctl's
08/28/03    ss     Updated to use ps_iface_id_type instead of name, instance
05/05/03    aku    Aded support for DSS_IFACE_IOCTL_GET_ALL_IFACES
03/31/03    aku    Added NULL checks for argval_ptr before deferencing it.
03/19/03    aku    Set the IP address type to IPV4 for secondary DNS address.
02/12/03    aku    Changed errnos to use ones in defined in dserrno.h
02/07/03    usb    Added ioctls for reg and dereg inbound IP filters.
01/17/03    atp    Added argument ps_iface_ptr to iface_ioctl_f_ptr.
01/17/03    usb    Added validation of iface_ptr in ps_iface_ioctl(),
                   included file dssocki.h to remove compiler warnings
12/24/02    aku    Added support for reg/dereg event callbacks
11/24/02    aku    Removed interface statistics IOCTL.
11/19/02    aku    Added interface statisitics IOCTL
11/11/02    aku    Initial version.
===========================================================================*/

/*===========================================================================

                       INCLUDE FILES FOR THE MODULE

===========================================================================*/
#include "comdef.h"
#include "customer.h"

#ifdef FEATURE_DATA_PS
#include "dserrno.h"
#include "dss_iface_ioctl.h" //need to re-visit this inclusion!
#include "ps_iface_ioctl.h"
#include "ps_iface_defs.h"
#include "ps_iface.h"
#include "msg.h"
#include "amssassert.h"
#include "ps_ifacei_utils.h"
#include "ps_aclrules.h"
#include "ps_in.h"
#include "ps_ifacei.h"

#ifdef FEATURE_DATA_PS_QOS
#include "ps_qos_spec_logging.h"
#endif /* FEATURE_DATA_PS_QOS */

#ifdef FEATURE_DATA_PS_IPV6
#include "ps_iface_addr_v6.h"
#include "ps_ifacei_addr_v6.h"
#include "ps_ip6_addr.h"
#endif /* FEATURE_DATA_PS_IPV6 */

#ifdef FEATURE_DATA_PS_DHCP
#include "dhcp_client.h"
#endif /* FEATURE_DATA_PS_DHCP */

#ifdef FEATURE_DATA_PS_DHCPV6
#include "dhcp6_client.h"
#endif /* FEATURE_DATA_PS_DHCPV6 */

#include "ds_Utils_DebugMsg.h"

/*===========================================================================

                      INTERNAL FUNCTION DEFINITIONS

===========================================================================*/
static int ps_iface_ioctli_get_all_ifaces
(
  ps_iface_type            *ps_iface_ptr,
  void                     *argval_ptr,
  sint15                   *ps_errno
)
{
  uint8                                    iface_index;
  dss_iface_ioctl_all_ifaces_type         *all_iface_info    = NULL;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (NULL == argval_ptr)
  {
    *ps_errno = DS_EFAULT;
    return -1;
  }

  all_iface_info = (dss_iface_ioctl_all_ifaces_type *) argval_ptr;
  all_iface_info->number_of_ifaces = 0;

  /*---------------------------------------------------------------------
    Search the global iface array
  ---------------------------------------------------------------------*/
  for (iface_index = 0; iface_index < MAX_SYSTEM_IFACES; iface_index++)
  {
    /*-------------------------------------------------------------------
      Since ifaces are added sequentially and they are never deleted, as
      soon as we get a NULL ptr there is no more iface pointers in the
      list.
    -------------------------------------------------------------------*/
    if (global_iface_ptr_array[iface_index] == NULL)
    {
      break;
    }
    all_iface_info->ifaces[iface_index] =
      PS_IFACE_GET_ID(global_iface_ptr_array[iface_index]);
    all_iface_info->number_of_ifaces++;
  }

  return 0;

}

static int ps_iface_ioctli_mode_handler_cback
(
  ps_iface_type            *ps_iface_ptr,
  ps_iface_ioctl_type      ioctl_name,
  void                     *argval_ptr,
  sint15                   *ps_errno
)
{
  ps_iface_type                           *assoc_iface_ptr = NULL;
  int                                      ret_val = -1;
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  /*-------------------------------------------------------------------------
    Call the mode handler registered ioctl callback.
  -------------------------------------------------------------------------*/
    LOG_MSG_INFO2( "MH specific IOCTL %d, iface 0x%x:%d",
                   ioctl_name, ps_iface_ptr->name, ps_iface_ptr->instance);

  /*-------------------------------------------------------------------------
    Handle logical ifaces.
  -------------------------------------------------------------------------*/
  if (PS_IFACEI_IS_LOGICAL(ps_iface_ptr))
  {
    /*-------------------------------------------------------------------
      Go through the chain of ifaces and execute the first one which
      has registered an ioctl hdlr
    -------------------------------------------------------------------*/
    assoc_iface_ptr = ps_iface_ptr;
    while(assoc_iface_ptr != NULL)
    {
      if (assoc_iface_ptr->iface_ioctl_f_ptr != NULL)
      {
        ret_val = assoc_iface_ptr->iface_ioctl_f_ptr(assoc_iface_ptr,
                                                     ioctl_name,
                                                     argval_ptr,
                                                     ps_errno);
        if (-1 == ret_val)
        {
          LOG_MSG_INFO1("Using cache for IOCTL %d, iface 0x%x:%d",
                        ioctl_name,
                        assoc_iface_ptr->name,
                        assoc_iface_ptr->instance);

          if (PS_IFACE_IOCTL_GET_BEARER_TECHNOLOGY == ioctl_name)
          {
            if (NULL == argval_ptr)
            {
              break;
            }

            memcpy(argval_ptr,
                   &(assoc_iface_ptr->event_info_cache.\
                       bearer_tech_changed_info.new_bearer_tech),
                   sizeof (ps_iface_bearer_technology_type));
            ret_val = 0;
          }
          else if (PS_IFACE_IOCTL_GET_DATA_BEARER_RATE == ioctl_name)
          {
            if (NULL == argval_ptr)
            {
              break;
            }

            memset(argval_ptr, 0, sizeof (ps_iface_ioctl_data_bearer_rate));
            ret_val = 0;
          }
        }

        break;
      }

      assoc_iface_ptr = PS_IFACE_GET_ASSOC_IFACE(assoc_iface_ptr);
    }

    /*-------------------------------------------------------------------
      If after looping through the chain of associated ifaces, we do not
      find an appropriate ioctl function handler for the specified IOCTL,
      return DS_EINVAL error code.
    -------------------------------------------------------------------*/
    if (assoc_iface_ptr == NULL)
    {
      LOG_MSG_ERROR( "Unsupported IOCTL 0x%x on iface 0x%x:%d",
                     ioctl_name, ps_iface_ptr->name, ps_iface_ptr->instance);
      *ps_errno = DS_EINVAL;
      ret_val = -1;
    }

    return ret_val;
  }

  /*-------------------------------------------------------------------------
    Handle non-logical ifaces.
  -------------------------------------------------------------------------*/
  if (ps_iface_ptr->iface_ioctl_f_ptr != NULL &&
      !PS_IFACEI_IS_LOGICAL(ps_iface_ptr))
  {
    LOG_MSG_INFO2( "Interface specific ioctl=0x%x, calling callback",
             ioctl_name,0,0);
    return ps_iface_ptr->iface_ioctl_f_ptr(ps_iface_ptr,
                                           ioctl_name,
                                           argval_ptr,
                                           ps_errno);
  }

  /*-------------------------------------------------------------------------
    Iface is neither logical nor has an associated IOCTL callback.
  -------------------------------------------------------------------------*/
  LOG_MSG_ERROR( "Inv ioctl %d, iface 0x%x:%d", ioctl_name, ps_iface_ptr->name,
             ps_iface_ptr->instance);

  *ps_errno = DS_EINVAL;
  return -1;

} /* ps_iface_ioctli_mode_handler_cback() */



/*===========================================================================

                      GLOBAL FUNCTION DEFINITIONS

===========================================================================*/
/*===========================================================================
FUNCTION PS_IFACE_IOCTL()

DESCRIPTION
  This function performs various operations on the given ps interface.
  Typically, these operations are to get or set a value.

DEPENDENCIES
  None.

PARAMETERS

  ps_iface_type             - ps_iface on which the specified operations
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
                              specified arguments for the IOCTL are correct
                              but dss_iface_ioctl() or a mode handler
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
int ps_iface_ioctl
(
  ps_iface_type            *ps_iface_ptr,
  ps_iface_ioctl_type      ioctl_name,
  void                     *argval_ptr,
  sint15                   *ps_errno
)
{
  ip_addr_type                              *ip_addr                = NULL;
  ps_ip_addr_type                            ret_ip_addr;
  uint32                                    *mtu                    = NULL;
  ps_iface_state_enum_type                  *iface_state_ptr        = NULL;
  ip_addr_type                               prim_dns_addr, sec_dns_addr;
  ps_iface_ioctl_hw_addr_type               *iface_hw_addr          = NULL;
  uint8                                      hw_addr_len;
  ps_iface_name_enum_type                   *iface_name_ptr         = NULL;
  ps_iface_addr_family_type                 *family_ptr             = NULL;
  boolean                                   *laptop_call_status_ptr = NULL;
  boolean                                    is_allowed             = FALSE;
  dss_iface_ioctl_delete_firewall_rule_type *delete_firewall_rule;
  uint32                                     handle;
  ps_iface_ioctl_get_firewall_table_type    *get_firewall_table;
  dss_iface_ioctl_get_firewall_rule_type    *get_firewall_rule;
  uint8					                       *hw_addr;

#if defined(FEATURE_DATA_PS_DHCP) || defined(FEATURE_DATA_PS_DHCPV6)
  void                                    *dhcp_client_handle;
#endif /* defined(FEATURE_DATA_PS_DHCP) || defined(FEATURE_DATA_PS_DHCPV6) */

#ifdef FEATURE_DATA_PS_IPV6
  dss_iface_ioctl_priv_ipv6_addr_type     *priv_addr;
  ps_ipv6_iid_params_type                  iid_params;
  dss_iface_ioctl_get_all_v6_prefixes_type *all_v6_prefixes = NULL;
  ps_iface_prefix_info_type                prefix_info[MAX_IPV6_PREFIXES];
  int                                      result;
#endif /* FEATURE_DATA_PS_IPV6 */
  uint8                                    index;
  dss_iface_ioctl_get_all_dns_addrs_type  *get_dns_addrs_ptr = NULL;
  dss_iface_ioctl_domain_name_search_list_type *domain_name_search_list_info = NULL;

  dss_iface_ioctl_sip_serv_addr_info_type         *sip_serv_addr_info;
  dss_iface_ioctl_sip_serv_domain_name_info_type  *sip_serv_domain_name_info;

#ifdef FEATURE_DATA_PS_QOS
  ps_iface_ioctl_qos_request_type         *qos_req_ptr   = NULL;
  ps_iface_ioctl_qos_request_ex_type      *qos_request_ex_ptr = NULL;
  uint8                                   qos_index;
  qsl_qos_opcode_enum_type                qsl_opcode;
#endif /* FEATURE_DATA_PS_QOS */
/*- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

  if (ps_errno == NULL)
  {
    LOG_MSG_ERROR( "NULL args", 0, 0, 0);
    ASSERT(0);
    return -1;
  }

  *ps_errno = DSS_SUCCESS;

  /*-------------------------------------------------------------------------
    GET_ALL_IFACES is a special case, iface pointer need not be valid here.
  -------------------------------------------------------------------------*/
  if (PS_IFACE_IOCTL_GET_ALL_IFACES == ioctl_name)
  {
    return ps_iface_ioctli_get_all_ifaces (ps_iface_ptr, argval_ptr, ps_errno);
  }

  if ( !(PS_IFACE_IS_VALID(ps_iface_ptr)) )
  {
    LOG_MSG_ERROR( "IOCTL %d failed, invalid iface 0x%p",
              ioctl_name, ps_iface_ptr, 0);
    *ps_errno = DS_EBADF;
    goto bail;
  }

  LOG_MSG_INFO2 ( "IOCTL %d, iface 0x%x:%d",
           ioctl_name, ps_iface_ptr->name, ps_iface_ptr->instance);

  /*-------------------------------------------------------------------------
    More special case: For the followign IOCTLs the iface addr family
    needs to verified.
  -------------------------------------------------------------------------*/
  switch(ioctl_name)
  {
    case PS_IFACE_IOCTL_GET_IPV4_ADDR:
    case PS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR:
    case PS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR:

      if ( !(ps_iface_addr_family_is_v4(ps_iface_ptr)) )
      {
        LOG_MSG_ERROR( "v4 iface required", 0, 0, 0);
        *ps_errno = DS_EINVAL;
        goto bail;
      }
      break;

#ifdef FEATURE_DATA_PS_IPV6
    case PS_IFACE_IOCTL_GET_IPV6_ADDR:
    case PS_IFACE_IOCTL_GET_ALL_V6_PREFIXES:
    case PS_IFACE_IOCTL_GET_IPV6_PRIM_DNS_ADDR:
    case PS_IFACE_IOCTL_GET_IPV6_SECO_DNS_ADDR:
    case PS_IFACE_IOCTL_GENERATE_PRIV_IPV6_ADDR:

      if ( !(ps_iface_addr_family_is_v6(ps_iface_ptr)) )
      {
        LOG_MSG_ERROR( "v6 iface required", 0, 0, 0);
        *ps_errno = DS_EINVAL;
        goto bail;
      }
    break;
#endif /* FEATURE_DATA_PS_IPV6 */

    default:
      break;

  } /* switch (ioctl_name) */

  /*-------------------------------------------------------------------------
    Service the IOCTL based on the ioctl name
  -------------------------------------------------------------------------*/
  switch (ioctl_name)
  {
    /*-----------------------------------------------------------------------
      Get IPV4 addr of iface
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_IPV4_ADDR:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }
      ip_addr = (ip_addr_type *)argval_ptr;
      ip_addr->type = IPV4_ADDR;
      ret_ip_addr.type = IPV4_ADDR;
      ps_iface_get_addr(ps_iface_ptr,&ret_ip_addr);
      ip_addr->addr.v4 = ret_ip_addr.addr.v4.ps_s_addr;

      IPV4_ADDR_MSG(ip_addr->addr.v4);
      break;

    /*-----------------------------------------------------------------------
      Get IPV4 Primary DNS addr of iface
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_IPV4_PRIM_DNS_ADDR:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }
      ps_iface_get_v4_dns_addrs (ps_iface_ptr,&prim_dns_addr, &sec_dns_addr);
      ip_addr = (ip_addr_type *)argval_ptr;
      ip_addr->addr.v4 = prim_dns_addr.addr.v4;
      ip_addr->type = prim_dns_addr.type;

      IPV4_ADDR_MSG(ip_addr->addr.v4);
      break;

    /*-----------------------------------------------------------------------
      Get IPV4 Secondary DNS addr of iface
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_IPV4_SECO_DNS_ADDR:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }

      ps_iface_get_v4_dns_addrs (ps_iface_ptr,&prim_dns_addr, &sec_dns_addr);
      ip_addr = (ip_addr_type *)argval_ptr;
      ip_addr->addr.v4 = sec_dns_addr.addr.v4;
      ip_addr->type = sec_dns_addr.type;

      IPV4_ADDR_MSG(ip_addr->addr.v4);
      break;

    case PS_IFACE_IOCTL_GET_ALL_DNS_ADDRS:
      get_dns_addrs_ptr = (dss_iface_ioctl_get_all_dns_addrs_type *)argval_ptr;

      if (NULL == get_dns_addrs_ptr ||
          0    == get_dns_addrs_ptr->num_dns_addrs ||
          NULL == get_dns_addrs_ptr->dns_addrs_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }

      ps_iface_get_all_dns_addrs (ps_iface_ptr,
                                  get_dns_addrs_ptr->dns_addrs_ptr,
                                  &(get_dns_addrs_ptr->num_dns_addrs));

      /*---------------------------------------------------------------------
        Print retrieved DNS addresses.
      ---------------------------------------------------------------------*/
      if(ps_iface_addr_family_is_v4 (ps_iface_ptr))
      {
        for(index = 0; index < get_dns_addrs_ptr->num_dns_addrs; index++)
        {
          IPV4_ADDR_MSG(get_dns_addrs_ptr->dns_addrs_ptr[index].addr.v4);
        }

      }
#ifdef FEATURE_DATA_PS_IPV6
      else if(ps_iface_addr_family_is_v6(ps_iface_ptr))
      {
        for(index = 0; index < get_dns_addrs_ptr->num_dns_addrs; index++)
        {
          IPV6_ADDR_MSG(get_dns_addrs_ptr->dns_addrs_ptr[index].addr.v6);
        }
      }
#endif /* FEATURE_DATA_PS_IPV6 */
      break;

#ifdef FEATURE_DATA_PS_IPV6

    /*------------------------------------------------------------------------
      Get IPV6 addr of iface
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_IPV6_ADDR:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }
      ip_addr = (ip_addr_type *)argval_ptr;
      ip_addr->type = IPV6_ADDR;
      ret_ip_addr.type = IPV6_ADDR;
      ps_iface_get_addr (ps_iface_ptr,&ret_ip_addr);
      ip_addr->addr.v6[0] = ret_ip_addr.addr.v6.ps_s6_addr64[0];
      ip_addr->addr.v6[1] = ret_ip_addr.addr.v6.ps_s6_addr64[1];

      IPV6_ADDR_MSG(ip_addr->addr.v6);
      break;

    /*------------------------------------------------------------------------
      Get all IPV6 prefixes of the interface
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_ALL_V6_PREFIXES:
      all_v6_prefixes = (dss_iface_ioctl_get_all_v6_prefixes_type *)argval_ptr;
      if (all_v6_prefixes == NULL || 0 == all_v6_prefixes->num_prefixes)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }

      ps_iface_get_all_v6_prefixes (ps_iface_ptr,
                                    &prefix_info[0],
                                    &all_v6_prefixes->num_prefixes);

      for(index = 0; index < all_v6_prefixes->num_prefixes; index++)
      {
        memcpy(&all_v6_prefixes->prefix_info_ptr[index].prefix,
               &prefix_info[index].prefix,
               sizeof(struct ps_in6_addr));

        all_v6_prefixes->prefix_info_ptr[index].prefix_state =
          (dss_iface_ioctl_ipv6_addr_state_enum_type) prefix_info[index].prefix_state;
        all_v6_prefixes->prefix_info_ptr[index].prefix_len =
          prefix_info[index].prefix_len;
      }

      break;

    /*-----------------------------------------------------------------------
      Get IPV6 Primary DNS addr of iface
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_IPV6_PRIM_DNS_ADDR:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }
      ps_iface_get_v6_dns_addrs (ps_iface_ptr,&prim_dns_addr, &sec_dns_addr);
      ip_addr = (ip_addr_type *)argval_ptr;
      ip_addr->addr.v6[0] = prim_dns_addr.addr.v6[0];
      ip_addr->addr.v6[1] = prim_dns_addr.addr.v6[1];
      ip_addr->type = prim_dns_addr.type;

      IPV6_ADDR_MSG(ip_addr->addr.v6);
      break;

    /*-----------------------------------------------------------------------
      Get IPV6 Secondary DNS addr of iface
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_IPV6_SECO_DNS_ADDR:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }
      ps_iface_get_v6_dns_addrs (ps_iface_ptr,&prim_dns_addr, &sec_dns_addr);
      ip_addr = (ip_addr_type *)argval_ptr;
      ip_addr->addr.v6[0] = sec_dns_addr.addr.v6[0];
      ip_addr->addr.v6[1] = sec_dns_addr.addr.v6[1];
      ip_addr->type = sec_dns_addr.type;

      IPV6_ADDR_MSG(ip_addr->addr.v6);
      break;

    case PS_IFACE_IOCTL_GENERATE_PRIV_IPV6_ADDR:
      //TODO: Register for events ???
      priv_addr = (dss_iface_ioctl_priv_ipv6_addr_type *) argval_ptr;

      if (NULL == priv_addr ||
          NULL == priv_addr->ip_addr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }

      /*---------------------------------------------------------------------
        This is an application initiated request to generate a new privacy
        address.
      ---------------------------------------------------------------------*/
      iid_params.app_request = TRUE;
      iid_params.is_unique   = priv_addr->iid_params.is_unique;

      result = ps_iface_generate_priv_ipv6_addr (ps_iface_ptr,
                                                 &ret_ip_addr,
                                                 &iid_params,
                                                 ps_errno);
      if (0 != result)
      {
        goto bail;
      }

      priv_addr->ip_addr->type       = ret_ip_addr.type;
      priv_addr->ip_addr->addr.v6[0] = ret_ip_addr.addr.v6.ps_s6_addr64[0];
      priv_addr->ip_addr->addr.v6[1] = ret_ip_addr.addr.v6.ps_s6_addr64[1];
      break;
#endif /* FEATURE_DATA_PS_IPV6 */

    /*-----------------------------------------------------------------------
      Get MTU of iface
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_MTU:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }
      mtu = (uint32 *)argval_ptr;
      *mtu = ps_iface_ptr->net_info.mtu;
      LOG_MSG_INFO2( "MTU is %d", *mtu, 0, 0);
      break;

    /*-----------------------------------------------------------------------
      Get IP addr of iface
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_IP_ADDR:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }

      ip_addr = (ip_addr_type *)argval_ptr;
      ip_addr->type = ps_iface_get_addr_family(ps_iface_ptr);
      ret_ip_addr.type = ip_addr->type;
      ps_iface_get_addr (ps_iface_ptr, &ret_ip_addr);

      switch(ip_addr->type)
      {
        case IPV4_ADDR:
          ip_addr->addr.v4 = ret_ip_addr.addr.v4.ps_s_addr;
          IPV4_ADDR_MSG(ip_addr->addr.v4);
          break;

#ifdef FEATURE_DATA_PS_IPV6
        case IPV6_ADDR:
          ip_addr->addr.v6[0] = ret_ip_addr.addr.v6.ps_s6_addr64[0];
          ip_addr->addr.v6[1] = ret_ip_addr.addr.v6.ps_s6_addr64[1];
          IPV6_ADDR_MSG(ip_addr->addr.v6);
          break;
#endif

        default:
          ip_addr->type = IP_ADDR_INVALID;
          *ps_errno = DS_EFAULT;
          break;
      }
      break;

    /*-----------------------------------------------------------------------
      Get state of iface
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_STATE:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }
      iface_state_ptr = (ps_iface_state_enum_type *)argval_ptr;
      *iface_state_ptr = ps_iface_state (ps_iface_ptr);
      LOG_MSG_INFO2 ( "Iface state is %d",*iface_state_ptr, 0, 0);
      break;

    /*-----------------------------------------------------------------------
      Get the address of the hardware.
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_HW_ADDR:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }
      iface_hw_addr = (ps_iface_ioctl_hw_addr_type *)argval_ptr;
      hw_addr_len   = PS_IFACE_HW_ADDR_LEN(ps_iface_ptr);

      /*-----------------------------------------------------------------------
        if HW lenght is 0, iface does not support HW Address
      -----------------------------------------------------------------------*/
      if (0 == hw_addr_len)
      {
        *ps_errno = DS_EINVAL;
        goto bail;
      }

      /*---------------------------------------------------------------------
        Validate if the size of buffer provided is enough for the hw address
      ---------------------------------------------------------------------*/
      if (iface_hw_addr->hw_addr_len < hw_addr_len)
      {
        iface_hw_addr->hw_addr_len = hw_addr_len;
        *ps_errno = DS_EFAULT;
        goto bail;
      }

      hw_addr = PS_IFACE_HW_ADDR_PTR(ps_iface_ptr);
      if (NULL == hw_addr)
      {
        iface_hw_addr->hw_addr_len = hw_addr_len;
        *ps_errno = DS_EINVAL;
        goto bail;
      } 

      memcpy(iface_hw_addr->hw_addr, hw_addr, hw_addr_len);
      iface_hw_addr->hw_addr_len = hw_addr_len;
      break;

    /*-----------------------------------------------------------------------
      Get iface name
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_IFACE_NAME:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }
      iface_name_ptr = (ps_iface_name_enum_type *)argval_ptr;
      if (ps_iface_ptr->name == IWLAN_3GPP_IFACE)
      {
        *iface_name_ptr = UMTS_IFACE;
      }
      else if (ps_iface_ptr->name == IWLAN_3GPP2_IFACE)
      {
        *iface_name_ptr = CDMA_SN_IFACE;
      }
      else
      {
        *iface_name_ptr = ps_iface_ptr->name;
      }
      break;

    /*-----------------------------------------------------------------------
      Get iface IP family
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_IP_FAMILY:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }
      family_ptr = (ps_iface_addr_family_type *) argval_ptr;
      *family_ptr = ps_iface_get_addr_family (ps_iface_ptr);
      LOG_MSG_INFO2( "IFACE family is %d", *family_ptr, 0, 0);
      break;

    /*-----------------------------------------------------------------------
      Check if iface is in laptop call
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_IS_LAPTOP_CALL_ACTIVE:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }

      laptop_call_status_ptr = (boolean *) argval_ptr;
      *laptop_call_status_ptr = PS_IFACE_IS_IN_LAPTOP_CALL(ps_iface_ptr);
      break;

    /*-----------------------------------------------------------------------
      Refresh DHCP config info
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_REFRESH_DHCP_CONFIG_INFO:

#if (!defined(FEATURE_DATA_PS_DHCP)) && (!defined(FEATURE_DATA_PS_DHCPV6))
      LOG_MSG_ERROR( "DHCP client ioctl not supported", 0,0,0);
      *ps_errno = DS_EOPNOTSUPP;
      goto bail;

#else
      if (NULL != argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }

      dhcp_client_handle = ps_iface_get_dhcp_client_handle (ps_iface_ptr);
      if (0 == dhcp_client_handle)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }


#ifdef FEATURE_DATA_PS_DHCP
      if (TRUE == ps_iface_addr_family_is_v4 (ps_iface_ptr))
      {
        /* Get already configured ip_addr */
        ret_ip_addr.type = IPV4_ADDR;
        ps_iface_get_addr (ps_iface_ptr, &ret_ip_addr);
        ASSERT(IPV4_ADDR == ret_ip_addr.type);

        /* Trigger non-address config refresh */
        dhcp_client_inform(dhcp_client_handle, ret_ip_addr.addr.v4.ps_s_addr);
        break;
      }
#endif /* FEATURE_DATA_PS_DHCP */

#ifdef FEATURE_DATA_PS_DHCPV6
      if (TRUE == ps_iface_addr_family_is_v6 (ps_iface_ptr))
      {
        dhcp6_client_inform(dhcp_client_handle);
        break;
      }
#endif /* FEATURE_DATA_PS_DHCPV6 */

      LOG_MSG_ERROR( "Unsupported addr family", 0, 0, 0);
      *ps_errno = DS_EAFNOSUPPORT;
      goto bail;

#endif /* (!defined(FEATURE_DATA_PS_DHCP)) && (!defined(FEATURE_DATA_PS_DHCPV6)) */

    /*-----------------------------------------------------------------------
      Get domain name search list
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_DOMAIN_NAME_SEARCH_LIST:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }

      domain_name_search_list_info =
        (dss_iface_ioctl_domain_name_search_list_type *)argval_ptr;
      ps_iface_get_domain_name_search_list
      (
        ps_iface_ptr,
        domain_name_search_list_info->name_array,
        &domain_name_search_list_info->count
      );

      break;

    /*-----------------------------------------------------------------------
      Get SIP address list.
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_SIP_SERV_ADDR:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }

      sip_serv_addr_info =
        (dss_iface_ioctl_sip_serv_addr_info_type *) argval_ptr;
      ps_iface_get_sip_serv_addr(ps_iface_ptr,
                                 sip_serv_addr_info->addr_array,
                                 &sip_serv_addr_info->count);
      break;

    /*-----------------------------------------------------------------------
      Get SIP domain name list.
    -----------------------------------------------------------------------*/
    case PS_IFACE_IOCTL_GET_SIP_SERV_DOMAIN_NAMES:
      if (NULL == argval_ptr)
      {
        *ps_errno = DS_EFAULT;
        break;
      }
      sip_serv_domain_name_info =
        (dss_iface_ioctl_sip_serv_domain_name_info_type *)argval_ptr;
      ps_iface_get_sip_domain_names( ps_iface_ptr,
                                     sip_serv_domain_name_info->name_array,
                                     &sip_serv_domain_name_info->count);
      break;

    case PS_IFACE_IOCTL_ENABLE_FIREWALL:
      if (NULL == argval_ptr) 
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }

      is_allowed = ( *(boolean *) argval_ptr );

      if(FALSE == ps_iface_enable_firewall( ps_iface_ptr,
                                            is_allowed))
      {
        LOG_MSG_INFO2("Error enabling firewall", 0, 0, 0);
        *ps_errno = DS_EINVAL;
        goto bail;
      }
      
      break;


    case PS_IFACE_IOCTL_DISABLE_FIREWALL:
    
    if(FALSE == ps_iface_disable_firewall( ps_iface_ptr ))
    {
      LOG_MSG_INFO2("Error disbling firewall", 0, 0, 0);
      *ps_errno = DS_EINVAL;
      goto bail;
    }

    break;


    case PS_IFACE_IOCTL_DELETE_FIREWALL_RULE:
      if (NULL == argval_ptr) 
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }
      
      delete_firewall_rule = 
        (dss_iface_ioctl_delete_firewall_rule_type *)argval_ptr;

      handle = ( delete_firewall_rule->handle );
      if(FALSE == ps_iface_delete_firewall_rule( ps_iface_ptr,
                                                 handle ))
      {
        LOG_MSG_ERROR("Error deleting firewall rule", 0, 0, 0);
        *ps_errno = DS_EINVAL;
        goto bail;
      }

      break;

    case PS_IFACE_IOCTL_GET_FIREWALL_RULE:
      if (NULL == argval_ptr) 
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }

      get_firewall_rule = 
        (dss_iface_ioctl_get_firewall_rule_type *)argval_ptr;

      if(FALSE == ps_iface_get_firewall_rule( ps_iface_ptr,
                                              &get_firewall_rule->fltr_spec, 
                                              get_firewall_rule->handle))
      {
        LOG_MSG_ERROR("Error getting firewall rule", 0, 0, 0);
        *ps_errno = DS_EINVAL;
        goto bail;
      }

      break;
    
    case PS_IFACE_IOCTL_GET_FIREWALL_TABLE:
      if (NULL == argval_ptr) 
      {
        *ps_errno = DS_EFAULT;
        goto bail;
      }

      get_firewall_table = 
        (ps_iface_ioctl_get_firewall_table_type *)argval_ptr;

      if(FALSE == ps_iface_get_firewall_table( ps_iface_ptr,
                                               get_firewall_table->fltr_spec_arr, 
                                               get_firewall_table->num_fltrs,
                                               &get_firewall_table->avail_num_fltrs,
                                               get_firewall_table->handle_arr))
      {
        LOG_MSG_ERROR("Error getting firewall table", 0, 0, 0);
        *ps_errno = DS_EINVAL;
        goto bail;
      }

    break;

    default:
      /*---------------------------------------------------------------------
        Interface specific operation. Call the registered callback.
      ---------------------------------------------------------------------*/
      if (-1 == ps_iface_ioctli_mode_handler_cback (ps_iface_ptr,
                                                    ioctl_name,
                                                    argval_ptr,
                                                    ps_errno))
      {
        goto bail;
      }

#ifdef FEATURE_DATA_PS_QOS
      /*---------------------------------------------------------------------
        Log qos_spec requested by the application
      ---------------------------------------------------------------------*/
      if (ioctl_name == PS_IFACE_IOCTL_QOS_REQUEST)
      {
        qos_req_ptr = (ps_iface_ioctl_qos_request_type *) argval_ptr;

        ps_qsl_log_requested_qos_spec(qos_req_ptr->qos_ptr,
                                      ps_iface_ptr,
                                      qos_req_ptr->flow_ptr,
                                      QSL_QOS_REQUEST);

      }
      else if (ioctl_name == PS_IFACE_IOCTL_QOS_REQUEST_EX)
      {
        qos_request_ex_ptr = (ps_iface_ioctl_qos_request_ex_type *) argval_ptr;

        if (qos_request_ex_ptr->opcode == PS_IFACE_IOCTL_QOS_REQUEST_OP)
        {
          qsl_opcode = QSL_QOS_REQUEST;
        }
        else
        {
          qsl_opcode = QSL_QOS_CONFIGURE;
        }

        for (qos_index = 0;
             qos_index < qos_request_ex_ptr->num_qos_specs;
             qos_index++)
        {
          ps_qsl_log_requested_qos_spec
          (
            qos_request_ex_ptr->qos_specs_ptr + qos_index,
            ps_iface_ptr,
            qos_request_ex_ptr->flows_ptr[qos_index],
            qsl_opcode
          );
        }
      }
#endif /* FEATURE_DATA_PS_QOS */

      break;
  } /* switch */

  /*-------------------------------------------------------------------------
    In success case, we break out of the switch. Please be aware of this
    if you are adding any code after the above switch statement.
  -------------------------------------------------------------------------*/
  return 0;

bail:
  LOG_MSG_ERROR ( "ps_iface_ioctl err %d", *ps_errno, 0, 0);
  return -1;

} /* ps_iface_ioctl() */

#endif /* FEATURE_DATA_PS */
