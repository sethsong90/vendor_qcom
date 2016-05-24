/******************************************************************************

                        N E T M G R _ K I F . C

******************************************************************************/

/******************************************************************************

  @file    netmgr_kif.c
  @brief   Network manager Kernel Interface Module

  DESCRIPTION
  Implementation of Kernel Interface module.

******************************************************************************/
/*===========================================================================

  Copyright (c) 2010-2014 Qualcomm Technologies, Inc. All Rights Reserved

  Qualcomm Technologies Proprietary and Confidential.

  Export of this technology or software is regulated by the U.S. Government.
  Diversion contrary to U.S. law prohibited.

  All ideas, data and information contained in or disclosed by
  this document are confidential and proprietary information of
  Qualcomm Technologies, Inc. and all rights therein are expressly reserved.
  By accepting this material the recipient agrees that this material
  and the information contained therein are held in confidence and in
  trust and will not be used, copied, reproduced in whole or in part,
  nor its contents revealed in any manner to others without the express
  written permission of Qualcomm Technologies, Inc.

===========================================================================*/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id:$

when       who        what, where, why
--------   ---        -------------------------------------------------------
02/11/10   ar         Initial version (derived from DSC file)
12/10/12   harouth    Added DNS route adding feature

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/udp.h>
#include <errno.h>
#include <stdint.h>
#include <linux/msm_rmnet.h>
#include <assert.h>
#include "stringl.h"
#ifdef FEATURE_DATA_LINUX_LE
#include <netinet/icmp6.h>
#else
#include <linux/in6.h>
#include <netinet/icmp6.h>
#include <netinet/in.h>
#include <sys/system_properties.h>
#include "cutils/properties.h"
#ifdef FEATURE_GB_NET_UTILS
#include "ifc_utils.h"
#else
#include "ifc.h"
#endif
#endif

#include "ds_cmdq.h"
#include "ds_string.h"
#include "ds_util.h"
#include "netmgr_defs.h"
#include "netmgr_util.h"
#include "netmgr_platform.h"
#include "netmgr_netlink.h"
#include "netmgr_exec.h"
#include "netmgr_kif.h"
#include "netmgr_main.h"

#ifdef NETMGR_OFFTARGET
#include "netmgr_stubs.h"
#endif

#define NETMGR_KIF_IPV6_MULTICAST_ROUTER_ADDR  "FF02::2"
#define NETMGR_KIF_MAX_COMMAND_LENGTH   200
#define NETMGR_KIF_ARR_SIZE(x)  (sizeof(x)/sizeof(x[0]))

#define NETMGR_NUMBER_OF_INT32_IN_IPV6_ADDR 4

#define NETMGR_KIF_DROP_SSDP_CMD_FORMAT "iptables -I OUTPUT 1 -o %s -p udp --dport 1900 -j DROP -m comment --comment \"Drop SSDP on WWAN\""
/* Defined in libnetutils.  These are not declared in any header file. */
#if defined FEATURE_DS_LINUX_ANDROID && defined FEATURE_GB_NET_UTILS
extern int ifc_reset_connections(const char *ifname);
#endif


/*---------------------------------------------------------------------------
  iptables rules for dropping outgoing reset packets when screen is off.
---------------------------------------------------------------------------*/
/* Command for installing rules after screen is OFF and ADB property on */
#define NETMGR_IPTABLES_RULE_DROP_TCP_RST_PKTS     "iptables -A OUTPUT -p tcp --tcp-flags RST RST -j DROP -m comment --comment \"Drop outgoing TCP resets\""

/* Command for deleting rules after screen is ON */
#define NETMGR_IPTABLES_RULE_DEL_DROP_TCP_RST_PKTS "iptables -D OUTPUT -p tcp --tcp-flags RST RST -j DROP -m comment --comment \"Drop outgoing TCP resets\""

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/


/*---------------------------------------------------------------------------
   Constant representing maximum length of interface/device name's prefix,
   i.e. without the numeric instance # (e.g. 'eth' in 'eth0')
---------------------------------------------------------------------------*/
#define NETMGR_KIF_NAME_PR_MAX_LEN (IFNAMSIZ - 3)

/*---------------------------------------------------------------------------
   Constant string representing default interface name
---------------------------------------------------------------------------*/
#define NETMGR_KIF_DEF_NAME "eth"

/*---------------------------------------------------------------------------
   Constant representing global IP address scope
---------------------------------------------------------------------------*/
#define NETMGR_KIF_GLOBAL_SCOPE   RT_SCOPE_UNIVERSE
#define NETMGR_KIF_LOCAL_SCOPE    RT_SCOPE_LINK

/*---------------------------------------------------------------------------
   Constants for Modem port open retry mechanism
---------------------------------------------------------------------------*/
#define NETMGR_KIF_MAX_RETRY_COUNT               120
#define NETMGR_KIF_WAIT_TIME_BEFORE_NEXT_RETRY   500000 /* usec */
#define NETMGR_KIF_USEC_TO_SEC(a)                ((a)/1000000)

/*---------------------------------------------------------------------------
   Constant string representing source address of unix domain socket
---------------------------------------------------------------------------*/
#define NETMGR_KIF_UDS_CLIENT_PATH "/tmp/uds_clnt.1234"

/*---------------------------------------------------------------------------
  Constant strings for kernel driver sysfs files to report transport port
  open status.  This is to supprot power subsystem test automation.
---------------------------------------------------------------------------*/
#define NETMGR_KIF_SYSFILE_OPEN_TIMEOUT "/data/data_test/modem_port_timeout"
#define NETMGR_KIF_SYSFILE_OPEN_STATUS  "/data/data_test/modem_port_status"
#define NETMGR_KIF_OPEN_SUCCESS (1)
#define NETMGR_KIF_OPEN_FAILURE (0)
#define NETMGR_KIF_PORT_OPENED NETMGR_KIF_OPEN_SUCCESS
#define NETMGR_KIF_PORT_CLOSED NETMGR_KIF_OPEN_FAILURE

/*---------------------------------------------------------------------------
   Constant string representing template for network interface flush
---------------------------------------------------------------------------*/
#if FEATURE_DATA_LINUX_LE
#define NETMGR_KIF_SYSCMD_FLUSHADDR "/bin/ip addr flush dev %s"
#else
#define NETMGR_KIF_SYSCMD_FLUSHADDR "/system/bin/ip addr flush dev %s"
#endif
#define NETMGR_KIF_SYSCMD_SIZ  (256)


#ifdef FEATURE_DS_LINUX_ANDROID
/*---------------------------------------------------------------------------
   Macros for Android DNS property assignment.
---------------------------------------------------------------------------*/
#define ADDR_FMT_V4( val )                  \
        (unsigned char)(val >>  0),         \
        (unsigned char)(val >>  8),         \
        (unsigned char)(val >> 16),         \
        (unsigned char)(val >> 24)

#define ADDR_FMT_V6( val )                  \
        (uint16)(ntohs(val[0] >>  0)),      \
        (uint16)(ntohs(val[0] >> 16)),      \
        (uint16)(ntohs(val[0] >> 32)),      \
        (uint16)(ntohs(val[0] >> 48)),      \
        (uint16)(ntohs(val[1] >>  0)),      \
        (uint16)(ntohs(val[1] >> 16)),      \
        (uint16)(ntohs(val[1] >> 32)),      \
        (uint16)(ntohs(val[1] >> 48))

static char name_buf[PROPERTY_KEY_MAX];
static char addr_buf[PROPERTY_VALUE_MAX];


/*---------------------------------------------------------------------------
   Macros for Android network interface MTU override
---------------------------------------------------------------------------*/
#define NETMGR_KIF_PROPERTY_MTU          "persist.data_netmgrd_mtu"
#define NETMGR_KIF_PROPERTY_MTU_SIZE     (4)
#define NETMGR_KIF_PROPERTY_MTU_DEFAULT  NETMGR_MTU_INVALID

#endif /* FEATURE_DS_LINUX_ANDROID */

/*---------------------------------------------------------------------------
   Macros for Android iWLAN
---------------------------------------------------------------------------*/
#ifdef FEATURE_DATA_IWLAN

#define NETMGR_KIF_PROPERTY_REKEY        "persist.data.iwlan.rekey"
#define NETMGR_KIF_PROPERTY_REKEY_SIZE   (10)
#ifndef UINT32_MAX
  #define UINT32_MAX                     (4294967295U)
#endif

#define NETMGR_KIF_GET_FORWARDING_ENABLED(family) \
  ((AF_INET == family) ?                          \
   netmgr_kif_cfg.xfrm.is_v4_forwarding_enabled : \
   netmgr_kif_cfg.xfrm.is_v6_forwarding_enabled)

#define NETMGR_KIF_SET_FORWARDING_ENABLED(family, status)    \
  *(((AF_INET == family) ?                                   \
    &netmgr_kif_cfg.xfrm.is_v4_forwarding_enabled :          \
    &netmgr_kif_cfg.xfrm.is_v6_forwarding_enabled)) = status

#define NETMGR_KIF_GET_POLICY_ROUTING_INSTALLED(family) \
  ((AF_INET == family) ?                                \
   netmgr_kif_cfg.xfrm.is_v4_policy_routing_installed : \
   netmgr_kif_cfg.xfrm.is_v6_policy_routing_installed)

#define NETMGR_KIF_SET_POLICY_ROUTING_INSTALLED(family, status)    \
  *(((AF_INET == family) ?                                         \
    &netmgr_kif_cfg.xfrm.is_v4_policy_routing_installed :          \
    &netmgr_kif_cfg.xfrm.is_v6_policy_routing_installed)) = status

typedef struct{
  boolean is_v4_forwarding_enabled;
  boolean is_v6_forwarding_enabled;
  boolean is_v4_policy_routing_installed;
  boolean is_v6_policy_routing_installed;
} netmgr_kif_xfrm_cfg_t;

#endif /* FEATURE_DATA_IWLAN */

/*---------------------------------------------------------------------------
   Type representing collection of configuration info for KIF module
---------------------------------------------------------------------------*/

typedef struct {
  int     nint;                                    /* number of interfaces */
  netmgr_ctl_port_config_type * link_array;          /* link enabled array */
  char    name[NETMGR_IF_NAME_MAX_LEN];                     /* device name */
  int     skip; /* Boolean, which if set, indicates that the device driver
                      module should not be loaded. Used only for debugging */
  char    dirpath[NETMGR_KIF_FILENAME_MAX_LEN];  /* Name of dir containing
                                                        module load script */
  char    modscript[NETMGR_KIF_FILENAME_MAX_LEN];    /* module load script */

#ifdef FEATURE_DATA_IWLAN
  netmgr_kif_xfrm_cfg_t  xfrm;                  /* XFRM configuration */
#endif /* FEATURE_DATA_IWLAN */
} netmgr_kif_cfg_t;

/*---------------------------------------------------------------------------
   Collection of shared configuration info for KIF module
---------------------------------------------------------------------------*/
LOCAL netmgr_kif_cfg_t netmgr_kif_cfg;

/*---------------------------------------------------------------------------
   Array of state/control info for each kif
---------------------------------------------------------------------------*/
LOCAL netmgr_kif_info_t netmgr_kif_info[NETMGR_MAX_LINK];


/*---------------------------------------------------------------------------
   Collection of control info pertaining to open sockets
---------------------------------------------------------------------------*/
typedef struct {
  netmgr_socklthrd_info_t  sk_thrd_info;    /* Listener thread info        */
  netmgr_socklthrd_fdmap_t sk_thrd_fdmap[1];/* Array of fdmap structs used
                                             ** by listener thread.        */
  netmgr_nl_sk_info_t      rt_sk;           /* Netlink routing socket info */
  netmgr_nl_sk_info_t      ev_sk;           /* Netlink event socket info   */
  netmgr_nl_sk_info_t      xfrm_sk;         /* Netlink xfrm socket info    */
} netmgr_kif_sk_info;

netmgr_kif_sk_info netmgr_kif_sk_route;
netmgr_kif_sk_info netmgr_kif_sk_grp;
netmgr_kif_sk_info netmgr_kif_sk_xfrm;

#ifdef FEATURE_DATA_IWLAN
  #define NETMGR_KIF_FWD_DEV_PER_MODEM  (8)
  #define NETMGR_KIF_REV_DEV_PER_MODEM  (9)
  #define NETMGR_KIF_DEV_PER_MODEM      (NETMGR_KIF_FWD_DEV_PER_MODEM + \
                                         NETMGR_KIF_REV_DEV_PER_MODEM)
#else
  #define NETMGR_KIF_DEV_PER_MODEM      (8)
#endif /* FEATURE_DATA_IWLAN */

#ifdef FEATURE_DATA_IWLAN
  #define TO_XSTR(x) TO_STR(x)
  #define TO_STR(x)  #x
  #define NETMGR_KIF_NATT_MODEM_PORT   32012
  #define NETMGR_KIF_NATT_SERVER_PORT  4500

LOCAL int natt_fd = -1;
#endif /* FEATURE_DATA_IWLAN */

/*---------------------------------------------------------------------------
   Inline accessor for getting kif state for a given link
---------------------------------------------------------------------------*/
LOCAL __inline__ netmgr_kif_state_t
netmgr_kif_get_state (int link)
{
  if (function_debug) {
    netmgr_log_med("netmgr_kif_get_state: link=%d, cur_state=%d\n",
                   link,
                   netmgr_kif_info[link].state);
  }
  return netmgr_kif_info[link].state;
}

/*---------------------------------------------------------------------------
  Inline mutator for setting kif state for a given link
  ---------------------------------------------------------------------------*/
LOCAL __inline__ void
netmgr_kif_set_state (int link, netmgr_kif_state_t state)
{
  if (function_debug) {
    netmgr_log_med("netmgr_kif_set_state: link=%d, cur_state=%d, new_state=%d\n",
                   link,
                   netmgr_kif_info[link].state,
                   state);
  }

  netmgr_kif_info[link].state = state;
}

/*---------------------------------------------------------------------------
   Inline accessor for getting pointer to client callback structure for a
   given link
---------------------------------------------------------------------------*/
LOCAL __inline__ const netmgr_kif_clntcb_t *
netmgr_kif_get_clntcb (int link)
{
  return netmgr_kif_info[link].clntcb;
}

/*---------------------------------------------------------------------------
  Inline mutator for setting pointer to client callback structure for a
  given link
---------------------------------------------------------------------------*/
LOCAL __inline__ void
netmgr_kif_set_clntcb (int link, const netmgr_kif_clntcb_t * clntcb)
{
  netmgr_kif_info[link].clntcb = clntcb;
}

/*---------------------------------------------------------------------------
  Inline accessor for getting client handle ptr for a given link
---------------------------------------------------------------------------*/
LOCAL __inline__ void *
netmgr_kif_get_clnt_hdl (int link)
{
  return netmgr_kif_info[link].clnt_hdl;
}

/*---------------------------------------------------------------------------
  Inline mutator for setting client handle ptr for a given link
---------------------------------------------------------------------------*/
LOCAL __inline__ void
netmgr_kif_set_clnt_hdl (int link, void * clnt_hdl)
{
  netmgr_kif_info[link].clnt_hdl = clnt_hdl;
}

/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/* Forward declarations */
LOCAL int netmgr_kif_close( int link,
                            netmgr_address_set_t * addr_info_ptr,
                            boolean   teardown_iface );

LOCAL void netmgr_kif_open_cnf (int link);

LOCAL int
netmgr_kif_remove_sa_and_routing_rules
(
  int                   link,
  int                   ip_family,
  netmgr_address_set_t  *addr_info_ptr
);

/*===========================================================================
  FUNCTION  netmgr_kif_verify_link
===========================================================================*/
/*!
@brief
  Helper function to verify validity of a link ID.

@return
  int - NETMGR_SUCCESS if link ID is valid, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_verify_link (int link)
{

  /* Range check */
  if( (link < 0) || (link >= NETMGR_MAX_LINK) ) {
    return NETMGR_FAILURE;
  }

  if( !netmgr_kif_cfg.link_array[link].enabled ) {
    return NETMGR_FAILURE;
  }

  return NETMGR_SUCCESS;
}

/*===========================================================================
  FUNCTION  netmgr_kif_ifioctl_get_flags
 ===========================================================================*/
/*!
@brief
  Helper function to get flags for the given kernel interface

@return
  int - NETMGR_SUCCESS if IOCTL is successfully executed,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_ifioctl_get_flags (const char * dev, short * p_flags)
{
  int fd;
  int rval = NETMGR_FAILURE;
  struct ifreq ifr;

  NETMGR_ASSERT(dev);
  NETMGR_ASSERT(p_flags);

  /* Open a datagram socket to use for issuing the ioctl */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    netmgr_log_sys_err("ifioctl_get: socket failed:\n");
    goto error;
  }

  /* Initialize the ioctl req struct to null */
  memset(&ifr, 0, sizeof(ifr));

  /* Set device name in ioctl req struct */
  (void)strlcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));

  /* Get current if flags for the device */
  if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
    netmgr_log_sys_err("ifioctl_get: SIOCGIFFLAGS ioctl failed:\n");
    close(fd);
    goto error;
  }

  *p_flags = ifr.ifr_flags;

  /* close temporary socket */
  close(fd);

  rval = NETMGR_SUCCESS;
 error:
  return rval;

}

/*===========================================================================
  FUNCTION  netmgr_kif_ifioctl_set_flags
===========================================================================*/
/*!
@brief
  Helper function to change specified SIOCSIFFLAGS on a given device.

@return
  int - NETMGR_SUCCESS if IOCTL is successfully executed,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_ifioctl_set_flags( const char * dev, short flags, short fmask )
{
  int fd;
  int rval = NETMGR_FAILURE;
  struct ifreq ifr;

  NETMGR_ASSERT(dev);

  /* Open a datagram socket to use for issuing the ioctl */
  if( (fd = socket( AF_INET, SOCK_DGRAM, 0 )) < 0 ) {
    netmgr_log_sys_err("ifioctl_set: socket failed:\n");
    goto error;
  }

  /* Initialize the ioctl req struct to null */
  memset(&ifr, 0, sizeof(ifr));

  /* Set device name in ioctl req struct */
  (void)strlcpy( ifr.ifr_name, dev, sizeof(ifr.ifr_name) );

  /* Get current if flags for the device */
  if( ioctl(fd, SIOCGIFFLAGS, &ifr) < 0 ) {
    netmgr_log_sys_err("ifioctl_get: SIOCGIFFLAGS ioctl failed:\n");
    close(fd);
    goto error;
  }

  /* fmask specifies which flag bits should be changed. flags specifies
  ** the value of those bits. Set bit positions indicated in fmask to the
  ** value specified in flags.
  */
  ifr.ifr_flags &= (short)(~(unsigned short)fmask);
  ifr.ifr_flags |= fmask & flags;

  /* Set if flags for the device */
  if( ioctl( fd, SIOCSIFFLAGS, &ifr ) < 0 ) {
    netmgr_log_sys_err("ifioctl_set: SIOCSIFFLAGS ioctl failed:\n");
    close(fd);
    goto error;
  }

  /* Close temporary socket */
  close(fd);

  rval = NETMGR_SUCCESS;

 error:
  return rval;
}

/*===========================================================================
  FUNCTION  netmgr_kif_ifioctl_set_mtu
===========================================================================*/
/*!
@brief
  Helper function to change SIOCSIFMTU on a given device.

@return
  int - NETMGR_SUCCESS if IOCTL is successfully executed,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_ifioctl_set_mtu( const char * dev, unsigned int mtu )
{
  int fd;
  int rval = NETMGR_FAILURE;
  struct ifreq ifr;

  NETMGR_ASSERT(dev);

  /* Open a datagram socket to use for issuing the ioctl */
  if( (fd = socket( AF_INET, SOCK_DGRAM, 0 )) < 0 ) {
    netmgr_log_sys_err("ifioctl_set: socket failed:\n");
    goto error;
  }

  /* Initialize the ioctl req struct to null */
  memset(&ifr, 0, sizeof(ifr));

  /* Set device name in ioctl req struct */
  (void)strlcpy( ifr.ifr_name, dev, sizeof(ifr.ifr_name) );

  ifr.ifr_data = (void*)mtu;

  /* Set if MTU for the device */
  if( ioctl( fd, SIOCSIFMTU, &ifr ) < 0 ) {
    netmgr_log_sys_err("ifioctl_set: SIOCSIFMTU ioctl failed:\n");
    close(fd);
    goto error;
  }

  /* Close temporary socket */
  close(fd);

  rval = NETMGR_SUCCESS;

 error:
  return rval;
}

/*===========================================================================
  FUNCTION  netmgr_kif_set_mtu
===========================================================================*/
/*!
@brief
  Helper function to change MTU on a given link.

@return
  int - NETMGR_SUCCESS if MTU is successfully changed,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
netmgr_kif_set_mtu(int link)
{
  /* any "failed" case below is not failing to set mtu because MTU will
     be assigned a default value, BUT we follow the same pattern as other
     netmgr HELPER function to return SUCCESS or FAILURE*/
  int rval = NETMGR_FAILURE;
  unsigned int mtu;

  /* Get Modem MTU for link and assign it to network interface */
  if( NETMGR_MTU_INVALID == (mtu = netmgr_qmi_get_mtu( link )) ) {
    netmgr_log_err( "netmgr_kif_cfg_cnf cannot get MTU, using default\n" );
  } else {

#ifdef FEATURE_DS_LINUX_ANDROID
    /* Configure Android MTU property value as follows:
     *   0 > value <= NETMGR_MTU_MAX - Override Modem MTU query value
     *   value == 0                  - Use Modem MTU query value
     *   value >  NETMGR_MTU_MAX     - Ignored
     */
    static char  args[PROP_VALUE_MAX];
    char  def[NETMGR_KIF_PROPERTY_MTU_SIZE+1];
    int   ret;

    memset( args, 0x0, sizeof(args) );
    memset( def, 0x0, sizeof(def) );

    /* Query Android proprerty for MTU override */
    snprintf( def, sizeof(def)-1, "%d", NETMGR_KIF_PROPERTY_MTU_DEFAULT );
    ret = property_get( NETMGR_KIF_PROPERTY_MTU, args, def );

    if( (NETMGR_KIF_PROPERTY_MTU_SIZE) < ret ) {
      netmgr_log_err( "System property %s has unexpected size(%d), skippng\n",
                      NETMGR_KIF_PROPERTY_MTU, ret );
    } else {
      ret = ds_atoi( args );
      if( NETMGR_MTU_MAX < (unsigned int)ret ) {
        netmgr_log_err( "System property %s has exceeded limit (%d), skippng\n",
                        NETMGR_KIF_PROPERTY_MTU, NETMGR_MTU_MAX );
      } else {
        if( NETMGR_MTU_INVALID != ret ) {
          /* Update MTU value using property */
          mtu = ret;
          netmgr_log_high( "MTU overide specified, using value %d\n", mtu );
        }
      }
    }
#endif /* FEATURE_DS_LINUX_ANDROID */

    if( NETMGR_FAILURE ==
        netmgr_kif_ifioctl_set_mtu( netmgr_kif_get_name(link), mtu ) ) {
      netmgr_log_err( "netmgr_kif_cfg_cnf failed to set MTU, using default\n" );
    } else {
      netmgr_log_high( "netmgr_kif_cfg_cnf assigned MTU %d on %s\n",
                       mtu,netmgr_kif_get_name(link) );
    }
  }

  rval = NETMGR_SUCCESS;

 error:
  return rval;
} /* netmgr_kif_set_mtu() */


/*===========================================================================
  FUNCTION  netmgr_kif_call_ioctl_on_dev
===========================================================================*/
/*!
@brief
  Helper function to call a specified IOCTL on a given device. Caller
  is responsible to initializing struct ifreq parameter.

@return
  int - NETMGR_SUCCESS if successful, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_call_ioctl_on_dev (const char * dev, unsigned int req, struct ifreq * ifr)
{
  int fd;
  int rval = NETMGR_FAILURE;

  /* Open a temporary socket of datagram type to use for issuing the ioctl */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    netmgr_log_sys_err("call_ioctl_on_dev: socket failed:\n");
    goto error;
  }

  /* Set device name in the ioctl req struct */
  (void)strlcpy(ifr->ifr_name, dev, sizeof(ifr->ifr_name));

  /* Issue ioctl on the device */
  if (ioctl(fd, req, ifr) < 0) {
    netmgr_log_sys_err("call_ioctl_on_dev: ioctl failed:\n");
    close(fd);
    goto error;
  }

  /* Close temporary socket */
  close(fd);
  rval = NETMGR_SUCCESS;

 error:
  return rval;
}

#ifndef FEATURE_DS_LINUX_DRIVER_LEGACY
#if 0  /* Unused for now */
/*===========================================================================
  FUNCTION  netmgr_kif_ifioctl_get_qosmode
 ===========================================================================*/
/*!
@brief
  Helper function to get QMI QoS mode for the given kernel interface.
  Driver mode values are from msm_rmnet.h.

@return
  int - NETMGR_SUCCESS if IOCTL is successfully executed,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_ifioctl_get_qosmode (const char * dev, uint32 * mode)
{
  int fd;
  int rval = NETMGR_FAILURE;
  struct ifreq ifr;

  NETMGR_ASSERT(dev);
  NETMGR_ASSERT(mode);

  /* Open a datagram socket to use for issuing the ioctl */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    netmgr_log_sys_err("ifioctl_get: socket failed:");
    goto error;
  }

  /* Initialize the ioctl req struct to null */
  memset(&ifr, 0, sizeof(ifr));

  /* Set device name in ioctl req struct */
  (void)strlcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));

  /* Get current if flags for the device */
  if (ioctl(fd, RMNET_IOCTL_GET_QOS, &ifr) < 0) {
    netmgr_log_sys_err("ifioctl_get: ioctl qosmode failed:");
    close(fd);
    goto error;
  }

  *mode = (uint32)ifr.ifr_ifru.ifru_data;

  /* close temporary socket */
  close(fd);

  rval = NETMGR_SUCCESS;
 error:
  return rval;

}
#endif /* 0 */

/*===========================================================================
  FUNCTION  netmgr_kif_ifioctl_get_llpmode
 ===========================================================================*/
/*!
@brief
  Helper function to get driver link-layer protocol mode for the
  given kernel interface.  Driver mode values are from msm_rmnet.h.

@return
  int - NETMGR_SUCCESS if IOCTL is successfully executed,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_ifioctl_get_llpmode( const char * dev, uint32 * mode )
{
  int fd;
  int rval = NETMGR_FAILURE;
  struct ifreq ifr;

  NETMGR_ASSERT(dev);
  NETMGR_ASSERT(mode);

  /* Open a datagram socket to use for issuing the ioctl */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    netmgr_log_sys_err("ifioctl_get: socket failed:\n");
    goto error;
  }

  /* Initialize the ioctl req struct to null */
  memset(&ifr, 0, sizeof(ifr));

  /* Set device name in ioctl req struct */
  (void)strlcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));

  /* Get current if flags for the device */
  if (ioctl(fd, RMNET_IOCTL_GET_LLP, &ifr) < 0) {
    netmgr_log_sys_err("ifioctl_get: ioctl llp failed:\n");
    close(fd);
    goto error;
  }

  *mode = (uint32)ifr.ifr_ifru.ifru_data;

  /* close temporary socket */
  close(fd);

  rval = NETMGR_SUCCESS;
 error:
  return rval;

}

/*===========================================================================
  FUNCTION  netmgr_kif_ifioctl_set_qosmode
 ===========================================================================*/
/*!
@brief
  Helper function to set QMI QoS mode for the given kernel interface.

@return
  int - NETMGR_SUCCESS if IOCTL is successfully executed,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_ifioctl_set_qosmode (const char * dev, boolean enable)
{
  int fd;
  int rval = NETMGR_FAILURE;
  struct ifreq ifr;
  int cmd = (enable)? RMNET_IOCTL_SET_QOS_ENABLE :
                      RMNET_IOCTL_SET_QOS_DISABLE;

  NETMGR_ASSERT(dev);

  /* Open a datagram socket to use for issuing the ioctl */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    netmgr_log_sys_err("ifioctl_set: socket failed:\n");
    goto error;
  }

  /* Initialize the ioctl req struct to null */
  memset(&ifr, 0, sizeof(ifr));

  /* Set device name in ioctl req struct */
  (void)strlcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));

  /* Get current if flags for the device */
  if (ioctl(fd, cmd, &ifr) < 0) {
    netmgr_log_sys_err("ifioctl_set: ioctl qosmode failed:\n");
    close(fd);
    goto error;
  }

  /* close temporary socket */
  close(fd);

  rval = NETMGR_SUCCESS;
