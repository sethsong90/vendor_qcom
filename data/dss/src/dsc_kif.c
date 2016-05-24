/******************************************************************************

                        D S C _ K I F . C

******************************************************************************/

/******************************************************************************

  @file    dsc_kif.c
  @brief   DSC's Kernel Interface Module

  DESCRIPTION
  Implementation of Kernel Interface module.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id$

when       who        what, where, why
--------   ---        -------------------------------------------------------
08/06/10   ar         Added supprot for setting link MTU
12/08/09   js         during dsc_kif_init, close all the open kif interfaces
04/20/09   js         dsc command objects for kif module are allocated/de-
                      allocated from heap instead of using a static array
05/30/08   vk         Added support for GET_DEVICE_NAME IOCTL
03/15/08   vk         Incorporated code review comments
11/30/07   vk         Cleaned up lint warnings
11/17/07   vk         Added function headers and other comments
11/06/07   vk         Using safe string functions
09/28/07   vk         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if.h>
#include <assert.h>
#ifdef FEATURE_DSS_LINUX_ANDROID
#include <sys/system_properties.h>
#include "cutils/properties.h"
#ifdef FEATURE_GB_NET_UTILS
#include "ifc_utils.h"
#else
#include "ifc.h"
#endif
#endif
#include "stringl.h"
#include "dsci.h"
#include "dsc_util.h"
#include "dsc_kif.h"
#include "dsc_cmd.h"
#include "dsc_dcmi.h"
#include "dsc_qmi_wds.h"

/* Defined in libnetutils.  These are not declared in any header file. */
extern int do_dhcp();
extern void ifc_close();

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Constant representing maximum length of interface/device name
---------------------------------------------------------------------------*/
#define DSC_KIF_NAME_MAX_LEN IFNAMSIZ

/*--------------------------------------------------------------------------- 
   Constant representing maximum length of interface/device name's prefix,
   i.e. without the numeric instance # (e.g. 'eth' in 'eth0')
---------------------------------------------------------------------------*/
#define DSC_KIF_NAME_PR_MAX_LEN (IFNAMSIZ - 3)

/*--------------------------------------------------------------------------- 
   Constant string representing default interface name
---------------------------------------------------------------------------*/
#define DSC_KIF_DEF_NAME "eth"

/*--------------------------------------------------------------------------- 
   Constant representing maximum number of interfaces supported
---------------------------------------------------------------------------*/
#define DSC_KIF_MAX_INT DSC_MAX_PRICALL

/*--------------------------------------------------------------------------- 
   Constant string representing source address of unix domain socket
---------------------------------------------------------------------------*/
#define DSC_KIF_UDS_CLIENT_PATH "/tmp/uds_clnt.1234"

/*--------------------------------------------------------------------------- 
   max number of outstanding kif module commands. if this is exceeded at 
   any given time, we print a high debug message
---------------------------------------------------------------------------*/
#define DSC_KIF_MAX_CMD 25

/*--------------------------------------------------------------------------- 
   Constant representing maximum length of filename
---------------------------------------------------------------------------*/
#define DSC_KIF_FILENAME_MAX_LEN 128

/*--------------------------------------------------------------------------- 
   Constant string representing default prefix name of pid file used by 
   udhcpc; DSC reads this file to determine the process id of udhcpc daemon
---------------------------------------------------------------------------*/
#define DSC_KIF_PIDFILE_DEF_NAME "/tmp/udhcpcd.pid."

/*--------------------------------------------------------------------------- 
   Macros for Android network interface MTU override
---------------------------------------------------------------------------*/
#define DSC_KIF_PROPERTY_MTU          "persist.data_dsc_mtu"
#define DSC_KIF_PROPERTY_MTU_SIZE     (4)
#define DSC_KIF_PROPERTY_MTU_DEFAULT  DSC_MTU_INVALID


/*--------------------------------------------------------------------------- 
   Type representing collection of configuration info for KIF module
---------------------------------------------------------------------------*/
typedef struct {
    int     nint; /* number of interfaces */
    char    name[DSC_KIF_NAME_MAX_LEN]; /* device name */
    int     skip; /* Boolean, which if set, indicates that the device driver
                     module should not be loaded. Used only for debugging */
    char    pidfile[DSC_KIF_FILENAME_MAX_LEN]; /* Name of file to use for 
                                                  writing pid of udhcpc */
    char    dirpath[DSC_KIF_FILENAME_MAX_LEN]; /* Name of dir containing 
                                                  module load script */
    char    modscript[DSC_KIF_FILENAME_MAX_LEN]; /* module load script */
    char    dhcpscript[DSC_KIF_FILENAME_MAX_LEN]; /* dhcp script */
} dsc_kif_cfg_t;

/*--------------------------------------------------------------------------- 
   Type representing enumeration of kif (kernel interface) states
---------------------------------------------------------------------------*/
typedef enum {
    DSC_KIF_CLOSED          = 0, /* Interface 'down' state */
    DSC_KIF_OPENING         = 1, /* Interface coming up */
    DSC_KIF_CONFIGURING     = 2, /* Interface up but not configured yet */
    DSC_KIF_DECONFIGURING   = 3, /* Interface up but deconfiguring */
    DSC_KIF_CLOSING         = 4, /* Interface deconfigured and going down */
    DSC_KIF_OPEN            = 5, /* Interface up and configured */
    DSC_KIF_RECONFIGURING   = 6  /* Interface up and being reconfigured */
} dsc_kif_state_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of state and control information of a kif 
---------------------------------------------------------------------------*/
typedef struct {
    int                         enabled; /* 0-disabled, 1-enabled */
    char                        name[DSC_KIF_NAME_MAX_LEN]; /* device name */
    int                         ifi_index; /* system assigned unique 
                                              device index */
    dsc_kif_state_t             state; /* interface state */
    const dsc_kif_clntcb_t    * clntcb; /* client's registered callbacks */
    void                      * clnt_hdl; /* Client's handle ptr */
    char                        pidfile[DSC_KIF_FILENAME_MAX_LEN]; 
                                    /* name of file containing udhcpc pid */
} dsc_kif_info_t;

/*--------------------------------------------------------------------------- 
   Type representing collection of control info related to a netlink socket
---------------------------------------------------------------------------*/
typedef struct {
    int                 sk_fd;       /* socket descriptor */
    struct sockaddr_nl  sk_addr_loc; /* local address of socket */
    struct sockaddr_nl  sk_addr_rem; /* remote endpoint's address */
} dsc_kif_nl_sk_info_t;

#define DSC_KIF_NL_PID_MASK  (0x7FFFFFFF)
#define DSC_KIF_NL_PID       (getpid() & DSC_KIF_NL_PID_MASK)
//#define DSC_KIF_NL_PID     ((getpid() | (pthread_self() << 16)) & DSC_KIF_NL_PID_MASK)

/*--------------------------------------------------------------------------- 
   Type representing an IOCTL command
---------------------------------------------------------------------------*/
typedef struct dsc_kif_ioctl_cmd_s {
    dsc_dcm_iface_ioctl_t ioctl; /* ioctl command name */
    int                   link;  /* link id on which command was issued */
} dsc_kif_ioctl_cmd_t;

/*--------------------------------------------------------------------------- 
   Collection of shared configuration info for KIF module
---------------------------------------------------------------------------*/
static dsc_kif_cfg_t dsc_kif_cfg;

/*--------------------------------------------------------------------------- 
   Array of state/control info for each kif
---------------------------------------------------------------------------*/
static dsc_kif_info_t dsc_kif_info[DSC_KIF_MAX_INT];

/*--------------------------------------------------------------------------- 
   Collection of control info pertaining to open sockets
---------------------------------------------------------------------------*/
static struct {
    dsc_kif_nl_sk_info_t  rt_sk;            /* Netlink routing socket info */
    dsc_socklthrd_hdl_t   sk_thrd_hdl;      /* Listener thread handle      */
    dsc_socklthrd_fdmap_t sk_thrd_fdmap[1]; /* Array of fdmap structs used 
                                            ** by listener thread.
                                            */
} dsc_kif_sk_info;

/*--------------------------------------------------------------------------- 
   Collection of control info for managing Kernel IF commands
---------------------------------------------------------------------------*/
static struct {
    unsigned char   num;                      /* number of outstanding cmds */
    pthread_mutex_t mutx;   /* Mutex for protecting cmd enq and deq ops */
} dsc_kif_cmd_ctrl;

/*--------------------------------------------------------------------------- 
   Forward declarations needed for subsequent definitions
---------------------------------------------------------------------------*/
static dsc_cmd_t * dsc_kif_cmd_alloc (void);
static void dsc_kif_cmd_free (dsc_cmd_t * cmd);

