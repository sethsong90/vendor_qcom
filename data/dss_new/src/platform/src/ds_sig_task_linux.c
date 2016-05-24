/******************************************************************************

                     D S _ S I G _ T A S K _ L I N U X . C

******************************************************************************/

/******************************************************************************

  @file    ds_sig_task_linux.c
  @brief   Linux data control plane signal thread

  DESCRIPTION
  Implementation of data control plane signal processing thread on Linux

  ---------------------------------------------------------------------------
  Copyright (c) 2009 - 2010 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id:$

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

#include "ds_util.h"
#include "ds_cmdq.h"
#include "ds_list.h"
#include "ds_sig_svc.h"

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*---------------------------------------------------------------------------
   Global maximum number of commands permitted to be enqueued at any time
---------------------------------------------------------------------------*/
#define DS_SIG_CMDQ_MAX 100

/*---------------------------------------------------------------------------
   Collection of control info of the Signal Command Thread
---------------------------------------------------------------------------*/
struct ds_cmdq_info_s  ds_sig_cmdq;  /* Command queue for async processing */

/*---------------------------------------------------------------------------
   Structure representing a generic command
---------------------------------------------------------------------------*/
typedef struct ds_sig_cmd_data_buf_s {
    ds_sig_cmd_enum_type  type;
    void *                data;
} ds_sig_cmd_data_buf_type;

/*---------------------------------------------------------------------------
   Mapping of ds_sig_cmd_enum_type to ds_sig_cmd_handler_type
---------------------------------------------------------------------------*/
typedef struct {
    ds_sig_cmd_enum_type e;
    ds_sig_cmd_handler_type h;
} ds_sig_enum_handler_map_type;

static void ds_sig_cmd_default_handler
(
  ds_sig_cmd_enum_type    cmd,           /* Command to be processed        */
  void                   *user_data_ptr  /* Command specific user data     */
);

static ds_sig_enum_handler_map_type ds_sig_e_h_map[DS_SIG_CMD_MAX] = 
{ 
  { DS_SIG_SIGNAL_DISPATCH_CMD, ds_sig_cmd_default_handler }
};


/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

static void ds_sig_cmd_default_handler
(
  ds_sig_cmd_enum_type    cmd,           /* Command to be processed        */
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
  FUNCTION  ds_sig_execute_cmd_buf
===========================================================================*/
/*!
@brief
  Callback to process DS_SIG command objects

@return
  void

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
static void ds_sig_execute_cmd_buf
(
  struct ds_cmd_s * cmd_buf,
  void *            data
)
{
  ds_sig_cmd_data_buf_type * cmd = NULL;
  unsigned int i;
  
  ds_assert(cmd_buf);
  ds_assert(data);

  cmd = (ds_sig_cmd_data_buf_type*)cmd_buf->data;
  
  /* Lookup registered command handler */
  for (i=0; i<DS_SIG_CMD_MAX; i++) {
    if (ds_sig_e_h_map[i].e == cmd->type)
      break;
  }

  /* Invoke registered command handler */
  if (i < DS_SIG_CMD_MAX) {
    ds_sig_e_h_map[i].h( cmd->type, cmd->data );
  } else {
    MSG_ERROR("Failed to find command handler! type=%d",cmd->type,0,0);
  }
  
}

/*===========================================================================
  FUNCTION  ds_sig_free_cmd_buf
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
static void ds_sig_free_cmd_buf
(
  struct ds_cmd_s * cmd_buf,
  void * data
)
{
  (void)cmd_buf;
  ds_free( data );
}


/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
===========================================================================*/


/*===========================================================================
  FUNCTION  ds_sig_get_cmd_data_buf
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
ds_sig_cmd_data_buf_type * ds_sig_get_cmd_data_buf( void )
{
  return (ds_sig_cmd_data_buf_type *)ds_malloc(sizeof(ds_sig_cmd_data_buf_type));
}


/*===========================================================================
  FUNCTION  ds_sig_set_cmd_handler
===========================================================================*/
/*!
@brief
  sets mapping from ds_sig_cmd_enum_type to ds_sig_cmd_handler_type in
  global variable ds_sig_e_h_map  

@return
  ds_sig_cmd_handler_type - Previous handler value, NULL otherwise

@note

  - Dependencies
    - None

  - Side Effects
    - None
*/
/*=========================================================================*/
ds_sig_cmd_handler_type ds_sig_set_cmd_handler
(
  ds_sig_cmd_enum_type e,
  ds_sig_cmd_handler_type h
)
{
  ds_sig_cmd_handler_type old_h = NULL;
  int index;
  
  ds_assert(e < DS_SIG_CMD_MAX);
  ds_assert(h != NULL);

  /* Assign command handler to table record */
  index = (int)e;
  old_h = ds_sig_e_h_map[index].h;
  
  ds_sig_e_h_map[index].e = e;
  ds_sig_e_h_map[index].h = h;

  return old_h;
}