error:
  return rval;

}

/*===========================================================================
  FUNCTION  netmgr_kif_ifioctl_flow_control
 ===========================================================================*/
/*!
@brief
  Helper function to enable/disable flow on a given handle.

@return
  int - NETMGR_SUCCESS if IOCTL is successfully executed,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
netmgr_kif_ifioctl_flow_control(const char * dev, int handle, int enable)
{
  int fd;
  int rval = NETMGR_FAILURE;
  struct ifreq ifr;
  int cmd = (enable)? RMNET_IOCTL_FLOW_ENABLE :
                      RMNET_IOCTL_FLOW_DISABLE;

  NETMGR_ASSERT(dev);

  /* Open a datagram socket to use for issuing the ioctl */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    netmgr_log_sys_err("ifioctl_set: socket failed:\n");
    goto error;
  }

  /* Initialize the ioctl req struct to null */
  memset(&ifr, 0, sizeof(ifr));

  /* Set device name in ioctl req struct */
  (void)strlcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));

  /* Set tc handle in ioctl req struct */
  ifr.ifr_data = (void *)handle;

  /* Get current if flags for the device */
  if (ioctl(fd, cmd, &ifr) < 0) {
    netmgr_log_sys_err("ifioctl_set: ioctl flow failed:\n");
    close(fd);
    goto error;
  }

  /* close temporary socket */
  close(fd);

  rval = NETMGR_SUCCESS;
error:
  return rval;

}

/*===========================================================================
  FUNCTION  netmgr_kif_ifioctl_set_llpmode
 ===========================================================================*/
/*!
@brief
  Helper function to set driver link-layer protocol mode for the
  given kernel interface.  Driver mode values are from msm_rmnet.h.

@return
  int - NETMGR_SUCCESS if IOCTL is successfully executed,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_ifioctl_set_llpmode( const char * dev, uint32 mode )
{
  int fd;
  int rval = NETMGR_FAILURE;
  struct ifreq ifr;
  uint32 cmd;

  NETMGR_ASSERT(dev);

  if( (RMNET_MODE_LLP_ETH != mode) &&
      (RMNET_MODE_LLP_IP  != mode) ) {
    netmgr_log_err("invalid set llp mode value specified\n");
    goto error;
  }

  cmd = (RMNET_MODE_LLP_ETH == mode)? RMNET_IOCTL_SET_LLP_ETHERNET :
                                      RMNET_IOCTL_SET_LLP_IP;

  /* Open a datagram socket to use for issuing the ioctl */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    netmgr_log_sys_err("ifioctl_set: socket failed:\n");
    goto error;
  }

  /* Initialize the ioctl req struct to null */
  memset(&ifr, 0, sizeof(ifr));

  /* Set device name in ioctl req struct */
  (void)strlcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));

  /* Get current if flags for the device */
  if (ioctl(fd, cmd, &ifr) < 0) {
    netmgr_log_sys_err("ifioctl_set: ioctl llp failed:\n");
    close(fd);
    goto error;
  }

  /* close temporary socket */
  close(fd);

  rval = NETMGR_SUCCESS;
 error:
  return rval;

}

/*===========================================================================
  FUNCTION  netmgr_kif_ifioctl_open_port
 ===========================================================================*/
/*!
@brief
  Helper function to open the RmNET driver transport port.

@return
  int - NETMGR_SUCCESS if IOCTL is successfully executed,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_ifioctl_open_port (const char * dev)
{
  int fd;
  int rval = NETMGR_FAILURE;
  struct ifreq ifr;

  NETMGR_ASSERT(dev);

  /* Open a datagram socket to use for issuing the ioctl */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    netmgr_log_sys_err("ifioctl: socket failed:\n");
    goto error;
  }

  /* Initialize the ioctl req struct to null */
  memset(&ifr, 0, sizeof(ifr));

  /* Set device name in ioctl req struct */
  (void)strlcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));

  /* Get current if flags for the device */
  if (ioctl(fd, RMNET_IOCTL_OPEN, &ifr) < 0) {
    netmgr_log_sys_err("ifioctl: open port failed:\n");
    close(fd);
    goto error;
  }

  /* close temporary socket */
  close(fd);

  netmgr_log_high("Open port success: %s\n", dev);
  rval = NETMGR_SUCCESS;
 error:
  return rval;

}

/*===========================================================================
  FUNCTION  netmgr_kif_ifioctl_close_port
 ===========================================================================*/
/*!
@brief
  Helper function to close the RmNET driver transport port.

@return
  int - NETMGR_SUCCESS if IOCTL is successfully executed,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_ifioctl_close_port (const char * dev)
{
  int fd;
  int rval = NETMGR_FAILURE;
  struct ifreq ifr;

  NETMGR_ASSERT(dev);

  /* Close a datagram socket to use for issuing the ioctl */
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    netmgr_log_sys_err("ifioctl: socket failed:\n");
    goto error;
  }

  netmgr_log_med("ifioctl: closing port [%s]\n", dev);

  /* Initialize the ioctl req struct to null */
  memset(&ifr, 0, sizeof(ifr));

  /* Set device name in ioctl req struct */
  (void)strlcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));

  /* Get current if flags for the device */
  if (ioctl(fd, RMNET_IOCTL_CLOSE, &ifr) < 0) {
    netmgr_log_sys_err("ifioctl: close port failed:\n");
    close(fd);
    goto error;
  }

  /* close temporary socket */
  close(fd);

  rval = NETMGR_SUCCESS;
 error:
  return rval;

}
#endif /* FEATURE_DS_LINUX_DRIVER_LEGACY */


#ifdef FEATURE_DS_LINUX_ANDROID
/*===========================================================================
  FUNCTION  netmgr_kif_set_addr_prop
===========================================================================*/
/*!
@brief
  Assign DNS and Gateway properties for specified network interface name.

@return
  int - NETMGR_SUCCESS if successful, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - Environment variable updated
*/
/*=========================================================================*/
LOCAL int netmgr_kif_set_addr_prop
(
  netmgr_ip_addr_t  family,
  const char      * ifname,
  const char      * suffix,
  union netmgr_ip_address_u * val_ptr
)
{
  char clear = FALSE;
  int ret = NETMGR_SUCCESS;

  NETMGR_ASSERT( val_ptr );

  NETMGR_LOG_FUNC_ENTRY;

  memset( name_buf, 0x0, sizeof(name_buf) );
  memset( addr_buf, 0x0, sizeof(addr_buf) );

  /* Format IP address based on family */
  switch( family )
  {
    case NETMGR_IPV4_ADDR:
      if( !val_ptr->v4 ) {
        clear = TRUE;
      } else {
        snprintf( addr_buf, sizeof(addr_buf),
                  "%d.%d.%d.%d", ADDR_FMT_V4( val_ptr->v4 ) );
      }
      break;

    case NETMGR_IPV6_ADDR:
      if( !val_ptr->v6_addr64[0] && !val_ptr->v6_addr64[1] ) {
        clear = TRUE;
      } else {
        snprintf( addr_buf, sizeof(addr_buf),
                  "%x:%x:%x:%x:%x:%x:%x:%x",
                  ADDR_FMT_V6( val_ptr->v6_addr64 ) );
      }
      break;

    default:
      netmgr_log_err( "netmgr_kif_set_addr_prop: unsupported IP address family\n" );
      NETMGR_LOG_FUNC_EXIT;
      return NETMGR_FAILURE;
  }

  snprintf( name_buf, sizeof(name_buf), "net.%s%s", ifname, suffix );

  /* Assign property */
  ret = property_set( name_buf, addr_buf );
  if( NETMGR_SUCCESS != ret ) {
    netmgr_log_err( "netmgr_kif_set_addr_prop: error assigning property %s\n",
                      name_buf );

  } else {
    netmgr_log_high( "netmgr_kif_set_addr_prop: assigned property %s = %s\n",
                     name_buf, addr_buf );
  }

  NETMGR_LOG_FUNC_EXIT;
  return ret;
} /* netmgr_kif_set_addr_prop() */

#endif /* FEATURE_DS_LINUX_ANDROID */

/*===========================================================================
  FUNCTION  netmgr_kif_clear_address
===========================================================================*/
/*!
@brief
  Routine to clear the specified address from the kernel network interface.
  This uses the libnetutils functions to reset socket connections using
  address and delete the address from the network interface.

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_purge_address( int link, netmgr_ip_address_t * addr_ptr, unsigned int mask )
{
  char buff[INET6_ADDRSTRLEN+1];
  int ret = NETMGR_FAILURE;
  int rc;

  NETMGR_LOG_FUNC_ENTRY;

#if defined(FEATURE_DS_LINUX_ANDROID) && !defined(FEATURE_GB_NET_UTILS)
  NETMGR_ASSERT(addr_ptr);

  memset( buff, 0x0, sizeof(buff) );


  do {
    if( NETMGR_IPV4_ADDR == addr_ptr->type )
    {
      struct in_addr addr;

      /* Convert address to string notation */
      memset( &addr, 0x0, sizeof(addr) );
      memcpy( &addr.s_addr, &addr_ptr->addr.v4, sizeof(addr.s_addr) );
      if( NULL == inet_ntop( AF_INET, &addr, buff, INET_ADDRSTRLEN ) )
      {
        netmgr_log_sys_err("error on inet_ntop():\n");
        break;
      }

      /* Reset sockets only on forward rmnets */
      if (!NETMGR_KIF_IS_REV_RMNET_LINK(link))
      {
        /* Force reset of socket connections using address */
        NETMGR_LOG_IPV4_ADDR( med, "reset socket connections for ", addr_ptr->addr.v4 );
        if( 0 > ifc_reset_connections( netmgr_kif_info[ link ].name, RESET_IPV4_ADDRESSES ) )
        {
          netmgr_log_sys_err("error on ifc_reset_connections():\n");
          break;
        }
      }
    }
    else
    {
      struct in6_addr addr;
      struct in6_ifreq ifr6;
      int fd;

      /* Convert address to string notation */
      memset( &addr, 0x0, sizeof(addr) );
      memcpy( addr.s6_addr, addr_ptr->addr.v6_addr8, sizeof(addr.s6_addr) );
      if( NULL == inet_ntop( AF_INET6, &addr, buff, INET6_ADDRSTRLEN ) )
      {
        netmgr_log_sys_err("error on inet_ntop():\n");
        break;
      }

      /* Reset sockets only on forward rmnets */
      if (!NETMGR_KIF_IS_REV_RMNET_LINK(link))
      {
        /* Force reset of socket connections using address */
        NETMGR_LOG_IPV6_ADDR( med, "reset socket connections for ",  addr_ptr->addr.v6_addr64 );
        if ((fd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
          netmgr_log_sys_err("error on socket():\n");
          break;
        }
        ifr6.ifr6_addr = addr;
        ifr6.ifr6_prefixlen = mask;
        ifr6.ifr6_ifindex = netmgr_kif_info[link].ifi_index;
        if( 0 > ioctl( fd, SIOCKILLADDR,  &ifr6 ) )
        {
          netmgr_log_sys_err("error on ioctl():\n");
          close(fd);
          break;
        }
        close(fd);
      }
    }

    /* Remove address from network interface */
    netmgr_log_med("deleting address [%s] from iface[%s]\n", buff, netmgr_kif_info[ link ].name );
    rc = ifc_del_address( netmgr_kif_info[ link ].name, buff, mask );
    if( 0 > rc )
    {
      netmgr_log_sys_err("error on ifc_del_address():\n");
      break;
    }

    ret = NETMGR_SUCCESS;
  } while (0);
#else
  netmgr_log_err("Address purge not supported in this build\n");
#endif /* FEATURE_DS_LINUX_ANDROID && !FEATURE_GB_NET_UTILS */


  NETMGR_LOG_FUNC_EXIT;
  return ret;
}

/*===========================================================================
  FUNCTION  netmgr_kif_clear_iface
===========================================================================*/
/*!
@brief
  Routine to clear the kernel interface configuration using system command.

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_clear_iface( int link )
{
  char cmd[NETMGR_KIF_SYSCMD_SIZ];
  unsigned int cmdlen = 0;
  int ret = NETMGR_SUCCESS;
  int rc;

  NETMGR_LOG_FUNC_ENTRY;

  netmgr_log_high("clearing kernel interface for link [%d]\n", link );

#ifdef FEATURE_DS_LINUX_ANDROID

  /* Force reset of socket connections before flushing IP addresses */
#ifdef FEATURE_GB_NET_UTILS
  if( 0 > ifc_reset_connections( netmgr_kif_info[ link ].name ) )
#else
  if( 0 > ifc_reset_connections( netmgr_kif_info[ link ].name, RESET_ALL_ADDRESSES ) )
#endif
  {
    netmgr_log_sys_err("error on ifc_reset_connections():\n");
    ret = NETMGR_FAILURE;
  }

#endif /* FEATURE_DS_LINUX_ANDROID */

  cmdlen = snprintf( cmd, sizeof(cmd), NETMGR_KIF_SYSCMD_FLUSHADDR,
                     netmgr_kif_info[ link ].name );

  rc = ds_system_call( cmd, cmdlen );
  if( 0 > rc ) {
    netmgr_log_sys_err("failed system call:\n");
    ret = NETMGR_FAILURE;
  }

#ifdef FEATURE_DS_LINUX_ANDROID
  /* Clear Android address properties for this interface */
  union netmgr_ip_address_u  null_addr;
  memset( &null_addr, 0x0, sizeof(null_addr) );

  netmgr_kif_set_addr_prop( NETMGR_IPV4_ADDR, netmgr_kif_get_name(link), ".dns1",
                            &null_addr );
  netmgr_kif_set_addr_prop( NETMGR_IPV4_ADDR, netmgr_kif_get_name(link), ".dns2",
                            &null_addr );
  netmgr_kif_set_addr_prop( NETMGR_IPV4_ADDR, netmgr_kif_get_name(link), ".gw",
                            &null_addr );
#endif /* FEATURE_DS_LINUX_ANDROID */

  NETMGR_LOG_FUNC_EXIT;
  return ret;
}

/*===========================================================================
  FUNCTION  netmgr_kif_nl_post_addr_msg
===========================================================================*/
/*!
@brief
  Post address event message to NetLink clients

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_nl_post_addr_msg( int link, const netmgr_nl_msg_t * nlmsg_info_ptr )
{
  netmgr_nl_event_info_t event_info;

  NETMGR_ASSERT( nlmsg_info_ptr );

  NETMGR_LOG_FUNC_ENTRY;

  /* Post message to clients for kernel address updates */
  if( NETMGR_NLA_PARAM_PREFIXADDR & nlmsg_info_ptr->addr_info.attr_info.param_mask ) {
    memset( &event_info, 0x0, sizeof(event_info) );

    if( RTM_NEWADDR == nlmsg_info_ptr->type ) {
      event_info.event = NET_PLATFORM_NEWADDR_EV;
    } else if( RTM_DELADDR == nlmsg_info_ptr->type ) {
      event_info.event = NET_PLATFORM_DELADDR_EV;
    } else {
      netmgr_log_err("netmgr_kif_nl_post_addr_msg unsupported event type\n" );
      return NETMGR_FAILURE;
    }

    /* Populate attributes */
    event_info.link = link;
    event_info.param_mask |= NETMGR_EVT_PARAM_LINK;

    memcpy( &event_info.dev_name,
            netmgr_kif_get_name(link),
            sizeof(event_info.dev_name) );
    event_info.param_mask |= NETMGR_EVT_PARAM_DEVNAME;

    memcpy( &event_info.addr_info.addr.ip_addr,
            &nlmsg_info_ptr->addr_info.attr_info.prefix_addr,
            sizeof(event_info.addr_info.addr.ip_addr) );
    event_info.addr_info.addr.mask =
      ds_get_num_bits_set_count(nlmsg_info_ptr->addr_info.metainfo.ifa_prefixlen);
    event_info.addr_info.flags = nlmsg_info_ptr->addr_info.metainfo.ifa_flags;
    event_info.param_mask |= NETMGR_EVT_PARAM_IPADDR;

    if( NETMGR_NLA_PARAM_CACHEINFO & nlmsg_info_ptr->addr_info.attr_info.param_mask ) {
      memcpy( &event_info.addr_info.cache_info,
              &nlmsg_info_ptr->addr_info.attr_info.cache_info,
              sizeof(event_info.addr_info.cache_info) );
      event_info.param_mask |= NETMGR_EVT_PARAM_CACHE;
    } else {
      netmgr_log_med("netmgr_kif_nl_post_addr_msg no address cacheinfo in NEWADDR indication\n" );
    }

    if( NETMGR_SUCCESS != netmgr_kif_send_event_msg( &event_info ) ) {
      netmgr_log_err("failed on send NET_PLATFORM_xxxADDR_EV\n");
      return NETMGR_FAILURE;
    }
  } else {
    netmgr_log_err("netmgr_kif_nl_post_addr_msg no address in ADDR indication\n" );
    return NETMGR_FAILURE;
  }

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;
}

/*===========================================================================
  FUNCTION  netmgr_kif_open_req
===========================================================================*/
/*!
@brief
  Issues IOCTL to bring up the specified interface

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_open_req( int link, netmgr_address_set_t * addr_info_ptr )
{
  char cmd[NETMGR_KIF_SYSCMD_SIZ];
  unsigned int cmdlen = 0;
  char mode;
  int rc;
  short flags;

  NETMGR_ASSERT( addr_info_ptr);

  NETMGR_LOG_FUNC_ENTRY;

  netmgr_log_high("bring up kernel interface for link [%d]  family[%d]\n",
                  link, addr_info_ptr->if_addr.type );

  /* Check kernel interface state to see if already UP. Another entity
   * may have already set the state. */
  if (netmgr_kif_ifioctl_get_flags(netmgr_kif_info[link].name, &flags) < 0) {
    NETMGR_ABORT("netmgr_kif_open_req: open req failed, aborting!\n");
  }

  if (flags & IFF_UP) {
    netmgr_log_high("kernel interface %d found to be open, reusing\n", link);
    /* Report interface open */
    netmgr_kif_open_cnf(link);
  }
  else
  {
    /* To bring up interface, issue ioctl to set IFF_UP flag to 1 */
    if( netmgr_kif_ifioctl_set_flags( netmgr_kif_get_name(link), IFF_UP, IFF_UP ) < 0 ) {
      NETMGR_ABORT("netmgr_kif_open_req: open req failed, aborting!\n");
    }
  }

  NETMGR_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
  FUNCTION  netmgr_kif_close_req
===========================================================================*/
/*!
@brief
  Issues IOCTL to bring down the specified interface.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_close_req (int link)
{
  NETMGR_LOG_FUNC_ENTRY;

  netmgr_log_high("bring down kernel interface for link %d\n", link );
  /* To bring down interface, issue ioctl to set IFF_UP flag to 0 */
  if (netmgr_kif_ifioctl_set_flags(netmgr_kif_get_name(link), ~IFF_UP, IFF_UP) < 0) {
    NETMGR_ABORT("netmgr_kif_close_req: close req failed, aborting!\n");
  }

  NETMGR_LOG_FUNC_EXIT;
  return;
}

#ifdef FEATURE_DATA_IWLAN
/*===========================================================================
  FUNCTION  netmgr_kif_get_modem_link_type
===========================================================================*/
/*!
@brief
  Returns the modem and link type for the given link index

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_get_modem_link_type
(
  int                    link,
  int                    *modem,
  netmgr_main_link_type  *link_type
)
{
  int ret = NETMGR_FAILURE;

  if (!modem || !link_type) {
    netmgr_log_err("netmgr_kif_get_modem_link_type: invalid input\n");
    goto bail;
  }

  if (link >= NETMGR_LINK_RMNET_0 && link <= NETMGR_LINK_RMNET_7) {
    *modem = NETMGR_MODEM_MSM;
    *link_type = NETMGR_FWD_LINK;
  }
  else if (link >= NETMGR_LINK_RMNET_8 && link <= NETMGR_LINK_RMNET_15) {
    *modem = NETMGR_MODEM_MDM;
    *link_type = NETMGR_FWD_LINK;
  }
  else if (link >= NETMGR_LINK_REV_RMNET_0 && link <= NETMGR_LINK_REV_RMNET_8) {
    *modem = NETMGR_MODEM_MSM;
    *link_type = NETMGR_REV_LINK;
  }
  else if (link >= NETMGR_LINK_REV_RMNET_9 && link <= NETMGR_LINK_REV_RMNET_17) {
    *modem = NETMGR_MODEM_MDM;
    *link_type = NETMGR_REV_LINK;
  }
  else {
    goto bail;
  }

  ret = NETMGR_SUCCESS;

bail:
  return ret;
}

/*===========================================================================
  FUNCTION  netmgr_kif_rev_rmnet_cfg_req
===========================================================================*/
/*!
@brief
  Handle the address configuration request for the given reverse rmnet link

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_rev_rmnet_cfg_req
(
  int link,
  netmgr_ip_addr_t addr_type
)
{
  char cmd[NETMGR_MAX_COMMAND_LENGTH] = "";
  char addr_buf[NETMGR_MAX_STR_LENGTH] = "";
  char *ip_type = NULL;
  netmgr_ip_address_t  *ip_addr = NULL;
  netmgr_address_info_t  *addr_info_ptr = netmgr_qmi_get_addr_info(link);
  const char *link_name = NULL;
  int result = NETMGR_FAILURE;
  unsigned int prefix_len = 0;
  int ip_family;

  NETMGR_LOG_FUNC_ENTRY;

  if (!addr_info_ptr)
  {
    netmgr_log_err("netmgr_kif_rev_rmnet_cfg_req: invalid input\n");
    goto bail;
  }

  if (NETMGR_IPV4_ADDR == addr_type)
  {
    ip_type = "-4";
    ip_addr = &addr_info_ptr->ipv4.if_addr;
    prefix_len = ds_get_num_bits_set_count(addr_info_ptr->ipv4.if_mask);
    ip_family = AF_INET;
  }
  else if (NETMGR_IPV6_ADDR == addr_type)
  {
    ip_type = "-6";
    ip_addr = &addr_info_ptr->ipv6.if_addr;
    prefix_len = addr_info_ptr->ipv6.if_mask;
    ip_family = AF_INET6;
  }
  else
  {
    netmgr_log_err("netmgr_kif_rev_rmnet_cfg_req: unknown addr_type=%d\n", addr_type);
    goto bail;
  }

  /* Find the associated forward rmnet (if any) and install the SA, routing rules */
  (void) netmgr_qmi_iwlan_update_link_assoc(link, NULL);

  if (NETMGR_SUCCESS != netmgr_kif_install_sa_and_routing_rules(link,
                                                                ip_family))
  {
    netmgr_log_err("failed installing SAs and routing rules\n");
  }

  if (NETMGR_SUCCESS != netmgr_util_convert_ip_addr_to_str(ip_addr,
                                                           prefix_len,
                                                           addr_buf,
                                                           sizeof(addr_buf)))
  {
    netmgr_log_err("netmgr_kif_rev_rmnet_cfg_req: failed to convert IP addr to string\n");
    goto bail;
  }
  else if (NULL == (link_name = netmgr_kif_get_name(link)))
  {
    netmgr_log_err("netmgr_kif_rev_rmnet_cfg_req: failed to obtain link name for link=%d\n",
                   link);
    goto bail;
  }

  snprintf(cmd,
           sizeof(cmd),
           "ip %s addr add %s dev %s",
           ip_type,
           addr_buf,
           link_name);

  netmgr_log_med("netmgr_kif_rev_rmnet_cfg_req: adding address=%s to link=%s\n",
                 addr_buf, link_name);

  if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
  {
    netmgr_log_err("netmgr_kif_rev_rmnet_cfg_req: ds_system_call() failed\n");
    goto bail;
  }

  result = NETMGR_SUCCESS;

bail:
  NETMGR_LOG_FUNC_EXIT;
  return result;
}
#endif /* FEATURE_DATA_IWLAN */

/*===========================================================================
  FUNCTION  netmgr_kif_cfg_req
===========================================================================*/
/*!
@brief
  Executes operations to kick-start interface configuration.

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_cfg_req (int link, netmgr_ip_addr_t addr_type)
{
  int result = NETMGR_SUCCESS;
  netmgr_address_info_t * addr_info_ptr = NULL;
  netmgr_address_set_t  * addr_ptr = NULL;
  int         sockfd;

#ifdef FEATURE_DATA_IWLAN
  /* If the given link is a reverse Rmnet link, directly assign the address */
  if (NETMGR_KIF_IS_REV_RMNET_LINK(link))
  {
    return netmgr_kif_rev_rmnet_cfg_req(link, addr_type);
  }
#endif /* FEATURE_DATA_IWLAN */

  if(NETMGR_IPV4_ADDR == addr_type)
  {
    struct ifreq ifr;
    struct sockaddr_in ipv4_addr;

    addr_info_ptr = netmgr_qmi_get_addr_info( link );

    if( !addr_info_ptr ) {
      netmgr_log_err("netmgr_kif_cfg_req: cannot get address info\n");
      return NETMGR_FAILURE;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd  < 0) {
      netmgr_log_sys_err("netmgr_kif_cfg_req: socket failed:");
      return NETMGR_FAILURE;
    }

    (void)strlcpy(ifr.ifr_name,
                      netmgr_kif_info[link].name,
                      sizeof(ifr.ifr_name));

    /* reset IP address */
    ipv4_addr.sin_family = AF_INET;
    ipv4_addr.sin_port = 0;
    ipv4_addr.sin_addr.s_addr = 0;

    memcpy(&ifr.ifr_addr,
           &ipv4_addr,
           sizeof(struct sockaddr));

    if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0)
    {
      netmgr_log_sys_err("netmgr_kif_cfg_req: Cannot clear ipv4 address:");
      perror(ifr.ifr_name);
      result = NETMGR_FAILURE;
    }

    ipv4_addr.sin_addr.s_addr =
           addr_info_ptr->ipv4.if_addr.addr.v4;

    /*configure new IP address*/
    memcpy(&ifr.ifr_addr,
           &ipv4_addr,
           sizeof(struct sockaddr));

    NETMGR_LOG_IPV4_ADDR( med, "netmgr_kif_cfg_req ",
                          addr_info_ptr->ipv4.if_addr.addr.v4);

    if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0)
    {
      netmgr_log_sys_err("netmgr_kif_cfg_req: Cannot set ipv4 address:");
      perror(ifr.ifr_name);
      result = NETMGR_FAILURE;
    }

    ipv4_addr.sin_addr.s_addr =
           addr_info_ptr->ipv4.if_mask;

    memcpy(&ifr.ifr_addr,
           &ipv4_addr,
           sizeof(struct sockaddr));

    if (ioctl(sockfd, SIOCSIFNETMASK, &ifr) < 0)
    {
      netmgr_log_sys_err("netmgr_kif_cfg_req: Cannot set Netmask:");
      perror(ifr.ifr_name);
      result = NETMGR_FAILURE;
    }

    close(sockfd);
  }

  return result;
}

/*===========================================================================
  FUNCTION  netmgr_kif_open_cnf
===========================================================================*/
/*!
@brief
  Processes confirmation of interface up event.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_open_cnf (int link)
{
  netmgr_kif_state_t state;
  netmgr_exec_cmd_t * cmd_buf = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  /* Verify that the specified link id is valid */
  if( netmgr_kif_verify_link(link) == NETMGR_FAILURE ) {
    netmgr_log_err("netmgr_kif_open_cnf called with invalid link %d\n", link);
    return;
  }

  /* Process based on current interface state */
  switch (state = netmgr_kif_get_state(link)) {
    case NETMGR_KIF_OPENING:
      /* Interface is in coming up state. Transition to configuring state */
      /* Allocate command object */
      cmd_buf = netmgr_exec_get_cmd();
      NETMGR_ASSERT(cmd_buf);
      cmd_buf->data.type = NETMGR_KIF_OPENED_EV;
      cmd_buf->data.link = link;

      /* Post command for processing in the command thread context */
      if( NETMGR_SUCCESS != netmgr_exec_put_cmd( cmd_buf ) ) {
        netmgr_exec_release_cmd(cmd_buf);
        NETMGR_ABORT("netmgr_kif_open_cnf: failed to put commmand");
        return;
      }
      break;

    default:
      /* Ignore in all other states. Effectively we are not supporting the
      ** case where the virtual ethernet devices are being externally
      ** controlled too. We assume that DSC has exclusive control over these
      ** devices.
      */
      netmgr_log_err("netmgr_kif_open_cnf called in state %d\n", state);
      break;
  }

  NETMGR_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
  FUNCTION  netmgr_kif_cfg_cnf
