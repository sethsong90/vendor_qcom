/******************************************************************************

                        N E T M G R _ M A I N . C

******************************************************************************/

/******************************************************************************

  @file    netmgr_main.c
  @brief   Network Manager main function implementation

  DESCRIPTION
  Implementation of NetMgr's main function.

******************************************************************************/
/*===========================================================================

  Copyright (c) 2010-2013 Qualcomm Technologies, Inc. All Rights Reserved

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
02/14/11   jas        change netmgrd uid to radio at power up
02/08/10   ar         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h> /* open, read */
#include <signal.h>
#include <linux/capability.h>
#include <strings.h>
#ifdef FEATURE_DS_LINUX_ANDROID
#include <cutils/properties.h>
#include <private/android_filesystem_config.h>
#endif

#include "ds_util.h"
#include "ds_string.h"
#include "netmgr_util.h"
#include "netmgr_defs.h"
#include "netmgr_exec.h"
#include "netmgr_kif.h"
#include "netmgr_qmi.h"
#include "netmgr_tc.h"
#include "netmgr_platform.h"
#include "netmgr_main.h"

#ifdef NETMGR_TEST
#include "netmgr_test.h"
#endif

/*===========================================================================
                     FORWARD DECLARATIONS
===========================================================================*/


/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

#define NETMGR_MAIN_DEFAULT_NINT         NETMGR_MAX_LINK

#ifdef FEATURE_DS_LINUX_ANDROID
/* Macros from Android property database */
#define NETMGR_MAIN_PROPERTY_NINT        "persist.data_netmgrd_nint"
#define NETMGR_MAIN_PROPERTY_BASEBAND    "ro.baseband"
#define NETMGR_MAIN_PROPERTY_NINT_SIZE   NETMGR_MAX_LINK
#define NETMGR_MAIN_PROPERTY_BASEBAND_SIZE   10
#define NETMGR_MAIN_BASEBAND_VALUE_MSM       "msm"
#define NETMGR_MAIN_BASEBAND_VALUE_APQ       "apq"
#define NETMGR_MAIN_BASEBAND_VALUE_SVLTE1    "svlte1"
#define NETMGR_MAIN_BASEBAND_VALUE_SVLTE2A   "svlte2a"
#define NETMGR_MAIN_BASEBAND_VALUE_SGLTE     "sglte"
#define NETMGR_MAIN_BASEBAND_VALUE_DSDA      "dsda"
#define NETMGR_MAIN_BASEBAND_VALUE_DSDA2     "dsda2"
#define NETMGR_MAIN_BASEBAND_VALUE_CSFB      "csfb"
#define NETMGR_MAIN_BASEBAND_VALUE_MDMUSB    "mdm"
#define NETMGR_MAIN_BASEBAND_VALUE_UNDEFINED "undefined"

#define NETMGR_MAIN_PROPERTY_QOS          "persist.data.netmgrd.qos.enable"
#define NETMGR_MAIN_PROPERTY_QOS_SIZE     (5)
#define NETMGR_MAIN_PROPERTY_QOS_DEFAULT  NETMGR_FALSE    /* true or false */

#ifdef FEATURE_DATA_IWLAN
  #define NETMGR_MAIN_PROPERTY_IWLAN          "persist.data.iwlan.enable"
  #define NETMGR_MAIN_PROPERTY_IWLAN_SIZE     (5)
  #define NETMGR_MAIN_PROPERTY_IWLAN_DEFAULT  NETMGR_FALSE    /* true or false */

  #define NETMGR_MAIN_PROPERTY_IWLAN_IMS          "persist.data.iwlan.ims.enable"
  #define NETMGR_MAIN_PROPERTY_IWLAN_IMS_SIZE     (5)
  #define NETMGR_MAIN_PROPERTY_IWLAN_IMS_DEFAULT  NETMGR_TRUE    /* true or false */
#endif /* FEATURE_DATA_IWLAN */

#define NETMGR_MAIN_PROPERTY_TCPACKPRIO          "persist.data.tcpackprio.enable"
#define NETMGR_MAIN_PROPERTY_TCPACKPRIO_DEFAULT  NETMGR_FALSE    /* true or false */

#endif /* FEATURE_DS_LINUX_ANDROID */

#ifdef FEATURE_DATA_IWLAN
  #define NETMGR_MAIN_GET_INST_MIN(MODEM)                         \
    netmgr_main_dev_prefix_tbl[MODEM][NETMGR_FWD_LINK].inst_min
  #define NETMGR_MAIN_GET_INST_MAX(MODEM)                         \
    netmgr_main_dev_prefix_tbl[MODEM][NETMGR_FWD_LINK].inst_max
  #define NETMGR_MAIN_GET_INST_MIN_REV(MODEM)                     \
    netmgr_main_dev_prefix_tbl[MODEM][NETMGR_REV_LINK].inst_min
  #define NETMGR_MAIN_GET_INST_MAX_REV(MODEM)                     \
    netmgr_main_dev_prefix_tbl[MODEM][NETMGR_REV_LINK].inst_max
  #define NETMGR_MAIN_GET_DEV_PREFIX(MODEM)                       \
    netmgr_main_dev_prefix_tbl[MODEM][NETMGR_FWD_LINK].prefix
#else
  #define NETMGR_MAIN_GET_INST_MIN(MODEM)                         \
    netmgr_main_dev_prefix_tbl[MODEM].inst_min
  #define NETMGR_MAIN_GET_INST_MAX(MODEM)                         \
    netmgr_main_dev_prefix_tbl[MODEM].inst_max
  #define NETMGR_MAIN_GET_DEV_PREFIX(MODEM)                       \
    netmgr_main_dev_prefix_tbl[MODEM].prefix
#endif /* FEATURE_DATA_IWLAN */

/* Default network interface name prefix; may be overridden */
LOCAL char netmgr_iname_default[] = "rmnet";

/*---------------------------------------------------------------------------
   Program configuration info
---------------------------------------------------------------------------*/
struct netmgr_main_cfg_s netmgr_main_cfg = {
  NETMGR_MAIN_RUNMODE_DEFAULT, DS_LOG_MODE_DFLT, -1, -1,
  netmgr_iname_default,
  NETMGR_KIF_SKIP, NULL, NULL, FALSE, FALSE, FALSE,
  NETMGR_LINK_RMNET_0
#ifdef FEATURE_DATA_IWLAN
  ,FALSE
  ,FALSE
#endif /* FEATURE_DATA_IWLAN */
  ,FALSE
}; /* Initialize everything to invalid values */

/* one of these files may hold the data control port string */
#define NETMGR_SYSFS_CONFIG_FILE_1 "/sys/module/f_rmnet/parameters/rmnet_ctl_ch"
#define NETMGR_SYSFS_CONFIG_FILE_2 "/sys/module/rmnet/parameters/rmnet_ctl_ch"

/* Array of strings used to identify which port is used by RmNET.
 * Order of strings is important as we use index of the string element
 * internally. First field is for USB driver port match, which may be
 * empty string if unused */
