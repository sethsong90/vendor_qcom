/******************************************************************************

                           D S _ F M C _ A P P . C

******************************************************************************/

/******************************************************************************

  @file    ds_fmc_app.c
  @brief   DS_FMC_APP main function

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
06/07/10   scb        Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include "ds_fmc_app_main.h"

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  main
===========================================================================*/
/*!
@brief
  The entry point main function of DS_FMC_APP process.

@return
  int - exit code 

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
int 
main (int argc, char *argv[])
{
    return ds_fmc_app_main(argc, argv);
}

