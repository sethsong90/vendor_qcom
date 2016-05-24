/******************************************************************************

                  D S _ F M C _ A P P _ M A I N . H

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app_main.h
  @brief   DS_FMC_APP Manager main function header file

  DESCRIPTION
  Header file containing definition of DS_FMC_APP's main function.

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
05/13/10   scb        Initial version

******************************************************************************/

#ifndef __DS_FMC_APP_MAIN_H__
#define __DS_FMC_APP_MAIN_H__

#include "comdef.h"

/* Macros representing program run mode */
#define  DS_FMC_APP_MAIN_RUNMODE_DEFAULT     0x00  /* Default runmode      */
#define  DS_FMC_APP_MAIN_RUNMODE_BACK        0x01  /* Run as forked deamon */
#define  DS_FMC_APP_CFG_PORT_NAME_LEN        32    /* Length of QMI Ctl port 
 */
/*===========================================================================
                     DEFINITIONS AND DECLARATIONS
===========================================================================*/


/* Collection of program configuration info */
struct ds_fmc_app_main_cfg_s {
  int runmode;         /* Process run mode */
  boolean debug;       /* Verbose debug flag */
  boolean initialized; /* Program initialization completed */
  char qmi_ctl_port[DS_FMC_APP_CFG_PORT_NAME_LEN]; 
                       /* Ctl Port for QMI services*/
};

extern struct ds_fmc_app_main_cfg_s ds_fmc_app_main_cfg;


/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

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
int ds_fmc_app_main(int argc, char ** argv);

#endif /* __DS_FMC_APP_MAIN_H__ */