/*--------------------------------------------------------------------------- 
   Inline accessor for getting kif state for a given link
---------------------------------------------------------------------------*/
static __inline__ dsc_kif_state_t 
dsc_kif_get_state (int lnk)
{
    return dsc_kif_info[lnk].state;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting kif state for a given link
---------------------------------------------------------------------------*/
static __inline__ void
dsc_kif_set_state (int lnk, dsc_kif_state_t state)
{
    dsc_kif_info[lnk].state = state;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting pointer to client callback structure for a 
   given link
---------------------------------------------------------------------------*/
static __inline__ const dsc_kif_clntcb_t * 
dsc_kif_get_clntcb (int lnk)
{
    return dsc_kif_info[lnk].clntcb;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting pointer to client callback structure for a 
   given link
---------------------------------------------------------------------------*/
static __inline__ void
dsc_kif_set_clntcb (int lnk, const dsc_kif_clntcb_t * clntcb)
{
    dsc_kif_info[lnk].clntcb = clntcb;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting client handle ptr for a given link
---------------------------------------------------------------------------*/
static __inline__ void * 
dsc_kif_get_clnt_hdl (int lnk)
{
    return dsc_kif_info[lnk].clnt_hdl;
}

/*--------------------------------------------------------------------------- 
   Inline mutator for setting client handle ptr for a given link
---------------------------------------------------------------------------*/
static __inline__ void
dsc_kif_set_clnt_hdl (int lnk, void * clnt_hdl)
{
   dsc_kif_info[lnk].clnt_hdl = clnt_hdl;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting device name for a given link
---------------------------------------------------------------------------*/
static __inline__ const char * 
dsc_kif_get_name (int lnk)
{
    return dsc_kif_info[lnk].name;
}

/*--------------------------------------------------------------------------- 
   Inline accessor for getting pid file name for a given link
---------------------------------------------------------------------------*/
static __inline__ const char * 
dsc_kif_get_pidfile (int lnk)
{
    return dsc_kif_info[lnk].pidfile;
}

/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_kif_verify_link
===========================================================================*/
/*!
@brief
  Helper function to verify validity of a link ID.

@return
  int - 0 if link ID is valid, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
dsc_kif_verify_link (int lnk)
{
    /* Range check */
    if ((lnk < 0) || (lnk >= DSC_KIF_MAX_INT)) {
        return -1;
    }

    if (dsc_kif_info[lnk].enabled == 0) {
        return -1;
    }

    return 0;
}

/*===========================================================================
  FUNCTION  dsc_kif_ifioctl_get
 ===========================================================================*/
/*!
@brief
  Helper function to get flags for the given kernel interface

@return
  int - 0 if IOCTL is successfully executed, -1 otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_kif_ifioctl_get (const char * dev, short * p_flags)
{
    int fd;
    int rval = -1;
    struct ifreq ifr;

    /* Open a datagram socket to use for issuing the ioctl */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        dsc_log_err("ifioctl_get: socket failed");
        dsc_log_sys_err();
        goto error;
    }

    /* Initialize the ioctl req struct to null */
    memset(&ifr, 0, sizeof(ifr));

    /* Set device name in ioctl req struct */
    (void)strlcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));

    /* Get current if flags for the device */
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        dsc_log_err("ifioctl_get: get ioctl failed");
        dsc_log_sys_err();
        close(fd);
        goto error;
    }

    *p_flags = ifr.ifr_flags;

    /* close temporary socket */
    close(fd);

    rval = 0;
error:
    return rval;

}

/*===========================================================================
  FUNCTION  dsc_kif_ifioctl
===========================================================================*/
/*!
@brief
  Helper function to change specified SIOCSIFFLAGS on a given device.

@return
  int - 0 if IOCTL is successfully executed, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_kif_ifioctl (const char * dev, short flags, short fmask)
{
    int fd;
    int rval = -1;
    struct ifreq ifr;

    /* Open a datagram socket to use for issuing the ioctl */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        dsc_log_err("ifioctl: socket failed");
        dsc_log_sys_err();
        goto error;
    }

    /* Initialize the ioctl req struct to null */
    memset(&ifr, 0, sizeof(ifr));

    /* Set device name in ioctl req struct */
    (void)strlcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));

    /* Get current if flags for the device */
    if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0) {
        dsc_log_err("ifioctl: get ioctl failed");
        dsc_log_sys_err();
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
    if (ioctl(fd, SIOCSIFFLAGS, &ifr) < 0) {
        dsc_log_err("ifioctl: set ioctl failed");
        dsc_log_sys_err();
        close(fd);
        goto error;
    }

    /* Close temporary socket */
    close(fd);

    rval = 0;

error:
    return rval;
}


/*===========================================================================
  FUNCTION  dsc_kif_set_mtu
===========================================================================*/
/*!
@brief
  Helper function to change SIOCSIFMTU on a given device.

@return
  int - 0 if IOCTL is successfully executed, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_kif_set_mtu (const char * dev, unsigned int mtu)
{
    int fd;
    int rval = -1;
    struct ifreq ifr;

    /* Open a datagram socket to use for issuing the ioctl */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        dsc_log_err("ifioctl: socket failed");
        dsc_log_sys_err();
        goto error;
    }

    /* Initialize the ioctl req struct to null */
    memset(&ifr, 0, sizeof(ifr));

    /* Set device name in ioctl req struct */
    (void)strlcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));

    ifr.ifr_data = (void*)mtu;
  
    /* Set if MTU for the device */
    if( ioctl( fd, SIOCSIFMTU, &ifr ) < 0 ) {
        dsc_log_err("ifioctl: set ioctl failed");
        dsc_log_sys_err();
        close(fd);
        goto error;
    }

    /* Close temporary socket */
    close(fd);

    rval = 0;

error:
    return rval;
}

/*===========================================================================
  FUNCTION  dsc_kif_start_dhcp_client
===========================================================================*/
/*!
@brief
  Starts DHCP client (udhcpc) for the specified link/interface.

@return
  void

@note

  - Dependencies
    - Assumes interface is already up  

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_kif_start_dhcp_client (int lnk)
{
    const char * ifname;
#ifndef FEATURE_DSS_LINUX_ANDROID
    char scr_buf[DSC_KIF_FILENAME_MAX_LEN];
#endif /* FEATURE_DSS_LINUX_ANDROID */
    
    /* Get device name for the link */
    ifname = dsc_kif_get_name(lnk);

    /* If configuration has 'skip' flag set, do not start dhcp client. This 
    ** is only used for debugging purposes and during normal operation we 
    ** should never be setting the 'skip' flag. Conceptually if this flag is
    ** set, the dhcp client should be started externally. 
    */
#ifndef FEATURE_DS_LINUX_NO_RPC
      if (dsc_kif_cfg.skip == DSC_KIF_SKIP)
          return;
#endif    
    /* Construct command to execute for starting udhcpc dhcp client. If name 
    ** of dhcp script to use was passed on the command line, pass it to the 
    ** udhcpc program. 
    */
#ifndef FEATURE_DSS_LINUX_ANDROID
    if (strlen(dsc_kif_cfg.dhcpscript) == 0) {
        (void)snprintf
        (
            scr_buf, 
            DSC_KIF_FILENAME_MAX_LEN, 
            "udhcpc -b -i %s -p %s", 
            ifname,
            dsc_kif_get_pidfile(lnk)
        );
    } else {
        (void)snprintf
        (
            scr_buf, 
            DSC_KIF_FILENAME_MAX_LEN,
            "udhcpc -b -i %s -p %s -s %s",
            ifname,
            dsc_kif_get_pidfile(lnk),
            dsc_kif_cfg.dhcpscript
        );
    }

    dsc_log_high("Starting DHCP client for %s", ifname);
    dsc_log_high("Issuing command: %s", scr_buf);

    /* Issue command to start udhcpc/netcfg */
    if (system(scr_buf) != 0) {
        dsc_log_err("Could not start netcfg/udhcpc");
        /* CR 190941 workaround */
        /* dsc_abort(); */
    }

    return;
}
#else
    dsc_log_high("Starting DHCP client for %s", ifname);

    if(ifc_init()) {
        dsc_log_err("Could not initiate dhcp");
    }
    else {
        do_dhcp(ifname);
        ifc_close();
    }

    return;
}
#endif

