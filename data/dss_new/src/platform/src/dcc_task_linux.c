/******************************************************************************

                        D C C _ T A S K _ L I N U X . C

******************************************************************************/

/******************************************************************************

  @file    dcc_task_linux.c
  @brief   Linux data control plane command thread

  DESCRIPTION
  Implementation of data control plane command processing thread on Linux

  ---------------------------------------------------------------------------
  Copyright (c) 2009 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dcc_cmd.c#2 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
04/28/10   ar         Migrate to use commond DS library code
11/09/09   js         Created

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "ds_util.h"
#include "ds_cmdq.h"
#include "ds_list.h"
#include "dcc_task_defs.h"
#include "ps_svc.h"

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Global maximum number of commands permitted to be enqueued at any time
---------------------------------------------------------------------------*/
#define DCC_CMDQ_MAX 100

/*--------------------------------------------------------------------------- 
   Collection of control info of the Command Thread
---------------------------------------------------------------------------*/
struct ds_cmdq_info_s     dcc_cmdq;  /* Command queue for async processing */

/*--------------------------------------------------------------------------- 
   Mapping of dcc_cmd_enum_type to dcc_cmd_handler_type
---------------------------------------------------------------------------*/
static void dcc_cmd_default_handler
(
  dcc_cmd_enum_type       cmd,           /* Command to be processed        */
  void                   *user_data_ptr  /* Command specific user data     */
);

static dcc_enum_handler_map_type dcc_e_h_map[DCC_MAX_DEFINED_CMD_TYPES];

/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

static void dcc_cmd_default_handler
(
  dcc_cmd_enum_type       cmd,           /* Command to be processed        */
  void                   *user_data_ptr  /* Command specific user data     */
)
{
  (void)cmd;
  (void)user_data_ptr;
  MSG_ERROR("Default command handler called, register a handler!",0,0,0);
  ASSERT(0);
  return;
}

/*===========================================================================
  FUNCTION  dcc_execute_cmd_buf
===========================================================================*/
/*!
@brief
  Callback to process DCC command objects

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static void dcc_execute_cmd_buf
(
  struct ds_cmd_s * cmd_buf,
  void *            data
)
{
  dcc_cmd_data_buf_type * cmd = NULL;
  unsigned int i;
  
  ds_assert(cmd_buf);
  ds_assert(data);

  cmd = (dcc_cmd_data_buf_type*)cmd_buf->data;
  
  /* Lookup registered command handler */
  for (i=0; i<DCC_MAX_DEFINED_CMD_TYPES; i++) {
    if (dcc_e_h_map[i].e == cmd->type)
      break;
  }

  /* Invoke registered command handler */
  if (i < DCC_MAX_DEFINED_CMD_TYPES) {
    dcc_e_h_map[i].h( cmd->type, cmd->data );
  } else {
    MSG_ERROR("Failed to find command handler! type=%d",cmd->type,0,0);
  }
}

/*===========================================================================
  FUNCTION  dcc_free_cmd_buf
===========================================================================*/
/*!
@brief
  Callback to release DCC command objects

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static void dcc_free_cmd_buf
(
  struct ds_cmd_s * cmd_buf,
  void * data
)
{
  ds_free(cmd_buf);
  dcc_free_cmd_data_buf( (dcc_cmd_data_buf_type *)data );
}


/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/


/*===========================================================================
  FUNCTION  dcc_get_cmd_data_buf
===========================================================================*/
/*!
@brief
  gets command of type ds_cmd_t from global heap

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - allocates memory that needs to be freed later
*/
/*=========================================================================*/
dcc_cmd_data_buf_type * dcc_get_cmd_data_buf( void )
{
  return (dcc_cmd_data_buf_type *)ds_malloc(sizeof(dcc_cmd_data_buf_type));
}

/*===========================================================================
  FUNCTION  dcc_free_cmd_data_buf
===========================================================================*/
/*!
@brief
  frees command of type ds_cmd_t to global heap

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
void dcc_free_cmd_data_buf
(
  dcc_cmd_data_buf_type * cmd
)
{
  ds_free((void *)cmd);
}

/*===========================================================================
  FUNCTION  dcc_set_cmd_handler
===========================================================================*/
/*!
@brief
  sets mapping from dcc_cmd_enum_type to dcc_cmd_handler_type in
  global variable dcc_e_h_map  

@return
  dcc_cmd_handler_type - Previous handler value, NULL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
dcc_cmd_handler_type dcc_set_cmd_handler
(
  dcc_cmd_enum_type e,
  dcc_cmd_handler_type h
)
{
  dcc_cmd_handler_type old_h = NULL;
  int index;
  
  ds_assert(e < DCC_MAX_DEFINED_CMD_TYPES);
  ds_assert(h != NULL);

  /* Assign command handler to table record */
  index = (int)e;
  old_h = dcc_e_h_map[index].h;
  
  dcc_e_h_map[index].e = e;
  dcc_e_h_map[index].h = h;

  return old_h;
}

