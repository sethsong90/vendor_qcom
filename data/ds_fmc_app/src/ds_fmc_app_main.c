/******************************************************************************

                        D S _ F M C _ A P P _ M A I N . C

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app_main.c
  @brief   DS_FMC_APP main function implementation

  DESCRIPTION
  Implementation of DS_FMC_APP's main function.

******************************************************************************/
/*===========================================================================

  Copyright (c) 2010 Qualcomm Technologies, Inc. All Rights Reserved

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
02/23/12   op         Remove root permissiosn after SMD channels are opened
05/13/10   scb        Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <cutils/properties.h>
#include <linux/capability.h>
#include <private/android_filesystem_config.h>

#include "qmi.h"

#include "ds_string.h"
#include "ds_util.h"
#include "ds_fmc_app_main.h"
#include "ds_fmc_app_util.h"
#include "ds_fmc_app_exec.h"
#include "ds_fmc_app_qmi.h"
#include "ds_fmc_app_call_mgr.h"
#include "ds_fmc_app_tunnel_mgr.h"
#include "ds_fmc_app_data_if.h"
#include "ds_fmc_app_data_ext_if.h"
#include "ds_fmc_app_data_mdm_if.h"


#ifdef FEATURE_DS_LINUX_ANDROID
/* Macros for operating environment validation */
#define DS_FMC_APP_MAIN_PROPERTY_ANDROID_MODE  "persist.data.ds_fmc_app.mode"
#define DS_FMC_APP_MAIN_PROPERTY_SIZE     (PROPERTY_VALUE_MAX)
#define DS_FMC_APP_MAIN_PROPERTY_DISABLE  0
#define DS_FMC_APP_MAIN_PROPERTY_ENABLE   1
#endif /* FEATURE_DS_LINUX_ANDROID */

/*===========================================================================
                     FORWARD DECLARATIONS
===========================================================================*/


/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Program configuration info
---------------------------------------------------------------------------*/
struct ds_fmc_app_main_cfg_s ds_fmc_app_main_cfg = {
  DS_FMC_APP_MAIN_RUNMODE_DEFAULT, FALSE, FALSE, ""
}; /* Initialize everything to invalid values */

/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  ds_fmc_app_main_process_arg
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
ds_fmc_app_main_process_arg(char argid, char * argval)
{
  switch (argid) {
    case 'b':
      /* run in background, i.e. run as a forked daemon process */
      ds_fmc_app_main_cfg.runmode |= DS_FMC_APP_MAIN_RUNMODE_BACK;
      fprintf(stderr, "running in background process\n");
      break;

    case 'p':
      /* QMI port to be used to create a client */
      memset((void*)ds_fmc_app_main_cfg.qmi_ctl_port, 0, 
       DS_FMC_APP_CFG_PORT_NAME_LEN);
      std_strlcpy((void*)ds_fmc_app_main_cfg.qmi_ctl_port, 
       (void*)argval, sizeof(ds_fmc_app_main_cfg.qmi_ctl_port));
      fprintf(stderr, "using argval %s, size: %d\n", argval, 
              (int)std_strlen(argval));
      fprintf(stderr, "copied qmi_ctl_port %s, size: %d\n", 
              ds_fmc_app_main_cfg.qmi_ctl_port, 
             (int)std_strlen(ds_fmc_app_main_cfg.qmi_ctl_port));
      break;

    case 'D':
      /* Verbose debug flag */
      ds_fmc_app_main_cfg.debug = TRUE;
      function_debug = TRUE;
      fprintf(stderr, "setting debug mode.\n");
      break;

    default:
      /* Ignore unknown argument */
      fprintf(stderr, "ignoring unknown arg '%c'\n", argid);
      break;
  }
  return;
} /* ds_fmc_app_main_process_arg */

/*===========================================================================
  FUNCTION  ds_fmc_app_main_parse_args
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
ds_fmc_app_main_parse_args (int argc, char ** argv)
{
  int i;
  char a;

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
      case 'p':
        /* These arguments have an associated value immediately following
        ** the argument.
        */
        if (++i < argc) {
          ds_fmc_app_main_process_arg(a, argv[i]);
        }
        break;
      case 'b':
      case 'D':
        /* These arguments do not have any value following the argument */
        ds_fmc_app_main_process_arg(a, 0);
        break;
      default:
        /* Everything else is an unknown arg that is ignored */
        fprintf(stderr, "unknown arg %s specified\n", argv[i]);
    }
  }

  return;
} /* ds_fmc_app_main_parse_args */