LOCAL netmgr_ctl_port_config_type
netmgr_ctl_port_array[NETMGR_MAX_LINK+1] =
{
  /* SMD/BAM transport */
  {"DATA5_CNTL",  QMI_PORT_RMNET_0,         NETMGR_LINK_RMNET_0,      TRUE,  TRUE},
  {"DATA6_CNTL",  QMI_PORT_RMNET_1,         NETMGR_LINK_RMNET_1,      FALSE, TRUE},
  {"DATA7_CNTL",  QMI_PORT_RMNET_2,         NETMGR_LINK_RMNET_2,      FALSE, TRUE},
  {"DATA8_CNTL",  QMI_PORT_RMNET_3,         NETMGR_LINK_RMNET_3,      FALSE, TRUE},
  {"DATA9_CNTL",  QMI_PORT_RMNET_4,         NETMGR_LINK_RMNET_4,      FALSE, TRUE},
  {"DATA12_CNTL", QMI_PORT_RMNET_5,         NETMGR_LINK_RMNET_5,      FALSE, TRUE},
  {"DATA13_CNTL", QMI_PORT_RMNET_6,         NETMGR_LINK_RMNET_6,      FALSE, TRUE},
  {"DATA14_CNTL", QMI_PORT_RMNET_7,         NETMGR_LINK_RMNET_7,      FALSE, TRUE},

  /* SDIO/USB transport */
  {"MDM0_CNTL",   QMI_PORT_RMNET_SDIO_0,    NETMGR_LINK_RMNET_8,      TRUE,  TRUE},
  {"MDM1_CNTL",   QMI_PORT_RMNET_SDIO_1,    NETMGR_LINK_RMNET_9,      FALSE, TRUE},
  {"MDM2_CNTL",   QMI_PORT_RMNET_SDIO_2,    NETMGR_LINK_RMNET_10,     FALSE, TRUE},
  {"MDM3_CNTL",   QMI_PORT_RMNET_SDIO_3,    NETMGR_LINK_RMNET_11,     FALSE, TRUE},
  {"MDM4_CNTL",   QMI_PORT_RMNET_SDIO_4,    NETMGR_LINK_RMNET_12,     FALSE, TRUE},
  {"MDM5_CNTL",   QMI_PORT_RMNET_SDIO_5,    NETMGR_LINK_RMNET_13,     FALSE, TRUE},
  {"MDM6_CNTL",   QMI_PORT_RMNET_SDIO_6,    NETMGR_LINK_RMNET_14,     FALSE, TRUE},
  {"MDM7_CNTL",   QMI_PORT_RMNET_SDIO_7,    NETMGR_LINK_RMNET_15,     FALSE, TRUE},

#ifdef FEATURE_DATA_IWLAN
  /* SMD/BAM reverse transport */
  {"DATA23_CNTL", QMI_PORT_REV_RMNET_0,     NETMGR_LINK_REV_RMNET_0,  FALSE, TRUE},
  {"DATA24_CNTL", QMI_PORT_REV_RMNET_1,     NETMGR_LINK_REV_RMNET_1,  FALSE, TRUE},
  {"DATA25_CNTL", QMI_PORT_REV_RMNET_2,     NETMGR_LINK_REV_RMNET_2,  FALSE, TRUE},
  {"DATA26_CNTL", QMI_PORT_REV_RMNET_3,     NETMGR_LINK_REV_RMNET_3,  FALSE, TRUE},
  {"DATA27_CNTL", QMI_PORT_REV_RMNET_4,     NETMGR_LINK_REV_RMNET_4,  FALSE, TRUE},
  {"DATA28_CNTL", QMI_PORT_REV_RMNET_5,     NETMGR_LINK_REV_RMNET_5,  FALSE, TRUE},
  {"DATA29_CNTL", QMI_PORT_REV_RMNET_6,     NETMGR_LINK_REV_RMNET_6,  FALSE, TRUE},
  {"DATA30_CNTL", QMI_PORT_REV_RMNET_7,     NETMGR_LINK_REV_RMNET_7,  FALSE, TRUE},
  {"DATA31_CNTL", QMI_PORT_REV_RMNET_8,     NETMGR_LINK_REV_RMNET_8,  FALSE, TRUE},

  /* USB reverse transport */
  {"RMDM0_CNTL",  QMI_PORT_REV_RMNET_USB_0, NETMGR_LINK_REV_RMNET_9,  FALSE, TRUE},
  {"RMDM1_CNTL",  QMI_PORT_REV_RMNET_USB_1, NETMGR_LINK_REV_RMNET_10, FALSE, TRUE},
  {"RMDM2_CNTL",  QMI_PORT_REV_RMNET_USB_2, NETMGR_LINK_REV_RMNET_11, FALSE, TRUE},
  {"RMDM3_CNTL",  QMI_PORT_REV_RMNET_USB_3, NETMGR_LINK_REV_RMNET_12, FALSE, TRUE},
  {"RMDM4_CNTL",  QMI_PORT_REV_RMNET_USB_4, NETMGR_LINK_REV_RMNET_13, FALSE, TRUE},
  {"RMDM5_CNTL",  QMI_PORT_REV_RMNET_USB_5, NETMGR_LINK_REV_RMNET_14, FALSE, TRUE},
  {"RMDM6_CNTL",  QMI_PORT_REV_RMNET_USB_6, NETMGR_LINK_REV_RMNET_15, FALSE, TRUE},
  {"RMDM7_CNTL",  QMI_PORT_REV_RMNET_USB_7, NETMGR_LINK_REV_RMNET_16, FALSE, TRUE},
  {"RMDM8_CNTL",  QMI_PORT_REV_RMNET_USB_8, NETMGR_LINK_REV_RMNET_17, FALSE, TRUE},
#endif /* FEATURE_DATA_IWLAN */

  /* Must be last record for validation */
  {"",            "",                       NETMGR_LINK_MAX,          FALSE, FALSE}
};

/* Table of transport device name prefix per Modem */
#ifdef FEATURE_DATA_IWLAN
  netmgr_main_dev_prefix_type netmgr_main_dev_prefix_tbl[NETMGR_MAX_MODEMS][NETMGR_MAX_LINK_TYPES] =
  {
    {
      { NETMGR_MAIN_RMNET_SMD_PREFIX,      NETMGR_LINK_RMNET_0,      NETMGR_LINK_RMNET_7      },
      { NETMGR_MAIN_REV_RMNET_SMD_PREFIX,  NETMGR_LINK_REV_RMNET_0,  NETMGR_LINK_REV_RMNET_8  },
    },
    {
      { NETMGR_MAIN_RMNET_SDIO_PREFIX,     NETMGR_LINK_RMNET_8,      NETMGR_LINK_RMNET_15     },
      { NETMGR_MAIN_REV_RMNET_USB_PREFIX,  NETMGR_LINK_REV_RMNET_9,  NETMGR_LINK_REV_RMNET_17 },
    }
  };
#else
  netmgr_main_dev_prefix_type netmgr_main_dev_prefix_tbl[] =
  {
    { NETMGR_MAIN_RMNET_SMD_PREFIX,  NETMGR_LINK_RMNET_0,  NETMGR_LINK_RMNET_7  },
    { NETMGR_MAIN_RMNET_SDIO_PREFIX, NETMGR_LINK_RMNET_8,  NETMGR_LINK_RMNET_15 },

    /* This must be the last entry in the table */
    { "",                            NETMGR_LINK_NONE,     NETMGR_LINK_NONE     }
  };
#endif /* FEATURE_DATA_IWLAN */