/*===========================================================================
  FUNCTION  dcc_send_cmd
===========================================================================*/
/*!
@brief
  Usees the dcc_e_h_map variable to find out execute_f to be used
  with ds_cmd_t command to be sent to the command thread

@return
  void

@note

  - Dependencies
    - dcc_set_cmd_handler must have been called

  - Side Effects
    - None
*/
/*=========================================================================*/
void dcc_send_cmd
(
  dcc_cmd_enum_type e,
  void *user_data_ptr
)
{
  ds_cmd_t * cmd_buf = NULL;
  unsigned char i=0;
  dcc_cmd_data_buf_type *cmd_data_ptr = NULL;

  ds_assert(e < DCC_MAX_DEFINED_CMD_TYPES);
  ds_assert(user_data_ptr != NULL);

  /* Lookup command handler */
  for (i=0; i<DCC_MAX_DEFINED_CMD_TYPES; i++) {
    if (dcc_e_h_map[i].e == e)
      break;
  }

  if (i < DCC_MAX_DEFINED_CMD_TYPES) {
    /* Allocate command buffer */

    cmd_data_ptr = dcc_get_cmd_data_buf();
    if( NULL == cmd_data_ptr ) {
      MSG_ERROR("Failure on ds_malloc(%d)",sizeof(dcc_cmd_data_buf_type),0,0);
      ds_assert(0);
      return;
    }
    
    memset(cmd_data_ptr, 0, sizeof(dcc_cmd_data_buf_type));

    cmd_buf = ds_malloc( sizeof(ds_cmd_t) );
    if( NULL == cmd_buf ) {
      MSG_ERROR("Failure on ds_malloc(%d)",sizeof(ds_cmd_t),0,0);
      goto bail1;
    }
    
    cmd_buf->execute_f = dcc_execute_cmd_buf;
    cmd_buf->free_f = dcc_free_cmd_buf;
    
    cmd_data_ptr->data = user_data_ptr; 
    cmd_data_ptr->type = e;

    cmd_buf->data = (void*)cmd_data_ptr;
    
    MSG_HIGH("dcc_send_cmd: enqueueing type %d cmd %p, data :%p",
             cmd_data_ptr->type, cmd_data_ptr, cmd_data_ptr->data );
    
    if( 0 > ds_cmdq_enq( &dcc_cmdq, cmd_buf ) ) {
      MSG_ERROR("Failure on ds_cmdq_enq",0,0,0);
      goto bail2;
    }
  }

  return;

bail2:
  ds_free( cmd_buf );
bail1:
  ds_free( cmd_data_ptr );
  ds_assert(0);
  return;
}

/*===========================================================================
  FUNCTION  dcc_cmdthrd_deinit
===========================================================================*/
/*!
@brief
  Function for teardown of Command Thread.

@return
  void 

@note

  - Dependencies
    - None  

  - Side Effects
    - None
*/
/*=========================================================================*/
void dcc_cmdthrd_deinit (void)
{
  /*-------------------------------------------------------------------------
    Purge command queue and terminate processing thread
  -------------------------------------------------------------------------*/
  if( 0 > ds_cmdq_deinit( &dcc_cmdq ) ) {
    MSG_ERROR("Failure on ds_cmdq_deinit",0,0,0);
  }
}

/*===========================================================================
  FUNCTION  dcc_cmd_handlers_init
===========================================================================*/
/*!
@brief 
  Initializes the command handler hash map. 

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
dcc_cmd_handlers_init
(
  void
)
{
  int index = 0;

  for (index = DCC_MIN_CMD + 1; index < DCC_MAX_DEFINED_CMD_TYPES; index++)
  {
    dcc_set_cmd_handler(index, dcc_cmd_default_handler);
  }
}


/*===========================================================================
  FUNCTION  dcc_cmdthrd_init
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
void dcc_cmdthrd_init (void)
{
  /*-------------------------------------------------------------------------
    Initialize command queue and processing thread for asynch processing
  -------------------------------------------------------------------------*/
  
  dcc_cmd_handlers_init();

  if( 0 > ds_cmdq_init( &dcc_cmdq, DCC_CMDQ_MAX ) ) {
    MSG_ERROR("Failure on ds_cmdq_deinit",0,0,0);
  }

}
