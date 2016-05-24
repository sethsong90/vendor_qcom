/******************************************************************************

                           D S C I . H

******************************************************************************/

/******************************************************************************

  @file    dsci.h
  @brief   DSC's common internal header file

  DESCRIPTION
  Internal header file for DSC containing common definitions for all 
  subcomponents.

  ---------------------------------------------------------------------------
  Copyright (c) 2007,2008,2011 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/inc/dsci.h#2 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
03/15/08   vk         Incorporated code review comments
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

#ifndef __DSCI_H__
#define __DSCI_H__

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include "ds_linux.h"

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Constant representing the maximum APN string length
---------------------------------------------------------------------------*/
#define DSC_MAX_APN_LEN 128

/*--------------------------------------------------------------------------- 
   Constant representing the maximum number of simultaneous primary calls 
   supported
---------------------------------------------------------------------------*/
#define DSC_MAX_PRICALL 3

/*--------------------------------------------------------------------------- 
   Constant representing MTU size limits supproted by RmNET driver
---------------------------------------------------------------------------*/
#define DSC_MTU_INVALID (0)
#define DSC_MTU_MAX     (2000)
#define DSC_MTU_DEFAULT (1500)

/*--------------------------------------------------------------------------- 
   Macro for obtaining the cardinality of an array
---------------------------------------------------------------------------*/
#define DSC_ARRSIZE(a) (sizeof(a)/sizeof(a[0]))

/*--------------------------------------------------------------------------- 
   Type representing a boolean result value
---------------------------------------------------------------------------*/
typedef enum {
    DSC_OP_SUCCESS = 0,
    DSC_OP_FAIL = 1
} dsc_op_status_t;

#endif /* __DSCI_H__ */