/*===========================================================================
  FUNCTION  ds_fmc_app_main_check_environment
===========================================================================*/
/*!
@brief
  Query the OS for operating paramaters. Adjust program configuration
  to comply with environment.

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - Execution may suspend
*/
/*=========================================================================*/
LOCAL void ds_fmc_app_main_check_environment()
{
#ifdef FEATURE_DS_LINUX_ANDROID
  char  args[DS_FMC_APP_MAIN_PROPERTY_SIZE];
  int   ret;

  /* Query Android property to indicate execution state */
  ret = property_get( DS_FMC_APP_MAIN_PROPERTY_ANDROID_MODE, args,
                      DS_FMC_APP_MAIN_PROPERTY_DISABLE );
  if( (DS_FMC_APP_MAIN_PROPERTY_SIZE - 1) < ret ) {
    DS_FMC_APP_STOP( "System property %s has unexpected value %d, stopping\n",
                 DS_FMC_APP_MAIN_PROPERTY_ANDROID_MODE, ret );
  }
  
  /* Check if process should be stopped */
  if(DS_FMC_APP_MAIN_PROPERTY_DISABLE == ds_atoi(args))
  {
    DS_FMC_APP_STOP( "DS_FMC_APP system property set to DISABLE, stopping\n" );
  }

  fprintf(stderr, "DS_FMC_APP system property set to %d\n", ds_atoi(args) );
#else
  fprintf(stderr, "DS_FMC_APP feature DS_LINUX_ANDROID undefined\n" );
#endif /* FEATURE_DS_LINUX_ANDROID */  
}


/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

void ds_fmc_app_sig_hdlr(int user_data)
{

  ds_fmc_app_log_high("ds_fmc_app_sig_hdlr: user data: %d\n", user_data);

  if(user_data == SIGUSR1)
  {
    pthread_t thrd_id = pthread_self();

    ds_fmc_app_log_high("ds_fmc_app_sig_hdlr: Thread id: %d\n", thrd_id);
  
    pthread_exit(NULL);
  }
}

/*===========================================================================
  FUNCTION  ds_fmc_app_main
===========================================================================*/
/*!
@brief
  Main entry point of the core program. Performs all program initialization.

@return
  int - DS_FMC_APP_SUCCESS always

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
int
ds_fmc_app_main (int argc, char ** argv)
{

  /* Validate execution environment */
  ds_fmc_app_main_check_environment();

  /* Parse command line arguments and populate configuration blob */
  ds_fmc_app_main_parse_args(argc, argv);

  /* Initialize the logging entity */
#ifdef FEATURE_DATA_LOG_QXDM
  /* Initialize Diag services */
  if ( TRUE != Diag_LSM_Init(NULL) )
  {
    ds_fmc_app_log_err("ds_fmc_app failed on Diag_LSM_Init\n" );
  }
#endif /* FEATURE_DATA_LOG_QXDM */

  /* Daemonize the ds_fmc_app process only if asked to run in the 
     bkground. By default this value won't be set for Android native 
     builds.
  */
  if( ds_fmc_app_main_cfg.runmode & DS_FMC_APP_MAIN_RUNMODE_BACK ) {
    ds_fmc_app_daemonize();
    ds_fmc_app_log_high( "ds_fmc_app_daemonize() completed\n" );
    (void)sleep(1);
  }

  /* Register signal handler */
  signal(SIGUSR1, ds_fmc_app_sig_hdlr);

  /* Initialize executive module */
  ds_fmc_app_exec_init();
  ds_fmc_app_log_high( "ds_fmc_app_exec_init() completed\n" );

  /* Initialize QMI module */
  ds_fmc_app_qmi_init( ds_fmc_app_main_cfg.qmi_ctl_port );
  ds_fmc_app_log_high( "ds_fmc_app_qmi_init() completed\n" );
  
  /* Initialize the unix domain sockets framework to talk to the Call mgmt
     entity */
  ds_fmc_app_call_mgr_init();
  ds_fmc_app_log_high( "ds_fmc_app_call_mgr_init() completed\n" );

  /* Initialize the tunnel management entity */
  ds_fmc_app_tunnel_mgr_init();
  ds_fmc_app_log_high( "ds_fmc_app_tunnel_mgr_init() completed\n" );

  /*Initialize the datapath module by initializing the rx_cb
    for the data coming in OTA */
  ds_fmc_app_data_ext_init(ds_fmc_app_data_mdm_send_data);
  ds_fmc_app_log_high( "ds_fmc_app_data_ext_init() completed\n" );

  /* Initialize the datapath module by initializing one data SMD
     channels to be associated with the datapath. */
  ds_fmc_app_data_mdm_init(ds_fmc_app_data_ext_send_data);
  ds_fmc_app_log_high( "ds_fmc_app_data_mdm_init() completed\n" );

  if(DS_FMC_APP_SUCCESS != 
    (ds_fmc_app_data_mdm_open_conn(
      DS_FMC_APP_DATA_CONN_ID_0, ds_fmc_app_data_mdm_rx_buf ) ) )
  {
    DS_FMC_APP_STOP("ds_fmc_app_data_mdm_open_conn failed for conn id %d\n",
                     DS_FMC_APP_DATA_CONN_ID_0 );
  }
  
  /* Remove root permissions for ds_fmc_appd  */
  if (ds_change_user_cap( AID_RADIO, AID_SYSTEM,
                          (1 << CAP_NET_ADMIN) | (1 << CAP_NET_RAW)) != 0)
  {
    ds_fmc_app_log_err("couldn't change uid and capabilities at power up");
    exit(EXIT_FAILURE);
  }
  
  ds_fmc_app_main_cfg.initialized = TRUE;
  ds_fmc_app_log_high("Initialization complete.\n");

  /* Indefinitely wait on command processing thread. */
  ds_fmc_app_exec_wait();
  ds_fmc_app_log_high("ds_fmc_app_exec_wait() completed\n");

  return DS_FMC_APP_SUCCESS;
} /* ds_fmc_app_main */
