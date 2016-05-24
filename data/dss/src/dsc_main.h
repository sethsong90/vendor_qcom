/******************************************************************************

                        D S C _ M A I N . H

******************************************************************************/

/******************************************************************************

  @file    dsc_main.h
  @brief   DSC's main function header file

  DESCRIPTION
  Header file containing definition of DSC's main function.

  ---------------------------------------------------------------------------
  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dsc_main.h#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

#ifndef __DSC_MAIN_H__
#define __DSC_MAIN_H__

/*===========================================================================
                            GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_main
===========================================================================*/
/*!
@brief
  Main entry point of the dsc program. Performs all program initialization.

@return
  int - 0 always

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
int dsc_main(int argc, char ** argv);

#endif /* __DSC_MAIN_H__ */