/*===========================================================================
  FUNCTION  ds_sig_send_cmd
===========================================================================*/
/*!
@brief
  Usees the ds_sig_e_h_map variable to find out execute_f to be used
  with ds_cmd_t command to be sent to the command thread

@return
  void

@note

  - Dependencies
    - ds_sig_set_cmd_handler must have been called

  - Side Effects
    - None
*/
/*=========================================================================*/
void ds_sig_send_cmd
(
  ds_sig_cmd_enum_type   cmd,            /* Command to be processed        */
  void                  *user_data_ptr   /* Command specific user data     */
)
{
  ds_cmd_t * cmd_buf = NULL;
  ds_sig_cmd_data_buf_type * cmd_data = NULL;
  unsigned char i;

  ds_assert(cmd < DS_SIG_CMD_MAX);

  /* Lookup command handler */
  for (i=0; i<DS_SIG_CMD_MAX; i++) {
    if (ds_sig_e_h_map[i].e == cmd)
      break;
  }

  if (i < DS_SIG_CMD_MAX) {
    /* Allocate command buffer */
    cmd_buf = ds_malloc( sizeof(ds_cmd_t) );
    if( NULL == cmd_buf ) {
      MSG_ERROR("Failure on ds_malloc(%d)",sizeof(ds_cmd_t),0,0);
      ds_assert(0);
      return;
    }
    cmd_data = ds_sig_get_cmd_data_buf();
    if( NULL == cmd_data ) {
      ds_free( cmd_buf );
      MSG_ERROR("Failure on ds_malloc(%d)",sizeof(ds_sig_cmd_data_buf_type),0,0);
      ds_assert(0);
      return;
    }

    cmd_data->type = cmd;
    cmd_data->data = user_data_ptr;
    
    cmd_buf->execute_f = ds_sig_execute_cmd_buf;
    cmd_buf->free_f = ds_sig_free_cmd_buf;
    cmd_buf->data = (void*)cmd_data;
    
    MSG_HIGH("ds_sig_send_cmd: enqueueing type %d cmd %p, data 0x%08x",
             cmd_data->type, cmd_buf, cmd_data->data );
    
    if( 0 > ds_cmdq_enq( &ds_sig_cmdq, cmd_buf ) ) {
      MSG_ERROR("Failure on ds_cmdq_enq",0,0,0);
      ds_free( cmd_buf );
      return;
    }
  }
  return;
}

/*===========================================================================
  FUNCTION  ds_sig_cmdthrd_deinit
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
void ds_sig_cmdthrd_deinit (void)
{
  /*-------------------------------------------------------------------------
    Purge command queue and terminate processing thread
  -------------------------------------------------------------------------*/
  if( 0 > ds_cmdq_deinit( &ds_sig_cmdq ) ) {
    MSG_ERROR("Failure on ds_cmdq_deinit",0,0,0);
  }
}

/*===========================================================================
  FUNCTION  ds_sig_cmdthrd_init
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
void ds_sig_cmdthrd_init (void)
{
  /*-------------------------------------------------------------------------
    Initialize command queue and processing thread for asynch processing
  -------------------------------------------------------------------------*/
  if( 0 > ds_cmdq_init( &ds_sig_cmdq, DS_SIG_CMDQ_MAX ) ) {
    MSG_ERROR("Failure on ds_cmdq_deinit",0,0,0);
  }
}

/*=========================================================================*/
/*!
  @function
  ds_sig_is_current_task

  @brief
  Checks if the current task is DS_SIG.

  @return TRUE  - If current task is DS_SIG
  @return FALSE - If current task is not DS_SIG
*/
/*=========================================================================*/
boolean ds_sig_is_current_task
(
  void
)
{
  if (!pthread_equal(pthread_self(), ds_sig_cmdq.thrd))
  {
    return FALSE;
  }
  else
  {
    return TRUE;
  }
}