===========================================================================*/
/*!
@brief
  Processes confirmation of event indicating successful configuration of
  the given interface.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_cfg_cnf( int link, const netmgr_nl_msg_t * nlmsg_info_ptr )
{
  const netmgr_kif_clntcb_t * clntcb;
  netmgr_kif_state_t state;
  void * clnt_hdl;
  netmgr_exec_cmd_t * cmd_buf = NULL;
  netmgr_sm_events_t  event = NETMGR_INVALID_EV;
  netmgr_address_info_t * addr_info_ptr = NULL;
  netmgr_address_set_t  * addr_ptr = NULL;
  const struct sockaddr_storage *sa_ptr = NULL;
  unsigned int mtu;

  NETMGR_LOG_FUNC_ENTRY;

  NETMGR_ASSERT( nlmsg_info_ptr );

  /* Verify that the specified link id is valid */
  if( netmgr_kif_verify_link( link ) == NETMGR_FAILURE ) {
    netmgr_log_err("netmgr_kif_cfg_cnf called with invalid link %d\n", link);
    return;
  }

  /* Process based on current interface state */
  switch( state = netmgr_kif_get_state( link ) ) {
    case NETMGR_KIF_OPENING:
      /* Interface is in configuring state. Transition to open state */
      netmgr_kif_set_state( link, NETMGR_KIF_OPEN );
      event = NETMGR_KIF_CONFIGURED_EV;

      (void)netmgr_kif_set_mtu(link);

      /* Call registered client handler to indicate that the interface has
      ** come up.
      */
      clntcb = netmgr_kif_get_clntcb(link);
      if( clntcb ) {
        clnt_hdl = netmgr_kif_get_clnt_hdl(link);
        (clntcb->opened_cb)(link, NETMGR_SUCCESS, clnt_hdl);
      }
      break;

    case NETMGR_KIF_RECONFIGURING:
      netmgr_kif_set_state(link, NETMGR_KIF_OPEN);
      event = NETMGR_KIF_RECONFIGURED_EV;

      /* Update address cache for later use */
      addr_info_ptr = netmgr_qmi_get_addr_info( link );
      if( !addr_info_ptr ) {
        netmgr_log_err("netmgr_kif_cfg_cnf cannot get address info\n");
        return;
      }

      sa_ptr = &nlmsg_info_ptr->addr_info.attr_info.prefix_addr;
      if( AF_INET == sa_ptr->ss_family )
      {
        memcpy( &addr_info_ptr->ipv4.if_addr.addr.v4,
                SASTORAGE_DATA(*sa_ptr),
                sizeof(addr_info_ptr->ipv4.if_addr.addr.v4) );
        NETMGR_LOG_IPV4_ADDR( med, "updated cached ",
                              addr_info_ptr->ipv4.if_addr.addr.v4 );
      }
      else if( AF_INET6 == sa_ptr->ss_family )
      {
        memcpy( addr_info_ptr->ipv6.if_addr.addr.v6_addr8,
                SASTORAGE_DATA(*sa_ptr),
                sizeof(addr_info_ptr->ipv6.if_addr.addr.v6_addr8) );
        NETMGR_LOG_IPV6_ADDR( med, "updated cached ",
                              addr_info_ptr->ipv6.if_addr.addr.v6_addr64 );
      }
      else
      {
        netmgr_log_err("netmgr_kif_cfg_cnf unsupported address family[%d]\n",
                       sa_ptr->ss_family );
        return;
      }

      clntcb = netmgr_kif_get_clntcb(link);
      if( clntcb ) {
        clnt_hdl = netmgr_kif_get_clnt_hdl(link);
        (clntcb->reconfigured_cb)(link, NETMGR_SUCCESS, clnt_hdl);
      }
      break;

    case NETMGR_KIF_OPEN:
      /* Update address cache for later use */
      addr_info_ptr = netmgr_qmi_get_addr_info( link );
      if( !addr_info_ptr ) {
        netmgr_log_err("netmgr_kif_cfg_cnf cannot get address info\n");
        return;
      }

      sa_ptr = &nlmsg_info_ptr->addr_info.attr_info.prefix_addr;
      if( AF_INET == sa_ptr->ss_family )
      {
        memcpy( &addr_info_ptr->ipv4.if_addr.addr.v4,
                SASTORAGE_DATA(*sa_ptr),
                sizeof(addr_info_ptr->ipv4.if_addr.addr.v4) );
        NETMGR_LOG_IPV4_ADDR( med, "updated cached ",
                              addr_info_ptr->ipv4.if_addr.addr.v4 );
      }
      else if( AF_INET6 == sa_ptr->ss_family )
      {
        memcpy( addr_info_ptr->ipv6.if_addr.addr.v6_addr8,
                SASTORAGE_DATA(*sa_ptr),
                sizeof(addr_info_ptr->ipv6.if_addr.addr.v6_addr8) );
        NETMGR_LOG_IPV6_ADDR( med, "updated cached ",
                              addr_info_ptr->ipv6.if_addr.addr.v6_addr64 );
      }
      else
      {
        netmgr_log_err("netmgr_kif_cfg_cnf unsupported address family[%d]\n",
                       sa_ptr->ss_family );
        return;
      }

      /* Post event to NetLink clients for local processing */
      if( NETMGR_SUCCESS != netmgr_kif_nl_post_addr_msg( link, nlmsg_info_ptr ) ) {
        netmgr_log_err("netmgr_kif_cfg_cnf failed on posting address event for link %d\n", link);
        return;
      }

      /* No event to executive in this case */
      break;

    default:
      /* Ignore in all other states */
      netmgr_log_high("netmgr_kif_cfg_cnf called in state %d, ignoring!\n", state);
      break;
  }

  sa_ptr = &nlmsg_info_ptr->addr_info.attr_info.prefix_addr;

  if((AF_INET6 == sa_ptr->ss_family)  &&
        netmgr_kif_info[link].dns_v6_queried == FALSE)
  {
    netmgr_log_high("netmgr_kif_cfg_cnf: fetching runtime settings\n");
    netmgr_qmi_get_modem_link_info(link, QMI_IP_FAMILY_PREF_IPV6);
    netmgr_kif_info[link].dns_v6_queried = TRUE;

    /* Fix IPv6 call, set the mtu after getting the modem link info */
    (void)netmgr_kif_set_mtu(link);
  }

  /* Check for successful indication processing */
  if( NETMGR_INVALID_EV != event ) {

    /* Query interface address into reported from QMI */
    addr_info_ptr = netmgr_qmi_get_addr_info( link );
    if( !addr_info_ptr ) {
      netmgr_log_err("netmgr_kif_cfg_cnf cannot get address info\n");
      return;
    }
    /* Assign address set for property assignment, giving IPV4 precedence for legacy support. */
    if( NETMGR_ADDRSET_MASK_INVALID == addr_info_ptr->valid_mask )
    {
      netmgr_log_err("netmgr_kif_cfg_cnf cannot get valid address info\n");
      return;
    }
    addr_ptr = (NETMGR_ADDRSET_MASK_IPV4 & addr_info_ptr->valid_mask)?
               &addr_info_ptr->ipv4 : &addr_info_ptr->ipv6;

#ifdef FEATURE_DS_LINUX_ANDROID
    /*-------------------------------------------------------------------------
      Configure Android DNS server properties
    -------------------------------------------------------------------------*/
    if( NETMGR_KIF_OPEN == netmgr_kif_get_state( link ) ) {
      /* Format based on address family type */
      switch( addr_ptr->if_addr.type ) {
        case NETMGR_IPV4_ADDR:
          /* Set iface-specific values */
          netmgr_kif_set_addr_prop( NETMGR_IPV4_ADDR, netmgr_kif_get_name(link), ".dns1",
                                    &addr_ptr->dns_primary.addr );
          netmgr_kif_set_addr_prop( NETMGR_IPV4_ADDR, netmgr_kif_get_name(link), ".dns2",
                                    &addr_ptr->dns_secondary.addr );
          netmgr_kif_set_addr_prop( NETMGR_IPV4_ADDR, netmgr_kif_get_name(link), ".gw",
                                    &addr_ptr->gateway.addr );
          break;

        case NETMGR_IPV6_ADDR:
          /* Set iface-specific values */
          netmgr_kif_set_addr_prop( NETMGR_IPV6_ADDR, netmgr_kif_get_name(link), ".dns1",
                                    &addr_ptr->dns_primary.addr );
          netmgr_kif_set_addr_prop( NETMGR_IPV6_ADDR, netmgr_kif_get_name(link), ".dns2",
                                    &addr_ptr->dns_secondary.addr );
          netmgr_kif_set_addr_prop( NETMGR_IPV6_ADDR, netmgr_kif_get_name(link), ".gw",
                                    &addr_ptr->gateway.addr );
          break;

        default:
          netmgr_log_high("netmgr_kif_cfg_cnf unsupported IP address type\n");
          break;
      }
    }
#endif /* FEATURE_DS_LINUX_ANDROID */

    /*-------------------------------------------------------------------------
      Post message to executive
    -------------------------------------------------------------------------*/
    cmd_buf = netmgr_exec_get_cmd();
    NETMGR_ASSERT(cmd_buf);

    cmd_buf->data.type = event;
    cmd_buf->data.link = link;
    cmd_buf->data.info.connect_msg.nlmsg_info = *nlmsg_info_ptr;
    cmd_buf->data.info.connect_msg.addr_info_ptr = addr_ptr;
    cmd_buf->data.info.connect_msg.reconfig_required = FALSE;

    /* Post command for processing in the command thread context */
    if( NETMGR_SUCCESS != netmgr_exec_put_cmd( cmd_buf ) ) {
      netmgr_exec_release_cmd(cmd_buf);
      NETMGR_ABORT("failed to put commmand");
    }
  }

  NETMGR_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
  FUNCTION  netmgr_kif_cfg_fail
===========================================================================*/
/*!
@brief
  Processes event indicating unsuccessful configuration of given interface.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_cfg_fail (int link, const netmgr_nl_msg_t * nlmsg_info_ptr)
{
  netmgr_kif_state_t state;
  const struct sockaddr_storage *sa_ptr = NULL;
  netmgr_address_info_t *addr_info_ptr = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  /* Verify that the specified link id is valid */
  if (netmgr_kif_verify_link(link) == NETMGR_FAILURE) {
    netmgr_log_err("netmgr_kif_cfg_fail called with invalid link %d\n", link);
    return;
  }

  /* Process based on current interface state */
  switch (state = netmgr_kif_get_state(link)) {
    case NETMGR_KIF_OPEN:
    case NETMGR_KIF_RECONFIGURING:
      /* Update address cache for later use */
      addr_info_ptr = netmgr_qmi_get_addr_info( link );
      if( !addr_info_ptr ) {
        netmgr_log_err("netmgr_kif_cfg_fail cannot get address info\n");
        return;
      }

      sa_ptr = &nlmsg_info_ptr->addr_info.attr_info.prefix_addr;
      if( AF_INET == sa_ptr->ss_family )
      {
        memcpy( &addr_info_ptr->ipv4.if_addr.addr.v4,
                SASTORAGE_DATA(*sa_ptr),
                sizeof(addr_info_ptr->ipv4.if_addr.addr.v4) );
        NETMGR_LOG_IPV4_ADDR( med, "updated cached ",
                              addr_info_ptr->ipv4.if_addr.addr.v4 );
      }
      else if( AF_INET6 == sa_ptr->ss_family )
      {
        memcpy( addr_info_ptr->ipv6.if_addr.addr.v6_addr8,
                SASTORAGE_DATA(*sa_ptr),
                sizeof(addr_info_ptr->ipv6.if_addr.addr.v6_addr8) );
        NETMGR_LOG_IPV6_ADDR( med, "updated cached ",
                              addr_info_ptr->ipv6.if_addr.addr.v6_addr64 );
      }
      else
      {
        netmgr_log_err("netmgr_kif_cfg_fail unsupported address family[%d]\n",
                       sa_ptr->ss_family );
        return;
      }

      /* Post event to NetLink clients for local processing */
      if( NETMGR_SUCCESS != netmgr_kif_nl_post_addr_msg( link, nlmsg_info_ptr ) ) {
        netmgr_log_err("netmgr_kif_cfg_cnf failed on posting address event for link %d\n", link);
        return;
      }
      break;

    default:
      /* Ignore in all other states */
      netmgr_log_err("netmgr_kif_cfg_fail called in state %d\n", state);
      break;
  }

  NETMGR_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
  FUNCTION  netmgr_kif_close_cnf
===========================================================================*/
/*!
@brief
  Processes event indicating the given interface has been successfully
  closed.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_close_cnf (int link)
{
  netmgr_kif_state_t state;
  const netmgr_kif_clntcb_t * clntcb;
  void * clnt_hdl;
  netmgr_exec_cmd_t * cmd_buf = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  /* Verify that the specified link id is valid */
  if (netmgr_kif_verify_link(link) == NETMGR_FAILURE) {
    netmgr_log_err("netmgr_kif_close_cnf called with invalid link %d\n", link);
    return;
  }

  /* Process based on current interface state */
  switch (state = netmgr_kif_get_state(link)) {
    case NETMGR_KIF_RECONFIGURING:
      /* Intentional fall through */
    case NETMGR_KIF_OPEN:
      /* intentional fall through */
    case NETMGR_KIF_OPENING:
    case NETMGR_KIF_CLOSING:
      /* The device has gone down. Transition to closed state and call
      ** registered client handler to notify of this event.
      */
      netmgr_kif_set_state(link, NETMGR_KIF_CLOSED);

      /* Purge address(es) from network interface */
      if( NETMGR_SUCCESS != netmgr_kif_clear_iface( link ) )
      {
        netmgr_log_err("failed to clear net iface on link %d\n", link);
      }

      clntcb = netmgr_kif_get_clntcb(link);
      if( clntcb ) {
        clnt_hdl = netmgr_kif_get_clnt_hdl(link);
        netmgr_kif_set_clntcb(link, NULL);
        netmgr_kif_set_clnt_hdl(link, NULL);
        (clntcb->closed_cb)(link, NETMGR_SUCCESS, clnt_hdl);
      }

      /* Allocate command object */
      cmd_buf = netmgr_exec_get_cmd();
      NETMGR_ASSERT(cmd_buf);
      cmd_buf->data.type = NETMGR_KIF_CLOSED_EV;
      cmd_buf->data.link = link;

      /* Post command for processing in the command thread context */
      if( NETMGR_SUCCESS != netmgr_exec_put_cmd( cmd_buf ) ) {
        NETMGR_ABORT("netmgr_kif_close_cnf: failed to put commmand\n");
        netmgr_exec_release_cmd(cmd_buf);
        return;
      }
      break;

    default:
      /* Ignore in all other states */
      netmgr_log_err("netmgr_kif_close_cnf called in state %d\n", state);
      break;
  }

  NETMGR_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
  FUNCTION  netmgr_kif_init_names
===========================================================================*/
/*!
@brief
  Helper function for initializing all device names in the module.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_init_names( void )
{
  int i;
  const char * prefix = NULL;
  unsigned int offset;

  /* Iterate over the array of interfaces, initializing device name for
  ** each. The device name prefix is suffixed with integer values starting
  ** from '0' for the first one and incrementing by one.
  */
  for (i = 0; i < NETMGR_MAX_LINK; i++) {
#ifdef FEATURE_DATA_IWLAN
    int modem;
    netmgr_main_link_type link_type;

    if (NETMGR_SUCCESS != netmgr_kif_get_modem_link_type(i, &modem, &link_type)) {
      netmgr_log_high("netmgr_kif_init_names: failed to get modem/link_type for link=%d\n",
                      i);
      return;
    }

    /* Lookup modem device configuration */
    prefix = netmgr_main_dev_prefix_tbl[ modem ][ link_type ].prefix;
    offset = netmgr_main_dev_prefix_tbl[ modem ][ link_type ].inst_min;
#else
    /* Lookup modem device configuration */
    prefix = netmgr_main_dev_prefix_tbl[ (i/NETMGR_KIF_DEV_PER_MODEM) ].prefix;
    offset = netmgr_main_dev_prefix_tbl[ (i/NETMGR_KIF_DEV_PER_MODEM) ].inst_min;
#endif /* FEATURE_DATA_IWLAN */

    /* Make sure prefix is not null or has valid length, otherwise use the
    ** statically defined default device name.
    */
    if ((prefix == NULL) ||
        (std_strlen(prefix) > NETMGR_KIF_NAME_PR_MAX_LEN))
    {
      prefix = NETMGR_KIF_DEF_NAME;
    }

    (void)snprintf( netmgr_kif_info[i].name,
                    sizeof(netmgr_kif_info[i].name),
                    "%s%d", prefix, (i-offset) );
    netmgr_log_high("netmgr_kif named link %d to %s\n",
                    i, netmgr_kif_info[i].name);

  }
  return;
}

/*===========================================================================
  FUNCTION  netmgr_kif_init_paths
===========================================================================*/
/*!
@brief
  Helper function for initializing all file pathnames in the module.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_init_paths
(
    const char * dirpath,
    const char * modscript
)
{
  /* If a directory name was passed on the command line, use it as a prefix
  ** to construct the path name of the module load script.
  */
  if (dirpath) {
    (void)strlcpy( netmgr_kif_cfg.dirpath,
                       dirpath,
                       NETMGR_KIF_FILENAME_MAX_LEN );

    (void)strlcpy( netmgr_kif_cfg.modscript,
                       netmgr_kif_cfg.dirpath,
                       NETMGR_KIF_FILENAME_MAX_LEN );
  }

  /* Set module load script name in configuration blob for later use, if one
  ** was specified on the command line.
  */
  if (modscript) {
    (void)strlcat( netmgr_kif_cfg.modscript,
                       modscript,
                       NETMGR_KIF_FILENAME_MAX_LEN );
  }

  return;
}


/*===========================================================================
  FUNCTION  netmgr_kif_init_link_ifindex
===========================================================================*/
/*!
@brief
  Helper function to initializes the specified link device index.

@return
  int - NETMGR_SUCCESS if successful, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_init_link_ifindex( int link )
{
  struct ifreq ifr;

  /* no need to init ifindex for disabled interface*/
  if (netmgr_kif_cfg.link_array[link].enabled == FALSE) {
    return NETMGR_SUCCESS;
  }

  /* Call ioctl on device to get the if index */
  memset( &ifr, 0, sizeof(struct ifreq) );
  if( NETMGR_SUCCESS !=
      netmgr_kif_call_ioctl_on_dev( netmgr_kif_info[link].name,
                                    SIOCGIFINDEX,
                                    &ifr ) )
  {
    netmgr_log_err("Cannot get ifindex for dev %s!\n",
                   netmgr_kif_info[link].name);
    return NETMGR_FAILURE;
  }

  /* Save if index in the interface info struct */
  netmgr_kif_info[link].ifi_index = ifr.ifr_ifindex;

  netmgr_log_high("netmgr_kif link %d, device %s has ifindex %d\n",
                  link, netmgr_kif_info[link].name, ifr.ifr_ifindex);
  return NETMGR_SUCCESS;
}


/*===========================================================================
  FUNCTION  netmgr_kif_get_link_for_ifindex
===========================================================================*/
/*!
@brief
  Returns the link ID for a specified system device index.

@return
  int - device index if index is valid and recognized, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_get_link_for_ifindex (int ifindex)
{
  int link = -1;
  int i;

  /* Iterate over the array of interfaces, and return the link id for
  ** the interface matching the specified if index.
  */
  for( i = 0; i < NETMGR_MAX_LINK; i++ ) {
    if( netmgr_kif_cfg.link_array[i].enabled && (netmgr_kif_info[i].ifi_index == ifindex) ) {
      link = i;
      break;
    }
  }

  return link;
}

/*===========================================================================
  FUNCTION  netmgr_kif_load_module
===========================================================================*/
/*!
@brief
  Loads the module providing implementation of virtual Ethernet interfaces.

@return
  int - 0 if module is successfully loaded, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_load_module (void)
{
  char scr_buf[NETMGR_KIF_FILENAME_MAX_LEN];

  NETMGR_LOG_FUNC_ENTRY;

  /* Construct command to load the module. Use module script name if one
  ** was specified on the command line, otherwise use modprobe to load
  ** module assuming the module name is derived from the interface name.
  */
  if (std_strlen(netmgr_kif_cfg.modscript) == 0) {
    (void)snprintf( scr_buf,
                    NETMGR_KIF_FILENAME_MAX_LEN,
                    "modprobe %s",
                    netmgr_kif_cfg.name );
    netmgr_log_high("Loading module %s\n", netmgr_kif_cfg.name);
  } else {
    (void)snprintf( scr_buf,
                    NETMGR_KIF_FILENAME_MAX_LEN,
                    "%s",
                    netmgr_kif_cfg.modscript );
    netmgr_log_high( "Running module load script %s\n",
                     netmgr_kif_cfg.modscript );
  }

  /* Issue command to load module */
  if (ds_system_call(scr_buf, std_strlen(scr_buf)) != 0) {
    return -1;
  }

  NETMGR_LOG_FUNC_EXIT;
  return 0;
}


/*===========================================================================
  FUNCTION  netmgr_kif_nl_recv_link_msg
===========================================================================*/
/*!
@brief
  Processes incoming NETLINK_ROUTE messages related to link/interface state.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_nl_recv_link_msg (const netmgr_nl_msg_t  * nlmsg_info_ptr)
{
  struct ifinfomsg * ifinfo;
  int link;

  NETMGR_ASSERT( nlmsg_info_ptr );

  NETMGR_LOG_FUNC_ENTRY;

  if( nlmsg_info_ptr->type == RTM_NEWLINK ) {
    netmgr_log_high("Received RTM_NEWLINK\n");
  } else if( nlmsg_info_ptr->type == RTM_DELLINK ) {
    netmgr_log_high("Received RTM_DELLINK\n");
  } else {
    netmgr_log_err("Unknown Netlink msg type LINK [%d], ignoring!\n",
                    nlmsg_info_ptr->type);
    goto ret;
  }

  /* For a LINK message, data is a ptr to struct ifinfomsg type */
  if( NETMGR_NL_PARAM_LINK & nlmsg_info_ptr->param_mask ) {
    ifinfo = (struct ifinfomsg*)&nlmsg_info_ptr->link_info.metainfo;
  } else {
    netmgr_log_err("Netlink msg missing LINK parmeters [%d], ignoring!\n",
                    nlmsg_info_ptr->type);
    goto ret;
  }

  /* Get link id for the device using the if index indicated in the msg */
  if( (link = netmgr_kif_get_link_for_ifindex(ifinfo->ifi_index)) < 0 ) {
    /* Could not get link id. This is probably an interface that we don't
    ** care about. */
    netmgr_log_err("unrecognized ifindex %d, disable link\n",
                   ifinfo->ifi_index);
    NETMGR_LOG_FUNC_EXIT;
    return;
  }

  /* Process message if there is a change in the state of the interface */
  if( ifinfo->ifi_change & IFF_UP ) {
    if( ifinfo->ifi_flags & IFF_UP ) {
      /* Interface came up. Process event based on current state */
      netmgr_log_high("link %d interface up\n", link);
      netmgr_kif_open_cnf(link);

      /* Print some debug messages */
      if( nlmsg_info_ptr->type == RTM_NEWLINK ) {
        netmgr_log_high("RTM_NEWLINK rcvd with interface up\n");
      } else {
        netmgr_log_high("not RTM_NEWLINK!\n");
      }
    }
    else {
      /* Interface went down. Process event based on current state */
      netmgr_log_high("link %d interface down\n", link);
      netmgr_kif_close_cnf(link);

      /* Print some debug messages */
      if( nlmsg_info_ptr->type == RTM_DELLINK ) {
        netmgr_log_high("RTM_DELLINK rcvd with interface down\n");
      } else {
        netmgr_log_high("not RTM_DELLINK!\n");
      }
    }
  }
  else {
    netmgr_log_high("not IFF_UP change, ignoring\n");
  }

ret:
  NETMGR_LOG_FUNC_EXIT;
  return;
}

#ifdef FEATURE_ADD_DNS_ROUTES
/*===========================================================================
  FUNCTION netmgr_check_ip_not_empty
===========================================================================*/
/*!
@brief
 Checks for non-zero IP addresses

@return
  int: 0 if empty, 1 otherwise
*/
/*=========================================================================*/

static int netmgr_check_ip_not_empty(uint32 *ip, int af)
{
  int i;
  int retval = 0;

  if(ip)
  {
    switch (af)
    {
    case AF_INET:
      retval = (*ip != 0);
      break;
    case AF_INET6:
      for (i = 0; i < NETMGR_NUMBER_OF_INT32_IN_IPV6_ADDR; i++)
        if (ip[i] != 0)
        {
          retval = 1;
          break;
        }
      break;
    }
  }

  if (!retval)
  {
    netmgr_log_low("%s(): Caught empty IP\n", __func__);
  }
  return retval;
}

/*===========================================================================
  FUNCTION netmgr_ip_route_add
===========================================================================*/
/*!
@brief
 Prepares and makes system call to add routes to the routing table

@return
  void
*/
/*=========================================================================*/
static void netmgr_ip_route_add(int addrtype,
                                netmgr_ip_address_t dns,
                                netmgr_ip_address_t gateway,
                                netmgr_ip_address_t if_addr)
{
   /* Something to store the presentation of the addresses. INET6 > INET4 */
   char addr[3][INET6_ADDRSTRLEN];
   char cmd[NETMGR_KIF_SYSCMD_SIZ];
   int cmd_result = -1;
   int valid = 0;
   const char *ipv4_ip_cmd = "/system/bin/ip route add %s/32 via %s src %s";
   const char *ipv6_ip_cmd = "/system/bin/ip -6 route add %s/128 via %s src %s";

   memset(cmd, 0, NETMGR_KIF_SYSCMD_SIZ);

   switch(addrtype)
   {
   case  NETMGR_ADDRSET_MASK_IPV4:
      if (netmgr_check_ip_not_empty(&(dns.addr.v4), AF_INET)
          && netmgr_check_ip_not_empty(&(gateway.addr.v4), AF_INET)
          && netmgr_check_ip_not_empty(&(if_addr.addr.v4), AF_INET))
      {
         inet_ntop(AF_INET, &(dns.addr.v4),     addr[0], INET_ADDRSTRLEN);
         inet_ntop(AF_INET, &(gateway.addr.v4), addr[1], INET_ADDRSTRLEN);
         inet_ntop(AF_INET, &(if_addr.addr.v4), addr[2], INET_ADDRSTRLEN);
         snprintf(cmd, NETMGR_KIF_SYSCMD_SIZ, ipv4_ip_cmd, addr[0], addr[1], addr[2]);
         valid = 1;
      }
      break;

   case  NETMGR_ADDRSET_MASK_IPV6:
      if (netmgr_check_ip_not_empty(dns.addr.v6_addr32, AF_INET6)
          && netmgr_check_ip_not_empty(gateway.addr.v6_addr32, AF_INET6)
          && netmgr_check_ip_not_empty(if_addr.addr.v6_addr32, AF_INET6))
      {
         inet_ntop(AF_INET6, dns.addr.v6_addr8,     addr[0], INET6_ADDRSTRLEN);
         inet_ntop(AF_INET6, gateway.addr.v6_addr8, addr[1], INET6_ADDRSTRLEN);
         inet_ntop(AF_INET6, if_addr.addr.v6_addr8, addr[2], INET6_ADDRSTRLEN);
         snprintf(cmd, NETMGR_KIF_SYSCMD_SIZ, ipv6_ip_cmd, addr[0], addr[1], addr[2]);
         valid = 1;
      }
      break;

   default:
      netmgr_log_err("%s(): Unkown address type %d\n", __func__, addrtype);
      return;
   }

   if (valid)
   {
      netmgr_log_med("%s(): Adding DNS route: %s\n", __func__, cmd);

      cmd_result = ds_system_call(cmd, strlen(cmd));
      /* Log error and continue */
      if (cmd_result != 0)
      {
         netmgr_log_err("%s(): Route add failed. Command: %s\n", __func__, cmd);
      }
   }
   else
   {
      netmgr_log_med("%s(): Can't add invalid route."
                     " One or more IPs are empty", __func__);
   }
}