/*===========================================================================
  FUNCTION  dsc_kif_get_dhcp_client_pid
===========================================================================*/
/*!
@brief
  Reads the pid of the dhcp client for the specified link from the pid file
  and returns it.

@return
  int - pid of the dhcp client if successfully read, -1 otherwise

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
#ifndef FEATURE_DSS_LINUX_ANDROID
static int
dsc_kif_get_dhcp_client_pid (int lnk)
{
    FILE * fp;
    const char * pidfile;
    int pid = -1;

    /* Get name of file containing pid of udhcpc for this link */
    pidfile = dsc_kif_get_pidfile(lnk);

    /* Open pid file */
    if ((fp = fopen(pidfile, "r")) == NULL) {
        dsc_log_err("Cannot open pidfile %s", pidfile);
        dsc_abort();
    }

    /* Read process id of udhcpc from pid file */
    if (fscanf(fp, "%d", &pid) != 1) {
        dsc_log_err("Cannot read pid from pidfile %s", pidfile);
        dsc_abort();
    }

    /* Close pid file */
    fclose(fp);

    /* Return process id */
        return pid;

}
#endif
/*===========================================================================
  FUNCTION  dsc_kif_dhcp_client_release_lease
===========================================================================*/
/*!
@brief
  Signals the dhcp client for the given interface to release the DHCP lease.

@return
  void

@note

  - Dependencies
    - Assumes DHCP client is running 

  - Side Effects
    - None
*/
/*=========================================================================*/
#ifndef FEATURE_DSS_LINUX_ANDROID
static void
dsc_kif_dhcp_client_release_lease (int lnk)
{
        int pid;

    /* If configuration has 'skip' flag set, do not do anything. This flag
    ** is only used for debugging purposes and during normal operation we 
    ** should never be setting the 'skip' flag. Conceptually if this flag is
    ** set, the dhcp client is controlled externally.
    */
#ifndef FEATURE_DS_LINUX_NO_RPC
      if (dsc_kif_cfg.skip == DSC_KIF_SKIP)
          return;
#endif
    /* Get process id of udhcpc dhcp client for this link */
    if ((pid = dsc_kif_get_dhcp_client_pid(lnk)) < 0) {
        return;
    }

    /* Send SIGUSR2 signal to udhcpc to release current lease */
    if (kill(pid, SIGUSR2) < 0) {
        dsc_log_sys_err("Kill with SIGUSR2 failed");
        dsc_abort();
    }

    return;
}
#endif
/*===========================================================================
  FUNCTION  dsc_kif_stop_dhcp_client
===========================================================================*/
/*!
@brief
  Terminates the dhcp client for the given interface.

@return
  void

@note

  - Dependencies
    - Assumes DHCP client is running 

  - Side Effects
    - None
*/
/*=========================================================================*/
#ifndef FEATURE_DSS_LINUX_ANDROID
static void
dsc_kif_stop_dhcp_client (int lnk)
{
        int pid;

    /* If configuration has 'skip' flag set, do not do anything. This flag
    ** is only used for debugging purposes and during normal operation we 
    ** should never be setting the 'skip' flag. Conceptually if this flag is
    ** set, the dhcp client is controlled externally.
    */
#ifndef FEATURE_DS_LINUX_NO_RPC
      if (dsc_kif_cfg.skip == DSC_KIF_SKIP)
          return;
#endif

    /* Get process id of udhcpc dhcp client for this link */
    if ((pid = dsc_kif_get_dhcp_client_pid(lnk)) < 0) {
        return;
    }

    /* Send SIGTERM signal to udhcpc to terminate it */
    if (kill(pid, SIGTERM) < 0) {
        dsc_log_sys_err("Kill with SIGTERM failed");
        dsc_abort();
    }

    return;
}
#endif
/*===========================================================================
  FUNCTION  dsc_kif_open_req
===========================================================================*/
/*!
@brief
  Issues IOCTL to bring up the specified interface. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_kif_open_req (int lnk)
{
    dsc_log_func_entry();
    dsc_log_high("bring up kernel interface for link [%d]", lnk );
    /* To bring up interface, issue ioctl to set IFF_UP flag to 1 */
    if (dsc_kif_ifioctl(dsc_kif_get_name(lnk), IFF_UP, IFF_UP) < 0) {
        dsc_log_err("open req failed, aborting!");
        dsc_abort();
    }
    dsc_log_func_exit();
    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_close_req
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
static void
dsc_kif_close_req (int lnk)
{
    dsc_log_func_entry();
    dsc_log_high("bring down kernel interface for link [%d]", lnk );
    /* To bring down interface, issue ioctl to set IFF_UP flag to 0 */
    if (dsc_kif_ifioctl(dsc_kif_get_name(lnk), ~IFF_UP, IFF_UP) < 0) {
        dsc_log_err("close req failed, aborting!");
        dsc_abort();
    }
    dsc_log_func_exit();
    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_cfg_req
===========================================================================*/
/*!
@brief
  Executes operations to kick-start interface configuration. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_kif_cfg_req (int lnk)
{
    /* Start dhcp client on the interface which will take care of interface 
    ** configuration.
    */
    dsc_kif_start_dhcp_client(lnk);
    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_open_cnf
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
static void 
dsc_kif_open_cnf (int lnk)
{
    dsc_kif_state_t state;

    /* Verify that the specified link id is valid */
    if (dsc_kif_verify_link(lnk) < 0) {
        dsc_log_err("dsc_kif_open_cnf called with invalid link %d", lnk);
        return;
    }

    /* Process based on current interface state */
    switch (state = dsc_kif_get_state(lnk)) {
    case DSC_KIF_OPENING:
        /* Interface is in coming up state. Transition to configuring state */
        dsc_kif_cfg_req(lnk);
        dsc_kif_set_state(lnk, DSC_KIF_CONFIGURING);
        break;
    default:
        /* Ignore in all other states. Effectively we are not supporting the 
        ** case where the virtual ethernet devices are being externally 
        ** controlled too. We assume that DSC has exclusive control over these
        ** devices.
        */
        dsc_log_err("dsc_kif_open_cnf called in state %d", state);
        break;
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_cfg_cnf
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
static void
dsc_kif_cfg_cnf (int lnk)
{
    const dsc_kif_clntcb_t * clntcb;
    dsc_kif_state_t state;
    void * clnt_hdl;
    dsc_dcm_iface_ioctl_t  iface_ioctl;
    unsigned int mtu = DSC_MTU_DEFAULT;
    
    /* Verify that the specified link id is valid */
    if (dsc_kif_verify_link(lnk) < 0) {
        dsc_log_err("dsc_kif_cfg_cnf called with invalid link %d", lnk);
        return;
    }

    /* Process based on current interface state */
    switch (state = dsc_kif_get_state(lnk)) {
    case DSC_KIF_CONFIGURING:
        /* Interface is in configuring state. Transition to open state */
        dsc_kif_set_state(lnk, DSC_KIF_OPEN);

        /* Get Modem MTU for link and assign it to network interface */
        iface_ioctl.name = DSS_IFACE_IOCTL_GET_MTU;
        iface_ioctl.info.mtu = DSC_MTU_INVALID;
        if( DSC_OP_SUCCESS != dsc_qmi_ioctl( lnk, &iface_ioctl ) ){
          dsc_log_err("dsc_kif_cfg_cnf failed on QMI IOCTL for link %d", lnk);
        } else {
          if( DSC_MTU_INVALID == iface_ioctl.info.mtu ) {
            dsc_log_err( "dsc_kif_cfg_cnf cannot get MTU, using default\n" );
          } else {
            /* Assign MTU value from Modem query */
            mtu = iface_ioctl.info.mtu;
          }
        }

#ifdef FEATURE_DSS_LINUX_ANDROID
        /* Configure Android MTU property value as follows:
         *   0 > value <= DSC_MTU_MAX - Override Modem MTU query value
         *   value == 0               - Use Modem MTU query value
         *   value >  DSC_MTU_MAX     - Ignored
         */
        static char  args[PROP_VALUE_MAX];
        char  def[DSC_KIF_PROPERTY_MTU_SIZE+1];
        int   ret;

        memset( args, 0x0, sizeof(args) );
        memset( def, 0x0, sizeof(def) );

        /* Query Android proprerty for MTU override */
        snprintf( def, sizeof(def), "%d", DSC_KIF_PROPERTY_MTU_DEFAULT );
        ret = property_get( DSC_KIF_PROPERTY_MTU, args, def );

        if( (DSC_KIF_PROPERTY_MTU_SIZE) < ret ) {
          dsc_log_err( "System property %s has unexpected size(%d), skipping\n",
                       DSC_KIF_PROPERTY_MTU, ret );
        } else {
          ret = atoi( args );
          if( DSC_MTU_MAX < (unsigned int)ret ) {
            dsc_log_err( "System property %s has exceeded limit (%d), skipping\n",
                         DSC_KIF_PROPERTY_MTU, DSC_MTU_MAX );
          } else {
            if( DSC_MTU_INVALID != ret ) {
              /* Update MTU value using property */
              mtu = ret;
              dsc_log_high( "MTU overide specified, using value %d\n", mtu );
            }
          }
        }
#endif /* FEATURE_DSS_LINUX_ANDROID */

        if( 0 > dsc_kif_set_mtu( dsc_kif_get_name(lnk), mtu ) ) {
          dsc_log_err( "dsc_kif_cfg_cnf failed to set MTU, using default\n" );
        } else {
          dsc_log_high( "dsc_kif_cfg_cnf assigned MTU %d on %s\n",
                        mtu, dsc_kif_get_name(lnk) );
        }

        /* Call registered client handler to indicate that the interface has 
        ** come up. 
        */
        clntcb = dsc_kif_get_clntcb(lnk);
        clnt_hdl = dsc_kif_get_clnt_hdl(lnk);
        (clntcb->opened_cb)(lnk, DSC_OP_SUCCESS, clnt_hdl);
        break;
    case DSC_KIF_RECONFIGURING:
        dsc_kif_set_state(lnk, DSC_KIF_OPEN);
        clntcb = dsc_kif_get_clntcb(lnk);
        clnt_hdl = dsc_kif_get_clnt_hdl(lnk);
        (clntcb->reconfigured_cb)(lnk, DSC_OP_SUCCESS, clnt_hdl);
        break;
    default:
        /* Ignore in all other states */
        dsc_log_err("dsc_kif_cfg_cnf called in state %d, ignoring!", state);
        break;
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_cfg_fail
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
static void
dsc_kif_cfg_fail (int lnk)
{
    dsc_kif_state_t state;

    /* Verify that the specified link id is valid */
    if (dsc_kif_verify_link(lnk) < 0) {
        dsc_log_err("dsc_kif_cfg_fail called with invalid link %d", lnk);
        return;
    }

    /* Process based on current interface state */
    switch (state = dsc_kif_get_state(lnk)) {
    case DSC_KIF_OPEN:
#ifdef  FEATURE_DSS_LINUX_ANDROID
      dsc_log_high("Recived RTM_DELADDR in Interface is in OPEN STATE. Ignoring DELADDR\n");
      break;
#endif
        /* Intentional fall through */
    case DSC_KIF_DECONFIGURING:
        /* We are in open or deconfiguring state. Transition to closing state 
        ** as we lost the configuration and bring the interface down.
        */
#ifndef FEATURE_DSS_LINUX_ANDROID
        dsc_kif_stop_dhcp_client(lnk);
#endif
        dsc_kif_close_req(lnk);
        dsc_kif_set_state(lnk, DSC_KIF_CLOSING);
        break;
    default:
        /* Ignore in all other states */
        dsc_log_err("dsc_kif_cfg_fail called in state %d", state);
        break;
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_close_cnf
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
static void 
dsc_kif_close_cnf (int lnk)
{
    const dsc_kif_clntcb_t * clntcb;
    dsc_kif_state_t state;
    void * clnt_hdl;

    /* Verify that the specified link id is valid */
    if (dsc_kif_verify_link(lnk) < 0) {
        dsc_log_err("dsc_kif_close_cnf called with invalid link %d", lnk);
        return;
    }

    /* Process based on current interface state */
    switch (state = dsc_kif_get_state(lnk)) {
    case DSC_KIF_CONFIGURING:
    case DSC_KIF_RECONFIGURING:
        /* Intentional fall through */
    case DSC_KIF_OPEN:
        /* If we are in open or configuring state, first terminate the dhcp 
        ** client. 
        */
#ifndef FEATURE_DSS_LINUX_ANDROID
         dsc_kif_stop_dhcp_client(lnk);
#endif
        /* intentional fall through */
    case DSC_KIF_CLOSING:
        /* The device has gone down. Transition to closed state and call 
        ** registered client handler to notify of this event. 
        */
        dsc_kif_set_state(lnk, DSC_KIF_CLOSED);
        clntcb = dsc_kif_get_clntcb(lnk);
        clnt_hdl = dsc_kif_get_clnt_hdl(lnk);
        dsc_kif_set_clntcb(lnk, NULL);
        dsc_kif_set_clnt_hdl(lnk, NULL);
        (clntcb->closed_cb)(lnk, DSC_OP_SUCCESS, clnt_hdl);
        break;
    default:
        /* Ignore in all other states */
        dsc_log_err("dsc_kif_close_cnf called in state %d", state);
        break;
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_init_names
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
static void
dsc_kif_init_names (char * prefix, int links[])
{
    int i;
    size_t plen;

    /* Make sure prefix is not null or has valid length, otherwise use the 
    ** statically defined default device name.
    */
    if ((prefix == NULL) || 
        ((plen = strlen(prefix)) > DSC_KIF_NAME_PR_MAX_LEN)) 
    {
        prefix = DSC_KIF_DEF_NAME;
        plen = strlen(DSC_KIF_DEF_NAME);
    }

    (void)strlcpy(dsc_kif_cfg.name, prefix, DSC_KIF_NAME_MAX_LEN);

    /* Iterate over the array of interfaces, initializing device name for 
    ** each. The device name prefix is suffixed with integer values starting 
    ** from '0' for the first one and incrementing by one. 
    */
    for (i = 0; i < DSC_KIF_MAX_INT; ++i) {
        /* no need to init name for interface that is disabled */
        if (links[i] == 0) {
            dsc_kif_info[i].enabled = 0;
            continue;
        }

        /* enable this interface */
        dsc_kif_info[i].enabled = 1;

        (void)strlcpy
        (
            dsc_kif_info[i].name, 
            prefix, 
            DSC_KIF_NAME_MAX_LEN
        );

        dsc_kif_info[i].name[plen] = '0' + (char)i;
        dsc_kif_info[i].name[plen+1] = '\0';

        dsc_log_high("dsc_kif named link %d to %s", i, dsc_kif_info[i].name);

        /* Also initialize the name of the udhcpc program's pid file */
        (void)strlcpy
        (
            dsc_kif_info[i].pidfile, 
            dsc_kif_cfg.pidfile, 
            DSC_KIF_FILENAME_MAX_LEN
        );

        (void)strlcat
        (
            dsc_kif_info[i].pidfile, 
            dsc_kif_info[i].name, 
            DSC_KIF_FILENAME_MAX_LEN
        );
    }
    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_init_paths
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
static void
dsc_kif_init_paths 
(
    const char * dirpath, 
    const char * modscript,
    const char * dhcpscript
)
{
    /* If a directory name was passed on the command line, use it as a prefix
    ** to construct the path name of the module load script.
    */
    if (dirpath) {
        (void)strlcpy
        (
            dsc_kif_cfg.dirpath, 
            dirpath, 
            DSC_KIF_FILENAME_MAX_LEN
        );

        (void)strlcpy
        (
            dsc_kif_cfg.modscript, 
            dsc_kif_cfg.dirpath, 
            DSC_KIF_FILENAME_MAX_LEN
        );
    }

    /* Set module load script name in configuration blob for later use, if one 
    ** was specified on the command line. 
    */
    if (modscript) {
        (void)strlcat
        (
            dsc_kif_cfg.modscript, 
            modscript, 
            DSC_KIF_FILENAME_MAX_LEN
        );
    }

    /* Set dhcp script name in configuration blob for later use, if one was
    ** specified on the command line. 
    */
    if (dhcpscript) {
        (void)strlcat
        (
            dsc_kif_cfg.dhcpscript, 
            dhcpscript, 
            DSC_KIF_FILENAME_MAX_LEN
        );
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_call_ioctl_on_dev
===========================================================================*/
/*!
@brief
  Helper function to call a specified IOCTL on a given device. 

@return
  int - 0 if IOCTL was issued successfully, -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
dsc_kif_call_ioctl_on_dev (const char * dev, unsigned int req, struct ifreq * ifr)
{
    int fd;
    int rval = -1;

    /* Open a temporary socket of datagram type to use for issuing the ioctl */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        dsc_log_sys_err();
        goto error;
    }

    /* Initialize ioctl req struct to null */
    memset(ifr, 0, sizeof(struct ifreq));

    /* Set device name in the ioctl req struct */
    (void)strlcpy(ifr->ifr_name, dev, sizeof(ifr->ifr_name));

    /* Issue ioctl on the device */
    if (ioctl(fd, req, ifr) < 0) {
        dsc_log_sys_err();
        close(fd);
        goto error;
    }

    /* Close temporary socket */
    close(fd);
    rval = 0;

error:
    return rval;
}

/*===========================================================================
  FUNCTION  dsc_kif_bind_sock_to_iface
===========================================================================*/
/*!
@brief
  Fetches socket descriptor from the client using the fd receive protocol 
  and binds it to the specified device. 

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_kif_bind_sock_to_iface (const char * dev, const char * uds_srvr_path)
{
    int ufd;
    int sfd;
        struct ifreq iface;

    /* Open unix domain socket client and connect to the server */
    if ((ufd = ds_open_uds_clnt(uds_srvr_path, DSC_KIF_UDS_CLIENT_PATH)) < 0) {
        dsc_log_sys_err("dsc_kif_bind_sock_to_iface: ds_open_uds_clnt failed!");
        goto error_none;
    }

    /* Send handshake message to tell server that i am up. Server should 
    ** respond with a message containing the socket descriptor to bind.
    */
    if (ds_send_handshake_over_uds(ufd) < 0) {
        dsc_log_sys_err("dsc_kif_bind_sock_to_iface: send_handshake failed!");
        goto error_ufd;
    }

    /* Wait for message containing socket descriptor to arrive */
    if ((sfd = ds_recv_fd_over_uds(ufd)) < 0) {
        dsc_log_sys_err("dsc_kif_bind_sock_to_iface: recv_fd failed!");
        goto error_ufd;
    }

    /* Populate request structure with the device name to bind the socket to */
    (void)strlcpy(iface.ifr_ifrn.ifrn_name, dev, IFNAMSIZ);

    /* Issue request to bind socket to the desired device/interface */
    if (setsockopt(sfd, SOL_SOCKET, SO_BINDTODEVICE, (char *)&iface, sizeof(iface)) < 0)
    /* if (setsockopt(sfd, SOL_SOCKET, SO_BINDTODEVICE, dev, sizeof(char *)) < 0) */
    {
        dsc_log_sys_err("dsc_kif_bind_sock_to_iface:");
        goto error_sfd;
    }

    /* Binding done. Send handshake message to server to indicate completion */
    if (ds_send_handshake_over_uds(ufd) < 0) {
        dsc_log_err("dsc_kif_bind_sock_to_iface: send_handshake after recv_fd failed!");
        goto error_sfd;
    }

error_sfd:
    /* Close temporary socket used to issue BINDTODEVICE request */
    close(sfd);

error_ufd:
    /* Close unix domain socket */
    close(ufd);

error_none:
    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_ioctl_cmd_exec
===========================================================================*/
/*!
@brief
  Virtual function used by the Command Thread to execute KIF command to 
  process IOCTLs.

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_kif_ioctl_cmd_exec (dsc_cmd_t * cmd, void * data)
{
    dsc_kif_ioctl_cmd_t * ioctl_cmd;
    int lnk;
    const char * dev;

    /* Make sure ptrs passed are valid before proceeding */
    ds_assert(cmd);
    ds_assert(data);

    /* Get ioctl command ptr from user data ptr */
    ioctl_cmd = (dsc_kif_ioctl_cmd_t *)data;

    /* Get link id from ioctl cmd object */
    lnk = ioctl_cmd->link;

    /* Verify that link id is valid */
    if (dsc_kif_verify_link(lnk) < 0) {
        dsc_log_err("dsc_kif_ioctl_cmd_exec called with invalid link %d", lnk);
        dsc_abort();
    }

    /* Get device name for link id */
    dev = dsc_kif_get_name(lnk);

    /* Process ioctl based on ioctl type in the command */
    switch (ioctl_cmd->ioctl.name) {
    case DSS_IFACE_IOCTL_BIND_SOCK_TO_IFACE:
        /* Process 'bind socket to interface' ioctl */
        dsc_kif_bind_sock_to_iface
        (
            dev, 
            ioctl_cmd->ioctl.info.bind_info.uds_path
        );
        break;
    default:
        /* Unexpected command received. Abort for debug purposes */
        dsc_log_err("Unexpected ioctl cmd %d received", ioctl_cmd->ioctl.name);
        dsc_abort();
    }

    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_ioctl_cmd_free
===========================================================================*/
/*!
@brief
  Virtual function used by the Command Thread to free KIF commands to process
  IOTCLs, after execution of the IOCTL is complete.

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_kif_ioctl_cmd_free (dsc_cmd_t * cmd, void * data)
{
    dsc_kif_ioctl_cmd_t * ioctl_cmd;

    /* Make sure ptrs passed are valid before proceeding */
    ds_assert(cmd);
    ds_assert(data);

    /* Get ioctl command ptr from user data ptr */
    ioctl_cmd = (dsc_kif_ioctl_cmd_t *)data;

    /* Deallocate memory for ioctl command object */
    dsc_free(ioctl_cmd);

    /* Free command object */
    dsc_kif_cmd_free(cmd);

    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_post_ioctl_cmd
===========================================================================*/
/*!
@brief
  Posts a command to execute the specified IOCTL in the Command Thread 
  context.

@return
  int - 0 if command successfully posted, -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_kif_post_ioctl_cmd (int lnk, dsc_dcm_iface_ioctl_t * iface_ioctl)
{
    dsc_kif_ioctl_cmd_t * ioctl_cmd;
    dsc_cmd_t * cmd;

    /* Allocate command object */
    if ((cmd = dsc_kif_cmd_alloc()) == NULL) {
        dsc_log_err("post_ioctl_cmd: dsc_kif_cmd_alloc failed!");
        dsc_abort();
    }

    /* Allocate memory for ioctl command object */
    if ((ioctl_cmd = dsc_malloc(sizeof(dsc_kif_ioctl_cmd_t))) == NULL) {
        dsc_log_err("dsc_kif_post_ioctl_cmd: dsc_malloc failed!");
        dsc_abort();
    }

    /* Set up the ioctl command object */
    ioctl_cmd->ioctl = *iface_ioctl;
    ioctl_cmd->link = lnk;

    /* Set up the command object */
    cmd->data = ioctl_cmd;
    cmd->execute_f = dsc_kif_ioctl_cmd_exec;
    cmd->free_f = dsc_kif_ioctl_cmd_free;

    /* Post command for processing in the command thread context */
    dsc_cmdq_enq(cmd);

    return 0;
}

/*===========================================================================
  FUNCTION  dsc_kif_init_ifindex
===========================================================================*/
/*!
@brief
  Helper function to initialize all device indices.

@return
  int - 0 if successful, -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static int
dsc_kif_init_ifindex (int links[])
{
    int i;
    struct ifreq ifr;

    /* Iterate over the array of interfaces, initializing the if index for 
    ** each one. The if index is used later to determine the device
    ** corresponding to received NETLINK ROUTE messages. 
    */
    for (i = 0; i < DSC_KIF_MAX_INT; ++i) {
        /* no need to init ifindex for disabled interface*/
        if (links[i] == 0) {
            continue;
        }

        /* Call ioctl on device to get the if index */
        if (dsc_kif_call_ioctl_on_dev
            (
                dsc_kif_info[i].name, 
                SIOCGIFINDEX, 
                &ifr
            ) != 0)
        {
            dsc_log_err("Cannot get ifindex for dev %s!", dsc_kif_info[i].name);
            return -1;
        }

        /* Save if index in the interface info struct */
        dsc_kif_info[i].ifi_index = ifr.ifr_ifindex;

        dsc_log_high("dsc_kif link %d, device %s has ifindex %d", i, dsc_kif_info[i].name, ifr.ifr_ifindex);
    }

    return 0;
}

/*===========================================================================
  FUNCTION  dsc_kif_get_link_for_ifindex
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
static int 
dsc_kif_get_link_for_ifindex (int ifindex)
{
    int lnk = -1;
    int i;

    /* Iterate over the array of interfaces, and return the link id for 
    ** the interface matching the specified if index. 
    */
    for (i = 0; i < DSC_KIF_MAX_INT; ++i) {
        if (dsc_kif_info[i].enabled && dsc_kif_info[i].ifi_index == ifindex) {
            lnk = i;
            break;
        }
    }

    return lnk;
}

/*===========================================================================
  FUNCTION  dsc_kif_load_module
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
static int
dsc_kif_load_module (void)
{
    char scr_buf[DSC_KIF_FILENAME_MAX_LEN];

    /* Construct command to load the module. Use module script name if one 
    ** was specified on the command line, otherwise use modprobe to load 
    ** module assuming the module name is derived from the interface name.
    */

    memset(scr_buf, 0, DSC_KIF_FILENAME_MAX_LEN);

    if (strlen(dsc_kif_cfg.modscript) == 0) {
        (void)snprintf
        (
            scr_buf, 
            DSC_KIF_FILENAME_MAX_LEN, 
            "modprobe %s", 
            dsc_kif_cfg.name
        );
        dsc_log_high("Loading module %s", dsc_kif_cfg.name);
    } else {
        (void)snprintf
        (
            scr_buf, 
            DSC_KIF_FILENAME_MAX_LEN, 
            "%s", 
            dsc_kif_cfg.modscript
        );
        dsc_log_high
        (
            "Running module load script %s", 
            dsc_kif_cfg.modscript
        );
    }

    /* Issue command to load module */
    if (system(scr_buf) != 0) {
        return -1;
    }

    return 0;
}

/*===========================================================================
  FUNCTION  dsc_kif_nl_open_sock
===========================================================================*/
/*!
@brief
  Opens a netlink socket for the specified protocol and multicast group
  memberships.

@return
  int - 0 if socket is successfully opened, -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static int 
dsc_kif_nl_open_sock 
(
    dsc_kif_nl_sk_info_t * sk_info, 
    int proto, 
    unsigned int grps
)
{
    int rval = -1;
    int * p_sk_fd;
    struct sockaddr_nl * p_sk_addr_loc, * p_sk_addr_rem;

    p_sk_fd = &sk_info->sk_fd;
    p_sk_addr_loc = &sk_info->sk_addr_loc;
    p_sk_addr_rem = &sk_info->sk_addr_rem;

    /* Open netlink socket for specified protocol */
    if ((*p_sk_fd = socket(AF_NETLINK, SOCK_RAW, proto)) < 0) {
        dsc_log_sys_err();
        goto error;
    }

    /* Initialize socket addresses to null */
    memset(p_sk_addr_loc, 0, sizeof(struct sockaddr_nl));
    memset(p_sk_addr_rem, 0, sizeof(struct sockaddr_nl));

    /* Populate local socket address using specified groups */
    p_sk_addr_loc->nl_family = AF_NETLINK;
    p_sk_addr_loc->nl_pid = DSC_KIF_NL_PID;
    p_sk_addr_loc->nl_groups = grps;

    /* Bind socket to the local address, i.e. specified groups. This ensures
    ** that multicast messages for these groups are delivered over this 
    ** socket. 
    */
    if (bind(*p_sk_fd, (struct sockaddr *)p_sk_addr_loc, sizeof(struct sockaddr_nl)) < 0) {
        dsc_log_sys_err();
        goto error;
    }

    rval = 0;
error:
    return rval;
}

/*===========================================================================
  FUNCTION  dsc_kif_nl_recv_msg
===========================================================================*/
/*!
@brief
  Reads a complete NETLINK message incoming over the specified socket 
  descriptor and returns it. Note that the memory for the message is 
  dynamically allocated.

@return
  struct msghdr * - pointer to message if message is successfully read, 
                    NULL otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static struct msghdr * 
dsc_kif_nl_recv_msg (int fd)
{
    unsigned char * buf = NULL;
    struct sockaddr_nl * nladdr = NULL;
    struct iovec * iov = NULL;
    struct msghdr * msgh = NULL;
    int rmsgl;

    /* Allocate memory for the message header */
    if ((msgh = dsc_malloc(sizeof(struct msghdr))) == NULL) {
        dsc_abort();
    }

    /* Allocate memory for the message address structure */
    if ((nladdr = dsc_malloc(sizeof(struct sockaddr_nl))) == NULL) {
        dsc_abort();
    }

    /* Allocate memory for the io vector */
    if ((iov = dsc_malloc(sizeof(struct iovec))) == NULL) {
        dsc_abort();
    }

    /* Allocate memory for the actual message contents */
    if ((buf = dsc_malloc(1024)) == NULL) {
        dsc_abort();
    }

    /* Populate message address */
    memset(nladdr, 0, sizeof(struct sockaddr_nl));
    nladdr->nl_family = AF_NETLINK;
    nladdr->nl_pid = 0;
    nladdr->nl_groups = 0;

    /* Populate message header */
    msgh->msg_name = nladdr;
    msgh->msg_namelen = sizeof(struct sockaddr_nl);
    msgh->msg_iov = iov;
    msgh->msg_iovlen = 1;

    /* Set io vector fields */
    iov->iov_base = buf;
    iov->iov_len = 1024;

    /* Receive message over the socket */
    rmsgl = recvmsg(fd, msgh, 0);

    /* Verify that something was read */
    if (rmsgl <= 0) {
        dsc_log_err("Received nl_msg, recvmsg returned %d", rmsgl);
        dsc_log_sys_err();
        goto error;
    }

    /* Verify that address length in the received message is expected value */
    if (msgh->msg_namelen != sizeof(struct sockaddr_nl)) {
        dsc_log_err("rcvd msg with namelen != sizeof sockaddr_nl");
        goto error;
    }

    /* Verify that message was not truncated. This should not occur */
    if (msgh->msg_flags & MSG_TRUNC) {
        dsc_log_err("Rcvd msg truncated!");
        goto error;
    }

    dsc_log_low("Received nl msg, recvmsg returned %d", rmsgl);

    /* Return message ptr. Caller is responsible for freeing the memory */
    return msgh;

error:
    /* An error occurred while receiving the message. Free all memory before 
    ** returning. 
    */
    dsc_free(msgh);
    dsc_free(nladdr);
    dsc_free(iov);
    dsc_free(buf);

    return 0;
}

/*===========================================================================
  FUNCTION  dsc_kif_nl_recv_link_msg
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
static void
dsc_kif_nl_recv_link_msg (struct nlmsghdr * nlmsgh)
{
    struct ifinfomsg * ifinfo;
    int lnk;

    dsc_log_func_entry();
   
    if (nlmsgh->nlmsg_type == RTM_NEWLINK) {
        dsc_log_high("Received RTM_NEWLINK");
    } else if (nlmsgh->nlmsg_type == RTM_DELLINK) {
        dsc_log_high("Received RTM_DELLINK");
    } else {
        dsc_log_err("Unknown Netlink msg type LINK [%d], ignoring!",
                    nlmsgh->nlmsg_type);
        goto ret;
    }

    /* For a LINK message, data is a ptr to struct ifinfomsg type */
    ifinfo = (struct ifinfomsg *) NLMSG_DATA(nlmsgh);

    dsc_log_high("ifi_index [%d], ifi_change [0x%x], ifi_flags [0x%x]", 
                 ifinfo->ifi_index, ifinfo->ifi_change, ifinfo->ifi_flags);

    /* Get link id for the device using the if index indicated in the msg */
    if ((lnk = dsc_kif_get_link_for_ifindex(ifinfo->ifi_index)) < 0) {
        /* Could not get link id. This is probably an interface that we don't 
        ** care about. Ignore message. 
        */
        dsc_log_err("unrecognized ifindex %d", ifinfo->ifi_index);
        dsc_log_func_exit();
        return;
    }

    /* Process message if there is a change in the state of the interface */
    if (ifinfo->ifi_change & IFF_UP) {
        if (ifinfo->ifi_flags & IFF_UP) {
            /* Interface came up. Process event based on current state */
            dsc_log_high("interface up");
            dsc_kif_open_cnf(lnk);

            /* Print some debug messages */
            if (nlmsgh->nlmsg_type == RTM_NEWLINK) {
                dsc_log_high("RTM_NEWLINK rcvd with interface up");
            } else {
                dsc_log_high("not RTM_NEWLINK!");
            }
        } else {
            /* Interface went down. Process event based on current state */
            dsc_log_high("interface down");
            dsc_kif_close_cnf(lnk);

            /* Print some debug messages */
            if (nlmsgh->nlmsg_type == RTM_DELLINK) {
                dsc_log_high("RTM_DELLINK rcvd with interface down");
            } else {
                dsc_log_high("not RTM_DELLINK!");
            }
        }
    }

ret:
    dsc_log_func_exit();
    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_nl_recv_addr_msg
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
static void
dsc_kif_nl_recv_addr_msg (struct nlmsghdr * nlmsgh)
{
    struct ifaddrmsg * ifaddr;
        int lnk;

    dsc_log_func_entry();

    /* For a ADDR message, data is a ptr to struct ifaddrmsg type */
    ifaddr = (struct ifaddrmsg *) NLMSG_DATA(nlmsgh);

    dsc_log_high("recv_addr_msg: ifa_index = %d", ifaddr->ifa_index);
    dsc_log_high("recv_addr_msg: ifa_family = %d", ifaddr->ifa_family);
    dsc_log_high("recv_addr_msg: ifa_prefixlen = %d", ifaddr->ifa_prefixlen);
    dsc_log_high("recv_addr_msg: ifa_flags = %x", ifaddr->ifa_flags);
    dsc_log_high("recv_addr_msg: ifa_scope = %d", ifaddr->ifa_scope);

    /* Get link id for the device using the if index indicated in the msg */
    if ((lnk = dsc_kif_get_link_for_ifindex(ifaddr->ifa_index)) < 0) {
        /* Could not get link id. This is probably an interface that we don't 
        ** care about. Ignore message. 
        */
        dsc_log_err("unrecognized ifindex %d", ifaddr->ifa_index);
        dsc_log_func_exit();
        return;
    }

    /* Process based on message type */
    if (nlmsgh->nlmsg_type == RTM_NEWADDR) {
        /* A new address was configured on the interface. Process based on 
        ** the current state of the interface. 
        */
        dsc_log_high("address configuration complete");
        dsc_kif_cfg_cnf(lnk);
    } else if (nlmsgh->nlmsg_type == RTM_DELADDR) {
        /* An address was deleted from the interface. Process based on the 
        ** current state of the interface. 
        */
        dsc_log_high("address configuration released");
        dsc_kif_cfg_fail(lnk);
    } else {
        dsc_log_err("Unknown Netlink msg type [%d], ignoring!", 
                    nlmsgh->nlmsg_type);
    }
    dsc_log_func_exit();
    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_cmd_init
===========================================================================*/
/*!
@brief
  Initializes the command buffers used by Kernel IF.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static void 
dsc_kif_cmd_init (void)
{
    /* Initialize the mutex used for protecting command list enqueue and 
    ** dequeue operations. 
    */
    (void)pthread_mutex_init(&dsc_kif_cmd_ctrl.mutx, NULL);

    /* set initial count of command to zero */
    dsc_kif_cmd_ctrl.num = 0;

    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_cmd_alloc
===========================================================================*/
/*!
@brief
  Allocates a command buffer from the heap.

@return
  dsc_cmd_t * - pointer to buffer is one is available, NULL otherwise

@note
  These buffers are not maintained on a dedicated list; rather, a counter
  is maintained to keep the check on the upper bound.

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_cmd_t *
dsc_kif_cmd_alloc (void)
{
    dsc_log_func_entry();

    dsc_cmd_t * cmd = NULL;

    if((cmd = dsc_malloc(sizeof(dsc_cmd_t))) == NULL ) {
        dsc_log_err("dsc_kif_cmd_alloc: dsc_malloc failed\n");
        dsc_abort();
    }

    /* acquire mutex to protect the counter */
    ds_assert(pthread_mutex_lock(&dsc_kif_cmd_ctrl.mutx) == 0);

    /* increment num of outstanding commands */
    if (++dsc_kif_cmd_ctrl.num > DSC_KIF_MAX_CMD) {
        dsc_log_high("dsc_kif_cmd_alloc: num of outstanding"
                     "commands %d exceeded max limit of %d\n",
                     dsc_kif_cmd_ctrl.num, DSC_KIF_MAX_CMD);
    }

    /* release mutex */
    ds_assert(pthread_mutex_unlock(&dsc_kif_cmd_ctrl.mutx) == 0);

    dsc_log_func_exit();

    return cmd;
}


/*===========================================================================
  FUNCTION  dsc_kif_cmd_free
===========================================================================*/
/*!
@brief
  Returns specified command buffer to the heap.

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_kif_cmd_free (dsc_cmd_t * cmd)
{
    dsc_log_func_entry();

    ds_assert(cmd != NULL);

    /* Reset command's data ptr and handle function ptrs */
    cmd->data = NULL;
    cmd->execute_f = NULL;
    cmd->free_f = NULL;

    /* acquire mutex to protect the counter */
    ds_assert(pthread_mutex_lock(&dsc_kif_cmd_ctrl.mutx) == 0);

    /* decrement num count */
    --dsc_kif_cmd_ctrl.num;

    /* release mutex */
    ds_assert(pthread_mutex_unlock(&dsc_kif_cmd_ctrl.mutx) == 0);

    dsc_free(cmd);

    dsc_log_func_exit();

    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_nl_msg_recv_cmd_exec
===========================================================================*/
/*!
@brief
  Virtual function called by the Command Thread to execute KIF command
  to process a received NETLINK message.

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_kif_nl_msg_recv_cmd_exec (dsc_cmd_t * cmd, void * data)
{
    struct msghdr * msgh;
    struct nlmsghdr * nlmsgh;
    (void)cmd;
    
    dsc_log_func_entry();

    /* Get message ptr from the user data ptr */
    msgh = (struct msghdr *)data;

    /* Get netlink message ptr from the message header */
    nlmsgh = (struct nlmsghdr *)(msgh->msg_iov->iov_base);

    dsc_log_high("Rcvd Netlink msg type [%d]", nlmsgh->nlmsg_type);

    /* Process based on netlink message type */
    switch (nlmsgh->nlmsg_type) {
    case RTM_NEWLINK:
    case RTM_DELLINK:
        /* Process NETLINK_ROUTE message of type LINK */
        dsc_log_high("Received NEWLINK/DELLINK");
        dsc_kif_nl_recv_link_msg(nlmsgh);
        break;
    case RTM_NEWADDR:
    case RTM_DELADDR:
        /* Process NETLINK_ROUTE message of type ADDR */
        dsc_log_high("Received NEWADDR/DELADDR");
        dsc_kif_nl_recv_addr_msg(nlmsgh);
        break;
    default:
        /* Ignore all other message types */
        dsc_log_err("received unknown nl msg");
        break;
    }

    dsc_log_func_exit();
    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_nl_msg_recv_cmd_free
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
static void
dsc_kif_nl_msg_recv_cmd_free (dsc_cmd_t * cmd, void * data)
{
    dsc_log_func_entry();
    struct msghdr * msgh;
    struct nlmsghdr * nlmsgh;

    /* Get message ptr from the user data ptr */
    msgh = (struct msghdr *)data;

    /* Get netlink message ptr from the message header */
    nlmsgh = (struct nlmsghdr *)(msgh->msg_iov->iov_base);

    /* Deallocate memory for the address structure */
    dsc_free(msgh->msg_name);

    /* Deallocate memory for the message buffer */
    dsc_free(nlmsgh);

    /* Deallocate memory for the io vector */
    dsc_free(msgh->msg_iov);

    /* Finally deallocate memory for the message header */
    dsc_free(msgh);

    /* Free command */
    dsc_kif_cmd_free(cmd);
    dsc_log_func_exit();
    return;
}

/*===========================================================================
  FUNCTION  dsc_kif_free_msgh
===========================================================================*/
/*!
@brief
  Function to free msgh(net link message)

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void
dsc_kif_free_msgh(struct msghdr * msgh)
{
  struct nlmsghdr * nlmsgh;

  dsc_log_func_entry();

  if(msgh)
  {
    /* Deallocate memory for the address structure */
    if (msgh->msg_name)
    {
      dsc_free(msgh->msg_name);
    }

    if(msgh->msg_iov)
    {
      /* Get netlink message ptr from the message header */
      nlmsgh = (struct nlmsghdr *)(msgh->msg_iov->iov_base);

      /* Deallocate memory for the message buffer */
      if (nlmsgh)
      {
        dsc_free(nlmsgh);
      }

      /* Deallocate memory for the io vector */
      dsc_free(msgh->msg_iov);
    }

    /* Finally deallocate memory for the message header */
    dsc_free(msgh);
  }
  else
  {
    dsc_log_err("dsc_kif_free_msgh: Bad input received(NULL input)!");
  }

  dsc_log_func_exit();
  return;
}


/*===========================================================================
  FUNCTION  dsc_kif_nl_recv_routing_msg
===========================================================================*/
/*!
@brief
  Virtual function registered with the socket listener thread to receive
  incoming messages over the NETLINK routing socket.

@return
  void

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void 
dsc_kif_nl_recv_routing_msg (int fd)
{
    struct msghdr * msgh;
    struct nlmsghdr * nlmsgh;
    dsc_cmd_t * cmd;
    struct ifinfomsg * ifinfo;
    struct ifaddrmsg * ifaddr;
    int lnk;
    dsc_log_func_entry();


    /* Read netlink message from the socket */
    if ((msgh = dsc_kif_nl_recv_msg(fd)) == NULL) {
        dsc_log_err("dsc_kif_nl_recv_msg failed!");
        dsc_abort();
    }

    /* Get netlink message ptr from the message header */
    nlmsgh = (struct nlmsghdr *)(msgh->msg_iov->iov_base);

    /* Check to see if we are interested in the NL message */
    switch (nlmsgh->nlmsg_type) {
    case RTM_NEWLINK:
    case RTM_DELLINK:
        /* Process NETLINK_ROUTE message of type LINK */
        ifinfo = (struct ifinfomsg *) NLMSG_DATA(nlmsgh);
        dsc_log_high("received link message type");

        /* Get link id for the device using the if index indicated in the msg */
        if ((lnk = dsc_kif_get_link_for_ifindex(ifinfo->ifi_index)) < 0) 
        {
          /* Could not get link id. This is probably an interface that we don't 
          ** care about. Ignore message. 
          */

          dsc_log_err("unrecognized ifindex %d", ifinfo->ifi_index);
          dsc_kif_free_msgh(msgh);
          dsc_log_func_exit();
          return;
        }

        break;
    case RTM_NEWADDR:
    case RTM_DELADDR:
        /* Process NETLINK_ROUTE message of type ADDR */
        ifaddr = (struct ifaddrmsg *) NLMSG_DATA(nlmsgh);
        dsc_log_high("received add msg type");
        /* Get link id for the device using the if index indicated in the msg */
        if ((lnk = dsc_kif_get_link_for_ifindex(ifaddr->ifa_index)) < 0) {
          /* Could not get link id. This is probably an interface that we don't 
          ** care about. Ignore message. 
          */
          dsc_log_err("unrecognized ifindex %d", ifaddr->ifa_index);
          dsc_kif_free_msgh(msgh);
          dsc_log_func_exit();
          return;
        }
        break;
    default:
        /* Ignore all other message types */
        dsc_log_err("received unknown nl msg");
        dsc_kif_free_msgh(msgh);
        dsc_log_func_exit();
        return;
    }

    /* Allocate command object */
    if ((cmd = dsc_kif_cmd_alloc()) == NULL) {
        dsc_log_err("dsc_kif_cmd_alloc failed!");
        dsc_abort();
    }
    /* Set data ptr in the command object to the netlink message header ptr */
    cmd->data = msgh;

    /* Set handler function ptrs for the command object */
    cmd->execute_f = dsc_kif_nl_msg_recv_cmd_exec;
    cmd->free_f = dsc_kif_nl_msg_recv_cmd_free;

    /* Post command for processing in the command thread context */
    dsc_cmdq_enq(cmd);
    dsc_log_func_exit();
    return;
}

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_kif_reconfigure
===========================================================================*/
/*!
@brief
  API to reconfigure virtual Ethernet interface for the specified link.

@return
  int - 0 if successful
       -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - DHCP discover is done
*/
/*=========================================================================*/
int dsc_kif_reconfigure (int link)
{
    int ret = -1;
    dsc_kif_state_t state;

    /* following do..while loop can be used in lieu of 
       the infamous goto error approach */
    do 
    {
        /* Verify that the specified link id is valid */
        if (dsc_kif_verify_link(link) < 0) {
            dsc_log_err("dsc_kif_open_cnf called with invalid link %d", link);
            break;
        }

        /* Process based on current interface state */
        switch (state = dsc_kif_get_state(link)) {
        case DSC_KIF_OPEN:
            dsc_kif_cfg_req(link);
            dsc_kif_set_state(link, DSC_KIF_RECONFIGURING);
            ret = 0;
            break; /* breaks from switch */
        default:
            dsc_log_err("dsc_kif_reconfigure not allowed in state %d",
                        state);
            break;
        }

        /* current assumption is that we always come out of 
         * do..while after this switch stmt is executed 
         * so, if you plan to add more code here, make sure 
         * it is enclosed in if (ret == desired_value) block.*/

    } while(0);

    return ret;
}

/*===========================================================================
  FUNCTION  dsc_kif_open
===========================================================================*/
/*!
@brief
  API to bring up virtual Ethernet interface for the specified link. Once
  interface is up, the associated client callback is called. 

@return
  int - 0 if command to bring up interface is successfully issued, 
        -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
int 
dsc_kif_open (int lnk, const dsc_kif_clntcb_t * clntcb, void * clnt_hdl)
{
    int rval = -1;
    dsc_kif_state_t state;

    /* Verify the link id first */
    if (dsc_kif_verify_link(lnk) < 0) {
        dsc_log_err("dsc_kif_open called with invalid link %d", lnk);
        goto error;
    }

    /* Process based on current interface state */
    switch (state = dsc_kif_get_state(lnk)) {
    case DSC_KIF_CLOSED:
        /* Interface is currently closed. Issue command to bring interface up
        ** and transition to the opening state.
        */
        dsc_kif_open_req(lnk);
        dsc_kif_set_clntcb(lnk, clntcb);
        dsc_kif_set_clnt_hdl(lnk, clnt_hdl);
        dsc_kif_set_state(lnk, DSC_KIF_OPENING);
        break;
    default:
        /* Ignore open request in all other states */
        dsc_log_err("dsc_kif_open called in state %d", state);
        goto error;
    }

    rval = 0;

error:
    return rval;
}

