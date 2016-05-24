/******************************************************************************

                          N E T M G R _ M A I N . H

******************************************************************************/

/******************************************************************************

  @file    netmgr_main.h
  @brief   Network Manager main function header file

  DESCRIPTION
  Header file containing definition of NetMgr's main function.

******************************************************************************/
/*===========================================================================

  Copyright (c) 2010,2011-2013 Qualcomm Technologies, Inc. All Rights Reserved

  Qualcomm Technologies Proprietary

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
02/08/10   ar         Initial version

******************************************************************************/

#ifndef __NETMGR_MAIN_H__
#define __NETMGR_MAIN_H__

#include "comdef.h"
#include "netmgr.h"
#include "netmgr_defs.h"

/*===========================================================================
                     DEFINITIONS AND DECLARATIONS
===========================================================================*/

/* Macros representing program run mode */
#define  NETMGR_MAIN_RUNMODE_DEFAULT     0x00  /* Default runmode          */
#define  NETMGR_MAIN_RUNMODE_BACK        0x01  /* Run as forked deamon     */
#define  NETMGR_MAIN_RUNMODE_ETHERNET    0x02  /* Use Ethernet framing     */
#define  NETMGR_MAIN_RUNMODE_QOSHDR      0x04  /* Use RmNET QoS header     */

#define  NETMGR_MAIN_RMNET_SMD_PREFIX      "rmnet"          /* Prefix for smd ports */
#define  NETMGR_MAIN_RMNET_SDIO_PREFIX     "rmnet_sdio"     /* Prefix for sdio ports */
#define  NETMGR_MAIN_RMNET_USB_PREFIX      "rmnet_usb"      /* Prefix for usb ports */
#define  NETMGR_MAIN_RMNET_SMUX_PREFIX     "rmnet_smux"     /* Prefix for smux ports */
#define  NETMGR_MAIN_RMNET2_USB_PREFIX     "rmnet2_usb"     /* Prefix for second MDM USB ports */

#ifdef FEATURE_DATA_IWLAN
  #define  NETMGR_MAIN_REV_RMNET_SMD_PREFIX  "rev_rmnet"      /* Prefix for reverse smd ports */
  #define  NETMGR_MAIN_REV_RMNET_USB_PREFIX  "rev_rmnet_usb"  /* Prefix for reverse usb ports */
#endif /* FEATURE_DATA_IWLAN */

/* Collection of program configuration info */
struct netmgr_main_cfg_s {
  int runmode;         /* Process run mode */
  int logmode;         /* Logging mode */
  int logthreshold;    /* Logging threshold */
  int nint;            /* Number of interfaces */
  char * iname;        /* Name of virtual ethernet interface */
  int skip;            /* Whether to skip driver module loading */
  char * dirpath;      /* Directory pathname to look for script files */
  char * modscr;       /* Name of script to use for loading module */
  boolean debug;       /* Verbose debug flag */
  boolean runtests;    /* Execute internal tests flag */
  boolean initialized; /* Program initialization completed */
  netmgr_link_id_t  def_link; /* Default link */
  boolean iwlan_enabled;      /* iWLAN feature is enabled */
  boolean iwlan_ims_enabled;  /* iWLAN IMS feature is enabled */
  boolean tcp_ack_prio;       /* TCP Ack Prioritization is enabled */
};

extern struct netmgr_main_cfg_s netmgr_main_cfg;

/* Structure for QMI data control devices. It also contains a boolean
 * member to identify if the link is enabled or not. */
#define NETMGR_CFG_PARAM_LEN  20
#define NETMGR_CFG_CONNID_LEN 30

typedef struct netmgr_ctl_port_config_s
{
  char              data_ctl_port[NETMGR_CFG_PARAM_LEN];
  char              qmi_conn_id[NETMGR_CFG_CONNID_LEN];
  netmgr_link_id_t  link_id;
  boolean           modem_wait;
  boolean           enabled;
}netmgr_ctl_port_config_type;

/* Table of transport device name prefix per Modem */
typedef struct netmgr_main_dev_prefix_s
{
  char    prefix[NETMGR_IF_NAME_MAX_LEN];
  byte    inst_min;
  byte    inst_max;
} netmgr_main_dev_prefix_type;

#ifdef FEATURE_DATA_IWLAN
  typedef enum
  {
    NETMGR_FWD_LINK,
    NETMGR_REV_LINK,
    NETMGR_MAX_LINK_TYPES
  } netmgr_main_link_type;

  extern netmgr_main_dev_prefix_type netmgr_main_dev_prefix_tbl[NETMGR_MAX_MODEMS][NETMGR_MAX_LINK_TYPES];
#else
  extern netmgr_main_dev_prefix_type netmgr_main_dev_prefix_tbl[];
#endif /* FEATURE_DATA_IWLAN */

/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
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
int netmgr_main(int argc, char ** argv);


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
int netmgr_main_get_qos_enabled( void );

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
int netmgr_main_get_tcpackprio_enabled( void );

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
int netmgr_main_get_iwlan_enabled( void );


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
int netmgr_main_get_iwlan_ims_enabled( void );


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
void netmgr_main_reset_links(void);

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
void netmgr_main_update_links(void);

#endif /* __NETMGR_MAIN_H__ */
