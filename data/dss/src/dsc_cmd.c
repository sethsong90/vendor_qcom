/******************************************************************************

                        D S C _ C M D . C

******************************************************************************/

/******************************************************************************

  @file    dsc_cmd.c
  @brief   DSC's command thread

  DESCRIPTION
  Implementation of DSC's command processing thread.

  ---------------------------------------------------------------------------
  Copyright (c) 2007-2008 Qualcomm Technologies, Inc.
  All Rights Reserved. Qualcomm Technologies Proprietary and Confidential.
  ---------------------------------------------------------------------------

******************************************************************************/

/******************************************************************************

                      EDIT HISTORY FOR FILE

  $Id: //source/qcom/qct/platform/linux/common/main/latest/apps/dsc/src/dsc_cmd.c#2 $

when       who        what, where, why
--------   ---        -------------------------------------------------------
03/15/08   vk         Incorporated code review comments
11/29/07   vk         Cleaned up lint warnings
11/17/07   vk         Added function headers and other comments
09/28/07   vk         Initial version

******************************************************************************/

/*===========================================================================
                              INCLUDE FILES
===========================================================================*/

#include <pthread.h>
#include <stdlib.h>
#include <assert.h>
#include "dsc_util.h"
#include "ds_list.h"
#include "dsc_cmd.h"

/*===========================================================================
                     LOCAL DEFINITIONS AND DECLARATIONS
===========================================================================*/

/*--------------------------------------------------------------------------- 
   Global maximum number of commands permitted to be enqueued at any time
---------------------------------------------------------------------------*/
#define DSC_CMDQ_MAX 50

/*--------------------------------------------------------------------------- 
   Collection of control info of the Command Thread
---------------------------------------------------------------------------*/
struct dsc_cmdq_info_s {
    ds_dll_el_t * head;   /* Head node of cmd queue */
    ds_dll_el_t * tail;   /* Tail node of cmd queue */
    int            nel;   /* Number of commands enqueued */
    pthread_t thrd;       /* Command thread */
    pthread_cond_t cond;  /* Condition variable for signaling */
    pthread_mutex_t mutx; /* Mutex for protecting the list operations */
} dsc_cmdq_info;

/*===========================================================================
                            LOCAL FUNCTION DEFINITIONS
===========================================================================*/

/*===========================================================================
  FUNCTION  dsc_cmdq_init
===========================================================================*/
/*!
@brief
  Initializes the control data structures of the Command Thread.

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
dsc_cmdq_init (void)
{
    /* Initialize the list of commands */
    if ((dsc_cmdq_info.head = ds_dll_init(NULL)) == NULL) {
        /* Error in initializing list. Abort.. */
        dsc_log_err("Failed to allocate memory for cmdq. Exiting..\n");
        dsc_abort();
    }
    dsc_cmdq_info.tail = dsc_cmdq_info.head;

    /* Initialize number of elements in the list */
    dsc_cmdq_info.nel = 0;

    /* Initialize the mutex and condition variables */
    (void)pthread_mutex_init(&dsc_cmdq_info.mutx, NULL);
    (void)pthread_cond_init(&dsc_cmdq_info.cond, NULL);
}

/*===========================================================================
  FUNCTION  dsc_cmdq_deq
===========================================================================*/
/*!
@brief
  Dequeues the first command from the FIFO list of commands pending 
  execution and returns a pointer to it.

@return
  dsc_cmd_t * - pointer to command if one is enqueued, NULL otherwise

@note

  - Dependencies
    - Caller must acquire the Command Thread mutex before calling this 
      function, as this function does not do any locking itself. 

  - Side Effects
    - None
*/
/*=========================================================================*/
static dsc_cmd_t *
dsc_cmdq_deq (void)
{
    ds_dll_el_t * node = NULL;
    dsc_cmd_t * cmd = NULL;

    /* Dequeue next command from the head of the list */
    if ((node = ds_dll_deq
                (
                    dsc_cmdq_info.head, 
                    &dsc_cmdq_info.tail, 
                    (void *)&cmd)
                ) != NULL) 
    {
        /* Valid command dequeued. Decrement number of cmds in list */
        --dsc_cmdq_info.nel;

        /* Free memory for this list node */
        ds_dll_free(node);
    }

    /* Return cmd ptr, or null if no cmd was dequeued */
    return cmd;
}