/*===========================================================================
  FUNCTION netmgr_set_dns_routes
===========================================================================*/
/*!
@brief
 Sets routes for DNS servers associated with a particular interface. This
 ensures that DNS packets are sent over the correct interface.

@return
  void
*/
/*=========================================================================*/
static void netmgr_set_dns_routes(netmgr_address_info_t *iface_conf)
{

  if (!iface_conf)
  {
    netmgr_log_err("%s(): called with null iface_conf", __func__);
    /* Nothing we can do here, just return */
    return;
  }

  /* Set IPv4 name servers if we have IPv4 */
  if (iface_conf->valid_mask & NETMGR_ADDRSET_MASK_IPV4)
  {
    netmgr_ip_route_add(NETMGR_ADDRSET_MASK_IPV4, iface_conf->ipv4.dns_primary,
                        iface_conf->ipv4.gateway, iface_conf->ipv4.if_addr);
    netmgr_ip_route_add(NETMGR_ADDRSET_MASK_IPV4, iface_conf->ipv4.dns_secondary,
                        iface_conf->ipv4.gateway, iface_conf->ipv4.if_addr);
  }

  /* Set IPv6 name servers if we have IPv6 */
  if (iface_conf->valid_mask & NETMGR_ADDRSET_MASK_IPV6)
  {
    netmgr_ip_route_add(NETMGR_ADDRSET_MASK_IPV6, iface_conf->ipv6.dns_primary,
                        iface_conf->ipv6.gateway, iface_conf->ipv6.if_addr);
    netmgr_ip_route_add(NETMGR_ADDRSET_MASK_IPV6, iface_conf->ipv6.dns_secondary,
                        iface_conf->ipv6.gateway, iface_conf->ipv6.if_addr);
  }
}

#endif /* FEATURE_ADD_DNS_ROUTES */

/*===========================================================================
  FUNCTION  netmgr_kif_nl_recv_addr_msg
===========================================================================*/
/*!
@brief
  Processes incoming NETLINK_ROUTE messages related to address configuration.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_nl_recv_addr_msg( const netmgr_nl_msg_t * nlmsg_info_ptr )
{
  struct ifaddrmsg * ifaddr;
  int link;

  NETMGR_ASSERT( nlmsg_info_ptr );

  NETMGR_LOG_FUNC_ENTRY;

  /* For a ADDR message, data is a ptr to struct ifaddrmsg type */
  if( NETMGR_NL_PARAM_ADDR & nlmsg_info_ptr->param_mask ) {
    ifaddr = (struct ifaddrmsg*)&nlmsg_info_ptr->addr_info.metainfo;
  } else {
    netmgr_log_err("Netlink msg missing ADDR parmeters [%d], ignoring!\n",
                   nlmsg_info_ptr->type);
    goto ret;
  }

  /* Get link id for the device using the if index indicated in the msg */
  if( (link = netmgr_kif_get_link_for_ifindex( ifaddr->ifa_index ) ) < 0) {
    /* Could not get link id. This is probably an interface that we don't
    ** care about. Ignore message.
    */
    netmgr_log_err("unrecognized ifindex %d\n", ifaddr->ifa_index);
    goto ret;
  }

  /* Process based on message type */
  if( nlmsg_info_ptr->type == RTM_NEWADDR ) {
    /* A new address was configured on the interface. Process based on
    ** the current state of the interface.
    */
#if defined (FEATURE_DATA_LINUX_LE) && defined (FEATURE_DATA_SOFTAP_V6)
    /* For SoftAP V6 architecture, Data call bringup is split into LAN bringup
    ** and WWAN bringup. For LAN bringup, which means the interface is only a
    ** part of the local link/LAN, link local address assigned to the interface
    ** is sufficient to confirm to clients that interface is configured for link
    ** local communication.

    ** At a later point in time, when the WWAN/backhaul is brought up, modem sends
    ** over an asynchronous RA to the linux kernel, which will then configure the
    ** interface with global IPV6 address attaining global scope.
    */
#else
    if (QMI_WDS_IFACE_NAME_MODEM_LINK_LOCAL == netmgr_qmi_wds_get_tech_name(link))
    /* For MODEM_LINK_LOCAL tech, there is no global address assigned, we follow
    ** same condition as SoftAP V6
    */
#endif/*defined (FEATURE_DATA_LINUX_LE ) && defined (FEATURE_DATA_SOFTAP_V6)*/
    {
      if ((ifaddr->ifa_family == AF_INET6) &&
            (ifaddr->ifa_scope == NETMGR_KIF_LOCAL_SCOPE))
      {
        netmgr_log_high("Link local address configured on link:%d\n", link);
        netmgr_kif_cfg_cnf( link, nlmsg_info_ptr );
      }
    }

    /* Check that the scope is global; ignore local adddresses */
    if( NETMGR_KIF_GLOBAL_SCOPE == ifaddr->ifa_scope ) {
      netmgr_log_high("address configuration update on link %d\n", link);
      netmgr_kif_cfg_cnf( link, nlmsg_info_ptr );
    }
#ifdef FEATURE_ADD_DNS_ROUTES
    netmgr_set_dns_routes(netmgr_qmi_get_addr_info(link));
#endif /* FEATURE_ADD_DNS_ROUTES */
  }
  else if( nlmsg_info_ptr->type == RTM_DELADDR ) {
    /* An IP address was deleted from the interface. Process based on the
    ** current state of the interface.
    */

    /* Check that the scope is global; ignore local adddresses */
    if( NETMGR_KIF_GLOBAL_SCOPE == ifaddr->ifa_scope ) {
      netmgr_log_high("address configuration release on link %d\n", link);
      netmgr_kif_cfg_fail( link, nlmsg_info_ptr );
    }
    else
    {
      netmgr_log_low("not global address, ignoring on link %d\n", link);
    }
  }
  else {
    netmgr_log_err("Unknown Netlink msg type [%d], ignoring link %d!\n",
                   nlmsg_info_ptr->type, link);
  }

ret:
  NETMGR_LOG_FUNC_EXIT;
  return;
}

#ifdef FEATURE_DATA_IWLAN
/*===========================================================================
  FUNCTION  netmgr_kif_nl_recv_xfrm_newae_msg
===========================================================================*/
/*!
@brief
  Processes incoming XFRM_MSG_NEWAE messages

@return
  void

@note

  - Dependencies
    - None
  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_nl_recv_xfrm_newae_msg (const netmgr_nl_msg_t  * nlmsg_info_ptr)
{
  int               result = NETMGR_FAILURE, reti = NETMGR_FAILURE;
  int               i = 0;
  unsigned long     spi = 0, seq_num = 0;
  netmgr_address_info_t  *addr_info_ptr = NULL;

  NETMGR_ASSERT( nlmsg_info_ptr );

  NETMGR_LOG_FUNC_ENTRY;
  do
  {
    if (NETMGR_XFRM_PARAM_ESPTHRESH & nlmsg_info_ptr->param_mask)
    {
      spi     = htonl(nlmsg_info_ptr->xfrm_info.xfrm_aevent.sa_id.spi);
      seq_num = htonl(nlmsg_info_ptr->xfrm_info.xfrm_replay.oseq);
      netmgr_log_med("SPI: 0x%08x", spi);
      netmgr_log_med("Sequence num: 0x%08x", seq_num);

      for (i = 0; i < NETMGR_MAX_LINK; i++)
      {
        if (NETMGR_KIF_IS_REV_RMNET_LINK(i))
        {
          /* Check the SPI with the one obtained from the netlink message */
          addr_info_ptr = netmgr_qmi_get_addr_info(i);

          if (NULL == addr_info_ptr)
          {
            netmgr_log_err("Failed to obtain address information for link [%d]",
                           i);
            /* Some rev_rmnet ports maybe inactive on certain devices
             * Continue with other ports if getting addr info fails */
            continue;
          }

          if (spi != addr_info_ptr->ipv4.sa.esp_spi_v4
              && spi != addr_info_ptr->ipv6.sa.esp_spi_v6)
          {
            /* We need to continue searching all rev_rmnet ports */
            continue;
          }
          else if (spi == addr_info_ptr->ipv4.sa.esp_spi_v4)
          {
            netmgr_log_low("ESP SPI v4: 0x%08x link [%d]",
                           addr_info_ptr->ipv4.sa.esp_spi_v4, i);
            reti = netmgr_qmi_initiate_esp_rekey(i, NETMGR_QMI_CLIENT_IPV4);
            break;
          }
          else if (spi == addr_info_ptr->ipv6.sa.esp_spi_v6)
          {
            netmgr_log_low("ESP SPI v6: 0x%08x link [%d]",
                           addr_info_ptr->ipv4.sa.esp_spi_v6, i);
            reti = netmgr_qmi_initiate_esp_rekey(i, NETMGR_QMI_CLIENT_IPV6);
            break;
          }
        }
      }
    }

    if (reti == NETMGR_FAILURE)
    {
      break;
    }

    result = NETMGR_SUCCESS;
  } while(0);

  if (NETMGR_SUCCESS != result)
  {
    netmgr_log_err("ESP rekey failed!");
  }
  else
  {
    netmgr_log_med("ESP rekey success!");
  }
}
#endif /* FEATURE_DATA_IWLAN */

/*===========================================================================
  FUNCTION  netmgr_kif_nl_msg_recv_cmd_exec
===========================================================================*/
/*!
@brief
  Virtual function called by the Command Thread to execute KIF command
  to process a received NETLINK message.

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_nl_msg_recv_cmd_exec (ds_cmd_t * cmd, void * data)
{
  struct msghdr * msgh;
  struct nlmsghdr * nlmsgh;
  netmgr_nl_msg_t   nlmsg_info;
  (void)cmd;

  NETMGR_LOG_FUNC_ENTRY;

  /* Get netlink message ptr from the message header */
  msgh = (struct msghdr *)((netmgr_msg_t*)data)->msg;
  nlmsgh = (struct nlmsghdr *)(msgh->msg_iov->iov_base);

  /* Decode the message in structure */
  memset( &nlmsg_info, 0x0, sizeof(nlmsg_info) );
  if( NETMGR_SUCCESS !=
      netmgr_nl_decode_nlmsg( (char*)nlmsgh,
                              ((netmgr_msg_t*)data)->msglen, &nlmsg_info ) ) {
    netmgr_log_err( "Error on  netmgr_nl_decode_nlmsg\n" );
    return NETMGR_FAILURE;
  }

  netmgr_log_med( "Rcvd Netlink msg type [%d]\n", nlmsg_info.type );

  /* Process based on netlink message type */
  switch( nlmsg_info.type ) {
    case RTM_NEWLINK:
    case RTM_DELLINK:
      /* Process NETLINK_ROUTE message of type LINK */
      netmgr_kif_nl_recv_link_msg( &nlmsg_info );
      break;
    case RTM_NEWADDR:
    case RTM_DELADDR:
      /* Process NETLINK_ROUTE message of type ADDR */
      netmgr_kif_nl_recv_addr_msg( &nlmsg_info );
      break;
    case RTM_NEWPREFIX:
      netmgr_log_high("Received NEWPREFIX, ignoring\n");
      /* Do nothing */
      break;
    case RTM_NEWNEIGH:
    case RTM_DELNEIGH:
      /* Do nothing */
      break;
#ifdef FEATURE_DATA_IWLAN
    case XFRM_MSG_NEWAE:
      netmgr_kif_nl_recv_xfrm_newae_msg ( &nlmsg_info );
      break;
#endif /* FEATURE_DATA_IWLAN */
    default:
      /* Ignore all other message types */
      netmgr_log_low("received unknown nl msg\n");
      break;
  }

  /* NetLink message buffer released in netmgr_kif_nl_msg_recv_cmd_free() */

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;
}

/*===========================================================================
  FUNCTION  netmgr_kif_nl_msg_recv_cmd_free
===========================================================================*/
/*!
@brief
  Virtual function called by the Command Thread to free KIF command
  to process a received NETLINK message, after execution of the command is
  complete.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_nl_msg_recv_cmd_free (ds_cmd_t * cmd, void * data)
{
  struct msghdr * msgh;
  struct nlmsghdr * nlmsgh;
  netmgr_exec_cmd_t * cmd_buf = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  NETMGR_ASSERT( cmd );

  cmd_buf = (netmgr_exec_cmd_t *)data;

  /* Get message ptr from the user data ptr */
  msgh = (struct msghdr *)cmd_buf->data.info.kif_msg.msg;

  /* Get netlink message ptr from the message header */
  nlmsgh = (struct nlmsghdr *)(msgh->msg_iov->iov_base);

  /* Deallocate memory for the address structure */
  netmgr_free(msgh->msg_name);

  /* Deallocate memory for the message buffer */
  netmgr_free(nlmsgh);

  /* Deallocate memory for the io vector */
  netmgr_free(msgh->msg_iov);

  /* Deallocate memory for the message header */
  netmgr_free(msgh);

  /* Release NetMgr command buffer */
  cmd_buf = cmd->data;
  NETMGR_ASSERT( cmd_buf );
  netmgr_free( cmd_buf );

  NETMGR_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
  FUNCTION  netmgr_kif_find_default_link
===========================================================================*/
/*!
@brief
  This function goes throughs the link array finds the first link
  that is enabled.

@return
  int - interface id on successful operation, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static inline int netmgr_kif_find_default_link(void)
{
  int i=0;
  for(i=0; i<NETMGR_MAX_LINK; i++)
  {
    if (TRUE == netmgr_kif_cfg.link_array[i].enabled)
    {
      return i;
    }
  }
  return -1;
}

/*===========================================================================
  FUNCTION  netmgr_kif_nl_recv_routing_msg
===========================================================================*/
/*!
@brief
  Virtual function registered with the socket listener thread to receive
  incoming messages over the NETLINK routing socket.

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_nl_recv_routing_msg (int fd)
{
  struct msghdr * msgh = NULL;
  netmgr_exec_cmd_t * cmd_buf = NULL;
  unsigned int  msglen = 0;
  int default_link = -1;

  NETMGR_LOG_FUNC_ENTRY;

  /* Read netlink message from the socket */
  if( NETMGR_SUCCESS != netmgr_nl_recv_msg( fd, &msgh, &msglen ) ) {
    netmgr_log_err( "netmgr_kif_nl_recv_routing_msg: netmgr_nl_recv_msg failed!\n" );
    NETMGR_LOG_FUNC_EXIT;
    return NETMGR_FAILURE;
  }

  /* find an available link where the command can be posted to */
  default_link = netmgr_kif_find_default_link();
  if (-1 == default_link)
  {
    netmgr_log_err( "netmgr_kif_nl_recv_routing_msg: could not find default link" );
    netmgr_nl_release_msg( msgh );
    NETMGR_LOG_FUNC_EXIT;
    return NETMGR_FAILURE;
  }

  /* Allocate command object */
  cmd_buf = netmgr_exec_get_cmd();
  NETMGR_ASSERT(cmd_buf);

  /* Override cmd buffer free method to release netline message */
  cmd_buf->cmd.free_f = netmgr_kif_nl_msg_recv_cmd_free;

  /* Set data ptr in the command object to the netlink message header ptr */
  cmd_buf->data.type = NETMGR_KIF_MSG_CMD;
  cmd_buf->data.link = default_link;  /* process message through first available link */
  cmd_buf->data.info.kif_msg.msg = msgh;
  cmd_buf->data.info.kif_msg.msglen = msglen;

  /* Post command for processing in the command thread context */
  if( NETMGR_SUCCESS != netmgr_exec_put_cmd( cmd_buf ) ) {
    netmgr_log_err("netmgr_kif_nl_recv_routing_msg: failed to put commmand\n");
    netmgr_exec_release_cmd( cmd_buf );
    NETMGR_LOG_FUNC_EXIT;
    return NETMGR_FAILURE;
  }

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;
}

#ifdef FEATURE_DATA_IWLAN
/*===========================================================================
  FUNCTION  netmgr_kif_nl_recv_xfrm_msg
===========================================================================*/
/*!
@brief
  Virtual function registered with the socket listener thread to receive
  incoming messages over the NETLINK XFRM events socket.

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_nl_recv_xfrm_msg (int fd)
{
  struct msghdr * msgh = NULL;
  netmgr_exec_cmd_t * cmd_buf = NULL;
  unsigned int msglen = 0;
  int default_link = -1;

  /* Read the netlink message from the socket */
  if (NETMGR_SUCCESS != netmgr_nl_recv_msg( fd, &msgh, &msglen ) )
  {
    netmgr_log_err("netmgr_kif_nl_recv_xfrm_msg: netmgr_nl_recv_msg failed!\n");
    NETMGR_LOG_FUNC_EXIT;
    return NETMGR_FAILURE;
  }

  /* Find an available link where the command can be posted to */
  default_link = netmgr_kif_find_default_link();
  if (-1 == default_link)
  {
    netmgr_log_err("netmgr_kif_nl_recv_xfrm_msg: could not find default link!\n");
    netmgr_nl_release_msg(msgh);
    NETMGR_LOG_FUNC_EXIT;
    return NETMGR_FAILURE;
  }

  /* Allocate command object */
  cmd_buf = netmgr_exec_get_cmd();
  NETMGR_ASSERT(cmd_buf);

  /* Override cmd buffer free method to release netline message */
  cmd_buf->cmd.free_f = netmgr_kif_nl_msg_recv_cmd_free;

  /* Set data ptr in the command object to the netlink message header ptr */
  cmd_buf->data.type = NETMGR_KIF_MSG_CMD;
  cmd_buf->data.link = default_link; /* Process message through first available link */
  cmd_buf->data.info.kif_msg.msg = msgh;
  cmd_buf->data.info.kif_msg.msglen = msglen;

  /* Post command for processing in the command thread context */
  if (NETMGR_SUCCESS != netmgr_exec_put_cmd( cmd_buf ))
  {
    netmgr_log_err("netmgr_kif_nl_recv_xfrm_msg: failed to put commmand\n");
    netmgr_exec_release_cmd( cmd_buf );
    NETMGR_LOG_FUNC_EXIT;
    return NETMGR_FAILURE;
  }

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;
}
#endif /* FEATURE_DATA_IWLAN */

/*===========================================================================
  FUNCTION  netmgr_kif_send_event
===========================================================================*/
/*!
@brief
  Generates an asynchronous event indicaiton messages using NETLINK socket.
  See kernel/include/net/netlink.h for details on message TLV formatting.

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_send_event
(
  const netmgr_nl_event_info_t *event_info
)
{
  int result = NETMGR_FAILURE;
  char * buffer = NULL;
  unsigned int buflen = 0;

  NETMGR_LOG_FUNC_ENTRY;

  result = netmgr_nl_encode_netmgr_event( event_info,
                                          &buffer,
                                          &buflen );

  if( NETMGR_SUCCESS == result ) {
    /* Validate buffer */
    if( (buffer && (buflen == 0)) ||
        (!buffer && (buflen != 0)) ){
      netmgr_log_err("invalid buffer allocated\n");
      return NETMGR_FAILURE;
    }

    /* Generate NETLINK message */
    result = netmgr_nl_send_msg( netmgr_kif_sk_grp.ev_sk.sk_fd,
                                 buffer,
                                 buflen );
  } else {
    netmgr_log_err("failed on netmgr_nl_encode_netmgr_event\n");
  }

  netmgr_free( buffer );

  NETMGR_LOG_FUNC_EXIT;
  return result;
}

/*===========================================================================
  FUNCTION  netmgr_kif_process_user_cmd
===========================================================================*/
/*!
@brief
  Processes user provided command

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise
*/
/*=========================================================================*/
static int
netmgr_kif_process_user_cmd
(
  int user_cmd
)
{
  char cmd[NETMGR_KIF_MAX_COMMAND_LENGTH];
  int result;

  netmgr_log_high("Process user command: %d\n", user_cmd);

  switch (user_cmd)
  {
    case NETMGR_USER_CMD_SCREEN_OFF:
      netmgr_log_med("Received SCREEN_OFF: Installing rules to drop output rst packets!\n");
      strlcpy(cmd, NETMGR_IPTABLES_RULE_DROP_TCP_RST_PKTS, sizeof(cmd));
      (void) ds_system_call(cmd, std_strlen(cmd));
      break;

    case NETMGR_USER_CMD_SCREEN_ON:
      netmgr_log_med("Received SCREEN_ON: Deleting rules to drop output rst packets!\n");
      strlcpy(cmd, NETMGR_IPTABLES_RULE_DEL_DROP_TCP_RST_PKTS, sizeof(cmd));
      (void) ds_system_call(cmd, std_strlen(cmd));
      break;
  }

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;
}


/*===========================================================================
  FUNCTION  netmgr_kif_nl_recv_ping_msg
===========================================================================*/
/*!
@brief
  Virtual function registered with the socket listener thread to receive
  incoming messages over the NETLINK GRP events socket.

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_nl_recv_ping_msg (int fd)
{
  struct msghdr      *msgh   = NULL;
  struct iovec       *iov    = NULL;
  netmgr_nl_msg_t    *nlmsg  = NULL;
  unsigned int        msglen = 0;

  int                 result = NETMGR_FAILURE;
  char               *buffer = NULL;
  unsigned int        buflen = 0;
  netmgr_nl_event_info_t *event_info = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  /* Allocate event message buffer */
  nlmsg = netmgr_malloc( sizeof(netmgr_nl_msg_t));
  if( NULL == nlmsg) {
    netmgr_log_err("failed to allocate message buffer!\n");
    goto bail;
  } else{
    /* Read netlink message from the socket */
    if( NETMGR_SUCCESS != netmgr_nl_recv_msg( fd, &msgh, &msglen ) ) {
      netmgr_log_err( "netmgr_kif_nl_recv_ping_msg: netmgr_nl_recv_msg failed!\n" );
      goto bail;
    }

    iov = msgh->msg_iov;

    /* Decode the message in structure */
    memset( nlmsg, 0x0, sizeof(netmgr_nl_msg_t));
    if(NETMGR_SUCCESS !=
       netmgr_nl_decode_nlmsg((char*)iov->iov_base, msglen, nlmsg)) {
      netmgr_log_err("netmgr_nl_decode_nlmsg failed!\n");
      goto bail;
    }

    netmgr_log_med( "Rcvd Netlink msg type [%d]\n", nlmsg->event_info.event );
    netmgr_log_med( "param mask: 0x%x, event %d\n", nlmsg->param_mask, nlmsg->event_info.event );

    if( (NETMGR_NL_PARAM_EVENT & nlmsg->param_mask) &&
        (NETMGR_READY_REQ == nlmsg->event_info.event)) {
      /* this is the ping message, prepare response */
      netmgr_log_high("netmgr_kif_nl_recv_ping_msg: Ping message recved!\n");

      /* Post event indication to clients */
      event_info = netmgr_malloc( sizeof(netmgr_nl_event_info_t) );
      if( NULL == event_info ) {
        netmgr_log_err("failed to allocate event buffer!\n");
        goto bail;
      } else {
        memset( event_info, 0x0, sizeof(netmgr_nl_event_info_t) );
        event_info->event = NETMGR_READY_RESP;

        if(NETMGR_FAILURE == netmgr_kif_send_event(event_info)) {
          netmgr_log_err("netmgr_kif_send_event failed!\n");
          goto bail;
        }
        netmgr_free(event_info);
      }
    }
    else if (NETMGR_USER_CMD == nlmsg->event_info.event) {
      netmgr_log_med("netmgr_kif_nl_recv_ping_msg: Received user command!\n");
      netmgr_kif_process_user_cmd(nlmsg->event_info.user_cmd);
    }
    else {
      netmgr_log_low("Unknown event message received %d\n", nlmsg->event_info.event);
    }

  }
  netmgr_free(nlmsg);
  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;
bail:

    if(nlmsg) {
      netmgr_free(nlmsg);
    }
    if(event_info) {
      netmgr_free(event_info);
    }
    NETMGR_LOG_FUNC_EXIT;
    return NETMGR_FAILURE;
}
/*===========================================================================
  FUNCTION  netmgr_kif_configure
===========================================================================*/
/*!
@brief
  API to configure virtual Ethernet interface for the specified link.

@return
  int - NETMGR_SUCCESS if successful,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_configure( int link, netmgr_ip_addr_t addr_type )
{
  int ret = NETMGR_FAILURE;
  netmgr_kif_state_t state;

  NETMGR_LOG_FUNC_ENTRY;

  /* following do..while loop can be used in lieu of
     the infamous goto error approach */
  do
  {
    /* Verify that the specified link id is valid */
    if( netmgr_kif_verify_link(link) == NETMGR_FAILURE ) {
      netmgr_log_err("netmgr_kif_open_cnf called with invalid link %d\n", link);
      break;
    }

    /* Process based on current interface state */
    switch( state = netmgr_kif_get_state(link) ) {
      case NETMGR_KIF_OPENING:
        netmgr_log_med("Starting address type %d configuration on link %d\n",
                       addr_type, link);
        if( NETMGR_SUCCESS == (ret = netmgr_kif_cfg_req(link, addr_type)) ) {
          ret = NETMGR_SUCCESS;
        } else {
          netmgr_log_err("netmgr_kif_cfg_req failed for link %d\n", link);
        }
        break; /* breaks from switch */

      case NETMGR_KIF_OPEN:
        netmgr_log_med("Starting address type %d configuration on link %d\n",
                       addr_type, link);
        if( NETMGR_SUCCESS == (ret = netmgr_kif_cfg_req(link, addr_type)) ) {
          ret = NETMGR_SUCCESS;
        } else {
          netmgr_log_err("netmgr_kif_cfg_req failed for link %d\n", link);
        }
        break;

      default:
        netmgr_log_err("netmgr_kif_configure not allowed in state %d link %d\n",
                       state, link);
        break;
    }

    /* current assumption is that we always come out of
     * do..while after this switch stmt is executed
     * so, if you plan to add more code here, make sure
     * it is enclosed in if (ret == desired_value) block.*/

  } while(0);

  NETMGR_LOG_FUNC_EXIT;
  return ret;
}

/*===========================================================================
  FUNCTION  netmgr_kif_reconfigure
===========================================================================*/
/*!
@brief
  API to reconfigure virtual Ethernet interface for the specified link.
  The address information is the previous address which must be purged.

@return
  int - NETMGR_SUCCESS if successful,
        NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_reconfigure( int link,
                        netmgr_address_set_t * addr_info_ptr )
{
  int ret = NETMGR_FAILURE;
  netmgr_kif_state_t state;

  NETMGR_LOG_FUNC_ENTRY;

  /* following do..while loop can be used in lieu of
     the infamous goto error approach */
  do
  {
    /* Verify that the specified link id is valid */
    if( netmgr_kif_verify_link(link) == NETMGR_FAILURE ) {
      netmgr_log_err("netmgr_kif_open_cnf called with invalid link %d\n", link);
      break;
    }

    /* Process based on current interface state */
    switch( state = netmgr_kif_get_state(link) ) {
      case NETMGR_KIF_OPEN:
      case NETMGR_KIF_RECONFIGURING:
        /* Force reset of socket connections */
        if( addr_info_ptr )
        {
          netmgr_kif_purge_address( link, &addr_info_ptr->if_addr, addr_info_ptr->if_mask );

          netmgr_log_med("Starting address reconfiguration on link %d\n", link);
          if( NETMGR_SUCCESS == (ret = netmgr_kif_cfg_req(link, addr_info_ptr->if_addr.type)) ) {
            netmgr_kif_set_state(link, NETMGR_KIF_RECONFIGURING);
            ret = NETMGR_SUCCESS;
          } else {
            netmgr_log_err("netmgr_kif_cfg_req failed for link %d\n", link);
          }
          netmgr_kif_info[link].dns_v6_queried = FALSE;
        }
        else
        {
          netmgr_log_err("No address information provided, ignoring input\n");
        }
        break; /* breaks from switch */
      default:
        netmgr_log_err("netmgr_kif_reconfigure not allowed in state %d\n",
                       state);
        break;
    }

    /* current assumption is that we always come out of
     * do..while after this switch stmt is executed
     * so, if you plan to add more code here, make sure
     * it is enclosed in if (ret == desired_value) block.*/

  } while(0);

  NETMGR_LOG_FUNC_EXIT;
  return ret;
}