/* DHCP mutex used for DHCP serialization. In case of thread exit,
 * need to release this mutex */
extern pthread_mutex_t     dhcp_mutex;

/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  netmgr_signal_handler
===========================================================================*/
/*!
@brief
  Callback registered as OS signal handler.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Toggles generation of debug messages
*/
/*=========================================================================*/
LOCAL void netmgr_signal_handler( int sig )
{
  int i = 0;

  switch (sig)
  {
  case SIGUSR1:
    /* On USR1 signal, toggle the debug flags */
    netmgr_main_cfg.debug = (netmgr_main_cfg.debug)? FALSE : TRUE;
    function_debug = (function_debug)? FALSE : TRUE;
    netmgr_log_med("Signal Handler - Setting debug flag: %d\n",netmgr_main_cfg.debug);
    netmgr_log_med("Runmode: 0x%x\n", netmgr_main_cfg.runmode);
    {
      /* Display security credentials */
      struct __user_cap_data_struct cap_data;
      struct __user_cap_header_struct cap_hdr;
      cap_hdr.version = _LINUX_CAPABILITY_VERSION;
      cap_hdr.pid = 0; /* 0 is considered self pid */
      (void)capget(&cap_hdr, &cap_data);
      netmgr_log_med("Running as: uid[%d] gid[%d] caps_perm/eff[0x%x/0x%x]\n",
                     getuid(), getgid(), cap_data.permitted, cap_data.effective);
    }

    /* Dump link state table for debug purposes */
    for( i=0; i<NETMGR_MAX_LINK; i++) {
      netmgr_log_med( "Link[%d] port[%s] name[%s] modem_wait[%d] state[%d] \n",
                      netmgr_ctl_port_array[i].link_id,
                      netmgr_ctl_port_array[i].data_ctl_port,
                      netmgr_kif_get_name( i ),
                      netmgr_ctl_port_array[i].modem_wait,
                      netmgr_ctl_port_array[i].enabled );
    }
    break;

  case SIGTERM:
    /* On TERM signal, exit() so atexit cleanup functions gets called */
    exit(0);
    break;

  default:
    break;
  }
}

/*===========================================================================
  FUNCTION  netmgr_main_get_qos_enabled
===========================================================================*/
/*!
@brief
  Return value for QOS enabled configuration item

@return
  int - TRUE/FALSE

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
inline int netmgr_main_get_qos_enabled( void )
{
  return (NETMGR_MAIN_RUNMODE_QOSHDR ==
          (netmgr_main_cfg.runmode & NETMGR_MAIN_RUNMODE_QOSHDR));
}

#ifdef FEATURE_DATA_IWLAN
/*===========================================================================
  FUNCTION  netmgr_main_get_iwlan_enabled
===========================================================================*/
/*!
@brief
  Return value for iWLAN enabled configuration item

@return
  int - TRUE/FALSE

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
inline int netmgr_main_get_iwlan_enabled( void )
{
  return netmgr_main_cfg.iwlan_enabled;
}

/*===========================================================================
  FUNCTION  netmgr_main_get_iwlan_ims_enabled
===========================================================================*/
/*!
@brief
  Return value for iWLAN IMS enabled configuration item

@return
  int - TRUE/FALSE

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
inline int netmgr_main_get_iwlan_ims_enabled( void )
{
  return netmgr_main_cfg.iwlan_ims_enabled;
}
#endif /* FEATURE_DATA_IWLAN */

#ifdef FEATURE_DS_LINUX_ANDROID
/*===========================================================================
  FUNCTION  netmgr_main_check_tcpackprio_enabled
===========================================================================*/
/*!
@brief
  Return value for TCP_ACK_PRIO enabled configuration item and sets the
  netmgr configuration property.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void netmgr_main_check_tcpackprio_enabled( void )
{
  char args[PROPERTY_VALUE_MAX];
  int ret;

  /* Retrieve value of TCP_ACK_PRIO */
  NETMGR_LOG_FUNC_ENTRY;
  memset(args, 0, sizeof(args));
  netmgr_main_cfg.tcp_ack_prio = FALSE;
  ret = property_get(NETMGR_MAIN_PROPERTY_TCPACKPRIO,
                     args,
                     NETMGR_MAIN_PROPERTY_TCPACKPRIO_DEFAULT);

  if (!strncmp(NETMGR_TRUE, args, sizeof(NETMGR_TRUE)))
  {
     netmgr_main_cfg.tcp_ack_prio = TRUE;
  }
  netmgr_log_med("property [%s] value[%s]",
                 NETMGR_MAIN_PROPERTY_TCPACKPRIO, args);
}

/*===========================================================================
  FUNCTION  netmgr_main_get_tcpackprio_enabled
===========================================================================*/
/*!
@brief
  Return value for TCP_ACK_PRIO enabled configuration item from the netmgr
  configuration property.

@return
  int - TRUE/FALSE

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int netmgr_main_get_tcpackprio_enabled( void )
{
  return netmgr_main_cfg.tcp_ack_prio;
}
#endif

/*===========================================================================
  FUNCTION  netmgr_main_process_arg
===========================================================================*/
/*!
@brief
  Populates program configuration information for the specified argument and
  argument value.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
*/
/*=========================================================================*/
void
netmgr_main_process_arg(char argid, char * argval)
{
  switch (argid) {
    case 'b':
      /* run in background, i.e. run as a forked daemon process */
      netmgr_main_cfg.runmode |= NETMGR_MAIN_RUNMODE_BACK;
      fprintf(stderr, "running in background process\n");
      break;
    case 'E':
      /* use Ethernet link protocol */
      netmgr_main_cfg.runmode |= NETMGR_MAIN_RUNMODE_ETHERNET;
      fprintf(stderr, "using Ethernet link protocol\n");
      break;
#ifdef NETMGR_QOS_ENABLED
    case 'Q':
      /* use RmNET QoS header prepended to TX packets */
      netmgr_main_cfg.runmode |= NETMGR_MAIN_RUNMODE_QOSHDR;
      fprintf(stderr, "QOS enabled, using QMI header\n");
      break;
#endif /* NETMGR_QOS_ENABLED */
    case 's':
      /* Log to syslog. By default program will log to stderr */
      netmgr_main_cfg.logmode = (int)DS_LOG_MODE_SYSLOG;
      fprintf(stderr, "using syslog\n");
      break;
    case 'l':
      /* Logging threshold as an integer value */
      netmgr_main_cfg.logthreshold = ds_atoi(argval);
      fprintf(stderr, "using log level %d\n", ds_atoi(argval));
      break;
    case 'n':
      /* Number of interfaces to create */
      netmgr_main_cfg.nint = ds_atoi(argval);
      fprintf(stderr, "cfging %d interfaces\n", ds_atoi(argval));
      break;
    case 'i':
      /* Interface name to use */
      netmgr_main_cfg.iname = argval;
      fprintf(stderr, "using interface name %s\n", argval);
      break;
    case 'k':
      /* Load kernel driver module and DHCP client */
      netmgr_main_cfg.skip = NETMGR_KIF_LOAD;
      fprintf(stderr, "perform module load\n");
      break;
    case 'd':
      /* Directory pathname to search for script files */
      netmgr_main_cfg.dirpath = argval;
      fprintf(stderr, "using relative path %s\n", argval);
      break;
    case 'm':
      /* Name of driver module load script */
      netmgr_main_cfg.modscr = argval;
      fprintf(stderr, "using module load script %s\n", argval);
      break;
    case 'D':
      /* Verbose debug flag */
      netmgr_main_cfg.debug = TRUE;
      function_debug = TRUE;
      fprintf(stderr, "setting debug mode.\n");
      break;
    case 'T':
      /* Execute internal tests flag */
      netmgr_main_cfg.runtests = TRUE;
      fprintf(stderr, "setting runtests mode.\n");
      break;
    default:
      /* Ignore unknown argument */
      fprintf(stderr, "ignoring unknown arg '%c'\n", argid);
      break;
  }
  return;
}