/*===========================================================================
  FUNCTION  dsc_kif_close
===========================================================================*/
/*!
@brief
  API to bring down virtual Ethernet interface for the specified link. Once
  interface is down, the associated client callback is called. 

@return
  int - 0 if command to bring down interface is successfully issued, 
        -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
int 
dsc_kif_close (int lnk)
{
    int rval = -1;
    dsc_kif_state_t state;

    /* Verify the link id first */
    if (dsc_kif_verify_link(lnk) < 0) {
        dsc_log_err("dsc_kif_close called with invalid link %d", lnk);
        goto error;
    }

    /* Process based on current interface state */
    switch (state = dsc_kif_get_state(lnk)) {
    case DSC_KIF_OPEN:
        /* Interface is in open state. Instruct dhcp client to release address
        ** and transition to the deconfiguring state.
        */
#ifndef FEATURE_DSS_LINUX_ANDROID
        dsc_kif_dhcp_client_release_lease(lnk);
                dsc_kif_set_state(lnk, DSC_KIF_DECONFIGURING);
#else
        dsc_kif_close_req(lnk);
        dsc_kif_set_state(lnk, DSC_KIF_CLOSING);
#endif
                break;
    case DSC_KIF_CONFIGURING:
    case DSC_KIF_RECONFIGURING: 
        /* Interface is in the (re)configuring state. Terminate dhcp
        ** client and issue command to bring interface down, and
        ** transition to the closing state.
        */
#ifndef FEATURE_DSS_LINUX_ANDROID
        dsc_kif_stop_dhcp_client(lnk);
#endif
        dsc_kif_close_req(lnk);
        dsc_kif_set_state(lnk, DSC_KIF_CLOSING);
        break;
    case DSC_KIF_OPENING:
        /* Interface is in opening state. Issue command to bring interface down 
        ** and transition to the closing state.
        */
        dsc_kif_close_req(lnk);
        dsc_kif_set_state(lnk, DSC_KIF_CLOSING);
        break;
    default:
        /* Ignore close request in all other states */
        dsc_log_err("dsc_kif_close called in state %d", state);
        goto error;
    }

    rval = 0;