/*===========================================================================
  FUNCTION  netmgr_kif_open
===========================================================================*/
/*!
@brief
  API to bring up virtual Ethernet interface for the specified link. Once
  interface is up, the associated client callback is called.

@return
  int - NETMGR_SUCCESS if command to bring up interface is successfully
        issued, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_open( int link,
                 netmgr_address_set_t * addr_info_ptr,
                 const netmgr_kif_clntcb_t * clntcb,
                 void * clnt_hdl )
{
  int rval = NETMGR_FAILURE;
  netmgr_kif_state_t state;

  NETMGR_ASSERT( addr_info_ptr);

  NETMGR_LOG_FUNC_ENTRY;

  /* Verify the link id first */
  if (netmgr_kif_verify_link(link) == NETMGR_FAILURE) {
    netmgr_log_err("netmgr_kif_open called with invalid link %d\n", link);
    goto error;
  }

  /* Process based on current interface state */
  switch (state = netmgr_kif_get_state(link)) {
    case NETMGR_KIF_CLOSED:
    case NETMGR_KIF_CLOSING:
      /* Interface is currently closed or closing. Issue command to
      ** bring interface up and transition to the opening state.
      */
      netmgr_kif_set_clntcb(link, clntcb);
      netmgr_kif_set_clnt_hdl(link, clnt_hdl);
      netmgr_kif_set_state(link, NETMGR_KIF_OPENING);
      netmgr_kif_open_req(link, addr_info_ptr);
      break;
    default:
      /* Ignore open request in all other states */
      netmgr_log_err("netmgr_kif_open called in state %d\n", state);
      goto error;
  }

  rval = NETMGR_SUCCESS;

 error:
  NETMGR_LOG_FUNC_EXIT;
  return rval;
}

/*===========================================================================
  FUNCTION  netmgr_kif_close
===========================================================================*/
/*!
@brief
  API to bring down virtual Ethernet interface for the specified
  link. Once interface is down, the associated client callback is
  called.  The teardown_iface flag control whether kernel interface
  state is changed.

  For dual-IP calls, it is possible one address family is to be
  removed, but the other lives on.  In this case, the interface does
  not teardown but the specified address is purged, and all socket
  connections using address are reset.


@return
  int - NETMGR_SUCCESS if command to bring down interface is successfully
        issued, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_close( int link,
                  netmgr_address_set_t * addr_info_ptr,
                  boolean   teardown_iface )
{
  int rval = NETMGR_FAILURE;
  netmgr_kif_state_t state;

  NETMGR_LOG_FUNC_ENTRY;

  /* Verify the link id first */
  if (netmgr_kif_verify_link(link) == NETMGR_FAILURE) {
    netmgr_log_err("netmgr_kif_close called with invalid link %d\n", link);
    goto error;
  }

#ifdef FEATURE_DATA_IWLAN
  if (NETMGR_KIF_IS_REV_RMNET_LINK(link) && NULL != addr_info_ptr)
  {
    int assoc_link;
    int ret;
    int ip_family;

    ip_family = (NETMGR_IPV4_ADDR == addr_info_ptr->if_addr.type) ?
                AF_INET :
                AF_INET6;

    /* Attempt to remove the SA and routing rules for a reverse rmnet link */
    ret = netmgr_kif_remove_sa_and_routing_rules(link, ip_family, addr_info_ptr);

    /* If the interface is not being torn down or if an iWLAN iface is being brought down
       but it is associated with a forward rmnet, send the config complete. Otherwise, it
       will be sent when the interface transistions to the INIT state */
    if (FALSE == teardown_iface)
    {
      netmgr_log_med("netmgr_kif_close: link=%d is not being torn down, sending config complete\n",
                     link);

      netmgr_qmi_send_rev_ip_config_complete(NETMGR_QMI_IWLAN_CALL_CLEANUP,
                                             link,
                                             ip_family,
                                             ret);
    }
    else if (QMI_WDS_IFACE_NAME_IWLAN_EPDG == netmgr_qmi_wds_get_tech_name(link) &&
             NETMGR_LINK_MAX != (assoc_link = netmgr_qmi_iwlan_get_link_assoc(link)))
    {
      /* Purge address from network interface but state unchanged */
      if (addr_info_ptr)
      {
        netmgr_kif_purge_address(link,
                                 &addr_info_ptr->if_addr,
                                 addr_info_ptr->if_mask);
        addr_info_ptr->is_addr_purge_pend = FALSE;
      }

      netmgr_log_med("netmgr_kif_close: rev_link=%d is associated with fwd_link=%d, delaying teardown\n",
                     link,
                     assoc_link);

      netmgr_qmi_send_rev_ip_config_complete(NETMGR_QMI_IWLAN_CALL_CLEANUP,
                                             link,
                                             ip_family,
                                             ret);

      netmgr_qmi_reset_link_wds_data(link);

      return NETMGR_SUCCESS;
    }
    else
    {
      netmgr_log_med("netmgr_kif_close: saving ip_family=%d status=%d for link=%d\n",
                     ip_family,
                     ret,
                     link);

      if (NETMGR_SUCCESS != netmgr_qmi_save_rev_ip_config_status(link, ip_family, ret))
      {
        netmgr_log_err("netmgr_kif_close: failed to save SA removal status link=%d\n",
                       link);
      }
    }
  }
#endif /* FEATURE_DATA_IWLAN */

  /* Process based on current interface state */
  switch (state = netmgr_kif_get_state(link)) {
    case NETMGR_KIF_OPEN:
    case NETMGR_KIF_RECONFIGURING:
    case NETMGR_KIF_OPENING:
      if( teardown_iface )
      {
        netmgr_kif_close_req(link);
        netmgr_kif_set_state(link, NETMGR_KIF_CLOSING);
      }
      else
      {
        /* Purge address from network interface but state unchanged */
        if( addr_info_ptr )
        {
          netmgr_kif_purge_address( link,
                                    &addr_info_ptr->if_addr,
                                    addr_info_ptr->if_mask );
          addr_info_ptr->is_addr_purge_pend = FALSE;
        }
      }
      break;

    default:
      /* Ignore close request in all other states */
      netmgr_log_err("netmgr_kif_close called in state %d\n", state);
      goto error;
  }
  netmgr_kif_info[link].dns_v6_queried = FALSE;
  rval = NETMGR_SUCCESS;

error:
  return rval;
}

/*===========================================================================
  FUNCTION  netmgr_kif_netlink_init
===========================================================================*/
/*!
@brief
  Initialization routine for the KIF NetLink sockets interface.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_netlink_init( void )
{
  NETMGR_LOG_FUNC_ENTRY;

  memset(&netmgr_kif_sk_route, 0, sizeof(netmgr_kif_sk_route));
  memset(&netmgr_kif_sk_grp, 0, sizeof(netmgr_kif_sk_grp));
  memset(&netmgr_kif_sk_xfrm, 0, sizeof(netmgr_kif_sk_xfrm));

  if( NETMGR_SUCCESS !=
      netmgr_nl_listener_init( &netmgr_kif_sk_route.sk_thrd_info,
                               netmgr_kif_sk_route.sk_thrd_fdmap,
                               ds_arrsize(netmgr_kif_sk_route.sk_thrd_fdmap),
                               &netmgr_kif_sk_route.rt_sk,
                               NETLINK_ROUTE,
                               ( RTMGRP_LINK |
                                 RTMGRP_IPV4_IFADDR |
                                 RTMGRP_IPV6_IFADDR |
                                 RTNLGRP_IPV6_PREFIX ),
                               netmgr_kif_nl_recv_routing_msg ) )
  {
    NETMGR_ABORT("netmgr_kif_netlink_init: Error on netmgr_nl_init\n");
    return;
  }

  /* Open a netlink socket for NETLINK_GENERIC protocol. This socket
  ** is used to receive/generate netlink messaging related to NetMgr
  ** event indications for client multicast group */
  if(NETMGR_SUCCESS !=
     netmgr_nl_listener_init( &netmgr_kif_sk_grp.sk_thrd_info,
                               netmgr_kif_sk_grp.sk_thrd_fdmap,
                              ds_arrsize(netmgr_kif_sk_grp.sk_thrd_fdmap),
                              &netmgr_kif_sk_grp.ev_sk,
                              NETMGR_NL_TYPE,
                              NETMGR_NL_GRP_EVENTS,
                              netmgr_kif_nl_recv_ping_msg))
  {
    NETMGR_ABORT("netmgr_kif_netlink_init: Error on netmgr_nl_init for NL_TYPE GRP_EVENTS\n");
    return;
  }

#ifdef FEATURE_DATA_IWLAN
  /* Register for XFRM events */
  if (NETMGR_SUCCESS !=
      netmgr_nl_listener_init( &netmgr_kif_sk_xfrm.sk_thrd_info,
                               netmgr_kif_sk_xfrm.sk_thrd_fdmap,
                               ds_arrsize(netmgr_kif_sk_xfrm.sk_thrd_fdmap),
                               &netmgr_kif_sk_xfrm.xfrm_sk,
                               NETLINK_XFRM,
                               NETMGR_XFRM_GRP_EVENTS,
                               netmgr_kif_nl_recv_xfrm_msg))
  {
    netmgr_log_err("%s", "netmgr_kif_netlink_init: Error on netmgr_nl_init for XFRMNLGRP_AEVENTS");
  }
#endif /* FEATURE_DATA_IWLAN */

  NETMGR_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
  FUNCTION  netmgr_kif_init_iface
===========================================================================*/
/*!
@brief
  Initialize kernel interface state

@return
  int -  NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_init_iface( int link )
{
  short flags;
  int mode;

  /* Refresh the kernel ifindex as on some targets it can change */
  if( NETMGR_SUCCESS != netmgr_kif_init_link_ifindex( link ) )
  {
    netmgr_log_err("netmgr_kif_init: netmgr_kif_init_link_ifindex() failed"
                   " for %s\n", netmgr_kif_info[link].name);
    return NETMGR_FAILURE;
  }

  /* close the interface if open; this ensures we are in DOWN state
   * initially. */
  flags = 0;
  if (netmgr_kif_ifioctl_get_flags(netmgr_kif_info[link].name, &flags) < 0) {
    /* Could not get device. This is probably an interface that we
    ** don't care about. */
    netmgr_log_err("netmgr_kif_init: netmgr_kif_ifioctl_get_flags() "
                   "failed for %s, disable link\n",
                   netmgr_kif_info[link].name);
    return NETMGR_FAILURE;
  }

  if (flags & IFF_UP) {
    netmgr_log_high("kernel interface %d found open at init\n", link);
    netmgr_log_high("kernel interface %d will be closed at init\n", link);
    /* close iface */
    netmgr_kif_close_req(link);
    /* blow away iface config */
    netmgr_kif_clear_iface(link);
    /* mark the status */
    netmgr_kif_set_link_powerup_state(link, NETMGR_KIF_LINK_POWERUP_STATE_UP);
  }

#ifndef FEATURE_DS_LINUX_DRIVER_LEGACY
  /* Set driver link-layer protcol mode */
#ifdef FEATURE_DATA_IWLAN
  /* For reverse rmnet ports always set to IP mode */
  if (NETMGR_KIF_IS_REV_RMNET_LINK(link))
  {
    mode = RMNET_MODE_LLP_IP;
  }
  else
#endif /* FEATURE_DATA_IWLAN */
  {
    mode = (netmgr_main_cfg.runmode & NETMGR_MAIN_RUNMODE_ETHERNET)?
           RMNET_MODE_LLP_ETH : RMNET_MODE_LLP_IP;
  }

  if( NETMGR_SUCCESS !=
      netmgr_kif_ifioctl_set_llpmode( netmgr_kif_info[link].name,
                                      mode ) ) {
    netmgr_log_err("netmgr_kif_init: netmgr_kif_ifioctl_set_llpmode() failed"
                   " for %s\n", netmgr_kif_info[link].name);
    return NETMGR_FAILURE;
  }

  /* Set driver QOS header mode */
#ifdef FEATURE_DATA_IWLAN
  /* For reverse rmnet ports always disable QoS */
  if (NETMGR_KIF_IS_REV_RMNET_LINK(link))
  {
    mode = FALSE;
  }
  else
#endif /* FEATURE_DATA_IWLAN */
  {
    mode = (netmgr_main_cfg.runmode & NETMGR_MAIN_RUNMODE_QOSHDR)?
           TRUE : FALSE;
  }

  if( NETMGR_SUCCESS !=
      netmgr_kif_ifioctl_set_qosmode( netmgr_kif_info[link].name,
                                      mode ) ) {
    netmgr_log_err("netmgr_kif_init: netmgr_kif_ifioctl_set_llpmode() failed"
                   " for %s\n", netmgr_kif_info[link].name);
    return NETMGR_FAILURE;
  }
#endif /* FEATURE_DS_LINUX_DRIVER_LEGACY */

  /* Initialize interface state to closed */
  netmgr_kif_info[link].state = NETMGR_KIF_CLOSED;

  /* Initialize client callback struct ptr to null */
  netmgr_kif_info[link].clntcb = NULL;

  netmgr_kif_info[link].dns_v6_queried = FALSE;
  return NETMGR_SUCCESS;
}

/*===========================================================================
  FUNCTION  netmgr_kif_init_ifaces
===========================================================================*/
/*!
@brief
  Initialize kernel interface state

@return
  int -  NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_init_ifaces( void )
{
  int i;
  int ret = NETMGR_SUCCESS;

  /* Iterate over the array of interface info, initializing each one */
  for (i = 0; i < NETMGR_MAX_LINK; ++i) {

    /* skip if this interface is not used */
    if (netmgr_kif_cfg.link_array[i].enabled == FALSE)
    {
      netmgr_log_low( "ignoring link[%d]\n", i );
      continue;
    }

    netmgr_log_high( "initing KIF link[%d]\n", i );

    ret = netmgr_kif_init_iface(i);

    if (NETMGR_FAILURE == ret)
    {
      netmgr_log_err( "initing KIF link[%d] failed\n", i );
      netmgr_kif_cfg.link_array[i].enabled = FALSE;
    }
  }

  return ret;
}


/*===========================================================================
  FUNCTION  netmgr_kif_update_link_config
===========================================================================*/
/*!
@brief
  Update the link specific network interface configurations

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void
netmgr_kif_update_link_config(void)
{
  unsigned int i;
  char cmd[NETMGR_KIF_MAX_COMMAND_LENGTH] = "";

  for (i = 0; i < NETMGR_MAX_LINK; ++i)
  {
    const char *link_name = NULL;

    if (netmgr_kif_cfg.link_array[i].enabled)
    {
      if (NULL == (link_name = netmgr_kif_get_name(i)))
      {
        netmgr_log_err("failed to obtain link name for link=%d\n",
                       i);
        continue;
      }

      snprintf(cmd,
               sizeof(cmd),
               "echo 2 > /proc/sys/net/ipv6/conf/%s/accept_ra",
               link_name);

      if (ds_system_call(cmd, std_strlen(cmd)) != NETMGR_SUCCESS)
      {
        netmgr_log_err("cmd: %s failed\n", cmd);
      }
    }
  }
}

/*===========================================================================
  FUNCTION  netmgr_kif_reset_link
===========================================================================*/
/*!
@brief
  Reinitialize link data structures on reset command.

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise.

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_reset_link
(
  int                 link,
  netmgr_sm_events_t  evt
)
{
  netmgr_nl_event_info_t  event_info;
  unsigned int i;

  NETMGR_LOG_FUNC_ENTRY;

  /* Reset kernel interface state */
  if( NETMGR_SUCCESS != netmgr_kif_init_iface(link) ) {
    /* Ignore service initialization failure during SSR */
    if (NETMGR_MODEM_IS_EV == evt)
    {
      netmgr_log_high( "Ignoring KIF init failure during NETMGR_MODEM_IS_EV "
                       "on link[%d]\n", link );
      return NETMGR_FAILURE;
    }
    else
    {
      NETMGR_ABORT("netmgr_kif_reset_link: cannot init iface[%d]\n", link);
      netmgr_kif_cfg.link_array[link].enabled = FALSE;
      return NETMGR_FAILURE;
    }
  }

  /* Notify clients that state reset occurred */
  memset( &event_info, 0x0, sizeof(event_info) );
  event_info.link = link;
  event_info.event = NET_PLATFORM_RESET_EV;
  event_info.param_mask = NETMGR_EVT_PARAM_NONE;

  if( NETMGR_SUCCESS != netmgr_kif_send_event( &event_info ) ) {
    netmgr_log_err("failed on NET_PLATFORM_RESET_EV\n");
  }

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;
}

/*===========================================================================
  FUNCTION  netmgr_kif_cleanup
===========================================================================*/
/*!
@brief
  Performs cleanup of kif module.  Invoked at process termination.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void netmgr_kif_cleanup( void )
{
  int i=0;
  NETMGR_LOG_FUNC_ENTRY;

  for(i=0; i<NETMGR_MAX_LINK; i++)
  {
    (void)netmgr_kif_reset_link(i, NETMGR_INVALID_EV);
  }

  NETMGR_LOG_FUNC_EXIT;
}

#ifdef FEATURE_DATA_IWLAN
/*===========================================================================
  FUNCTION  netmgr_kif_enable_forwarding
===========================================================================*/
/*!
@brief
  Enable IP forwarding

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_enable_forwarding(int ip_family)
{
  unsigned int i, num_rules = 0;
  int rc = NETMGR_SUCCESS;
  const char **forwarding_rules = NULL;
  const char *ip4_forwarding_rules[] =
  {
    /* Enable IPv4 forwarding */
    "echo 1 > /proc/sys/net/ipv4/ip_forward"
  };
  const char *ip6_forwarding_rules[] =
  {
    /* Enable IPv6 forwarding */
    "echo 1 > /proc/sys/net/ipv6/conf/all/forwarding"
  };

  if (TRUE == NETMGR_KIF_GET_FORWARDING_ENABLED(ip_family))
  {
    netmgr_log_med("netmgr_kif_enable_forwarding: already enabled for family=%d!\n",
                   ip_family);
    goto ret;
  }

  if (AF_INET == ip_family)
  {
    forwarding_rules = ip4_forwarding_rules;
    num_rules = NETMGR_KIF_ARR_SIZE(ip4_forwarding_rules);
  }
  else
  {
    forwarding_rules = ip6_forwarding_rules;
    num_rules = NETMGR_KIF_ARR_SIZE(ip6_forwarding_rules);
  }

  for (i = 0; i < num_rules; ++i)
  {
    if (ds_system_call2(forwarding_rules[i], strlen(forwarding_rules[i]), function_debug))
    {
      netmgr_log_err("cmd: %s failed\n", forwarding_rules[i]);
      rc = NETMGR_FAILURE;
      break;
    }
  }

  if (NETMGR_SUCCESS == rc)
  {
    NETMGR_KIF_SET_FORWARDING_ENABLED(ip_family, TRUE);
    netmgr_log_med("netmgr_kif_enable_forwarding: enable complete for family=%d\n",
                   ip_family);
  }

ret:
  return rc;
}

/*===========================================================================
  FUNCTION  netmgr_kif_disable_forwarding
===========================================================================*/
/*!
@brief
  Disable IP forwarding

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_disable_forwarding(int ip_family)
{
  unsigned int i, num_rules = 0;
  int rc = NETMGR_SUCCESS;
  const char **forwarding_rules = NULL;
  const char *ip4_forwarding_rules[] =
  {
    /* Disable IPv4 forwarding */
    "echo 0 > /proc/sys/net/ipv4/ip_forward"
  };
  const char *ip6_forwarding_rules[] =
  {
    /* Disable IPv6 forwarding */
    "echo 0 > /proc/sys/net/ipv6/conf/all/forwarding"
  };

  if (FALSE == NETMGR_KIF_GET_FORWARDING_ENABLED(ip_family))
  {
    netmgr_log_med("netmgr_kif_disable_forwarding: already disabled for family=%d!\n",
                   ip_family);
    goto ret;
  }

  if (AF_INET == ip_family)
  {
    forwarding_rules = ip4_forwarding_rules;
    num_rules = NETMGR_KIF_ARR_SIZE(ip4_forwarding_rules);
  }
  else
  {
    forwarding_rules = ip6_forwarding_rules;
    num_rules = NETMGR_KIF_ARR_SIZE(ip6_forwarding_rules);
  }

  for (i = 0; i < num_rules; ++i)
  {
    if (ds_system_call2(forwarding_rules[i], strlen(forwarding_rules[i]), function_debug))
    {
      netmgr_log_err("cmd: %s failed\n", forwarding_rules[i]);
      rc = NETMGR_FAILURE;
      break;
    }
  }

  if (NETMGR_SUCCESS == rc)
  {
    NETMGR_KIF_SET_FORWARDING_ENABLED(ip_family, FALSE);
    netmgr_log_med("netmgr_kif_disable_forwarding: disable complete for family=%d\n",
                   ip_family);
  }

ret:
  return rc;
}

/*===========================================================================
  FUNCTION  netmgr_kif_install_policy_routing_rules
===========================================================================*/
/*!
@brief
  Install routing policy rules for forwarding Modem destined traffic

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_install_policy_routing_rules(int ip_family)
{
  unsigned int i, num_rules = 0;
  int rc = NETMGR_SUCCESS;
  const char **ip_rules = NULL;
  const char *ip4_rules[] =
  {
    /* IPv4 rules */
    "ip rule add from all lookup local prio 1",
    "ip rule del from all lookup local prio 0",
    "ip rule add fwmark " NETMGR_KIF_FWMARK " table " NETMGR_KIF_FWMARK " prio 0",
    "ip rule add from all lookup local prio 0",
    "ip rule del from all lookup local prio 1"
  };
  const char *ip6_rules[] =
  {
    /* IPv6 rules */
    "ip -6 rule add from all lookup local prio 1",
    "ip -6 rule del from all lookup local prio 0",
    "ip -6 rule add fwmark " NETMGR_KIF_FWMARK " table " NETMGR_KIF_FWMARK " prio 0",
    "ip -6 rule add from all lookup local prio 0",
    "ip -6 rule del from all lookup local prio 1"
  };

  if (TRUE == NETMGR_KIF_GET_POLICY_ROUTING_INSTALLED(ip_family))
  {
    netmgr_log_med("netmgr_kif_install_policy_routing_rules: already installed for family=%d!\n",
                   ip_family);
    goto ret;
  }

  if (AF_INET == ip_family)
  {
    ip_rules = ip4_rules;
    num_rules = NETMGR_KIF_ARR_SIZE(ip4_rules);
  }
  else
  {
    ip_rules = ip6_rules;
    num_rules = NETMGR_KIF_ARR_SIZE(ip6_rules);
  }

  for (i = 0; i < num_rules; ++i)
  {
    if (ds_system_call2(ip_rules[i], strlen(ip_rules[i]), function_debug))
    {
      netmgr_log_err("cmd: %s failed\n", ip_rules[i]);
      rc = NETMGR_FAILURE;
      break;
    }
  }

  if (NETMGR_SUCCESS == rc)
  {
    NETMGR_KIF_SET_POLICY_ROUTING_INSTALLED(ip_family, TRUE);
    netmgr_log_med("netmgr_kif_install_policy_routing_rules: installation complete for family=%d\n",
                   ip_family);
  }

ret:
  return rc;
}

/*===========================================================================
  FUNCTION  netmgr_kif_cleanup_policy_routing_rules
===========================================================================*/
/*!
@brief
  Cleanup (previously installed) custom routing policy rules

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_cleanup_policy_routing_rules(int ip_family)
{
  unsigned int i, num_rules = 0;
  int rc = NETMGR_SUCCESS;
  const char **ip_rules = NULL;
  const char *ip4_rules[] =
  {
    /* IPv4 rules */
    "ip rule del fwmark " NETMGR_KIF_FWMARK " table " NETMGR_KIF_FWMARK " prio 0"
  };
  const char *ip6_rules[] =
  {
    /* IPv6 rules */
    "ip -6 rule del fwmark " NETMGR_KIF_FWMARK " table " NETMGR_KIF_FWMARK " prio 0"
  };

  if (FALSE == NETMGR_KIF_GET_POLICY_ROUTING_INSTALLED(ip_family))
  {
    netmgr_log_med("netmgr_kif_cleanup_policy_routing_rules: already cleaned up for family=%d!\n",
                   ip_family);
    goto ret;
  }

  if (AF_INET == ip_family)
  {
    ip_rules = ip4_rules;
    num_rules = NETMGR_KIF_ARR_SIZE(ip4_rules);
  }
  else
  {
    ip_rules = ip6_rules;
    num_rules = NETMGR_KIF_ARR_SIZE(ip6_rules);
  }

  for (i = 0; i < NETMGR_KIF_ARR_SIZE(ip_rules); ++i)
  {
    if (ds_system_call2(ip_rules[i], strlen(ip_rules[i]), function_debug))
    {
      netmgr_log_err("cmd: %s failed\n", ip_rules[i]);
      rc = NETMGR_FAILURE;
      break;
    }
  }

  if (NETMGR_SUCCESS == rc)
  {
    NETMGR_KIF_SET_POLICY_ROUTING_INSTALLED(ip_family, FALSE);
    netmgr_log_med("netmgr_kif_cleanup_policy_routing_rules: clean-up complete for family=%d\n",
                   ip_family);
  }

ret:
  return rc;
}

