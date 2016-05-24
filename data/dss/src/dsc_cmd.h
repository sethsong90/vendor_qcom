/******************************************************************************

                        D S C _ C M D . H

******************************************************************************/

/******************************************************************************

  @file    dsc_cmd.h
  @brief   DSC's command thread header file

  DESCRIPTION
  Header file for DSC's command processing thread.

  ---------------------------------------------------------------------------
  Copyright (c) 2007 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dsc_cmd.h#1 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

#ifndef __DSC_CMD_H__
#define __DSC_CMD_H__

/*===========================================================================
                     GLOBAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Forward declaration needed by subsequent type definitions
---------------------------------------------------------------------------*/
struct dsc_cmd_s;

/*--------------------------------------------------------------------------- 
   Type definition of virtual function to execute command
---------------------------------------------------------------------------*/
typedef void (* dsc_cmd_execute_f) (struct dsc_cmd_s *, void *);

/*--------------------------------------------------------------------------- 
   Type definition of virtual function to free (deallocate) command
---------------------------------------------------------------------------*/
typedef void (* dsc_cmd_free_f)    (struct dsc_cmd_s *, void *);

/*--------------------------------------------------------------------------- 
   Structure representing a generic command
---------------------------------------------------------------------------*/
typedef struct dsc_cmd_s {
    dsc_cmd_execute_f execute_f;
    dsc_cmd_free_f    free_f;
    void            * data;
} dsc_cmd_t;

/*===========================================================================
                     GLOBAL FUNCTION DECLARATIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_cmdq_enq
===========================================================================*/
/*!
@brief
  Used by clients to enqueue a command to the Command Thread's list of 
  pending commands and execute it in the Command Thread context.  

@return
  void 

@note

  - Dependencies
    - Assumes Command Thread has been initialized and is running.  

  - Side Effects
    - None
*/
/*=========================================================================*/
void dsc_cmdq_enq (const dsc_cmd_t * cmd);

/*===========================================================================
  FUNCTION  dsc_cmdthrd_init
===========================================================================*/
/*!
@brief
  Function for initializing and starting Command Thread. Must be called 
  before clients can post commands for execution in Command Thread context. 

@return
  void 

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
void dsc_cmdthrd_init(void);

#endif /* __DSC_CMD_H__ */