error:
    return rval;
}

/*===========================================================================
  FUNCTION  dsc_kif_ioctl
===========================================================================*/
/*!
@brief
  Generic IOCTL handler of the KIF module. 

@return
  int - 0 if IOCTL is successfully processed, -1 otherwise

@note

  - Dependencies
    - None 

  - Side Effects
    - None
*/
/*=========================================================================*/
int 
dsc_kif_ioctl (int lnk, dsc_dcm_iface_ioctl_t * iface_ioctl)
{
    int rval = -1;
    struct ifreq ifr;
    unsigned int req;
    const char * dev;

    /* Verify the link id first */
    if (dsc_kif_verify_link(lnk) < 0) {
        dsc_log_err("dsc_kif_ioctl called with invalid link %d", lnk);
        goto error;
    }

    /* Get device name for the associated link on which the ioctl was issued */
    dev = dsc_kif_get_name(lnk);

    /* Process based on ioctl type */
    switch (iface_ioctl->name) {
    case DSS_IFACE_IOCTL_BIND_SOCK_TO_IFACE:
        /* Special case for this IOCTL, where it must be processed in the 
        ** command thread context. This way, processing of this command for 
        ** multiple clients is serialized. 
        */
        return dsc_kif_post_ioctl_cmd(lnk, iface_ioctl);
    case DSS_IFACE_IOCTL_GET_DEVICE_NAME:
        /* Return device name */
        (void)strlcpy
              (
                  iface_ioctl->info.device_name_info.device_name, 
                  dev,
                  DSS_MAX_DEVICE_NAME_LEN
              );
        return 0;
    case DSS_IFACE_IOCTL_GET_IPV4_ADDR:
        /* Set system ioctl to get interface address */
        req = SIOCGIFADDR;
        break;
    default:
        dsc_log_err("dsc_kif_ioctl: invalid ioctl %d called\n", 
                    iface_ioctl->name);
        goto error;
    }

    /* Call linux system ioctl on the device to perform the requested 
    ** operation. 
    */
    if (dsc_kif_call_ioctl_on_dev(dev, req, &ifr) < 0) {
        goto error;
    }

    /* Process result based on ioctl type */
    switch (iface_ioctl->name) {
    case DSS_IFACE_IOCTL_GET_IPV4_ADDR:
        /* Copy ip address in the ioctl struct */
        iface_ioctl->info.ipv4_addr.type = IPV4_ADDR;
        iface_ioctl->info.ipv4_addr.addr.v4 = 
            ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
        break;
    default:
        /* We should not get here as we already checked earlier for unknown 
        ** ioctl type. Abort for debug purposes. 
        */
        dsc_abort();
    }

    rval = 0;

error:
    return rval;
}