/*===========================================================================
  FUNCTION  netmgr_kif_get_active_iwlan_calls
===========================================================================*/
/*!
@brief
  Returns the number of active iWLAN calls for the given family

@return
  The number of active iWLAN calls

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL unsigned int
netmgr_kif_get_active_iwlan_calls(int ip_family)
{
  int i;
  unsigned int active_calls = 0, addrset_mask = 0;

  addrset_mask = (AF_INET == ip_family) ? NETMGR_ADDRSET_MASK_IPV4 : NETMGR_ADDRSET_MASK_IPV6;

  for (i = 0; i < NETMGR_MAX_LINK; i++)
  {
    if (TRUE == netmgr_kif_cfg.link_array[i].enabled &&
        NETMGR_KIF_IS_REV_RMNET_LINK(i))
    {
      netmgr_address_info_t *addr_info = netmgr_qmi_get_addr_info(i);

      if (NULL != addr_info &&
          0 != (addr_info->valid_mask & addrset_mask))
      {
        ++active_calls;
      }
    }
  }

  return active_calls;
}

/*===========================================================================
  FUNCTION  netmgr_kif_update_config_and_routing_rules
===========================================================================*/
/*!
@brief
1. Insert the custom routing table at a higher priority than that of the local
   table
2. Enable the following kernel configs:
    accept_local - to allow packets coming from the modem with the same source
                   IP address as that of AP side rmnets
    ip_forward   - to enable AP to act as a router on modem's behalf
    ip_local_reserved_ports - to mark the modem port range as reserved on the AP side
    ip_local_port_range - to set the port range from which Apps on AP can be
                          allocated ephemeral ports

@return
  int - NETMGR_SUCCESS on successful operation, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_update_config_and_routing_rules(void)
{
  unsigned int i, rc = NETMGR_SUCCESS;
  char cmd[NETMGR_MAX_COMMAND_LENGTH] = "";

  const char *modem_port_rng =
    "echo \"" NETMGR_KIF_MODEM_PORT_START "-" NETMGR_KIF_MODEM_PORT_END "\" > /proc/sys/net/ipv4/ip_local_reserved_ports";

  const char *modem_ims_port_rng =
    "echo \"" NETMGR_KIF_MODEM_PORT_START "-" NETMGR_KIF_MODEM_PORT_END ","
              NETMGR_KIF_IMS_SIP_PORT ","
              NETMGR_KIF_IMS_PORT_START "-" NETMGR_KIF_IMS_PORT_END "\" > /proc/sys/net/ipv4/ip_local_reserved_ports";

  const char *resv_ports = NULL;

  const char *config_rules[] =
  {
    /* Allow RA in forwarding mode */
    "echo 2 > /proc/sys/net/ipv6/conf/all/accept_ra",

    /* Reserve Apps port range*/
    "echo \"" NETMGR_KIF_APPS_PORT_START " " NETMGR_KIF_APPS_PORT_END "\" > /proc/sys/net/ipv4/ip_local_port_range",
  };

  char xfrm_replay_thresh[NETMGR_MAX_STR_LENGTH] = "";
  char xfrm_timer_thresh[NETMGR_MAX_STR_LENGTH] = "";
  static char args[PROP_VALUE_MAX];
  char def[NETMGR_KIF_PROPERTY_REKEY_SIZE+1];
  int ret;
  unsigned long rekey_val;

  memset( args, 0x0, sizeof(args) );
  memset( def, 0x0, sizeof(def) );

  if (TRUE != netmgr_main_get_iwlan_enabled())
  {
    netmgr_kif_update_link_config();
    return NETMGR_SUCCESS;
  }

  snprintf( def, sizeof(def), "%u", UINT32_MAX );
  ret = property_get( NETMGR_KIF_PROPERTY_REKEY, args, def);

  if (NETMGR_KIF_PROPERTY_REKEY_SIZE < ret)
  {
    netmgr_log_err("System property %s has unexpected size(%d), setting default\n",
                   NETMGR_KIF_PROPERTY_REKEY, ret);
    rekey_val = UINT32_MAX;
  }
  else
  {
    rekey_val = strtoul(args, NULL, 10);
    if (UINT32_MAX < rekey_val)
    {
      netmgr_log_err("System property %s has exceeded limit, setting default");
      rekey_val = UINT32_MAX;
    }
  }

  /* Set the sequence number threshold */
  netmgr_log_med("Issuing command to set the xfrm replay threshold");
  snprintf(xfrm_replay_thresh, sizeof(xfrm_replay_thresh),
           "echo %lu > /proc/sys/net/core/xfrm_aevent_rseqth",
           rekey_val);

  if (ds_system_call2(xfrm_replay_thresh, strlen(xfrm_replay_thresh), function_debug))
  {
    netmgr_log_err("cmd: %s failed\n", xfrm_replay_thresh);
    rc = NETMGR_FAILURE;
  }

  /* xfrm_aevent_etime controls the delay between indications. By default the kernel
   * sends the indication 1 sec after the event is triggered. We want the indications
   * to be sent immediately and we set the value to zero */
  snprintf(xfrm_timer_thresh, sizeof(xfrm_timer_thresh),
           "echo %d > /proc/sys/net/core/xfrm_aevent_etime",
           0);

  if (ds_system_call2(xfrm_timer_thresh, strlen(xfrm_timer_thresh), function_debug))
  {
    netmgr_log_err("cmd: %s failed\n", xfrm_timer_thresh);
    rc = NETMGR_FAILURE;
  }

  resv_ports = (TRUE == netmgr_main_get_iwlan_ims_enabled()) ?
               modem_ims_port_rng :
               modem_port_rng;

  netmgr_log_low("issuing port reservation commands\n");
  if (ds_system_call2(resv_ports, strlen(resv_ports), function_debug))
  {
    netmgr_log_err("cmd: %s failed\n",resv_ports);
    rc = NETMGR_FAILURE;
  }

  netmgr_log_low("issuing config rule commands\n");
  for (i = 0; i < NETMGR_KIF_ARR_SIZE(config_rules); ++i)
  {
    if (ds_system_call2(config_rules[i], strlen(config_rules[i]), function_debug))
    {
      netmgr_log_err("cmd: %s failed\n",config_rules[i]);
      rc = NETMGR_FAILURE;
      break;
    }
  }

  netmgr_log_low("issuing command to accept local packets for reverse rmnets\n");
  for (i = 0; i < NETMGR_MAX_LINK; ++i)
  {
    const char *link_name = NULL;

    if (netmgr_kif_cfg.link_array[i].enabled)
    {
      if (NULL == (link_name = netmgr_kif_get_name(i)))
      {
        netmgr_log_err("failed to obtain link name for link=%d\n",
                       i);
        continue;
      }

      snprintf(cmd,
               sizeof(cmd),
               "echo 2 > /proc/sys/net/ipv6/conf/%s/accept_ra",
               link_name);

      if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
      {
        netmgr_log_err("cmd: %s failed, disabling link=%d\n",
                       cmd,
                       i);
        netmgr_kif_cfg.link_array[i].enabled = FALSE;
        continue;
      }

      if (NETMGR_KIF_IS_REV_RMNET_LINK(i))
      {
        snprintf(cmd,
                 sizeof(cmd),
                 "echo 1 > /proc/sys/net/ipv4/conf/%s/accept_local",
                 link_name);

        if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
        {
          netmgr_log_err("cmd: %s failed, disabling link=%d\n",
                         cmd,
                         i);
          netmgr_kif_cfg.link_array[i].enabled = FALSE;
        }

        /* Disable DAD on reverse Rmnets to speed up iface bring-up */
        snprintf(cmd,
                 sizeof(cmd),
                 "echo 0 > /proc/sys/net/ipv6/conf/%s/dad_transmits",
                 link_name);

        if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
        {
          netmgr_log_err("cmd: %s failed, disabling link=%d\n",
                         cmd,
                         i);
          netmgr_kif_cfg.link_array[i].enabled = FALSE;
        }
      }
    }
  }

  return rc;
}


/*===========================================================================
  FUNCTION  netmgr_kif_install_iptable_rules
===========================================================================*/
/*!
@brief
  Installs IPTables rule to mark packets that belong the modem source
  port range

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_install_iptable_rules(void)
{
  int rc = NETMGR_SUCCESS;
  unsigned int i;
  static boolean iptable_rules_installed = FALSE;

  /*
     Handling of modem destined fragmented UDP packets
     * Fragmented IPv4 UDP packets are automatically reassembled by the Linux connection
       tracking module before being forwarded.
     * Fragmented IPv6 UDP packets are not reassembled as routers (in this case AP) are
       not supposed to do so. In order to properly forward such packets we use connection
       marking mechanism via CONNMARK target.
       - Use the connection mark (if exists) on the packets belonging to that connection
       - Accept the packets which are marked (i.e. belong to an existing connection)
       - Mark the remaining traffic that is destined to modem  port range
       - Save the packet mark to the connection mark (for future traffic on the connection)
  */
  static const char *connmark_iptable_rules_begin[] =
  {
    /* Restore the connection mark to the packet if it exists */
    "ip6tables -t mangle -A PREROUTING -j CONNMARK --restore-mark",
    /* Accept all packets belonging to existing connections belonging to Modem traffic */
    "ip6tables -t mangle -A PREROUTING --match mark --mark " NETMGR_KIF_FWMARK
      " -j ACCEPT"
  };

  static const char *connmark_iptable_rules_end[] =
  {
    /* Save the packet mark to the connection mark */
    "ip6tables -t mangle -A PREROUTING -j CONNMARK --save-mark"
  };

  static const char *iptable_rules[] =
  {
    /* IPv4 iptable rules */

    /* All IKE in UDP packets must be sent to the Modem and ESP in UDP to the
       AP network stack for decryption and subsequent forwarding based on innner
       packet contents. This is how we can differentiate between the two:
       the first 4 bytes of the UDP payload of IKE in UDP is always 0 and
       non-zero for ESP in UDP packets (contains SPI) */

    /* Start at the IP header, extract IHL and add the offset to get to the
       start of UDP header and then offset 8 (UDP header) bytes to get to the
       payload. If the 4 bytes are 0 then mark the packet so that it gets
       forwarded to the Modem */
    "iptables -t mangle -A PREROUTING -p udp --sport " TO_XSTR(NETMGR_KIF_NATT_SERVER_PORT)
      " -m u32 --u32 \"0>>22 & 0x3C @ 8 = 0\" -j MARK --set-mark " NETMGR_KIF_FWMARK,

    /* Send the rest of the ESP in UDP packets originating from NATT server port
       up the networking stack */
    "iptables -t mangle -A PREROUTING -p udp --sport " TO_XSTR(NETMGR_KIF_NATT_SERVER_PORT)
      " -j ACCEPT",

    "iptables -t mangle -A PREROUTING -p tcp --dport " NETMGR_KIF_MODEM_PORT_START ":" NETMGR_KIF_MODEM_PORT_END
      " -j MARK --set-mark " NETMGR_KIF_FWMARK,
    "iptables -t mangle -A PREROUTING -p udp --dport " NETMGR_KIF_MODEM_PORT_START ":" NETMGR_KIF_MODEM_PORT_END
      " -j MARK --set-mark " NETMGR_KIF_FWMARK,

    /* Remove marking from iWLAN marked packets to prevent confusion with QoS marking */
    "iptables -t mangle -A POSTROUTING -m mark --mark " NETMGR_KIF_FWMARK
      " -j MARK --set-mark " NETMGR_KIF_DEFAULT_FLOW,

    /* IPv6 iptable rules */
    "ip6tables -t mangle -A PREROUTING -p tcp --dport " NETMGR_KIF_MODEM_PORT_START ":" NETMGR_KIF_MODEM_PORT_END
      " -j MARK --set-mark " NETMGR_KIF_FWMARK,
    "ip6tables -t mangle -A PREROUTING -p udp --dport " NETMGR_KIF_MODEM_PORT_START ":" NETMGR_KIF_MODEM_PORT_END
      " -j MARK --set-mark " NETMGR_KIF_FWMARK,

    /* Remove marking from iWLAN marked packets to prevent confusion with QoS marking */
    "ip6tables -t mangle -A POSTROUTING -m mark --mark " NETMGR_KIF_FWMARK
      " -j MARK --set-mark " NETMGR_KIF_DEFAULT_FLOW
  };

  static const char *ims_iptable_rules[] =
  {
    /* IPv4 iptable rules */
    "iptables -t mangle -A PREROUTING -p tcp --dport " NETMGR_KIF_IMS_PORT_START ":" NETMGR_KIF_IMS_PORT_END
      " -j MARK --set-mark " NETMGR_KIF_FWMARK,
    "iptables -t mangle -A PREROUTING -p tcp --dport " NETMGR_KIF_IMS_SIP_PORT
      " -j MARK --set-mark " NETMGR_KIF_FWMARK,
    "iptables -t mangle -A PREROUTING -p udp --dport " NETMGR_KIF_IMS_PORT_START ":" NETMGR_KIF_IMS_PORT_END
      " -j MARK --set-mark " NETMGR_KIF_FWMARK,
    "iptables -t mangle -A PREROUTING -p udp --dport " NETMGR_KIF_IMS_SIP_PORT
      " -j MARK --set-mark " NETMGR_KIF_FWMARK,
    /* Disable connection tracking on SIP port */
    "iptables -t raw -A PREROUTING -p tcp --match multiport --ports " NETMGR_KIF_IMS_SIP_PORT " -j NOTRACK",
    "iptables -t raw -A PREROUTING -p udp --match multiport --ports " NETMGR_KIF_IMS_SIP_PORT " -j NOTRACK",

    /* IPv6 iptable rules */
    "ip6tables -t mangle -A PREROUTING -p tcp --dport " NETMGR_KIF_IMS_PORT_START ":" NETMGR_KIF_IMS_PORT_END
      " -j MARK --set-mark " NETMGR_KIF_FWMARK,
    "ip6tables -t mangle -A PREROUTING -p tcp --dport " NETMGR_KIF_IMS_SIP_PORT
      " -j MARK --set-mark " NETMGR_KIF_FWMARK,
    "ip6tables -t mangle -A PREROUTING -p udp --dport " NETMGR_KIF_IMS_PORT_START ":" NETMGR_KIF_IMS_PORT_END
      " -j MARK --set-mark " NETMGR_KIF_FWMARK,
    "ip6tables -t mangle -A PREROUTING -p udp --dport " NETMGR_KIF_IMS_SIP_PORT
      " -j MARK --set-mark " NETMGR_KIF_FWMARK,
    /* Disable connection tracking on SIP port */
    "ip6tables -t raw -A PREROUTING -p tcp --match multiport --ports " NETMGR_KIF_IMS_SIP_PORT " -j NOTRACK",
    "ip6tables -t raw -A PREROUTING -p udp --match multiport --ports " NETMGR_KIF_IMS_SIP_PORT " -j NOTRACK"
  };

  NETMGR_LOG_FUNC_ENTRY;

  if (iptable_rules_installed)
  {
    netmgr_log_low("netmgr_kif_install_iptable_rules: iptable rules already installed\n");
    goto bail;
  }

  netmgr_log_low("netmgr_kif_install_iptable_rules: installing iptables marking rules\n");


  /* Connection marking iptable rules to install at the beginning */
  for (i = 0; i < NETMGR_KIF_ARR_SIZE(connmark_iptable_rules_begin); ++i)
  {
    if (ds_system_call2(connmark_iptable_rules_begin[i], strlen(connmark_iptable_rules_begin[i]), function_debug))
    {
      netmgr_log_err("netmgr_kif_install_iptable_rules: iptable cmd %s failed\n",
                     connmark_iptable_rules_begin[i]);
      rc = NETMGR_FAILURE;
      break;
    }
  }

  /* Install common iptable rules */
  for (i = 0; i < NETMGR_KIF_ARR_SIZE(iptable_rules); ++i)
  {
    if (ds_system_call2(iptable_rules[i], strlen(iptable_rules[i]), function_debug))
    {
      netmgr_log_err("netmgr_kif_install_iptable_rules: iptable cmd %s failed\n",
                     iptable_rules[i]);
      rc = NETMGR_FAILURE;
      break;
    }
  }

  if (TRUE == netmgr_main_get_iwlan_ims_enabled())
  {
    /* Install IMS iptable rules */
    for (i = 0; i < NETMGR_KIF_ARR_SIZE(ims_iptable_rules); ++i)
    {
      if (ds_system_call2(ims_iptable_rules[i], strlen(ims_iptable_rules[i]), function_debug))
      {
        netmgr_log_err("netmgr_kif_install_iptable_rules: iptable cmd %s failed\n",
                       ims_iptable_rules[i]);
        rc = NETMGR_FAILURE;
        break;
      }
    }
  }

  /* Connection marking iptable rules to install at the end */
  for (i = 0; i < NETMGR_KIF_ARR_SIZE(connmark_iptable_rules_end); ++i)
  {
    if (ds_system_call2(connmark_iptable_rules_end[i], strlen(connmark_iptable_rules_end[i]), function_debug))
    {
      netmgr_log_err("netmgr_kif_install_iptable_rules: iptable cmd %s failed\n",
                     connmark_iptable_rules_end[i]);
      rc = NETMGR_FAILURE;
      break;
    }
  }

  if (NETMGR_SUCCESS == rc)
  {
    iptable_rules_installed = TRUE;
  }

bail:
  NETMGR_LOG_FUNC_EXIT;
  return rc;
}


/*===========================================================================
  FUNCTION  netmgr_kif_install_iptable_rules_ex
===========================================================================*/
/*!
@brief
  This function installs rules to enable forwarding between
  rev_rmnetX interfaces and wlan

@param
  link - The link for which we need to install the rules

@return
  int  - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note
  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_install_iptable_rules_ex
(
  int link
)
{
  char cmd[NETMGR_MAX_COMMAND_LENGTH] = "";
  const char *link_name = NULL;
  int rc = NETMGR_FAILURE;

  NETMGR_LOG_FUNC_ENTRY;

  do
  {
    netmgr_log_low("netmgr_kif_install_iptable_rules_ex: Installing IP rules for link=%d",
                     link);

    if (NULL == (link_name = netmgr_kif_get_name(link)))
    {
      netmgr_log_err("netmgr_kif_install_forwarding_rules: failed to obtain link name for link=%d\n",
                     link);
      break;
    }

    /* Install forwarding rules between rev_rmnet and wlan0 as the first rules in the
     * FORWARD chain of filter table to override any packet drop rules */
    /* V4 rules */
    snprintf(cmd,
             sizeof(cmd),
             "iptables -t filter -I FORWARD -i %s -o wlan0 -j ACCEPT",
             link_name);

    if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
    {
      netmgr_log_err("netmgr_kif_install_iptable_rules_ex: ds_system_call() failed\n");
      break;
    }

    snprintf(cmd,
             sizeof(cmd),
             "iptables -t filter -I FORWARD -i wlan0 -o %s -j ACCEPT",
             link_name);

    if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
    {
      netmgr_log_err("netmgr_kif_install_iptable_rules_ex: ds_system_call() failed\n");
      break;
    }

    /* V6 rules */
    snprintf(cmd,
             sizeof(cmd),
             "ip6tables -t filter -I FORWARD -i %s -o wlan0 -j ACCEPT",
             link_name);

    if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
    {
      netmgr_log_err("netmgr_kif_install_iptable_rules_ex: ds_system_call() failed\n");
      break;
    }

    snprintf(cmd,
             sizeof(cmd),
             "ip6tables -t filter -I FORWARD -i wlan0 -o %s -j ACCEPT",
             link_name);

    if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
    {
      netmgr_log_err("netmgr_kif_install_iptable_rules_ex: ds_system_call() failed\n");
      break;
    }

    rc = NETMGR_SUCCESS;
  } while (0);

  return rc;
}


/*===========================================================================
  FUNCTION  netmgr_kif_remove_iptable_rules_ex
===========================================================================*/
/*!
@brief
  This function removes the rules to enable forwarding between
  rev_rmnetX interfaces and wlan

@param
  link - The link for which we need to install the rules

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note
  - Dependencies
    - None

  - Side Effects
   - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_remove_iptable_rules_ex
(
  int link
)
{
  char cmd[NETMGR_MAX_COMMAND_LENGTH] = "";
  const char *link_name = NULL;
  int rc = NETMGR_FAILURE;

  do
  {
    netmgr_log_low("netmgr_kif_install_iptable_rules_ex: Removing IP rules for link=%d",
                     link);

    if (NULL == (link_name = netmgr_kif_get_name(link)))
    {
      netmgr_log_err("netmgr_kif_install_forwarding_rules: failed to obtain link name for link=%d\n",
                     link);
      break;
    }

    /* Remove forwarding rules between rev_rmnet and wlan0 */
    /* V4 rules */
    snprintf(cmd,
             sizeof(cmd),
             "iptables -t filter -D FORWARD -i %s -o wlan0 -j ACCEPT",
             link_name);

    if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
    {
      netmgr_log_err("netmgr_kif_install_iptable_rules_ex: ds_system_call() failed\n");
      break;
    }

    snprintf(cmd,
             sizeof(cmd),
             "iptables -t filter -D FORWARD -i wlan0 -o %s -j ACCEPT",
             link_name);

    if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
    {
      netmgr_log_err("netmgr_kif_install_iptable_rules_ex: ds_system_call() failed\n");
      break;
    }

    /* V6 rules */
    snprintf(cmd,
             sizeof(cmd),
             "ip6tables -t filter -D FORWARD -i %s -o wlan0 -j ACCEPT",
             link_name);

    if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
    {
      netmgr_log_err("netmgr_kif_install_iptable_rules_ex: ds_system_call() failed\n");
      break;
    }

    snprintf(cmd,
             sizeof(cmd),
             "ip6tables -t filter -D FORWARD -i wlan0 -o %s -j ACCEPT",
             link_name);

    if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
    {
      netmgr_log_err("netmgr_kif_install_iptable_rules_ex: ds_system_call() failed\n");
      break;
    }

    rc = NETMGR_SUCCESS;
  } while (0);

  return rc;
}


/*===========================================================================
  FUNCTION  netmgr_kif_install_forwarding_rules
===========================================================================*/
/*!
@brief
  Installs the forwarding rules for the given reverse rmnet link to enable
  forwarding the downlink traffic to the modem

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_install_forwarding_rules
(
  int ip_family,
  int link
)
{
  char cmd[NETMGR_MAX_COMMAND_LENGTH] = "";
  char addr_buf[NETMGR_MAX_STR_LENGTH] = "";
  char *ip_type = NULL;
  netmgr_ip_address_t  *ip_addr = NULL;
  netmgr_address_info_t  *addr_info_ptr = netmgr_qmi_get_addr_info(link);
  const char *link_name = NULL;
  int rc = NETMGR_FAILURE;
  unsigned int prefix_len = 0;

  NETMGR_LOG_FUNC_ENTRY;

  if (!addr_info_ptr)
  {
    netmgr_log_err("netmgr_kif_install_forwarding_rules: invalid input\n");
    goto bail;
  }

  if (AF_INET == ip_family)
  {
    ip_type = "-4";
    ip_addr = &addr_info_ptr->ipv4.if_addr;
    prefix_len = ds_get_num_bits_set_count(addr_info_ptr->ipv4.if_mask);
  }
  else if (AF_INET6 == ip_family)
  {
    ip_type = "-6";
    ip_addr = &addr_info_ptr->ipv6.if_addr;
    prefix_len = addr_info_ptr->ipv6.if_mask;
  }
  else
  {
    netmgr_log_err("netmgr_kif_install_forwarding_rules: unknown ip_family=%d\n", ip_family);
    goto bail;
  }

  if (NETMGR_SUCCESS != netmgr_util_convert_ip_addr_to_str(ip_addr,
                                                           prefix_len,
                                                           addr_buf,
                                                           sizeof(addr_buf)))
  {
    netmgr_log_err("netmgr_kif_install_forwarding_rules: failed to convert IP addr to string\n");
    goto bail;
  }
  else if (NULL == (link_name = netmgr_kif_get_name(link)))
  {
    netmgr_log_err("netmgr_kif_install_forwarding_rules: failed to obtain link name for link=%d\n",
                   link);
    goto bail;
  }

  snprintf(cmd,
           sizeof(cmd),
           "ip %s route add table " NETMGR_KIF_FWMARK " %s dev %s",
           ip_type,
           addr_buf,
           link_name);

  netmgr_log_low("netmgr_kif_install_forwarding_rules: installing ip forwarding rule for link=%s type=%s\n",
                 link_name, ip_type);

  if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
  {
    netmgr_log_err("netmgr_kif_install_forwarding_rules: ds_system_call() failed\n");
    goto bail;
  }

  rc = NETMGR_SUCCESS;

bail:
  NETMGR_LOG_FUNC_EXIT;
  return rc;
}


/*===========================================================================
  FUNCTION  netmgr_kif_remove_forwarding_rules
===========================================================================*/
/*!
@brief
  Removes the forwarding rules for the given reverse rmnet link

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_remove_forwarding_rules
(
  int                   link,
  netmgr_address_set_t  *ip_addr
)
{
  char cmd[NETMGR_MAX_COMMAND_LENGTH] = "";
  char addr_buf[NETMGR_MAX_STR_LENGTH] = "";
  char *ip_type = NULL;
  const char *link_name = NULL;
  int rc = NETMGR_FAILURE;
  unsigned int prefix_len = 0;

  NETMGR_LOG_FUNC_ENTRY;

  if (!ip_addr)
  {
    netmgr_log_err("netmgr_kif_remove_forwarding_rules: invalid input\n");
    goto bail;
  }

  if (NETMGR_IPV4_ADDR == ip_addr->if_addr.type)
  {
    ip_type = "-4";
    prefix_len = ds_get_num_bits_set_count(ip_addr->if_mask);
  }
  else if (NETMGR_IPV6_ADDR == ip_addr->if_addr.type)
  {
    ip_type = "-6";
    prefix_len = ip_addr->if_mask;
  }
  else
  {
    netmgr_log_err("netmgr_kif_remove_forwarding_rules: unknown ip_family=%d\n",
                   ip_addr->if_addr.type);
    goto bail;
  }

  if (NETMGR_SUCCESS != netmgr_util_convert_ip_addr_to_str(&ip_addr->if_addr,
                                                           prefix_len,
                                                           addr_buf,
                                                           sizeof(addr_buf)))
  {
    netmgr_log_err("netmgr_kif_remove_forwarding_rules: failed to convert IP addr to string\n");
    goto bail;
  }
  else if (NULL == (link_name = netmgr_kif_get_name(link)))
  {
    netmgr_log_err("netmgr_kif_remove_forwarding_rules: failed to obtain link name for link=%d\n",
                   link);
    goto bail;
  }

  snprintf(cmd,
           sizeof(cmd),
           "ip %s route del table " NETMGR_KIF_FWMARK " %s dev %s",
           ip_type,addr_buf,link_name);

  netmgr_log_med("netmgr_kif_remove_forwarding_rules: removing ip forwarding rule for link=%s, type=%s\n",
                 link_name, ip_type);

  if (ds_system_call2(cmd, std_strlen(cmd), function_debug) != NETMGR_SUCCESS)
  {
    netmgr_log_err("netmgr_kif_remove_forwarding_rules: ds_system_call() failed\n");
    goto bail;
  }

  rc = NETMGR_SUCCESS;

bail:
  NETMGR_LOG_FUNC_EXIT;
  return rc;
}

/*===========================================================================
  FUNCTION  netmgr_kif_get_xfrm_policy_sel_str
===========================================================================*/
/*!
@brief
  Returns the IPSec policy selector string to be used with the ip xfrm policy
  command

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_get_xfrm_policy_sel_str
(
  netmgr_address_info_t  *addr_info_ptr,
  netmgr_ip_addr_t       ip_type,
  netmgr_ipsec_sa_dir_t  dir,
  char                   *policy_sel_buf,
  unsigned int           policy_sel_buf_len
)
{
  int rc = NETMGR_FAILURE;
  char iface_addr_buf[NETMGR_MAX_STR_LENGTH] = "";
  netmgr_ip_address_t  *ip_addr = NULL;
  unsigned int prefix_len;

  NETMGR_LOG_FUNC_ENTRY;

  if (!addr_info_ptr || !policy_sel_buf || 0 == policy_sel_buf_len)
  {
    netmgr_log_err("netmgr_kif_get_xfrm_policy_sel_str: invalid input\n");
    goto bail;
  }

  if (NETMGR_IPV4_ADDR == ip_type)
  {
    ip_addr = &addr_info_ptr->ipv4.if_addr;
    prefix_len = ds_get_num_bits_set_count(addr_info_ptr->ipv4.if_mask);
  }
  else if (NETMGR_IPV6_ADDR == ip_type)
  {
    ip_addr = &addr_info_ptr->ipv6.if_addr;
    prefix_len = addr_info_ptr->ipv6.if_mask;
  }
  else
  {
    netmgr_log_err("netmgr_kif_get_xfrm_policy_sel_str: unknown addr type=%d\n",
                   ip_type);
    goto bail;
  }

  if (NETMGR_SUCCESS != netmgr_util_convert_ip_addr_to_str(ip_addr,
                                                           prefix_len,
                                                           iface_addr_buf,
                                                           sizeof(iface_addr_buf)))
  {
    netmgr_log_err("netmgr_kif_get_xfrm_policy_sel_str: failed to convert iface addr to string\n");
    goto bail;
  }

  if (NETMGR_SA_DIR_TX == dir)
  {
    snprintf(policy_sel_buf,
             policy_sel_buf_len,
             "src %s",
             iface_addr_buf);
  }
  else
  {
    snprintf(policy_sel_buf,
             policy_sel_buf_len,
             "dst %s",
             iface_addr_buf);
  }

  netmgr_log_med("netmgr_kif_get_xfrm_policy_sel_str: policy_sel_buf=%s\n", policy_sel_buf);

  rc = NETMGR_SUCCESS;

bail:
  NETMGR_LOG_FUNC_EXIT;
  return rc;
}

/*===========================================================================
  FUNCTION  netmgr_kif_get_xfrm_state_encryption_str
===========================================================================*/
/*!
@brief
  Stores the IPSec encryption string to be used with the "ip xfrm state"
  command

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_get_xfrm_state_encryption_str
(
  qmi_wds_ipsec_sa_config_type  *sa_config,
  netmgr_ipsec_sa_dir_t         dir,
  char                          *enc_buf,
  unsigned int                  enc_buf_len
)
{
  int rc = NETMGR_FAILURE;
  const char *ealgo = NULL;
  char ekey_str[NETMGR_MAX_STR_LENGTH] = "";
  qmi_wds_ipsec_key_type  *ekey = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  if (!sa_config || !enc_buf || 0 == enc_buf_len)
  {
    netmgr_log_err("netmgr_kif_get_xfrm_state_encryption_str: invalid input\n");
    goto bail;
  }

  if (!(sa_config->param_mask & QMI_WDS_IPSEC_CRYPTO_ALGO_PARAM_MASK))
  {
    netmgr_log_err("netmgr_kif_get_xfrm_state_encryption_str: crypto mask not set\n");
    goto bail;
  }

  if ((NETMGR_SA_DIR_TX == dir &&
       !(sa_config->param_mask & QMI_WDS_IPSEC_CRYPTO_KEY_TX_PARAM_MASK)) ||
      (NETMGR_SA_DIR_RX == dir &&
       !(sa_config->param_mask & QMI_WDS_IPSEC_CRYPTO_KEY_RX_PARAM_MASK)))
  {
    netmgr_log_err("netmgr_kif_install_sa_rules: crypto key not present in dir=%d", dir);
    goto bail;
  }

  if (!(ealgo = netmgr_util_get_ipsec_algo_str(NETMGR_IPSEC_ALGO_CRYPTO,
                                               sa_config->crypto_algo)))
  {
    netmgr_log_err("netmgr_kif_get_xfrm_state_encryption_str: failed to convert crypto algo=%d to string",
                   sa_config->crypto_algo);
    goto bail;
  }

  ekey = (NETMGR_SA_DIR_TX == dir) ? &sa_config->crypto_key_tx : &sa_config->crypto_key_rx;

  if (NETMGR_SUCCESS != netmgr_util_convert_qmi_ipsec_key_to_str(ekey,
                                                                 ekey_str,
                                                                 sizeof(ekey_str)))
  {
    netmgr_log_err("netmgr_kif_get_xfrm_state_encryption_str: failed to convert crypto key to string");
    goto bail;
  }

  snprintf(enc_buf,
           enc_buf_len,
           "enc %s %s",
           ealgo,
           ekey_str);

  rc = NETMGR_SUCCESS;

bail:
  NETMGR_LOG_FUNC_EXIT;
  return rc;
}

/*===========================================================================
  FUNCTION  netmgr_kif_get_xfrm_state_authentication_str
===========================================================================*/
/*!
@brief
  Stores the IPSec authentication string to be used with the "ip xfrm state"
  command

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_get_xfrm_state_authentication_str
(
  qmi_wds_ipsec_sa_config_type  *sa_config,
  netmgr_ipsec_sa_dir_t         dir,
  char                          *auth_buf,
  unsigned int                  auth_buf_len
)
{
  int rc = NETMGR_FAILURE;
  const char *halgo = NULL;
  char hkey_str[NETMGR_MAX_STR_LENGTH] = "";
  qmi_wds_ipsec_key_type  *hkey = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  if (!sa_config || !auth_buf || 0 == auth_buf_len)
  {
    netmgr_log_err("netmgr_kif_get_xfrm_state_authentication_str: invalid input");
    goto bail;
  }

  if (!(sa_config->param_mask & QMI_WDS_IPSEC_HASH_ALGO_PARAM_MASK))
  {
    netmgr_log_err("netmgr_kif_get_xfrm_state_authentication_str: hash mask not set");
    goto bail;
  }

  if ((NETMGR_SA_DIR_TX == dir &&
       !(sa_config->param_mask & QMI_WDS_IPSEC_HASH_KEY_TX_PARAM_MASK)) ||
      (NETMGR_SA_DIR_RX == dir &&
       !(sa_config->param_mask & QMI_WDS_IPSEC_HASH_KEY_RX_PARAM_MASK)))
  {
    netmgr_log_err("netmgr_kif_install_sa_rules: hash key not present in dir=%d", dir);
    goto bail;
  }

  if (!(halgo = netmgr_util_get_ipsec_algo_str(NETMGR_IPSEC_ALGO_HASH,
                                               sa_config->hash_algo)))
  {
    netmgr_log_err("netmgr_kif_get_xfrm_state_authentication_str: failed to convert hash algo=%d to string",
                   sa_config->hash_algo);
    goto bail;
  }

  hkey = (NETMGR_SA_DIR_TX == dir) ? &sa_config->hash_key_tx : &sa_config->hash_key_rx;

  if (NETMGR_SUCCESS != netmgr_util_convert_qmi_ipsec_key_to_str(hkey,
                                                                 hkey_str,
                                                                 sizeof(hkey_str)))
  {
    netmgr_log_err("netmgr_kif_get_xfrm_state_authentication_str: failed to convert hash key to string");
    goto bail;
  }

  snprintf(auth_buf,
           auth_buf_len,
           "auth %s %s",
           halgo,
           hkey_str);
  rc = NETMGR_SUCCESS;

bail:
  NETMGR_LOG_FUNC_EXIT;
  return rc;
}

/*===========================================================================
  FUNCTION  netmgr_kif_get_xfrm_state_encap_str
===========================================================================*/
/*!
@brief
  Stores the IPSec encapsulation string to be used with the "ip xfrm state"
  command

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_get_xfrm_state_encap_str
(
  qmi_wds_ipsec_sa_config_type  *sa_config,
  netmgr_ipsec_sa_dir_t         dir,
  char                          *encap_buf,
  unsigned int                  encap_buf_len
)
{
  int rc = NETMGR_FAILURE;

  NETMGR_LOG_FUNC_ENTRY;

  if (!sa_config || !encap_buf || 0 == encap_buf_len)
  {
    netmgr_log_err("netmgr_kif_get_xfrm_state_encap_str: invalid input");
    goto bail;
  }

  if ((sa_config->param_mask & QMI_WDS_IPSEC_UDP_ENCAP_PARAM_MASK) &&
      TRUE == sa_config->is_udp_encap)
  {
    /* For NATT to work, we need to have at least one UDP socket open on the
       Apps side which is bound to the NATT Modem port along with the ESP in UDP
       socket option set. */
    /* Set the UDP_ENCAP socket option */
    if (natt_fd < 0)
    {
      int type = UDP_ENCAP_ESPINUDP;
      struct sockaddr_in saddr;

      netmgr_log_med("netmgr_kif_get_xfrm_state_encap_str: setting ESPINUDP sockopt");

      /* Open a datagram socket to use for issuing the ioctl */
      if ((natt_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        netmgr_log_sys_err("natt: socket failed:\n");
        goto bail;
      }

      if (setsockopt(natt_fd, SOL_UDP, UDP_ENCAP, &type, sizeof(type)) < 0)
      {
        netmgr_log_sys_err("natt: failed to set ESPINUDP option:\n");
        goto bail;
      }

      memset((char *) &saddr, 0, sizeof(saddr));
      saddr.sin_family = AF_INET;
      saddr.sin_port = htons(NETMGR_KIF_NATT_MODEM_PORT);
      saddr.sin_addr.s_addr = htonl(INADDR_ANY);

      if (bind(natt_fd, (const struct sockaddr *)&saddr, sizeof(saddr)) < 0)
      {
        netmgr_log_sys_err("natt: failed to bind to port\n");
        goto bail;
      }
    }

    snprintf(encap_buf,
             encap_buf_len,
             "encap espinudp %s %s 0.0.0.0",
             (dir == NETMGR_SA_DIR_TX) ? TO_XSTR(NETMGR_KIF_NATT_MODEM_PORT) : TO_XSTR(NETMGR_KIF_NATT_SERVER_PORT),
             (dir == NETMGR_SA_DIR_TX) ? TO_XSTR(NETMGR_KIF_NATT_SERVER_PORT) : TO_XSTR(NETMGR_KIF_NATT_MODEM_PORT));
  }

  rc = NETMGR_SUCCESS;