/*===========================================================================
  FUNCTION  netmgr_main_parse_args
===========================================================================*/
/*!
@brief
  Parses all specified program command line arguments and populates
  configuration information.

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
netmgr_main_parse_args (int argc, char ** argv)
{
  int i;
  char a;

  NETMGR_LOG_FUNC_ENTRY;

  for (i = 1; i < argc; ++i) {
    if (std_strlen(argv[i]) < 2) {
      /* Minimum length of a valid argument is 2, as each arg is
      ** prefixed by a '-' char.
      */
      continue;
    }

    if (*argv[i] != '-') {
      /* Every valid arg should begin with a '-' */
      continue;
    }

    /* Get next char in argument, which is the arg type */
    a = *(argv[i] + 1);

    /* Process based on type of argument */
    switch (a) {
      case 'l':
      case 'n':
      case 'i':
      case 'd':
      case 'm':
      case 'u':
      case 't':
        /* These arguments have an associated value immediately following
        ** the argument.
        */
        if (++i < argc) {
          netmgr_main_process_arg(a, argv[i]);
        }
        break;
      case 'b':
      case 'E':
      case 'Q':
      case 's':
      case 'k':
      case 'D':
      case 'T':
        /* These arguments do not have any value following the argument */
        netmgr_main_process_arg(a, 0);
        break;
      default:
        /* Everything else is an unknown arg that is ignored */
        fprintf(stderr, "unknown arg %s specified\n", argv[i]);
    }
  }

#if 0
  /* Verify Ethernet and QoS modes are not specified simultaneously;
   * this is currently not supported in Linux RmNET driver
   * (due to insufficent skb headroom) */
  if( (netmgr_main_cfg.runmode & NETMGR_MAIN_RUNMODE_ETHERNET) &&
      (netmgr_main_cfg.runmode & NETMGR_MAIN_RUNMODE_QOSHDR) ) {
    NETMGR_STOP("Ethernet protocol with QoS Header not supproted!!");
  }
#endif

  NETMGR_LOG_FUNC_EXIT;
  return;
}


/*===========================================================================
  FUNCTION  netmgr_main_reset_links
===========================================================================*/
/*!
@brief
  selects all the links/interfaces for use by NetMgr. Typically,
  this is the default behavior unless another subsystem (e.g.
  USB rmnet) wanted to use one of the default SMD ports.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
inline void
netmgr_main_reset_links(void)
{
  int i=0;

#ifdef FEATURE_DS_LINUX_ANDROID
  char  args[PROPERTY_VALUE_MAX];
  char  def[NETMGR_MAIN_PROPERTY_NINT_SIZE];
  int   ret;

  /* Query Android property database to see if nint set */
  snprintf( def, sizeof(def), "%d", NETMGR_MAIN_DEFAULT_NINT );
  ret = property_get( NETMGR_MAIN_PROPERTY_NINT, args, def );
  if( (NETMGR_MAIN_PROPERTY_NINT_SIZE-1) < ret ) {
    netmgr_log_err( "System property %s has unexpected size(%d), skippng\n",
                    NETMGR_MAIN_PROPERTY_NINT, ret );
  } else {
    ret = ds_atoi( args );
    if( NETMGR_MAX_LINK < ret ) {
      netmgr_log_err( "System property %s has exceeded limit (%d), skippng\n",
                      NETMGR_MAIN_PROPERTY_NINT, NETMGR_MAX_LINK );
    } else {
      /* Update number of active interfaces */
      netmgr_log_high( "System property %s set (%d)\n",
                       NETMGR_MAIN_PROPERTY_NINT, ret );
      netmgr_main_cfg.nint = ret;
    }
  }
#endif

  netmgr_log_high("netmgr_main_reset_links: " \
                  "reset netmgr_main_nint to %d",
                  netmgr_main_cfg.nint);

  /* Initialize link state table; this may be updated later in
   * netmgr_main_update_links() */
  for(i=0; i<NETMGR_MAX_LINK; i++)
  {
    netmgr_ctl_port_array[i].enabled = TRUE;
  }
}

/*===========================================================================
  FUNCTION  netmgr_read_data_ctl_port
===========================================================================*/
/*!
@brief
  Reads a data control port string - one at a time

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL int netmgr_read_data_ctl_port
(
  int fd,
  char * buf
)
{
  int bytes_read = 0, temp = 0;
  char ch;

  NETMGR_LOG_FUNC_ENTRY;

  if (buf == NULL)
    return 0;

  do
  {
    temp = read(fd, &ch, 1);

    if (temp)
    {
      /* we only care about alphanumeric chars
         and '_'; rest are ignored */
      if (('0' <= ch && ch <= '9') ||
          ('a' <= ch && ch <= 'z') ||
          ('A' <= ch && ch <= 'Z') ||
          ('_' == ch))
      {
        bytes_read += temp;
        *buf++ = ch;
        *buf = '\0';
        /* read the delimiter if we reached max len */
        if( NETMGR_CFG_PARAM_LEN == bytes_read )
        {
          read(fd, &ch, 1);
          break;
        }
      }
      else
      {
        break;
      }
    }

  } while ( bytes_read != 0 );

  NETMGR_LOG_FUNC_EXIT;
  return bytes_read;
}

/*===========================================================================
  FUNCTION  netmgr_read_sysfs_config
===========================================================================*/
/*!
@brief
  Reads from syscfg files to determine which data control ports are
  in-use by other subsystems (e.g. USB)

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void netmgr_read_sysfs_config(void)
{
  int fd = 0, i = 0, bytes_read = 0;
  /* buffer to hold the value read */
  char buffer[NETMGR_CFG_PARAM_LEN+1];

  NETMGR_LOG_FUNC_ENTRY;

  fd = open(NETMGR_SYSFS_CONFIG_FILE_1, O_RDONLY);
  if (fd < 0)
  {
    netmgr_log_err( "couldn't open file %s\n",
                    NETMGR_SYSFS_CONFIG_FILE_1 );
    /* if previous file doesn't exist, try another file */
    fd = open(NETMGR_SYSFS_CONFIG_FILE_2, O_RDONLY);
    if (fd < 0)
    {
      netmgr_log_err( "couldn't open %s\n",
                      NETMGR_SYSFS_CONFIG_FILE_2 );
      return;
    }
  }

  /* read value into local buffer */
  do
  {
    /* reset buffer */
    memset( buffer, 0x0, sizeof(buffer) );

    /* read next data control port from the config file into buffer  */
    bytes_read = netmgr_read_data_ctl_port( fd, buffer );

    if (!bytes_read)
    {
      netmgr_log_med( "no more data control ports found in cfg file\n" );
      break;
    }
    else
    {
      netmgr_log_high( "data control port %s found in cfg file\n",
                       buffer );
    }

    /* go through list of data control ports and disable
       the one we just read in buffer */
    for(i=0; i<NETMGR_MAX_LINK; i++)
    {
      /*
         if a match found, disable corresponding qmi_conn_id
         as it might be used by some other module (like usb rmnet)
      */
      if( !strcmp( netmgr_ctl_port_array[i].data_ctl_port,
                   buffer ) )
      {
        netmgr_ctl_port_array[i].enabled = FALSE;
        netmgr_log_high( "link %d will not be used\n", i);
      }
    }

  } while (TRUE); /* infinite loop */

  NETMGR_LOG_FUNC_EXIT;
  return;
}