/*===========================================================================
  FUNCTION  dsc_kif_init
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
dsc_kif_init 
(
    int nint,
    int links[],
    char * iname, 
    int skip, 
    char * dirpath, 
    char * modscript, 
    char * dhcpscript
)
{
    int i;
    short flags;

    /* Make sure number of interfaces specified on the command line is in the 
    ** valid range, otherwise use default configuration of maximum interfaces 
    ** supported. 
    */
    if ((nint <= 0) || (nint > DSC_KIF_MAX_INT)) {
        dsc_log_err("nint range check fails. programming error, aborting");
        dsc_abort();
    }

    /* Set number of interfaces in the configuration blob */
    dsc_kif_cfg.nint = nint;

    dsc_log_high("dsc_kif_init: set dsc_kif_cfg.nint to %d", nint);

    /* Set 'skip' flag. This indicates if the ethernet driver module should 
    ** be loaded or not. This functionality is strictly for debugging purposes.
    */
    dsc_kif_cfg.skip = skip;

    /* Initialize module load script name and dhcp script names */
    dsc_kif_init_paths(dirpath, modscript, dhcpscript);

    /* Set udhcpc pid file name in configuration blob */
    (void)strlcpy
    (
        dsc_kif_cfg.pidfile, 
        DSC_KIF_PIDFILE_DEF_NAME, 
        DSC_KIF_FILENAME_MAX_LEN
    );

    /* Initialize device names */
    dsc_kif_init_names(iname, links);

    /* Iterate over the array of interface info, initializing each one */
    for (i = 0; i < DSC_KIF_MAX_INT; ++i) {

        /* skip if this interface is not used */
        if (links[i] == 0)
        {
            continue;
        }

        /* close the interface if open */
        flags = 0;
        if (dsc_kif_ifioctl_get(dsc_kif_info[i].name, &flags) < 0) {
            dsc_log_err("dsc_kif_init: dsc_kif_ifioctl_get() failed");
            dsc_abort();
        }

        if (flags & IFF_UP) {
            dsc_log_high("kernel interface %d found open at init", i);
            dsc_log_high("kernel interface %d will be closed at init", i);
            dsc_kif_close_req(i);
        }

        /* Initialize interface state to closed */
        dsc_kif_info[i].state = DSC_KIF_CLOSED;

        /* Initialize client callback struct ptr to null */
        dsc_kif_info[i].clntcb = NULL;
    }

    /* Load the module, if skip flag is not set */
    if ((skip != DSC_KIF_SKIP) && (dsc_kif_load_module() < 0)) {
        dsc_log_err("cannot load kernel module");
        dsc_abort();
    }

    /* Initialize device if indices. Of course, this must be done after the 
    ** devices are created, i.e. the module is loaded. 
    */
    if (dsc_kif_init_ifindex(links) < 0) {
        dsc_log_err("cannot init ifindex");
        dsc_abort();
    }

    /* Initialize data structures related to command processing */
    dsc_kif_cmd_init();

    /* Initialize socket listener thread. This thread is used to listen for 
    ** incoming messages on all netink sockets. 
    */
    if (dsc_socklthrd_init
        (
            &dsc_kif_sk_info.sk_thrd_hdl, 
            dsc_kif_sk_info.sk_thrd_fdmap, 
            DSC_ARRSIZE(dsc_kif_sk_info.sk_thrd_fdmap)
        ) < 0) 
    {  
        dsc_log_err("cannot init sock listener thread");
        dsc_abort();
    }

    /* Open a netlink socket for NETLINK_ROUTE protocol. This socket is used 
    ** for receiving netlink messaging related to interface state and address
    ** configuration events.
    */
    if (dsc_kif_nl_open_sock(
            &dsc_kif_sk_info.rt_sk, 
            NETLINK_ROUTE,
            RTMGRP_LINK | RTMGRP_IPV4_IFADDR
        ) < 0)
    {
        dsc_log_err("cannot open nl routing sock");
        dsc_abort();
    }

    /* Add the NETLINK_ROUTE socket to the list of sockets that the listener 
    ** thread should listen on. 
    */
    if (dsc_socklthrd_addfd
        (
            &dsc_kif_sk_info.sk_thrd_hdl, 
            dsc_kif_sk_info.rt_sk.sk_fd, 
            dsc_kif_nl_recv_routing_msg
        ) < 0) 
    {
        dsc_log_err("cannot add nl routing sock for reading");
        dsc_abort();
    }

    /* Start the socket listener thread */
    if (dsc_socklthrd_start(&dsc_kif_sk_info.sk_thrd_hdl) < 0) {
        dsc_log_err("cannot start sock listener thread");
        dsc_abort();
    }

    return;
}