/*===========================================================================
  FUNCTION  dsc_cmdthrd_main
===========================================================================*/
/*!
@brief
  The main function of Command Thread. Sits in a forever loop, waiting for 
  commands to be enqueued, and executes them one by one in a FIFO manner. 

@return
  void * - NULL (This function does not return.)

@note

  - Dependencies
    - Requires Command Thread data structures to have been initialized 

  - Side Effects
    - None
*/
/*=========================================================================*/
static void * dsc_cmdthrd_main (void * dummy)
{
    dsc_cmd_t * cmd;
    (void)dummy;
    
    /* Loop forever, processing commands one by one */
    for (;;) {
        /* Acquire mutex before dequeuing command */
        if (pthread_mutex_lock(&dsc_cmdq_info.mutx) != 0) {
        	dsc_log_err("pthread_mutex_lock failed\n");
        	dsc_abort();
        }

        /* Try to dequeue command */
        if ((cmd = dsc_cmdq_deq()) == NULL) {
            /* No command in queue. Go to sleep on condition variable */
            if (pthread_cond_wait(&dsc_cmdq_info.cond, &dsc_cmdq_info.mutx) 
                != 0) 
            {
        		dsc_log_err("pthread_cond_wait failed\n");
        		dsc_abort();
            }

            /* Signaled to wake up. Release mutex and continue with loop */
            if (pthread_mutex_unlock(&dsc_cmdq_info.mutx) != 0) {
        		dsc_log_err("pthread_mutex_unlock failed\n");
        		dsc_abort();
            }
        } else {
            /* Dequeued valid command. Release mutex first before processing 
            ** command. 
            */
            if (pthread_mutex_unlock(&dsc_cmdq_info.mutx) != 0) {
        		dsc_log_err("pthread_mutex_unlock failed\n");
        		dsc_abort();
            }

            /* Double check that execute func ptr is set in the cmd object */
            ds_assert(cmd->execute_f != NULL);

            /* Execute command */
            (*(cmd->execute_f))(cmd, cmd->data);

            /* Call free handler, if set */
            if (cmd->free_f)
                (*(cmd->free_f))(cmd, cmd->data);
        }
    } /* end of for (;;) */
}

/*===========================================================================
                            GLOBAL FUNCTION DEFINITIONS
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
void
dsc_cmdq_enq (const dsc_cmd_t * cmd)
{
    ds_dll_el_t * node;

    /* Doesn't make sense to have a null execute function ptr */
    ds_assert(cmd->execute_f != NULL);

    /* Make sure we have space for enqueuing the command */
    if (dsc_cmdq_info.nel == DSC_CMDQ_MAX) {
        /* Maximum length of command list exceeded so cannot enqueue. Abort */
        dsc_log_err("Cannot insert in full cmdq. Exiting..\n");
        dsc_abort();
    }

    /* Acquire mutex before enqueuing to list */
    if (pthread_mutex_lock(&dsc_cmdq_info.mutx) < 0) {
        dsc_log_err("pthread_mutex_lock failed.\n");
        dsc_abort();
    }

    /* Enqueue command to the tail of the command list */
    if ((node = ds_dll_enq(dsc_cmdq_info.tail, NULL, cmd)) == NULL) {
        dsc_log_err("Failed to insert in cmdq. Exiting..\n");
        dsc_abort();
    }
    dsc_cmdq_info.tail = node;

    /* Increment number of commands in list */
    ++dsc_cmdq_info.nel;

    /* If list was empty before we enqueued this command, signal the command 
    ** thread to wake up and process the command. 
    */
    if (dsc_cmdq_info.nel == 1) {
        if (pthread_cond_signal(&dsc_cmdq_info.cond) != 0) {
        	dsc_log_err("pthread_cond_signal failed\n");
        	dsc_abort();
        }
    }

    /* Release the mutex before returning from the function */
    if (pthread_mutex_unlock(&dsc_cmdq_info.mutx) < 0) {
        dsc_log_err("pthread_mutex_unlock failed\n");
        dsc_abort();
    }

    return;
}

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
void 
dsc_cmdthrd_init (void)
{
    /* Initialize the command queue and associated data structures */
    dsc_cmdq_init();

    /* Start command thread */
    if (pthread_create
        (
            &dsc_cmdq_info.thrd, 
            NULL, 
            dsc_cmdthrd_main, 
            NULL
        ) != 0) 
    {
        dsc_log_err("Cannot start cmdthrd. Exiting..\n");
        dsc_abort();
    }
}