#ifdef FEATURE_DATA_IWLAN
/*===========================================================================
  FUNCTION  netmgr_main_disable_modem_reverse_links
===========================================================================*/
/*!
@brief
 This function disables all the reverse links in the given modem.

@return
  void

*/
/*=========================================================================*/
LOCAL void netmgr_main_disable_modem_reverse_links
(
  int  modem
)
{
  int i;
  int modem_start_index = NETMGR_MAIN_GET_INST_MIN_REV(modem);
  int modem_end_index = NETMGR_MAIN_GET_INST_MAX_REV(modem);

  netmgr_log_low("disabling modem [%d] reverse ports start=[%d], end=[%d]",
                 modem,
                 modem_start_index,
                 modem_end_index);

  for (i = modem_start_index; i <= modem_end_index; i++)
  {
    netmgr_ctl_port_array[i].enabled = FALSE;
  }
}
#endif /* FEATURE_DATA_IWLAN */

/*===========================================================================
  FUNCTION  netmgr_main_disable_modem_links
===========================================================================*/
/*!
@brief
 This function disables all the links in the modem which is disabled.
 Which modem is enabled/disabled is determined by runtime configuration
 of baseband value.

@return
  void

*/
/*=========================================================================*/
LOCAL void netmgr_main_disable_modem_links
(
  int   *modems_enabled,
  int   num_modems
)
{
  int i;
  int j;
  int modem_start_index;
  int modem_end_index;

  ds_assert(modems_enabled != NULL);
  ds_assert(((num_modems >= 0) && (num_modems <= NETMGR_MAX_MODEMS)));

  for(i=0; i < num_modems; i++)
  {
    netmgr_log_low("modem_enable[%d]=[%d]", i, modems_enabled[i]);
    if (TRUE != modems_enabled[i])
    {
      /* Disable the forward links on the modem */
      modem_start_index = NETMGR_MAIN_GET_INST_MIN(i);
      modem_end_index = NETMGR_MAIN_GET_INST_MAX(i);

      netmgr_ctl_port_array[modem_start_index].modem_wait = FALSE;

      netmgr_log_low("disabling modem [%d] forward ports start=[%d], end=[%d]",
                     i,
                     modem_start_index,
                     modem_end_index);

      for (j = modem_start_index; j <= modem_end_index; j++)
      {
        netmgr_ctl_port_array[j].enabled = FALSE;
      }

#ifdef FEATURE_DATA_IWLAN
      /* Disable all reverse links on the modem */
      netmgr_main_disable_modem_reverse_links(i);
#endif /* FEATURE_DATA_IWLAN */
    }
  }
}


#ifdef FEATURE_DS_LINUX_ANDROID

#ifdef FEATURE_DATA_IWLAN
/*===========================================================================
  FUNCTION  netmgr_main_process_iwlan_enabled
===========================================================================*/
/*!
@brief
 This function disables all the reverse links in the given modem.

@return
  void

*/
/*=========================================================================*/
LOCAL void netmgr_main_process_iwlan_enabled
(
  int   *modems_enabled,
  int   num_modems
)
{
  int i, ret;
  char args[PROPERTY_VALUE_MAX];
  int is_iwlan_enabled = FALSE;
  int is_iwlan_ims_enabled = FALSE;

  if (!modems_enabled || (num_modems < 0) || (num_modems > NETMGR_MAX_MODEMS)) {
    netmgr_log_err("invalid parameters\n");
    return;
  }

  /* Retrieve value of NETMGR_MAIN_PROPERTY_IWLAN */
  memset(args, 0, sizeof(args));

  ret = property_get( NETMGR_MAIN_PROPERTY_IWLAN, args, NETMGR_MAIN_PROPERTY_IWLAN_DEFAULT );

  if (ret > NETMGR_MAIN_PROPERTY_IWLAN_SIZE)
  {
    netmgr_log_err("property [%s] has size [%d] that exceeds max [%d]\n",
                   NETMGR_MAIN_PROPERTY_IWLAN,
                   ret,
                   NETMGR_MAIN_PROPERTY_IWLAN_SIZE);
  }
  else
  {
    netmgr_log_med("property [%s] value[%s]", NETMGR_MAIN_PROPERTY_IWLAN, args);

    if( !strncasecmp( NETMGR_TRUE, args, sizeof(NETMGR_TRUE) ) )
    {
      is_iwlan_enabled = TRUE;
    }
  }

  /* Disable reverse links on all enabled modems */
  if (FALSE == is_iwlan_enabled)
  {
    for (i = 0; i < num_modems; i++)
    {
      netmgr_log_low("modem_enable[%d]=[%d]", i, modems_enabled[i]);
      if (TRUE == modems_enabled[i])
      {
        /* Disable all reverse links on the modem */
        netmgr_main_disable_modem_reverse_links(i);
      }
    }
  }
  else
  {
    is_iwlan_ims_enabled = TRUE;

    /* Retrieve value of NETMGR_MAIN_PROPERTY_IWLAN_IMS */
    memset(args, 0, sizeof(args));

    ret = property_get( NETMGR_MAIN_PROPERTY_IWLAN_IMS, args, NETMGR_MAIN_PROPERTY_IWLAN_IMS_DEFAULT );

    if (ret > NETMGR_MAIN_PROPERTY_IWLAN_IMS_SIZE)
    {
      netmgr_log_err("property [%s] has size [%d] that exceeds max [%d]\n",
                     NETMGR_MAIN_PROPERTY_IWLAN_IMS,
                     ret,
                     NETMGR_MAIN_PROPERTY_IWLAN_IMS_SIZE);
    }
    else
    {
      netmgr_log_med("property [%s] value[%s]", NETMGR_MAIN_PROPERTY_IWLAN_IMS, args);

      if( !strncasecmp( NETMGR_FALSE, args, sizeof(NETMGR_FALSE) ) )
      {
        is_iwlan_ims_enabled = FALSE;
      }
    }
  }

  netmgr_main_cfg.iwlan_enabled = is_iwlan_enabled;
  netmgr_main_cfg.iwlan_ims_enabled = is_iwlan_ims_enabled;
}
#endif /* FEATURE_DATA_IWLAN */