bail:
  if (NETMGR_SUCCESS != rc)
  {
    if (natt_fd >= 0)
      close(natt_fd);
    natt_fd = -1;
  }

  NETMGR_LOG_FUNC_EXIT;
  return rc;
}

#define NETMGR_KIF_UTIL_INSTALL_IPSEC_XFRM_STATE(DIR,mode,state_id,enc,auth,encap)     \
do                                                                                     \
{                                                                                      \
  if (*state_id[NETMGR_SA_DIR_##DIR] != '\0')                                          \
  {                                                                                    \
    char xfrm_state_cmd[NETMGR_MAX_COMMAND_LENGTH] = "";                               \
    snprintf(xfrm_state_cmd,                                                           \
             sizeof(xfrm_state_cmd),                                                   \
             "ip xfrm state add %s mode %s %s %s flag af-unspec %s",                   \
             state_id[NETMGR_SA_DIR_##DIR],                                            \
             mode,                                                                     \
             enc,                                                                      \
             auth,                                                                     \
             encap[NETMGR_SA_DIR_##DIR]);                                              \
                                                                                       \
    if (ds_system_call2(xfrm_state_cmd,                                                \
                        std_strlen(xfrm_state_cmd),                                    \
                        function_debug) != NETMGR_SUCCESS)                             \
    {                                                                                  \
      netmgr_log_err("ip xfrm state cmd failed\n");                                    \
      goto bail;                                                                       \
    }                                                                                  \
  }                                                                                    \
  else                                                                                 \
  {                                                                                    \
    netmgr_log_err("ip xfrm state cmd failed, invalid state\n");                       \
    goto bail;                                                                         \
  }                                                                                    \
}                                                                                      \
while (0)

#define NETMGR_KIF_UTIL_INSTALL_IPSEC_XFRM_POLICY(DIR,ipsec_dir,mode,state_id,policy_sel) \
do                                                                                        \
{                                                                                         \
  if (*policy_sel[NETMGR_SA_DIR_##DIR] != '\0' && *state_id[NETMGR_SA_DIR_##DIR] != '\0') \
  {                                                                                       \
    char xfrm_policy_cmd[NETMGR_MAX_COMMAND_LENGTH] = "";                                 \
    snprintf(xfrm_policy_cmd,                                                             \
             sizeof(xfrm_policy_cmd),                                                     \
             "ip xfrm policy add dir "#ipsec_dir" %s tmpl %s mode %s",                    \
             policy_sel[NETMGR_SA_DIR_##DIR],                                             \
             state_id[NETMGR_SA_DIR_##DIR],                                               \
             mode);                                                                       \
                                                                                          \
    if (ds_system_call2(xfrm_policy_cmd,                                                  \
                        std_strlen(xfrm_policy_cmd),                                      \
                        function_debug) != NETMGR_SUCCESS)                                \
    {                                                                                     \
      netmgr_log_err("ip xfrm policy cmd failed");                                        \
      goto bail;                                                                          \
    }                                                                                     \
  }                                                                                       \
  else                                                                                    \
  {                                                                                       \
    netmgr_log_err("ip xfrm policy cmd failed, invalid policy/state\n");                  \
    goto bail;                                                                            \
  }                                                                                       \
}                                                                                         \
while (0)

/*===========================================================================
  FUNCTION  netmgr_kif_install_sa_rules
===========================================================================*/
/*!
@brief
  Installs the SAs to the SAD (Security Association Database) and
  SPD (Security Policy Database) for the given family and link

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_install_sa_rules
(
  int link,
  int ip_family
)
{
  char dest_addr_buf[NETMGR_MAX_STR_LENGTH]  = "";
  char local_addr_buf[NETMGR_MAX_STR_LENGTH] = "";
  char encryption[NETMGR_MAX_STR_LENGTH]     = "";
  char authentication[NETMGR_MAX_STR_LENGTH] = "";
  char encap[NETMGR_SA_DIR_MAX][NETMGR_MAX_STR_LENGTH] = {"",""};
  const char *proto = NULL;
  const char *mode  = NULL;
  netmgr_ipsec_sa_t  *sa = NULL;
  netmgr_ip_addr_t   ip_type;
  netmgr_address_info_t  *addr_info_ptr = netmgr_qmi_get_addr_info(link);
  int rc = NETMGR_FAILURE;
  unsigned int prefix_len = 0;
  qmi_wds_ipsec_sa_config_type  sa_config;

  NETMGR_LOG_FUNC_ENTRY;

  if (!addr_info_ptr)
  {
    netmgr_log_err("netmgr_kif_install_sa_rules: invalid input\n");
    goto bail;
  }

  if (AF_INET != ip_family && AF_INET6 != ip_family)
  {
    netmgr_log_err("netmgr_kif_install_sa_rules: invalid ip_famliy=%d\n",
                   ip_family);
    goto bail;
  }

  if (NETMGR_SUCCESS != netmgr_qmi_query_ipsec_sa_config(ip_family, link, &sa_config))
  {
    netmgr_log_err("netmgr_kif_install_sa_rules: failed to query SAs for ip_family=%d, link=%d",
                   ip_family,
                   link);
    goto bail;
  }

  /* Make sure that the basic parameters are present */
  if (!(sa_config.param_mask & QMI_WDS_IPSEC_SA_PROTO_PARAM_MASK)       ||
      !(sa_config.param_mask & QMI_WDS_IPSEC_ENCAP_MODE_PARAM_MASK)     ||
      !(sa_config.param_mask & QMI_WDS_IPSEC_DEST_ADDR_PARAM_MASK)      ||
      !(sa_config.param_mask & QMI_WDS_IPSEC_LOCAL_ADDR_PARAM_MASK)     ||
      (!(sa_config.param_mask & QMI_WDS_IPSEC_SPI_RX_PARAM_MASK) &&
       !(sa_config.param_mask & QMI_WDS_IPSEC_SPI_TX_PARAM_MASK))       ||
      (!(sa_config.param_mask & QMI_WDS_IPSEC_HASH_ALGO_PARAM_MASK) &&
       !(sa_config.param_mask & QMI_WDS_IPSEC_CRYPTO_ALGO_PARAM_MASK)))
  {
    netmgr_log_err("netmgr_kif_install_sa_rules: SA missing key parameters");
    goto bail;
  }

  if (AF_INET == ip_family)
  {
    ip_type = NETMGR_IPV4_ADDR;
    sa = &addr_info_ptr->ipv4.sa;
  }
  else
  {
    ip_type = NETMGR_IPV6_ADDR;
    sa = &addr_info_ptr->ipv6.sa;
  }

  if (NETMGR_SUCCESS != netmgr_util_convert_qmi_ip_addr_to_str(&sa_config.dest_addr,
                                                               dest_addr_buf,
                                                               sizeof(dest_addr_buf)))
  {
    netmgr_log_err("netmgr_kif_install_sa_rules: failed to convert dest IP addr to string");
    goto bail;
  }

  netmgr_log_med("netmgr_kif_install_sa_rules: dest_addr=%s", dest_addr_buf);

  if (NETMGR_SUCCESS != netmgr_util_convert_qmi_ip_addr_to_str(&sa_config.local_addr,
                                                               local_addr_buf,
                                                               sizeof(local_addr_buf)))
  {
    netmgr_log_err("netmgr_kif_install_sa_rules: failed to convert local IP addr to string");
    goto bail;
  }

  netmgr_log_med("netmgr_kif_install_sa_rules: local_addr=%s", local_addr_buf);

  /* Get the protocol string */
  if (!(proto = netmgr_util_get_ipsec_proto_str(sa_config.proto)))
  {
    netmgr_log_err("netmgr_kif_install_sa_rules: failed to get proto string");
    goto bail;
  }

  /* Get the encapsulation mode string */
  if (!(mode = netmgr_util_get_ipsec_mode_str(sa_config.encap_mode)))
  {
    netmgr_log_err("netmgr_kif_install_sa_rules: failed to get mode string");
    goto bail;
  }

  /* Get the ESP in UDP encapsulation string (if any) in TX and RX directions */
  if (NETMGR_SUCCESS != netmgr_kif_get_xfrm_state_encap_str(&sa_config,
                                                            NETMGR_SA_DIR_TX,
                                                            encap[NETMGR_SA_DIR_TX],
                                                            sizeof(encap[NETMGR_SA_DIR_TX])))
  {
    netmgr_log_err("netmgr_kif_install_sa_rules: failed to get encap string in TX dir");
    goto bail;
  }

  if (NETMGR_SUCCESS != netmgr_kif_get_xfrm_state_encap_str(&sa_config,
                                                            NETMGR_SA_DIR_RX,
                                                            encap[NETMGR_SA_DIR_RX],
                                                            sizeof(encap[NETMGR_SA_DIR_RX])))
  {
    netmgr_log_err("netmgr_kif_install_sa_rules: failed to get encap string in RX dir");
    goto bail;
  }

  /* Save the encapsulation mode */
  sa->mode = mode;

  /* Install the SA in the 'out' direction */
  if (sa_config.param_mask & QMI_WDS_IPSEC_SPI_TX_PARAM_MASK)
  {
    netmgr_ipsec_sa_dir_t sa_dir = NETMGR_SA_DIR_TX;

    snprintf(sa->sa_state_id[sa_dir],
             NETMGR_MAX_STR_LENGTH,
             "src %s dst %s proto %s spi 0x%lx",
             local_addr_buf,
             dest_addr_buf,
             proto,
             sa_config.spi_tx);

    /* Save the SPI value. This would be used to compare against
     * the SPI value returned by the kernel xfrm netlink message
     * when sequence number rolls over */
    if (AF_INET == ip_family)
    {
      addr_info_ptr->ipv4.sa.esp_spi_v4 = sa_config.spi_tx;
    }
    else
    {
      addr_info_ptr->ipv6.sa.esp_spi_v6 = sa_config.spi_tx;
    }

    if (NETMGR_SUCCESS != netmgr_kif_get_xfrm_policy_sel_str(addr_info_ptr,
                                                             ip_type,
                                                             sa_dir,
                                                             sa->sa_policy_sel[sa_dir],
                                                             NETMGR_MAX_STR_LENGTH))
    {
      netmgr_log_err("netmgr_kif_install_sa_rules: failed to get xfrm policy sel string");
      goto bail;
    }

    if (NETMGR_SUCCESS != netmgr_kif_get_xfrm_state_encryption_str(&sa_config,
                                                                   sa_dir,
                                                                   encryption,
                                                                   sizeof(encryption)))
    {
      netmgr_log_err("netmgr_kif_install_sa_rules: failed to get auth string");
      goto bail;
    }

    if (NETMGR_SUCCESS != netmgr_kif_get_xfrm_state_authentication_str(&sa_config,
                                                                       sa_dir,
                                                                       authentication,
                                                                       sizeof(authentication)))
    {
      netmgr_log_err("netmgr_kif_install_sa_rules: failed to get auth string");
      goto bail;
    }

    /* Install the 'state' and 'policy' in the "out" direction */
    NETMGR_KIF_UTIL_INSTALL_IPSEC_XFRM_STATE(TX,mode,sa->sa_state_id,encryption,authentication,encap);
    NETMGR_KIF_UTIL_INSTALL_IPSEC_XFRM_POLICY(TX,out,mode,sa->sa_state_id,sa->sa_policy_sel);
  }

  /* Install the SA in the 'in' direction */
  if (sa_config.param_mask & QMI_WDS_IPSEC_SPI_RX_PARAM_MASK)
  {
    netmgr_ipsec_sa_dir_t sa_dir = NETMGR_SA_DIR_RX;

    snprintf(sa->sa_state_id[sa_dir],
             NETMGR_MAX_STR_LENGTH,
             "src %s dst %s proto %s spi 0x%lx",
             dest_addr_buf,
             local_addr_buf,
             proto,
             sa_config.spi_rx);

    if (NETMGR_SUCCESS != netmgr_kif_get_xfrm_policy_sel_str(addr_info_ptr,
                                                             ip_type,
                                                             sa_dir,
                                                             sa->sa_policy_sel[sa_dir],
                                                             NETMGR_MAX_STR_LENGTH))
    {
      netmgr_log_err("netmgr_kif_install_sa_rules: failed to get xfrm policy sel string");
      goto bail;
    }

    if (NETMGR_SUCCESS != netmgr_kif_get_xfrm_state_encryption_str(&sa_config,
                                                                   sa_dir,
                                                                   encryption,
                                                                   sizeof(encryption)))
    {
      netmgr_log_err("netmgr_kif_install_sa_rules: failed to get encryption string");
      goto bail;
    }

    if (NETMGR_SUCCESS != netmgr_kif_get_xfrm_state_authentication_str(&sa_config,
                                                                       sa_dir,
                                                                       authentication,
                                                                       sizeof(authentication)))
    {
      netmgr_log_err("netmgr_kif_install_sa_rules: failed to get auth string");
      goto bail;
    }

    /* Install the 'state' and 'policy' in the "in" direction and also allow forwarding */
    NETMGR_KIF_UTIL_INSTALL_IPSEC_XFRM_STATE(RX,mode,sa->sa_state_id,encryption,authentication,encap);
    NETMGR_KIF_UTIL_INSTALL_IPSEC_XFRM_POLICY(RX,in,mode,sa->sa_state_id,sa->sa_policy_sel);
    NETMGR_KIF_UTIL_INSTALL_IPSEC_XFRM_POLICY(RX,fwd,mode,sa->sa_state_id,sa->sa_policy_sel);
  }

  sa->is_sa_valid = TRUE;

  rc = NETMGR_SUCCESS;

bail:
  NETMGR_LOG_FUNC_EXIT;
  return rc;
}


/*===========================================================================
  FUNCTION  netmgr_kif_remove_sa_rules
===========================================================================*/
/*!
@brief
  Removes the SAs from the SAD (Security Association Database) and
  SPD (Security Policy Database) for the given link and family

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int
netmgr_kif_remove_sa_rules
(
  int link,
  int ip_family
)
{
  char xfrm_state_cmd[NETMGR_MAX_COMMAND_LENGTH]  = "";
  char xfrm_policy_cmd[NETMGR_MAX_COMMAND_LENGTH] = "";
  netmgr_ip_address_t  *ip_addr = NULL;
  netmgr_address_info_t  *addr_info_ptr = netmgr_qmi_get_addr_info(link);
  int rc = NETMGR_FAILURE;
  char (*sa_state_id)[NETMGR_MAX_STR_LENGTH]   = NULL;
  char (*sa_policy_sel)[NETMGR_MAX_STR_LENGTH] = NULL;
  netmgr_ipsec_sa_t  *sa = NULL;
  netmgr_ipsec_sa_t  *v4_sa = NULL;
  netmgr_ipsec_sa_t  *v6_sa = NULL;
  boolean remove_sa = TRUE;

  NETMGR_LOG_FUNC_ENTRY;

  if (!addr_info_ptr)
  {
    netmgr_log_err("netmgr_kif_remove_sa_rules: invalid input");
    goto bail;
  }

  v4_sa = &addr_info_ptr->ipv4.sa;
  v6_sa = &addr_info_ptr->ipv6.sa;

  if (AF_INET == ip_family)
  {
    sa = v4_sa;
    ip_addr = &addr_info_ptr->ipv4.if_addr;
    sa_state_id   = sa->sa_state_id;
    sa_policy_sel = sa->sa_policy_sel;

    /* If the SA is shared and the other family is still UP, don't remove the SA */
    if (sa->is_sa_shared && v6_sa->is_sa_valid)
    {
      remove_sa = FALSE;
    }
  }
  else
  {
    sa = v6_sa;
    ip_addr = &addr_info_ptr->ipv6.if_addr;
    sa_state_id   = sa->sa_state_id;
    sa_policy_sel = sa->sa_policy_sel;

    /* If the SA is shared and the other family is still UP, don't remove the SA */
    if (sa->is_sa_shared && v4_sa->is_sa_valid)
    {
      netmgr_log_med("netmgr_kif_remove_sa_rules: SA shared, not removing SA state link=%d, family=%d",
                     link, ip_family);
      remove_sa = FALSE;
    }
  }

  if (TRUE != sa->is_sa_valid)
  {
    netmgr_log_err("netmgr_kif_remove_sa_rules: SA is invalid for ip_family=%d, link=%d",
                   ip_family,
                   link);
    goto bail;
  }

  if (TRUE == remove_sa && *sa_state_id[NETMGR_SA_DIR_TX] != '\0')
  {
    snprintf(xfrm_state_cmd,
             sizeof(xfrm_state_cmd),
             "ip xfrm state deleteall %s",
             sa_state_id[NETMGR_SA_DIR_TX]);

    netmgr_log_med("netmgr_kif_remove_sa_rules: removing TX SA state for link=%d, family=%d",
                   link, ip_family);

    if (ds_system_call2(xfrm_state_cmd, std_strlen(xfrm_state_cmd), function_debug) != NETMGR_SUCCESS)
    {
      netmgr_log_err("netmgr_kif_remove_sa_rules: xfrm_state_cmd failed");
      goto bail;
    }

    *sa_state_id[NETMGR_SA_DIR_TX] = '\0';
  }
  else
  {
    *sa_state_id[NETMGR_SA_DIR_TX] = '\0';
  }

  if (*sa_policy_sel[NETMGR_SA_DIR_TX] != '\0')
  {
    snprintf(xfrm_policy_cmd,
             sizeof(xfrm_policy_cmd),
             "ip xfrm policy deleteall dir out %s",
             sa_policy_sel[NETMGR_SA_DIR_TX]);

    netmgr_log_med("netmgr_kif_remove_sa_rules: removing TX SA policy for link=%d, family=%d",
                   link, ip_family);

    if (ds_system_call2(xfrm_policy_cmd, std_strlen(xfrm_policy_cmd), function_debug) != NETMGR_SUCCESS)
    {
      netmgr_log_err("netmgr_kif_remove_sa_rules: xfrm_policy_cmd failed");
      goto bail;
    }

    *sa_policy_sel[NETMGR_SA_DIR_TX] = '\0';
  }

  if (TRUE == remove_sa && *sa_state_id[NETMGR_SA_DIR_RX] != '\0')
  {
    snprintf(xfrm_state_cmd,
             sizeof(xfrm_state_cmd),
             "ip xfrm state deleteall %s",
             sa_state_id[NETMGR_SA_DIR_RX]);

    netmgr_log_med("netmgr_kif_remove_sa_rules: removing RX SA state for link=%d, family=%d",
                   link, ip_family);

    if (ds_system_call2(xfrm_state_cmd, std_strlen(xfrm_state_cmd), function_debug) != NETMGR_SUCCESS)
    {
      netmgr_log_err("netmgr_kif_remove_sa_rules: xfrm_state_cmd failed");
      goto bail;
    }

    *sa_state_id[NETMGR_SA_DIR_RX] = '\0';
  }
  else
  {
    *sa_state_id[NETMGR_SA_DIR_RX] = '\0';
  }

  if (*sa_policy_sel[NETMGR_SA_DIR_RX] != '\0')
  {
    snprintf(xfrm_policy_cmd,
             sizeof(xfrm_policy_cmd),
             "ip xfrm policy deleteall dir in %s",
             sa_policy_sel[NETMGR_SA_DIR_RX]);

    netmgr_log_med("netmgr_kif_remove_sa_rules: removing RX SA policy for link=%d, family=%d",
                   link, ip_family);

    if (ds_system_call2(xfrm_policy_cmd, std_strlen(xfrm_policy_cmd), function_debug) != NETMGR_SUCCESS)
    {
      netmgr_log_err("netmgr_kif_remove_sa_rules: xfrm_policy_cmd failed");
      goto bail;
    }

    snprintf(xfrm_policy_cmd,
             sizeof(xfrm_policy_cmd),
             "ip xfrm policy deleteall dir fwd %s",
             sa_policy_sel[NETMGR_SA_DIR_RX]);

    netmgr_log_med("netmgr_kif_remove_sa_rules: removing FWD SA policy for link=%d, family=%d",
                   link, ip_family);

    if (ds_system_call2(xfrm_policy_cmd, std_strlen(xfrm_policy_cmd), function_debug) != NETMGR_SUCCESS)
    {
      netmgr_log_err("netmgr_kif_remove_sa_rules: xfrm_policy_cmd failed");
      goto bail;
    }

    *sa_policy_sel[NETMGR_SA_DIR_RX] = '\0';
  }

  sa->is_sa_valid = FALSE;
  rc = NETMGR_SUCCESS;

bail:
  NETMGR_LOG_FUNC_EXIT;
  return rc;
}
#endif /* FEATURE_DATA_IWLAN */

/*===========================================================================
  FUNCTION netmgr_kif_set_iptables_rule_on_iface
===========================================================================*/
/*!
@brief
  Block SSDP broadcast packets from being trandmitten on an interface

@return
  None

@param *dev_name ASCII string representation of network device name

@note
  Underlying ds_system_call() will return 0 even on failure of system command.
  As such, this will return void because there is no way to determine a failure
  condition.
*/
LOCAL void
netmgr_kif_set_ssdp_rule_on_iface
(
  const char *dev_name
)
{
  char cmd[NETMGR_KIF_SYSCMD_SIZ];
  int cmdlen, rc;

  cmdlen = snprintf(cmd,
                    sizeof(cmd),
                    NETMGR_KIF_DROP_SSDP_CMD_FORMAT,
                    dev_name);
  if (cmdlen == NETMGR_KIF_SYSCMD_SIZ)
  {
    netmgr_log_err("%s(): Command buffer overflow", __func__);
    return;
  }
  rc = ds_system_call( cmd, cmdlen );
  netmgr_log_high("%s(): Command returns %d. cmd: %s", __func__, rc, cmd);
}

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  netmgr_kif_powerup_init
===========================================================================*/
/*!
@brief
  Perform power-up initialization.

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
netmgr_kif_powerup_init
(
  netmgr_ctl_port_config_type links[],
  char * iname
)
{
  netmgr_kif_cfg.link_array = links;
  (void)iname;

  /* Initialize device names */
  netmgr_kif_init_names();

#ifndef FEATURE_DS_LINUX_DRIVER_LEGACY
  char cmd[NETMGR_KIF_SYSCMD_SIZ];
  int cmdlen;
  int i,j,rc=0;
  int delay=0,inc=0;
  int modem_status = NETMGR_KIF_OPEN_SUCCESS;
  int port_opened = NETMGR_KIF_PORT_CLOSED;
  uint32 mode = RMNET_MODE_LLP_ETH;

  cmdlen = snprintf( cmd, sizeof(cmd), "rm %s", NETMGR_KIF_SYSFILE_OPEN_STATUS );
  rc = ds_system_call( cmd, cmdlen );
  if( 0 > rc ) {
    netmgr_log_sys_err("failed system call:");
  }

  /* Report to power subsystem the maximum timeout for transport port
   * open operations.  This is to support test automation. */
  inc = NETMGR_KIF_USEC_TO_SEC( NETMGR_KIF_MAX_RETRY_COUNT * NETMGR_KIF_WAIT_TIME_BEFORE_NEXT_RETRY );
  for (i = 0; i < NETMGR_MAX_LINK; i++) {
    if( TRUE == netmgr_kif_cfg.link_array[i].modem_wait ) {
      delay += inc;
    }
  }
  cmdlen = snprintf( cmd, sizeof(cmd), "echo %d > %s",
                           delay, NETMGR_KIF_SYSFILE_OPEN_TIMEOUT );
  rc = ds_system_call( cmd, cmdlen );
  if( 0 > rc ) {
    netmgr_log_sys_err("failed system call:");
  }

  /* Test the RMNET driver QMI link-layer protocol mode to see if
   * already in IP mode (driver uses Ethernet default).  This is an
   * indirect approach to determine if NetMgr has reset without device
   * powercyle.  Goal is to avoid closing ports as this toggle DTR on
   * Modem, which will clear QMI client registrations.  NetMgr may
   * restart after other processes have already registered with Modem
   * QMI, so this must be avoided. */
  if( NETMGR_LINK_MAX > netmgr_main_cfg.def_link ) {
    netmgr_link_id_t link = netmgr_main_cfg.def_link;
    if( NETMGR_SUCCESS !=
        netmgr_kif_ifioctl_get_llpmode( netmgr_kif_info[link].name,
                                        &mode ) ) {
      netmgr_log_err("netmgr_kif_ifioctl_get_llpmode() failed"
                     " for %s\n", netmgr_kif_info[link].name);
    }
  } else {
      netmgr_log_err("Invalid default link[%d]", netmgr_main_cfg.def_link);
  }

  if( RMNET_MODE_LLP_IP != mode ) {
    /* Loop over all links to close ports.  Do this in separate step to
     * ensure Modem will have all ports closed when later attempt to
     * open is done. */
    for (i = 0; i < NETMGR_MAX_LINK; i++) {
      (void)netmgr_kif_ifioctl_close_port( netmgr_kif_info[i].name );
    }
  }
  else
  {
    netmgr_log_med("Driver in IP mode, skipping port close\n");
  }


  /* infinite loop till at least one port is open */
  do
  {
    /* Loop over all links to open ports */
    for (i = 0; i < NETMGR_MAX_LINK; i++)
    {
      /* open driver transport port.  Doing this here to avoid race
       * condition where DTR toggle clears QMI/RmNET configuration
       * settings */

      /* Retry mechanism for first transport port to Modem */
      if( netmgr_kif_cfg.link_array[i].modem_wait )
      {
        j = 0;
        do
        {
          if( NETMGR_SUCCESS !=
            netmgr_kif_ifioctl_open_port( netmgr_kif_info[i].name ) )
          {
            /* Failed to open port */
            netmgr_log_err("netmgr_kif_ifioctl_open_port() failed for %s on attempt %d\n",
                           netmgr_kif_info[i].name, j);
            usleep(NETMGR_KIF_WAIT_TIME_BEFORE_NEXT_RETRY);
          }
          else
          {
            /* Even if we are able to open port successfully, wait a bit
             * before trying to open the next one to give enough time to
             * modem to open them all */
            usleep(NETMGR_KIF_WAIT_TIME_BEFORE_NEXT_RETRY);
            break;
          }
        } while ( NETMGR_KIF_MAX_RETRY_COUNT > j++ );

        /* Disable link if port not opened */
        if( NETMGR_KIF_MAX_RETRY_COUNT < j )
        {
          netmgr_log_err("netmgr_kif_ifioctl_open_port() failed for %s\n",
                         netmgr_kif_info[i].name);
          netmgr_kif_cfg.link_array[i].enabled = FALSE;
          modem_status = NETMGR_KIF_OPEN_FAILURE;
        }
        else
        {
          netmgr_kif_set_ssdp_rule_on_iface( netmgr_kif_info[i].name );
          port_opened= NETMGR_KIF_PORT_OPENED;
        }
      }
      else if ( netmgr_kif_cfg.link_array[i].enabled )
      {
        /* No retry mechanism for subsequent ports */
        if( NETMGR_SUCCESS !=
            netmgr_kif_ifioctl_open_port( netmgr_kif_info[i].name ) )
        {
          netmgr_log_err("netmgr_kif_ifioctl_open_port() failed for %s\n",
                         netmgr_kif_info[i].name);
          netmgr_kif_cfg.link_array[i].enabled = FALSE;
        }
        else
        {
          netmgr_kif_set_ssdp_rule_on_iface( netmgr_kif_info[i].name );
          port_opened= NETMGR_KIF_PORT_OPENED;
        }
      }
      else
      {
        netmgr_log_high("kif link [%d] is disabled\n", i);
      }
    }

    /* get out of infinite loop at least one port opened */
    if(port_opened == NETMGR_KIF_PORT_OPENED )
    {
      break;
    }
    else
    {
      netmgr_log_high("No port opened yet, retry .....\n");
      /* before next loop interation, reset and update all links */
      netmgr_main_reset_links();
      netmgr_main_update_links();

      /* reset modem_status */
      modem_status = NETMGR_KIF_OPEN_SUCCESS;
    }
  }while (1);

  /* Report to power subsystem transport port open completion.  This
   * is to support test automation. */
  cmdlen = snprintf( cmd, sizeof(cmd), "echo %d > %s", modem_status, NETMGR_KIF_SYSFILE_OPEN_STATUS );
  rc = ds_system_call( cmd, cmdlen );
  if( 0 > rc ) {
    netmgr_log_sys_err("failed system call:");
  }

#ifdef FEATURE_DATA_IWLAN
  if (NETMGR_SUCCESS != netmgr_kif_update_config_and_routing_rules())
  {
    netmgr_log_err( "failed to update config and routing rules\n" );
  }
#else
  netmgr_kif_update_link_config();
#endif /* FEATURE_DATA_IWLAN */

#endif /* FEATURE_DS_LINUX_DRIVER_LEGACY */
}

/*===========================================================================
  FUNCTION  netmgr_kif_get_name
===========================================================================*/
/*!
@brief
  Accessor for getting device name for a given link.

@return
  char* - Pointer to device name

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
const char *
netmgr_kif_get_name (int link)
{
  if( NETMGR_SUCCESS == netmgr_kif_verify_link( link ) )
    return netmgr_kif_info[link].name;
  else
    return NULL;
}

/*===========================================================================
  FUNCTION  netmgr_kif_get_link_powerup_state
===========================================================================*/
/*!
@brief
  Accessor for getting netmgr power-up state for a given link.

@return
  netmgr_kif_link_pwrup_state_t

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
netmgr_kif_link_pwrup_state_t
netmgr_kif_get_link_powerup_state(int link)
{
   return netmgr_kif_info[link].pwrup_status;
}

/*===========================================================================
  FUNCTION  netmgr_kif_set_link_powerup_state
===========================================================================*/
/*!
@brief
  Assign netmgr power-up state for a given link.

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
netmgr_kif_set_link_powerup_state
(
   int link,
   netmgr_kif_link_pwrup_state_t pwrup_state
)
{
   netmgr_kif_info[link].pwrup_status = pwrup_state;
}

/*===========================================================================
  FUNCTION  netmgr_kif_send_icmpv6_router_solicitation
===========================================================================*/
/*!
@brief
  Sends a ICMPV6 router solicitation message

@return
  NETMGR_SUCCESS
  NETMGR_FAILURE

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
netmgr_kif_send_icmpv6_router_solicitation (int link)
{
#if defined FEATURE_DS_LINUX_ANDROID || defined FEATURE_DATA_LINUX_LE
  int sock_fd = -1;
  struct icmp6_hdr router_solicit;
  struct sockaddr_in6 dest6;
  int ret = NETMGR_FAILURE;
  int hop_limit = 255;

  if ((sock_fd = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)) < 0)
  {
    netmgr_log_sys_err("router solicitation socket() failed:");
    goto bail;
  }

  netmgr_log_med("router solicitation setting hoplimit[%d] interface[%s]",
                 hop_limit, netmgr_kif_get_name( link ));

  /* Set the multicast hop limit */
  if( -1 == setsockopt( sock_fd,
                        IPPROTO_IPV6,
                        IPV6_MULTICAST_HOPS,
                        (char *) &hop_limit,
                        sizeof(hop_limit)) )
  {
    netmgr_log_sys_err("router solicitation setsockopt() failed to set hop limit:");
    goto bail;
  }

  /* Bind to the specific link interface */
  if( -1 == setsockopt( sock_fd,
                        SOL_SOCKET,
                        SO_BINDTODEVICE,
                        (char *)netmgr_kif_get_name( link ),
                        (strlen(netmgr_kif_get_name( link ))+1) ))
  {
    netmgr_log_sys_err("router solicitation setsockopt() failed on iface bind:");
    goto bail;
  }

  router_solicit.icmp6_type = ND_ROUTER_SOLICIT;
  router_solicit.icmp6_code = 0;

  memset(&dest6, 0, sizeof(dest6));
  inet_pton(AF_INET6, NETMGR_KIF_IPV6_MULTICAST_ROUTER_ADDR, &dest6.sin6_addr);
  dest6.sin6_family = AF_INET6;

  netmgr_log_med("sending router solicitation");

  if (sendto(sock_fd,
             &router_solicit,
             sizeof(router_solicit),
             0,
             (struct sockaddr *)&dest6,
             sizeof(dest6)) < 0)
  {
    netmgr_log_sys_err("router solication sendto() failed:");
    goto bail;
  }

  ret = NETMGR_SUCCESS;

bail:
  if (-1 != sock_fd)
  {
    close(sock_fd);
  }

  return ret;
#else
  netmgr_log_err("sending router solicitation not supported in this build");
  return NETMGR_FAILURE;
#endif
}

#ifdef FEATURE_DATA_IWLAN
/*===========================================================================
  FUNCTION  netmgr_kif_iwlan_update_dynamic_config
===========================================================================*/
/*!
@brief
  Enable IP forwarding and update the policy routing rules for the first
  iWLAN call bring-up

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
netmgr_kif_iwlan_update_dynamic_config (int link, int ip_family)
{
  unsigned int active_calls = netmgr_kif_get_active_iwlan_calls(ip_family);

  netmgr_log_med("netmgr_kif_iwlan_update_dynamic_config: link=%d, family=%d, "
                 "active_calls=%u, forwarding=%d, policy_routing=%d\n",
                 link,
                 ip_family,
                 active_calls,
                 NETMGR_KIF_GET_FORWARDING_ENABLED(ip_family),
                 NETMGR_KIF_GET_POLICY_ROUTING_INSTALLED(ip_family));

  /* If a first iWLAN call is being brought up, enable forwarding and update
     the policy routing rules*/
  if (1 == active_calls)
  {
    if (NETMGR_SUCCESS != netmgr_kif_enable_forwarding(ip_family))
    {
      netmgr_log_err("netmgr_kif_iwlan_update_dynamic_config: failed to enable forwarding\n");
    }

    if (NETMGR_SUCCESS != netmgr_kif_install_policy_routing_rules(ip_family))
    {
      netmgr_log_err("netmgr_kif_iwlan_update_dynamic_config: failed to install policy routing rules\n");
    }
  }
}

/*===========================================================================
  FUNCTION  netmgr_kif_iwlan_cleanup_dynamic_config
===========================================================================*/
/*!
@brief
  Disable IP forwarding and cleanup the policy routing rules when the last
  iWLAN call goes down

@return
  None

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
netmgr_kif_iwlan_cleanup_dynamic_config(int link, int ip_family)
{
  unsigned int active_calls = netmgr_kif_get_active_iwlan_calls(ip_family);

  netmgr_log_med("netmgr_kif_iwlan_cleanup_dynamic_config: link=%d, family=%d, "
                 "active_calls=%u, forwarding=%d, policy_routing=%d\n",
                 link,
                 ip_family,
                 active_calls,
                 NETMGR_KIF_GET_FORWARDING_ENABLED(ip_family),
                 NETMGR_KIF_GET_POLICY_ROUTING_INSTALLED(ip_family));

  /* If the last iWLAN call is being brought down, disable forwarding and cleanup
     the policy routing rules*/
  if (0 == active_calls)
  {
    if (NETMGR_SUCCESS != netmgr_kif_disable_forwarding(ip_family))
    {
      netmgr_log_err("netmgr_kif_iwlan_cleanup_dynamic_config: failed to disable forwarding\n");
    }

    if (NETMGR_SUCCESS != netmgr_kif_cleanup_policy_routing_rules(ip_family))
    {
      netmgr_log_err("netmgr_kif_iwlan_cleanup_dynamic_config: failed to cleanup policy routing rules\n");
    }
  }
}

/*===========================================================================
  FUNCTION  netmgr_kif_install_sa_and_routing_rules
===========================================================================*/
/*!
@brief
  Installs forwarding rules and security associations (SA) (if any)
  for given reverse rmnet link

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
netmgr_kif_install_sa_and_routing_rules
(
  int  link,
  int  ip_family
)
{
  int ret = NETMGR_FAILURE;
  netmgr_address_info_t  *addr_info_ptr = NULL;
  netmgr_ipsec_sa_t  *v4_sa = NULL;
  netmgr_ipsec_sa_t  *v6_sa = NULL;
  netmgr_ip_address_t  *ip_addr = NULL;


  NETMGR_LOG_FUNC_ENTRY;

  if (NETMGR_SUCCESS != netmgr_kif_verify_link(link))
  {
    netmgr_log_err("netmgr_kif_install_sa_and_routing_rules: invalid link=%d",
                   link);
    return NETMGR_FAILURE;
  }
  else if (AF_INET  != ip_family &&
           AF_INET6 != ip_family)
  {
    netmgr_log_err("netmgr_kif_install_sa_and_routing_rules: invalid family=%d",
                   ip_family);
    return NETMGR_FAILURE;
  }

  /* Return SUCESS if it is not a reverse Rmnet link */
  if (!NETMGR_KIF_IS_REV_RMNET_LINK(link))
  {
    return NETMGR_SUCCESS;
  }

  if (!(addr_info_ptr = netmgr_qmi_get_addr_info(link)))
  {
    netmgr_log_err("netmgr_kif_install_sa_and_routing_rules: invalid addr_info_ptr");
    goto bail;
  }

  /* Install iptable rules */
  if (NETMGR_SUCCESS != netmgr_kif_install_iptable_rules())
  {
    netmgr_log_err("netmgr_kif_install_sa_and_routing_rules: install iptable rules failed");
    goto bail;
  }

  /* Install specific rules */
  if (NETMGR_SUCCESS != netmgr_kif_install_iptable_rules_ex(link))
  {
    netmgr_log_err("netmgr_kif_install_sa_and_routing_rules: install iptable ex rules failed");
    goto bail;
  }

  /* Install routing rules */
  if (NETMGR_SUCCESS != netmgr_kif_install_forwarding_rules(ip_family, link))
  {
    netmgr_log_err("netmgr_kif_install_sa_and_routing_rules: install fwd rule failed");
    goto bail;
  }

  /* Install the SAs only for an ePDG call */
  if (QMI_WDS_IFACE_NAME_IWLAN_EPDG == netmgr_qmi_wds_get_tech_name(link))
  {
    if (NETMGR_ADDRSET_MASK_IPV4 & addr_info_ptr->valid_mask)
    {
      v4_sa = &addr_info_ptr->ipv4.sa;
    }
    if (NETMGR_ADDRSET_MASK_IPV6 & addr_info_ptr->valid_mask)
    {
      v6_sa = &addr_info_ptr->ipv6.sa;
    }

    if ((AF_INET  == ip_family && !v4_sa) ||
        (AF_INET6 == ip_family && !v6_sa))
    {
      netmgr_log_err("netmgr_kif_install_sa_and_routing_rules: invalid SA for ip_family=%d",
                     ip_family);
      goto bail;
    }

    /* If the SA is already installed, return SUCCESS */
    if ((AF_INET  == ip_family && TRUE == v4_sa->is_sa_valid) ||
        (AF_INET6 == ip_family && TRUE == v6_sa->is_sa_valid))
    {
      netmgr_log_err("netmgr_kif_install_sa_and_routing_rules: SA already installed for ip_family=%d",
                     ip_family);
      return NETMGR_SUCCESS;
    }

    /* If the SA is shared and already installed in the other address family, return SUCCESS */
    if (AF_INET == ip_family && v4_sa->is_sa_shared && v6_sa && v6_sa->is_sa_valid)
    {
      netmgr_log_med("netmgr_kif_install_sa_and_routing_rules: V6 SA already installed, only installing V4 policy");

      /* We only need to install the 'policy' in both directions for the new address family */
      if (NETMGR_SUCCESS != netmgr_kif_get_xfrm_policy_sel_str(addr_info_ptr,
                                                               NETMGR_IPV4_ADDR,
                                                               NETMGR_SA_DIR_TX,
                                                               v4_sa->sa_policy_sel[NETMGR_SA_DIR_TX],
                                                               NETMGR_MAX_STR_LENGTH))
      {
        netmgr_log_err("netmgr_kif_install_sa_and_routing_rules: failed to get xfrm policy sel string");
        goto bail;
      }

      if (NETMGR_SUCCESS != netmgr_kif_get_xfrm_policy_sel_str(addr_info_ptr,
                                                               NETMGR_IPV4_ADDR,
                                                               NETMGR_SA_DIR_RX,
                                                               v4_sa->sa_policy_sel[NETMGR_SA_DIR_RX],
                                                               NETMGR_MAX_STR_LENGTH))
      {
        netmgr_log_err("netmgr_kif_install_sa_and_routing_rules: failed to get xfrm policy sel string");
        goto bail;
      }

      v4_sa->mode = v6_sa->mode;
      strlcpy(v4_sa->sa_state_id[NETMGR_SA_DIR_TX], v6_sa->sa_state_id[NETMGR_SA_DIR_TX], NETMGR_MAX_STR_LENGTH);
      strlcpy(v4_sa->sa_state_id[NETMGR_SA_DIR_RX], v6_sa->sa_state_id[NETMGR_SA_DIR_RX], NETMGR_MAX_STR_LENGTH);

      NETMGR_KIF_UTIL_INSTALL_IPSEC_XFRM_POLICY(TX,out,v4_sa->mode,v4_sa->sa_state_id,v4_sa->sa_policy_sel);
      NETMGR_KIF_UTIL_INSTALL_IPSEC_XFRM_POLICY(RX,in,v4_sa->mode,v4_sa->sa_state_id,v4_sa->sa_policy_sel);
      NETMGR_KIF_UTIL_INSTALL_IPSEC_XFRM_POLICY(RX,fwd,v4_sa->mode,v4_sa->sa_state_id,v4_sa->sa_policy_sel);

      v4_sa->is_sa_valid = TRUE;
      ret = NETMGR_SUCCESS;

      goto bail;
    }
    else if (AF_INET6 == ip_family && v6_sa->is_sa_shared && v4_sa && v4_sa->is_sa_valid)
    {
      netmgr_log_med("netmgr_kif_install_sa_and_routing_rules: V4 SA already installed, only installing V6 policy");

      /* We only need to install the 'policy' in both directions for the new address family */
      if (NETMGR_SUCCESS != netmgr_kif_get_xfrm_policy_sel_str(addr_info_ptr,
                                                               NETMGR_IPV6_ADDR,
                                                               NETMGR_SA_DIR_TX,
                                                               v6_sa->sa_policy_sel[NETMGR_SA_DIR_TX],
                                                               NETMGR_MAX_STR_LENGTH))
      {
        netmgr_log_err("netmgr_kif_install_sa_and_routing_rules: failed to get xfrm policy sel string");
        goto bail;
      }

      if (NETMGR_SUCCESS != netmgr_kif_get_xfrm_policy_sel_str(addr_info_ptr,
                                                               NETMGR_IPV6_ADDR,
                                                               NETMGR_SA_DIR_RX,
                                                               v6_sa->sa_policy_sel[NETMGR_SA_DIR_RX],
                                                               NETMGR_MAX_STR_LENGTH))
      {
        netmgr_log_err("netmgr_kif_install_sa_and_routing_rules: failed to get xfrm policy sel string");
        goto bail;
      }

      v6_sa->mode = v4_sa->mode;
      strlcpy(v6_sa->sa_state_id[NETMGR_SA_DIR_TX], v4_sa->sa_state_id[NETMGR_SA_DIR_TX], NETMGR_MAX_STR_LENGTH);
      strlcpy(v6_sa->sa_state_id[NETMGR_SA_DIR_RX], v4_sa->sa_state_id[NETMGR_SA_DIR_RX], NETMGR_MAX_STR_LENGTH);

      NETMGR_KIF_UTIL_INSTALL_IPSEC_XFRM_POLICY(TX,out,v6_sa->mode,v6_sa->sa_state_id,v6_sa->sa_policy_sel);
      NETMGR_KIF_UTIL_INSTALL_IPSEC_XFRM_POLICY(RX,in,v6_sa->mode,v6_sa->sa_state_id,v6_sa->sa_policy_sel);
      NETMGR_KIF_UTIL_INSTALL_IPSEC_XFRM_POLICY(RX,fwd,v6_sa->mode,v6_sa->sa_state_id,v6_sa->sa_policy_sel);

      v6_sa->is_sa_valid = TRUE;
      ret = NETMGR_SUCCESS;

      goto bail;
    }

    if (NETMGR_SUCCESS != netmgr_kif_install_sa_rules(link, ip_family))
    {
      netmgr_log_err("netmgr_kif_install_sa_and_routing_rules: failed to install SA rules, "
                     "link=%d, ip_family=%d",
                     link,
                     ip_family);
      goto bail;
    }
  }

  ret = NETMGR_SUCCESS;

bail:
  /* Send the success/failure update to the modem */
  netmgr_qmi_send_rev_ip_config_complete(NETMGR_QMI_IWLAN_CALL_BRINGUP,
                                         link,
                                         ip_family,
                                         ret);
  NETMGR_LOG_FUNC_EXIT;
  return ret;
}


/*===========================================================================
  FUNCTION  netmgr_kif_remove_sa_and_routing_rules
===========================================================================*/
/*!
@brief
  Removes the forwarding rules and security associations (SA) (if any)
  for given reverse rmnet link

@return
  int - NETMGR_SUCCESS on operation success, NETMGR_FAILURE otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
netmgr_kif_remove_sa_and_routing_rules
(
  int                   link,
  int                   ip_family,
  netmgr_address_set_t  *addr_info_ptr
)
{
  int ret = NETMGR_FAILURE;
  netmgr_address_info_t  *addr_info = NULL;
  netmgr_ipsec_sa_t  *v4_sa = NULL;
  netmgr_ipsec_sa_t  *v6_sa = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  if (NETMGR_SUCCESS != netmgr_kif_verify_link(link))
  {
    netmgr_log_err("netmgr_kif_remove_sa_and_routing_rules: invalid link=%d",
                   link);
    return NETMGR_FAILURE;
  }

  if (!addr_info_ptr)
  {
    netmgr_log_err("netmgr_kif_remove_sa_and_routing_rules: invalid addr_info_ptr");
    return NETMGR_FAILURE;
  }

  if (NETMGR_IPV4_ADDR != addr_info_ptr->if_addr.type &&
      NETMGR_IPV6_ADDR != addr_info_ptr->if_addr.type)
  {
    netmgr_log_err("netmgr_kif_remove_sa_and_routing_rules: invalid family_type=%d",
                   addr_info_ptr->if_addr.type);
    return NETMGR_FAILURE;
  }

  /* Return SUCESS if it is not a reverse Rmnet link */
  if (!NETMGR_KIF_IS_REV_RMNET_LINK(link))
  {
    return NETMGR_SUCCESS;
  }

   /* Remove traffic specific IP table rules */
  if (NETMGR_SUCCESS != netmgr_kif_remove_iptable_rules_ex(link))
  {
    netmgr_log_err("netmgr_kif_remove_iptable_rules_ex: failed to remove IP table rules");
    goto bail;
  }

  if (NETMGR_SUCCESS != netmgr_kif_remove_forwarding_rules(link, addr_info_ptr))
  {
    netmgr_log_err("netmgr_kif_remove_sa_and_routing_rules: failed to remove forwarding rules");
    goto bail;
  }

  /* Remove the SAs only for an ePDG call */
  if (QMI_WDS_IFACE_NAME_IWLAN_EPDG == netmgr_qmi_wds_get_tech_name(link))
  {
    if (NETMGR_SUCCESS != netmgr_kif_remove_sa_rules(link, ip_family))
    {
      netmgr_log_err("netmgr_kif_remove_sa_and_routing_rules: failed to remove SA rules");
      goto bail;
    }
  }

  ret = NETMGR_SUCCESS;

bail:
  NETMGR_LOG_FUNC_EXIT;
  return ret;
}
#endif /* FEATURE_DATA_IWLAN */


