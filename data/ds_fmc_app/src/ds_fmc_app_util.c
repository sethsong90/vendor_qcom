/******************************************************************************

                    D S _ F M C _ A P P _ U T I L . C

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app_util.c
  @brief   DS_FMC_APP Utility Functions Implementation File

  DESCRIPTION
  Implementation file for DS_FMC_APP utility functions.

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
05/23/10   scb        Initial version (derived from Netmgr file)

******************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>

#include "ds_fmc_app_util.h"

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/
/* Control function entry/exit debug messages */
boolean function_debug = FALSE;
/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  ds_fmc_app_daemonize
===========================================================================*/
/*!
@brief
 Performs typical tasks required to run a program as a daemon process.

@return
  void

@note

  - Dependencies
    - None  

  - Side Effects
    - Original program will exit and a child is forked which will continue
      execution as the daemon.
*/
/*=========================================================================*/
void ds_fmc_app_daemonize (void)
{
  pid_t pid;
  pid_t sid;

  /* Fork and exit parent process to ensure that process is not a process
  ** group leader. 
  */
  if ((pid = fork()) > 0) 
  {
    exit(0);
  }

  if (pid < 0) 
  {
    /* Could not create child process. Exit */
    DS_FMC_APP_STOP("ds_fmc_app_daemonize: Could not create child process\n");
    return;
  }

  /* Become session group leader to disassociate from controlling terminal */
  sid = setsid();

  if (sid < 0) 
  {
    DS_FMC_APP_STOP("ds_fmc_app_daemonize: setsid() failed\n");
    return;
  }

  /* Set file mode creation mask to 0, to avoid having permissions of created
  ** files being inadvertently changed. 
  */
  (void)umask(0);

  /* Change directory to root */
  if ((chdir("/")) < 0) 
  {
    DS_FMC_APP_STOP("ds_fmc_app_daemonize: chdir to root failed\n");
    return;
  }

  /* Redirect stdin, stdout and stderr to /dev/null. If running as a daemon,
  ** it is assumed that logging will be to syslog. 
  */
  if (freopen("/dev/null", "r", stdin) == NULL) 
  {
    DS_FMC_APP_STOP("ds_fmc_app_daemonize: freopen of stdin failed\n");
    return;
  }

  if (freopen("/dev/null", "w", stdout) == NULL) 
  {
    DS_FMC_APP_STOP("ds_fmc_app_daemonize: freopen of stdout failed\n");
    return;
  }

  if (freopen("/dev/null", "w", stderr) == NULL) 
  {
    DS_FMC_APP_STOP("ds_fmc_app_daemonize: freopen of stderr failed\n");
    return;
  }
} /*ds_fmc_app_daemonize*/