/*===========================================================================
  FUNCTION  netmgr_main_process_baseband
===========================================================================*/
/*!
@brief
  Updates netmgr links based on baseband property value

@return
  void

*/
/*=========================================================================*/
LOCAL void netmgr_main_process_baseband()
{
  int i = 0;
  int j = 0;
  int ret = 0;
  int modem_enable[NETMGR_MAX_MODEMS];
  int modem_base_index = 0;
  char args[PROPERTY_VALUE_MAX];
  char def[NETMGR_MAIN_PROPERTY_BASEBAND_SIZE];
  char *prefix = NULL;

  /* retrieve value of NETMGR_MAIN_PROPERTY_BASEBAND */
  (void)strlcpy(def,
                NETMGR_MAIN_BASEBAND_VALUE_UNDEFINED,
                NETMGR_MAIN_PROPERTY_BASEBAND_SIZE);
  memset(args, 0, sizeof(args));
  ret = property_get(NETMGR_MAIN_PROPERTY_BASEBAND, args, def);
  if (ret > NETMGR_MAIN_PROPERTY_BASEBAND_SIZE)
  {
    netmgr_log_err("property [%s] has size [%d] that exceeds max [%d]",
                   NETMGR_MAIN_PROPERTY_BASEBAND,
                   ret,
                   NETMGR_MAIN_PROPERTY_BASEBAND_SIZE);
    return;
  }

  netmgr_log_med("baseband property is set to [%s]", args);
  memset(modem_enable, 0, sizeof(modem_enable));
  if(!strcmp(NETMGR_MAIN_BASEBAND_VALUE_MSM, args))
  {
    modem_enable[NETMGR_MODEM_MSM]=TRUE;
    modem_enable[NETMGR_MODEM_MDM]=FALSE;
    netmgr_main_cfg.def_link = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MSM);
  }
  else if(!strcmp(NETMGR_MAIN_BASEBAND_VALUE_APQ, args))
  {
    modem_enable[NETMGR_MODEM_MSM]=FALSE;
    modem_enable[NETMGR_MODEM_MDM]=FALSE;
    netmgr_main_cfg.def_link = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MSM);
  }
  else if(!strcmp(NETMGR_MAIN_BASEBAND_VALUE_SVLTE1, args))
  {
    modem_enable[NETMGR_MODEM_MSM]=TRUE;
    modem_enable[NETMGR_MODEM_MDM]=TRUE;
    netmgr_main_cfg.def_link = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MDM);
  }
  else if(!strcmp(NETMGR_MAIN_BASEBAND_VALUE_SVLTE2A, args))
  {
    modem_enable[NETMGR_MODEM_MSM]=TRUE;
    modem_enable[NETMGR_MODEM_MDM]=TRUE;
    netmgr_main_cfg.def_link = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MDM);
  }
  else if(!strcmp(NETMGR_MAIN_BASEBAND_VALUE_CSFB, args))
  {
    modem_enable[NETMGR_MODEM_MSM]=FALSE;
    modem_enable[NETMGR_MODEM_MDM]=TRUE;
    netmgr_main_cfg.def_link = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MDM);
  }
  else if(!strcmp(NETMGR_MAIN_BASEBAND_VALUE_MDMUSB, args))
  {
    int index = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MDM);

    modem_enable[NETMGR_MODEM_MSM]=FALSE;
    modem_enable[NETMGR_MODEM_MDM]=TRUE;
    netmgr_main_cfg.def_link = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MDM);

    /* Replace SDIO transport with USB */
    strlcpy( NETMGR_MAIN_GET_DEV_PREFIX(NETMGR_MODEM_MDM),
             NETMGR_MAIN_RMNET_USB_PREFIX,
             NETMGR_IF_NAME_MAX_LEN );

    /* Replace SDIO transport with USB */
    std_strlcpy( netmgr_ctl_port_array[ index+0 ].qmi_conn_id,
                 QMI_PORT_RMNET_USB_0,
                 NETMGR_CFG_CONNID_LEN );
    std_strlcpy( netmgr_ctl_port_array[ index+1 ].qmi_conn_id,
                 QMI_PORT_RMNET_USB_1,
                 NETMGR_CFG_CONNID_LEN );
    std_strlcpy( netmgr_ctl_port_array[ index+2 ].qmi_conn_id,
                 QMI_PORT_RMNET_USB_2,
                 NETMGR_CFG_CONNID_LEN );
    std_strlcpy( netmgr_ctl_port_array[ index+3 ].qmi_conn_id,
                 QMI_PORT_RMNET_USB_3,
                 NETMGR_CFG_CONNID_LEN );
    std_strlcpy( netmgr_ctl_port_array[ index+4 ].qmi_conn_id,
                 QMI_PORT_RMNET_USB_4,
                 NETMGR_CFG_CONNID_LEN );
    std_strlcpy( netmgr_ctl_port_array[ index+5 ].qmi_conn_id,
                 QMI_PORT_RMNET_USB_5,
                 NETMGR_CFG_CONNID_LEN );
    std_strlcpy( netmgr_ctl_port_array[ index+6 ].qmi_conn_id,
                 QMI_PORT_RMNET_USB_6,
                 NETMGR_CFG_CONNID_LEN );
    std_strlcpy( netmgr_ctl_port_array[ index+7 ].qmi_conn_id,
                 QMI_PORT_RMNET_USB_7,
                 NETMGR_CFG_CONNID_LEN );
  }
  else if(!strcmp(NETMGR_MAIN_BASEBAND_VALUE_DSDA, args))
  {
    int index = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MDM);

    modem_enable[NETMGR_MODEM_MSM]=TRUE;
    modem_enable[NETMGR_MODEM_MDM]=TRUE;
    netmgr_main_cfg.def_link = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MDM);

    /* Replace SDIO transport with USB */
    strlcpy( NETMGR_MAIN_GET_DEV_PREFIX(NETMGR_MODEM_MDM),
             NETMGR_MAIN_RMNET_USB_PREFIX,
             NETMGR_IF_NAME_MAX_LEN );

    strlcpy( netmgr_ctl_port_array[ index+0 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_0,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+1 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_1,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+2 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_2,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+3 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_3,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+4 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_4,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+5 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_5,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+6 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_6,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+7 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_7,
             NETMGR_CFG_CONNID_LEN );

    /* Replace MSM ports with SMUX port */
    index = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MSM);

    strlcpy( NETMGR_MAIN_GET_DEV_PREFIX(NETMGR_MODEM_MSM),
             NETMGR_MAIN_RMNET_SMUX_PREFIX,
             NETMGR_IF_NAME_MAX_LEN );

    strlcpy( netmgr_ctl_port_array[ index+0 ].qmi_conn_id,
             QMI_PORT_RMNET_SMUX_0,
             NETMGR_CFG_CONNID_LEN );

    netmgr_ctl_port_array[ index+1 ].enabled = FALSE;
    netmgr_ctl_port_array[ index+2 ].enabled = FALSE;
    netmgr_ctl_port_array[ index+3 ].enabled = FALSE;
    netmgr_ctl_port_array[ index+4 ].enabled = FALSE;
    netmgr_ctl_port_array[ index+5 ].enabled = FALSE;
    netmgr_ctl_port_array[ index+6 ].enabled = FALSE;
    netmgr_ctl_port_array[ index+7 ].enabled = FALSE;

  }
  else if(!strcmp(NETMGR_MAIN_BASEBAND_VALUE_DSDA2, args))
  {
    int index = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MSM);

    modem_enable[NETMGR_MODEM_MSM]=TRUE;
    modem_enable[NETMGR_MODEM_MDM]=TRUE;
    netmgr_main_cfg.def_link = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MSM);

    /* Replace first modem ports with USB */
    strlcpy( NETMGR_MAIN_GET_DEV_PREFIX(NETMGR_MODEM_MSM),
             NETMGR_MAIN_RMNET_USB_PREFIX,
             NETMGR_IF_NAME_MAX_LEN );

    strlcpy( netmgr_ctl_port_array[ index+0 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_0,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+1 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_1,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+2 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_2,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+3 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_3,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+4 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_4,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+5 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_5,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+6 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_6,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+7 ].qmi_conn_id,
             QMI_PORT_RMNET_USB_7,
             NETMGR_CFG_CONNID_LEN );

    /* Replace second modem ports with 2 modem USB */
    index = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MDM);

    strlcpy( NETMGR_MAIN_GET_DEV_PREFIX(NETMGR_MODEM_MDM),
             NETMGR_MAIN_RMNET2_USB_PREFIX,
             NETMGR_IF_NAME_MAX_LEN );

    strlcpy( netmgr_ctl_port_array[ index+0 ].qmi_conn_id,
             QMI_PORT_RMNET2_USB_0,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+1 ].qmi_conn_id,
             QMI_PORT_RMNET2_USB_1,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+2 ].qmi_conn_id,
             QMI_PORT_RMNET2_USB_2,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+3 ].qmi_conn_id,
             QMI_PORT_RMNET2_USB_3,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+4 ].qmi_conn_id,
             QMI_PORT_RMNET2_USB_4,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+5 ].qmi_conn_id,
             QMI_PORT_RMNET2_USB_5,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+6 ].qmi_conn_id,
             QMI_PORT_RMNET2_USB_6,
             NETMGR_CFG_CONNID_LEN );
    strlcpy( netmgr_ctl_port_array[ index+7 ].qmi_conn_id,
             QMI_PORT_RMNET2_USB_7,
             NETMGR_CFG_CONNID_LEN );
  }
  else if(!strcmp(NETMGR_MAIN_BASEBAND_VALUE_SGLTE, args))
  {
    int index = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MDM);

    modem_enable[NETMGR_MODEM_MSM]=TRUE;
    modem_enable[NETMGR_MODEM_MDM]=TRUE;
    netmgr_main_cfg.def_link = NETMGR_MAIN_GET_INST_MIN(NETMGR_MODEM_MSM);

    /* Replace SDIO transport with USB */
    strlcpy( NETMGR_MAIN_GET_DEV_PREFIX(NETMGR_MODEM_MDM),
             NETMGR_MAIN_RMNET_SMUX_PREFIX,
             NETMGR_IF_NAME_MAX_LEN );

    strlcpy( netmgr_ctl_port_array[ index+0 ].qmi_conn_id,
             QMI_PORT_RMNET_SMUX_0,
             NETMGR_CFG_CONNID_LEN );
  }
  else
  {
    netmgr_log_med("property [%s] value [%s] not processed",
                   NETMGR_MAIN_PROPERTY_BASEBAND, args);
    return;
  }

  netmgr_main_disable_modem_links(modem_enable, NETMGR_MAX_MODEMS);