/*===========================================================================
  FUNCTION  netmgr_kif_init
===========================================================================*/
/*!
@brief
  Initialization routine for the KIF module.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void
netmgr_kif_init
(
  int    nint,
  int    skip,
  char * dirpath,
  char * modscript
)
{
  struct kif_vtbl vtable;
  pthread_mutexattr_t attr;

  NETMGR_LOG_FUNC_ENTRY;

  /* Set number of interfaces in the configuration blob */
  netmgr_log_high("netmgr_kif_init: set netmgr_kif_cfg.nint to %d\n", nint);
  netmgr_kif_cfg.nint = nint;

  /* Set 'skip' flag. This indicates if the ethernet driver module should
  ** be loaded or not. This functionality is strictly for debugging purposes.
  */
  netmgr_kif_cfg.skip = skip;

  /* Initialize module load script name and dhcp script names */
  netmgr_kif_init_paths(dirpath, modscript);

  /* Register with Platform layer */
  vtable.reset             = netmgr_kif_reset_link;
  vtable.dispatch          = netmgr_kif_nl_msg_recv_cmd_exec;
  vtable.send_event        = netmgr_kif_send_event;
  vtable.iface_open        = netmgr_kif_open;
  vtable.iface_close       = netmgr_kif_close;
  vtable.iface_configure   = netmgr_kif_configure;
  vtable.iface_reconfigure = netmgr_kif_reconfigure;
  vtable.flow_control      = NULL;

  if( NETMGR_SUCCESS !=
      netmgr_platform_register_vtbl( NETMGR_PLATFORM_VTBL_KIF,
                                     (void*)&vtable ) )
  {
    NETMGR_ABORT("netmgr_kif_init: cannot register vtable with platform layer\n");
    return;
  }

  /* Initialize NetLink socket interface and listening thread */
  netmgr_kif_netlink_init();

  /* Load the kernel module, if skip flag is not set */
  if ((skip != NETMGR_KIF_SKIP) && (netmgr_kif_load_module() < 0)) {
    NETMGR_ABORT("netmgr_kif_init: cannot load kernel module\n");
    return;
  }

  /* Initialize device interfaces. Of course, this must be done after
  ** the devices are created, i.e. the module is loaded.
  */
  if( NETMGR_SUCCESS != netmgr_kif_init_ifaces() ) {
    NETMGR_ABORT("netmgr_kif_init: cannot init ifaces\n");
    return;
  }

  /* Register process termination cleanup handler */
  atexit( netmgr_kif_cleanup );

  NETMGR_LOG_FUNC_EXIT;
  return;
}