#ifdef FEATURE_DATA_IWLAN
  netmgr_main_process_iwlan_enabled(modem_enable, NETMGR_MAX_MODEMS);
#endif /* FEATURE_DATA_IWLAN */
}

/*===========================================================================
  FUNCTION  netmgr_main_process_qos_enabled
===========================================================================*/
/*!
@brief
  Updates netmgr configuration based on QOS enabled property value

@return
  void

*/
/*=========================================================================*/
LOCAL void netmgr_main_process_qos_enabled()
{
  char args[PROPERTY_VALUE_MAX];
  int ret;

  /* Retrieve value of NETMGR_MAIN_PROPERTY_QOS */
  memset(args, 0, sizeof(args));
  ret = property_get( NETMGR_MAIN_PROPERTY_QOS, args, NETMGR_MAIN_PROPERTY_QOS_DEFAULT );
  if (ret > NETMGR_MAIN_PROPERTY_BASEBAND_SIZE)
  {
    netmgr_log_err("property [%s] has size [%d] that exceeds max [%d]\n",
                   NETMGR_MAIN_PROPERTY_QOS,
                   ret,
                   NETMGR_MAIN_PROPERTY_QOS_SIZE);
    return;
  }

  netmgr_log_med("property [%s] value[%s]", NETMGR_MAIN_PROPERTY_QOS, args);
  if( !strncmp( NETMGR_FALSE, args, sizeof(NETMGR_FALSE) ) )
  {
    /* Clear QOS enabled flag */
    netmgr_main_cfg.runmode &= ~NETMGR_MAIN_RUNMODE_QOSHDR;
  }
  else if( !strncmp( NETMGR_TRUE, args, sizeof(NETMGR_TRUE) ) )
  {
    /* Set QOS enabled flag */
    netmgr_main_cfg.runmode |= NETMGR_MAIN_RUNMODE_QOSHDR;
  }
  else
  {
    netmgr_log_err("Unsupported state value, using default[%s]\n",
                   NETMGR_MAIN_PROPERTY_QOS_DEFAULT);
  }

}
#endif /* FEATURE_DS_LINUX_ANDROID */

/*===========================================================================
  FUNCTION  netmgr_main_update_links
===========================================================================*/
/*!
@brief
  Update the link array to disable those for any SMD port used by
  external subsystem, and any over the number of links requested.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
inline void netmgr_main_update_links(void)
{
  int i = 0;
  int cnt = 0;

  /* Verify number of specified links supported by configuration */
  if( NETMGR_MAX_LINK < netmgr_main_cfg.nint )
  {
    NETMGR_STOP( "Number links (%d) exceeds limit (%d), stopped!!",
                 netmgr_main_cfg.nint, NETMGR_MAX_LINK );
  }

  /* Verify there are active links requested */
  if( 0 == netmgr_main_cfg.nint )
  {
    NETMGR_STOP( "All links disabled, stopped!!" );
  }

  /* Check for external sybsystem config file in SYSFS */
  netmgr_read_sysfs_config();

#if 0  // TEMPORARY: SDIO RmNET driver crashing on close IOCTL
  /*  Validate link table against preconfigured & requested configuration */
  cnt = netmgr_main_cfg.nint;
  for(i=0; i<NETMGR_MAX_LINK; i++)
  {
    /* Ensure preconfigured links not exhausted */
    if( cnt && (NETMGR_LINK_MAX == netmgr_ctl_port_array[i].link_id) ) {
      goto bail;
    }

    /* Disable any link over number configured as it is not required */
    if( cnt && netmgr_ctl_port_array[i].enabled ) {
      /* Decrement active link counter */
      cnt--;
    } else {
      /* Suppress further processing for this link */
      netmgr_ctl_port_array[i].enabled = FALSE;
    }
  }

  netmgr_log_med( "netmgr_main_update_links: %d links enabled\n",
                  (netmgr_main_cfg.nint-cnt) );

bail:
#else
  /* Loop over array for unused links */
  for(i=netmgr_main_cfg.nint; i<NETMGR_MAX_LINK; i++)
  {
    /* Suppress further processing for this link */
    netmgr_ctl_port_array[i].enabled = FALSE;
  }
#endif

  /* Verify configured link number available; cnt should be zero */
  if( 0 < cnt ) {
    netmgr_log_err( "WARNING Number requested links (%d) exceed those available",
                    netmgr_main_cfg.nint );
  }

#if defined(FEATURE_DS_LINUX_ANDROID)
#if !defined(FEATURE_DS_SVLTE1)
  /* Process NETMGR_MAIN_PROPERTY_BASEBAND property */
  netmgr_main_process_baseband();
#endif

  /* Process NETMGR_MAIN_PROPERTY_QOS property */
  netmgr_main_process_qos_enabled();
#elif defined(FEATURE_DATA_LINUX_LE)
  {
    /* This is a temporay solution for LE until the runtime
     * configuration using configdb is used, across all linux flavors.
     */
    int modem_enable[NETMGR_MAX_MODEMS];

    /*set MSM only configuration to open smd control and data ports.*/
    modem_enable[NETMGR_MODEM_MSM] = TRUE;
    modem_enable[NETMGR_MODEM_MDM] = FALSE;
    netmgr_main_disable_modem_links(modem_enable, NETMGR_MAX_MODEMS);
  }
#endif/*FEATURE_DATA_LINUX_LE*/

  NETMGR_LOG_FUNC_EXIT;
  return;
}

/*===========================================================================
  FUNCTION  netmgr_main_sm_inited
===========================================================================*/
/*!
@brief
  posts NETMGR_INITED_EV to each state machine instance

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
LOCAL void netmgr_main_sm_inited(void)
{
  int i=0;
  netmgr_exec_cmd_t * cmd = NULL;

  NETMGR_LOG_FUNC_ENTRY;

  for(i=0; i<NETMGR_MAX_LINK; i++)
  {
    /* Skip disabled links */
    if( netmgr_ctl_port_array[i].enabled == FALSE )
    {
      netmgr_log_low( "netmgr_main_sm_inited: ignoring link[%d]\n", i );
      continue;
    }

    /* Allocate a command object */
    cmd = netmgr_exec_get_cmd();
    NETMGR_ASSERT(cmd);

    /* Set command object parameters */
    cmd->data.type = NETMGR_INITED_EV;
    cmd->data.link = i;
    memset(&cmd->data.info, 0, sizeof(cmd->data.info));

    /* Post command for processing in the command thread context */
    if( NETMGR_SUCCESS != netmgr_exec_put_cmd( cmd ) ) {
      netmgr_log_err("failed to put commmand\n");
      netmgr_exec_release_cmd( cmd );
    }
  }

  NETMGR_LOG_FUNC_EXIT;
}

/*===========================================================================
  FUNCTION  netmgr_diag_cleanup
===========================================================================*/
/*!
@brief
  Performs cleanup of Diag LSM resources.  Invoked at process termination.

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
netmgr_diag_cleanup(void)
{
#ifdef FEATURE_DATA_LOG_QXDM
  (void) Diag_LSM_DeInit();
#endif
}

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  netmgr_main
===========================================================================*/
/*!
@brief
  Main entry point of the core program. Performs all program initialization.

@return
  int - NETMGR_SUCCESS always

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int
netmgr_main (int argc, char ** argv)
{
#ifdef FEATURE_DATA_LOG_QXDM
  /* Initialize Diag services */
  if ( TRUE != Diag_LSM_Init(NULL) )
  {
    netmgr_log_err("failed on Diag_LSM_Init\n" );
  }
  else
  {
    atexit(netmgr_diag_cleanup);
  }
#endif
#ifdef FEATURE_DATA_LOG_SYSLOG
  /* Initialize logging as per desired mode */
  netmgr_log_init(netmgr_main_cfg.logthreshold, netmgr_main_cfg.logmode);
#endif

#ifdef FEATURE_WAIT_FOR_MODEM
  /* Interim solution for device open latency in driver layer */
  sleep( FEATURE_WAIT_FOR_MODEM );
#endif

  NETMGR_LOG_FUNC_ENTRY;

  /* Initialize number of active interfaces to default; may be overridden
   * by system property or command-line argument */
  netmgr_main_cfg.nint = NETMGR_MAIN_DEFAULT_NINT;

  /* Initialze number of links; may be overridded */
  netmgr_main_reset_links();

#ifdef NETMGR_QOS_ENABLED
  /* Turn QoS header on by default */
  netmgr_main_cfg.runmode |= NETMGR_MAIN_RUNMODE_QOSHDR;
#endif /* NETMGR_QOS_ENABLED */

  /* Parse command line arguments and populate configuration blob */
  netmgr_main_parse_args(argc, argv);

  /* Update links state based on external subsystem usage */
  netmgr_main_update_links();

  /* Run as a daemon, if requested */
  if( netmgr_main_cfg.runmode & NETMGR_MAIN_RUNMODE_BACK ) {
    netmgr_daemonize();
    netmgr_log_low( "daemonize completed\n" );
    (void)sleep(1);
  }

  /* Register signal handler */
  signal(SIGUSR1, netmgr_signal_handler);
  signal(SIGUSR2, netmgr_signal_handler);
  signal(SIGTERM, netmgr_signal_handler);

  /* Initialize executive module */
  netmgr_exec_init( netmgr_main_cfg.nint, netmgr_ctl_port_array );

  /* Initialize platform layer */
  netmgr_platform_init();
  netmgr_log_med( "platform init completed\n" );

  netmgr_kif_powerup_init(netmgr_ctl_port_array, netmgr_main_cfg.iname);

  /* Initialize QMI interface module */
  netmgr_qmi_init( netmgr_main_cfg.nint, netmgr_ctl_port_array );
  netmgr_log_med( "qmi init completed\n" );

  /* Initialize kernel interface module */
  netmgr_kif_init( netmgr_main_cfg.nint,
                   netmgr_main_cfg.skip,
                   netmgr_main_cfg.dirpath,
                   netmgr_main_cfg.modscr);
  netmgr_log_med( "kif init completed\n" );

#ifdef NETMGR_QOS_ENABLED
  netmgr_main_check_tcpackprio_enabled();
  if( netmgr_main_get_qos_enabled() )
  {
    /* Initialize traffic control module */
    netmgr_tc_init( netmgr_main_cfg.nint, netmgr_ctl_port_array );
    netmgr_log_low( "tc init completed\n" );
  }
#endif // NETMGR_QOS_ENABLED

#if (!defined(NETMGR_OFFTARGET) && defined(FEATURE_DS_LINUX_ANDROID))
  /* adjust uid/gid and capabilities */
  if (ds_change_user_cap( AID_RADIO, AID_SYSTEM,
                          (1 << CAP_NET_ADMIN) | (1 << CAP_NET_RAW)) != 0)
  {
    netmgr_log_err("couldn't change uid and capabilities at power up");
    exit(EXIT_FAILURE);
  }
#endif

  netmgr_main_cfg.initialized = TRUE;
  netmgr_log_high("Initialization complete.\n");

  /* bring up each SM instance in INITED state */
  netmgr_main_sm_inited();

#ifdef NETMGR_TEST
  /* Launch unit test suite */
  if( netmgr_main_cfg.runtests ) {
    netmgr_test_init();
    netmgr_test_execute();
  }
#else
  /* Indefinitely wait on command processing thread. */
  netmgr_exec_wait();
#endif /* NETMGR_TEST */

  NETMGR_LOG_FUNC_EXIT;
  return NETMGR_SUCCESS;
}
