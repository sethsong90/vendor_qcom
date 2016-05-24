/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                      O N C R P C _ M A I N _ L I N U X . C

GENERAL DESCRIPTION
    Implementation of the OS specific ONCRPC support routines

 Copyright (c) 2002-2011, 2013 Qualcomm Technologies, Inc.  All Rights Reserved.
 Qualcomm Technologies Proprietary and Confidential.
*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================

                        EDIT HISTORY FOR MODULE

  $Id: //linux/pkgs/proprietary/oncrpc/main/source/oncrpc/oncrpc_main_linux.c#10 $

when        who    what, where, why
--------    ---    ----------------------------------------------------------
09/18/09    rr     Fix race condition in router read task stop
05/26/09    rr     Add support to stop proxy tasks
04/06/09    rr     Added support for oncrpc_task_stop
02/23/09    rr     Add body of oncrpc_main_os_deinit for close handles
11/11/08    rr     Add restart task
10/15/08    rr     Add restart handler
01/30/08    rr     Add protection against re-initialization of tasks.
12/11/07    al     Added library initialization and exit functions that will
                   be called automatically by the Linux system
11/08/07    rr     Added oncrpc_proxy_lock_init to be init from proxy task
                   fixes race condition on init
11/05/07    ih     Added support for dynamic proxy task creation
10/31/07    ptm    Add code to set task names.
10/08/07    ptm    Clean up oncrpc thread handle references and move OS
                   specific proxy code here.
09/14/07    rr     Thread synchronization on startup
08/22/07    ptm    Unified access to thread local storage.
07/10/07    ptm    Add DSM item code and cleanup featurization.
05/08/07    RJS    Derived from oncrpc_main_r e x.c

===========================================================================*/

/*===========================================================================
                     INCLUDE FILES FOR MODULE
===========================================================================*/
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include "oncrpc_dsm.h"
#include "oncrpc.h"
#include "oncrpci.h"
#include "oncrpc_os.h"
#include "oncrpc_proxy.h"
#include "oncrpc_main.h"
#include "oncrpc_taski.h"
#include "oncrpc_rtr_os.h"
#include "oncrpc_dsm.h"

#include "oncrpc_pacmark.h"
#include "oncrpc_rtr_os.h"
#include "oncrpc_xdr_types.h"


/*===========================================================================
            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE
===========================================================================*/
#define ONCRPC_BW_COMP_LOCAL_VERS     ( RPC_ROUTER_LOCAL_PROCESSOR_ID )
#define ONCRPC_BW_COMP_REMOTE_VERS    ( RPC_ROUTER_REMOTE_DEFAULT_PROCESSOR_ID )

#define ROUTER_ADDRESS_ENCODE( ADDR1, ADDR2 ) \
    ( ((uint64)(ADDR2) << 32) | (uint64)(ADDR1) )


//#define ONCRPC_DEBUG_ENABLE
#if defined ONCRPC_DEBUG_ENABLE
#define DEBUG_PRINT(p...) do { \
	printf(p); \
} while (0)
#else
#define DEBUG_PRINT(p...) do { } while (0)
#endif
#define ONCRPC_WAIT_10MS_NS  (10000000)

/* Task Handles */
static oncrpc_thread_handle oncrpc_task_thread_handle;
static oncrpc_thread_handle oncrpc_read_thread_handle;
static oncrpc_thread_handle oncrpc_restart_thread_handle;

static pthread_t  modem_restart_pthread;
static pthread_t  router_read_pthread;
static pthread_t  main_oncrpc_thread;
static struct sigaction  action_to_catch;

static int             thread_state;                /* Local State */
static pthread_mutex_t thread_state_mutex;          /* Mutex                    */
static pthread_mutex_t start_stop_mutex;            /* Mutex                    */

/*-----------------------------------------------------------------------------------
  State bits
  Global states for ONCRPC
  START_INIT -> Requested start, prevents multiple calls to start
  START_COMPLETE -> Running
  STOP_INIT  -> Requested stop, prevents multiple calls to stop
  When stop is complete, all START_STOP_MASK bits are cleared

  For each sub-thread
  <Thread>_RUN -> Thread is running
  <Thread>_STOP -> Thread is stopping, signal sent to thread, waiting for join
  <Thread> all bits cleared, thread is stopped and ready to be restarted.
 -------------------------------------------------------------------------------------*/
#define THREAD_STATE_START_COMPLETE  0x00000001    /* Start Complete */
#define THREAD_STATE_START_INIT      0x00000002    /* Start */
#define THREAD_STATE_START_MASK      0x00000003
#define THREAD_STATE_STOP_INIT       0x00000004
#define THREAD_STATE_STOP_MASK       0x0000000c
#define THREAD_STATE_START_STOP_MASK 0x0000000f

#define THREAD_STATE_READER_RUN      0x00000010
#define THREAD_STATE_RESTART_RUN     0x00000100
#define THREAD_STATE_MAIN_TASK_RUN   0x00001000


/* oncrpcmem.h needs a CS for it's mem alloc routines. Need to allocate *
 * static memory for it here */
pthread_mutex_t  oncrpc_mem_crit_sect_mutex = PTHREAD_MUTEX_INITIALIZER;

extern  void rpc_handle_rpc_msg(xdr_s_type *xdr);

/*-----------------------------------------------------------------------
 * Size, Count, Few, many and do not exceed counts for ONCRPC items
 *-----------------------------------------------------------------------*/
#if defined ONCRPC_64K_RPC
#define ONCRPC_DSM_ITEM_CNT                   200
#else
#define ONCRPC_DSM_ITEM_CNT                   100
#endif

#define RPC_EXIT_SIG                          0x00001000
/*----------------------------------------------------------------------
 * Define the static array that stores the ONCRPC items.
 *----------------------------------------------------------------------*/
#define ONCRPC_DSM_ITEM_ARRAY_SIZ (ONCRPC_DSM_ITEM_CNT * \
        (ONCRPC_DSM_ITEM_SIZ + DSM_ITEM_HEADER_SIZE + sizeof(void*)))

static uint32 oncrpc_dsm_item_array[ONCRPC_DSM_ITEM_ARRAY_SIZ/sizeof(uint32)];

/*----------------------------------------------------------------------
 * Define the pool mangement table for ONCRPC items.
 *----------------------------------------------------------------------*/
dsm_pool_mgmt_table_type oncrpc_dsm_item_pool;

/*----------------------------------------------------------------------
 * Proxy Task Definitions
 *----------------------------------------------------------------------*/

xdr_s_type           *oncrpc_router_read_xdr;

static oncrpc_crit_sect_ptr     proxy_task_count_crit_sect;

/*===========================================================================
                       LOCAL FUNCTIONS
===========================================================================*/
/*===========================================================================
                       LOCAL FUNCTIONS
===========================================================================*/
static void  oncrpc_router_read_task_start(void);
static void * oncrpc_router_read_task (void * ignored);
static void  oncrpc_modem_restart_task_start(void);
static void * oncrpc_modem_restart_task (void * ignored);

/*===========================================================================
FUNCTION ONCRPC_RESTART_HANDLER

DESCRIPTION
  Handle Restart for a specific processor and client id

PARAMETERS
  None.

RETURN VALUE
  NULL

DEPENDENCIES
  None

SIDE EFFECTS
  Waits for SIGIO for an indication of the restart event.
===========================================================================*/
void oncrpc_restart_handler(uint32 handle)
{
   oncrpc_tls_type *tls;
   uint64 client_addr =  (uint64)handle;

   /* Note, we do not remove clients at addr as we want to keep the
    * same file handles openend */

   /* Clean up pacmark inmessage structure */
   pacmark_client_died( oncrpc_router_read_xdr->xport, client_addr );

   tls = oncrpc_tls_get_self();
   rpc_svc_callback_deregister( client_addr, tls->protocol );

   oncrpc_clean_reply_queue_by_client(client_addr);

   /* Clean up server */
   printf("Execute exitcb's for handle %d \n",(unsigned int)handle);
   oncrpc_lookup_execute_exitcbs(&client_addr);
} /* oncrpc_daemon_msg_client_died */

/*===========================================================================
FUNCTION oncrpc_restart_up_handler

DESCRIPTION
  Handle Restart up for a specific processor and client id

PARAMETERS
  None.

RETURN VALUE
  NULL

DEPENDENCIES
  None

SIDE EFFECTS
  Waits for SIGIO for an indication of the restart event.
===========================================================================*/
void oncrpc_restart_up_handler(uint32 handle)
{
   uint64 client_addr =  (uint64)handle;

   /* notify clients */
   printf("Execute restart cb's for handle %d \n",(unsigned int)handle);
   oncrpc_lookup_execute_restartcbs(&client_addr);
}

extern void xprtrtr_os_read_abort(void);
extern void xprtrtr_os_restart_abort(void);

/*===========================================================================
FUNCTION ONCRPC_ROUTER_READ_TASK

DESCRIPTION
  Main function of the oncrpc reply task. It handles the reply messages that
  arrive on the SM reply port (SIO_PORT_SMD_RPC_REPLY).

PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
static void * oncrpc_router_read_task (void * parent_thread_handle)
{
   int rc = -1;
   sigset_t blockset;
   oncrpc_read_thread_handle = oncrpc_thread_handle_get();
   /* set the task name in the TLS */
   oncrpc_set_task_name( "ONCRPC RDR" );

   DEBUG_PRINT("Router Read Task Starting \n");

#ifdef FEATURE_ONCRPC_LO
   oncrpc_router_read_xdr = svclo_create(0, 0);
#else
   oncrpc_router_read_xdr = svcrtr_create(0,0);
#endif

   sigemptyset(&blockset);         /* Block SIGABRT */
   sigaddset(&blockset, SIGABRT);
   sigprocmask(SIG_BLOCK, &blockset, NULL);
   if( oncrpc_router_read_xdr == NULL )
   {
      ERR_FATAL("Error Router Server Create Failed",0,0,0);
   }

   pthread_mutex_lock(&thread_state_mutex);
   thread_state  |= THREAD_STATE_READER_RUN;
   pthread_mutex_unlock(&thread_state_mutex);
   DEBUG_PRINT("%d thread state 0x%08x\n",__LINE__,(unsigned int)thread_state);
   oncrpc_event_set((oncrpc_thread_handle)parent_thread_handle,RPC_SYNC_SIG);
   while(1)
   {
      rc = XDR_READ( oncrpc_router_read_xdr );

      if(oncrpc_event_get(oncrpc_read_thread_handle,RPC_EXIT_SIG))
      {
         oncrpc_event_clr(oncrpc_read_thread_handle,RPC_EXIT_SIG);
         break;
      }
      if(rc)
      {
         rpc_handle_rpc_msg( oncrpc_router_read_xdr );
         continue;
      }
      ERR("XDR_READ FAILED %d \n",rc,0,0);

   }
   return(void *)rc;
} /* oncrpc_router_read_task */

/*===========================================================================
FUNCTION ONCRPC_ROUTER_READ_TASK_START

DESCRIPTION
  Starts the oncrpc router read task (waits for a signal saying the reply task has
  started).

PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
static void oncrpc_router_read_task_start( void )
{

   oncrpc_thread_handle this_thread_handle;
   this_thread_handle = oncrpc_thread_handle_get();

   oncrpc_event_clr(this_thread_handle,RPC_SYNC_SIG);
   pthread_create( &router_read_pthread,
      NULL,
      oncrpc_router_read_task,
      this_thread_handle
      );

   oncrpc_event_wait(this_thread_handle,RPC_SYNC_SIG);
   oncrpc_event_clr(this_thread_handle,RPC_SYNC_SIG);
} /* oncrpc_router_read_task_start */


extern void xprtrtr_os_process_restarting_handles(void);

/*===========================================================================
FUNCTION ONCRPC_MODEM_RESTART_TASK

DESCRIPTION
  Process handles ready for restart

PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
static void * oncrpc_modem_restart_task (void * parent_thread_handle)
{
   oncrpc_restart_thread_handle = oncrpc_thread_handle_get();

   /* set the task name in the TLS */
   oncrpc_set_task_name( "MODEM RESTART" );

   pthread_mutex_lock(&thread_state_mutex);
   thread_state  |= THREAD_STATE_RESTART_RUN;
   pthread_mutex_unlock(&thread_state_mutex);

   while( 1 )
   {
      if(oncrpc_event_get(oncrpc_restart_thread_handle,RPC_EXIT_SIG))
      {
         oncrpc_event_clr(oncrpc_restart_thread_handle,RPC_EXIT_SIG);
         break;
      }
      xprtrtr_os_process_restarting_handles();
   }
   return(void *)0;
} /* oncrpc_router_read_task */


/*===========================================================================
FUNCTION ONCRPC_MODEM_RESTART_TASK_START

DESCRIPTION


PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
static void oncrpc_modem_restart_task_start( void )
{
   pthread_create( &modem_restart_pthread,
      NULL,
      oncrpc_modem_restart_task,
      NULL
      );
} /* oncrpc_router_read_task_start */



/*===========================================================================
                          EXTERNAL FUNCTIONS
===========================================================================*/

/*===========================================================================
FUNCTION ONCRPC_TASK

DESCRIPTION
  This function is the task and processes signals/events for the task.
  A single event type is assumed, ie the set signal mask is never actually
  checked other than for a non zero value.

PARAMETERS
  Ignored.

RETURN VALUE
  Never returns.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void* oncrpc_task(void* parent_thread_handle)
{
   oncrpc_event_t events=0;
   /* set the task name in the TLS */
   oncrpc_set_task_name( "ONCRPC" );

   /* Save the handle */
   oncrpc_task_thread_handle = oncrpc_thread_handle_get();

   /* Initialise */
   oncrpc_router_read_task_start();
   oncrpc_modem_restart_task_start();

   /* Process all signalled command messages */
   if( (int)parent_thread_handle != 0 )
   {
      oncrpc_event_set((oncrpc_thread_handle)parent_thread_handle,RPC_SYNC_SIG);
   } else
   {
      ERR("Parent Task NULL, cannot synchronize",0,0,0);
   }

   while( !(events & RPC_EXIT_SIG) )
   {
      events = oncrpc_event_wait(oncrpc_task_thread_handle, RPC_CMD_Q_SIG|RPC_EXIT_SIG);
      DEBUG_PRINT("oncrpc_main_task received events 0x%08x \n",(unsigned int)events);
      oncrpc_event_clr(oncrpc_task_thread_handle, events);
      if( events & RPC_EXIT_SIG )
      {
         goto exit;
      }
      if( events & RPC_CMD_Q_SIG )
      {
         DEBUG_PRINT("oncrpc_main_task handling cmd_q_sig  0x%08x \n",(unsigned int)events);
         oncrpc_main();
      }
   }

   exit:
   pthread_mutex_lock(&thread_state_mutex);
   thread_state &= ~(THREAD_STATE_MAIN_TASK_RUN);
   pthread_mutex_unlock(&thread_state_mutex);

   return(void*)0;
} /* oncrpc_task */

/*===========================================================================
FUNCTION ONCRPC_TASK_START

DESCRIPTION
  Main entry into oncrpc, ... Starts the oncrpc task/thread

PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  oncrpc_init() must be called prior to calling this function.

SIDE EFFECTS
  Will start pthreads and put oncrpc stack into active state.
===========================================================================*/
void oncrpc_task_start(void)
{
   DEBUG_PRINT("ONCRPC TASK START \n");
   oncrpc_thread_handle this_thread_handle;
   this_thread_handle = oncrpc_thread_handle_get();

   pthread_mutex_lock(&thread_state_mutex);
   if( thread_state & THREAD_STATE_START_MASK )
   {
      pthread_mutex_unlock(&thread_state_mutex);
      oncrpc_printf("ONCRPC Task already started \n");
      return;
   }
   thread_state |= THREAD_STATE_START_INIT;
   pthread_mutex_unlock(&thread_state_mutex);


   /* Create the task */
   oncrpc_event_clr(this_thread_handle,RPC_SYNC_SIG);
   pthread_create( &main_oncrpc_thread,
      NULL,
      oncrpc_task,
      (void*)this_thread_handle
      );

   if( (int)this_thread_handle != 0 )
   {
      oncrpc_event_wait(this_thread_handle,RPC_SYNC_SIG);
      oncrpc_event_clr(this_thread_handle,RPC_SYNC_SIG);
   } else
   {
      ERR("Parent Task NULL, cannot synchronize",0,0,0);
   }

   pthread_mutex_lock(&thread_state_mutex);
   thread_state  |= THREAD_STATE_MAIN_TASK_RUN | THREAD_STATE_START_COMPLETE;
   thread_state &= ~THREAD_STATE_START_INIT;
   pthread_mutex_unlock(&thread_state_mutex);
   DEBUG_PRINT("Oncrpc Start complete state 0x%08x \n",(unsigned int)thread_state);

   oncrpc_proxy_task_start();

} /* oncrpc_task_start */


/*===========================================================================
FUNCTION ONCRPC_TASK_STOP

DESCRIPTION
  Stop oncrpc main, read and restart tasks.

PARAMETERS
  None.

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  Will stop main, read and modem_restart pthreads and put oncrpc stack into
  inactive state.
  If the modem restarts, oncrpc stack will not process the required cleanup
  of clients following a restart while in the inactive state.
===========================================================================*/
void oncrpc_task_stop(void)
{
   void *thread_rc;
   int rc = 0;

   /* Stop Reader Thread and Join */
   pthread_mutex_lock(&thread_state_mutex);
   if( thread_state & THREAD_STATE_STOP_MASK )
   {
      pthread_mutex_unlock(&thread_state_mutex);
      oncrpc_printf("ONCRPC Task Stop Failed, stop already in progress \n");
      return;
   }
   if( !( thread_state & THREAD_STATE_START_COMPLETE) )
   {
      pthread_mutex_unlock(&thread_state_mutex);
      oncrpc_printf("ONCRPC Task Stop Failed, tasks not started \n");
      return;
   }
   thread_state |= THREAD_STATE_STOP_INIT;
   pthread_mutex_unlock(&thread_state_mutex);

   /* Set state for Reader Task to terminate, bit will auto-clear */
   if( thread_state & THREAD_STATE_READER_RUN )
   {
      /* Router Read thread is waiting on read,
         send signal to break-out of read */

      oncrpc_event_set(oncrpc_read_thread_handle,RPC_EXIT_SIG);
      xprtrtr_os_read_abort();

      if(pthread_join(router_read_pthread, &thread_rc) != 0)
      {
        ERR("Failed to join router thread %d \n",rc,0,0);
      }
      else
      {
         pthread_mutex_lock(&thread_state_mutex);
         thread_state  &= ~(THREAD_STATE_READER_RUN);
         pthread_mutex_unlock(&thread_state_mutex);
      }
   }

   /* Set state for Reader Task to terminate, bit will auto-clear */
   if( thread_state & THREAD_STATE_RESTART_RUN )
   {
     oncrpc_event_set(oncrpc_restart_thread_handle,RPC_EXIT_SIG);
     xprtrtr_os_restart_abort();

     if(pthread_join(modem_restart_pthread, &thread_rc) != 0)
     {
       ERR("Failed to join modem_restart thread %d \n",rc,0,0);
     }
     else
     {
       pthread_mutex_lock(&thread_state_mutex);
       thread_state  &= ~(THREAD_STATE_RESTART_RUN);
       pthread_mutex_unlock(&thread_state_mutex);
     }
   }

   /* Oncrpc thread is locally in a loop, send it EXIT_SIG */
   if( thread_state & THREAD_STATE_MAIN_TASK_RUN )
   {
      oncrpc_event_set(oncrpc_task_thread_handle,RPC_EXIT_SIG);
      if(pthread_join(main_oncrpc_thread,&thread_rc) != 0)
      {
         printf("Failed to join main oncrpc thread %d \n",rc);
      }
   }

   oncrpc_task_thread_handle = NULL;
   /* Stop Complete is represented by clearing Start/Stop mask */
   /* This will allow start to be re-initiated*/
   pthread_mutex_lock(&thread_state_mutex);
   thread_state &= ~THREAD_STATE_START_STOP_MASK;
   pthread_mutex_unlock(&thread_state_mutex);
   DEBUG_PRINT("Main Oncrpc Thread Stop Complete 0x%08x \n",thread_state);
   oncrpc_proxy_task_stop();

} /* oncrpc_task_stop*/





/*===========================================================================
FUNCTION: oncrpc_proxy_task_add
DESCRIPTION:
   Add a proxy task and store thread key in pointer specified.

DEPENDENCIES:
   None

ARGUMENTS:
   None

RETURN VALUE:
   TRUE/FALSE for success/fail.

SIDE EFFECTS:
   None
===========================================================================*/
uint32 oncrpc_proxy_task_add(void *key, void *parent )
{
   int rc;
   DEBUG_PRINT("\nAdding proxy task 0x%08x",(unsigned int)key);
   if(!key)
   {
      ONCRPC_ERR("Null pointer in oncrpc_proxy_task_add\n",0,0,0);
      return FALSE;
   }
   rc = pthread_create((pthread_t *)key,   /* pthread handle                   */
                       NULL,                               /* Attribute                        */
                       (void *)oncrpc_proxy_task,          /* Start routine                    */
                       parent                              /* Argument passed to start routine */
      );
   DEBUG_PRINT(" thread 0x%08x \n",(unsigned int)*((pthread_t *)key));
   if(rc == 0)
   {
      return  TRUE;
   }
   else
   {
      return FALSE;
   }
}

/*===========================================================================
FUNCTION ONCRPC_TASK_JOIN

DESCRIPTION
  Join thread with parent

PARAMETERS
  void pointer (key) as initialized in oncrpc_proxy_task_add

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void oncrpc_task_join(void *key)
{
   void *thread_rc;
   DEBUG_PRINT("Pthread joind 0x%08x \n",(unsigned int)key);
   pthread_join((pthread_t)key,&thread_rc);
}

/*===========================================================================
FUNCTION ONCRPC_SIGNAL_RPC_THREAD

DESCRIPTION
  Send the requested signals to the RPC thread

PARAMETERS
  events  - Event mask

RETURN VALUE
  None.

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void oncrpc_signal_rpc_thread( uint32 events )
{
   oncrpc_event_set(oncrpc_task_thread_handle, events);
}

/*===========================================================================
FUNCTION ONCRPC_IS_RPC_THREAD

DESCRIPTION
  Checks to see if the currently executing thread is the ONCRPC thread

PARAMETERS
  None.

RETURN VALUE
  TRUE  - Thread is RPC thread
  FALSE - Thread is not the RPC thread

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
boolean oncrpc_is_rpc_thread(void)
{
   return( (oncrpc_thread_handle_get() == oncrpc_task_thread_handle)? TRUE : FALSE);
}

/*===========================================================================
FUNCTION TERMINATION_HANDLER

DESCRIPTION
  Catch termination signal sent to threads.

PARAMETERS
  None.

RETURN VALUE
  N/A

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void termination_handler(int sigNumber)
{
   DEBUG_PRINT("Thread Termination Handler Called \n");
}

/*===========================================================================
FUNCTION ONCRPC_MAIN_OS_INIT

DESCRIPTION
  Initialize OS specific data structures.
  For linux, initialize the call back registeration control.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void oncrpc_main_os_init(void)
{
   dsm_init_pool( ONCRPC_DSM_ITEM_POOL,
      (uint8 *) oncrpc_dsm_item_array,
      ONCRPC_DSM_ITEM_ARRAY_SIZ,
      ONCRPC_DSM_ITEM_SIZ );

   /* Catch SIGABRT */
   action_to_catch.sa_handler = termination_handler;
   sigemptyset(&action_to_catch.sa_mask);
   action_to_catch.sa_flags = 0;
   sigaction(SIGABRT,&action_to_catch,NULL);

   pthread_mutex_init(&start_stop_mutex, NULL);
   pthread_mutex_init(&thread_state_mutex,NULL);

   pthread_mutex_lock(&thread_state_mutex);
   thread_state=0;
   pthread_mutex_unlock(&thread_state_mutex);


} /* oncrpc_main_os_init */


/*===========================================================================
FUNCTION ONCRPC_MAIN_OS_DEINIT

DESCRIPTION
  Unallocation of resources

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void oncrpc_main_os_deinit(void)
{
   oncrpc_addr_type addr;
   uint32 prog;
   uint32 vers;
   oncrpc_control_close_handle_type  close_args;
   while( oncrpc_lookup_get(&addr,&prog,&vers) )
   {
      /* Set the destination in lower layer */
      close_args.handle = addr;
      close_args.prog_ver.prog = prog;
      close_args.prog_ver.ver = vers;

      DEBUG_PRINT("Closing handle:%d prog:0x%08x vers:0x%08x \n",(unsigned int)close_args.handle,
         (unsigned int)close_args.prog_ver.prog, (unsigned int)close_args.prog_ver.ver);
      XDR_CONTROL(oncrpc_router_read_xdr,ONCRPC_CONTROL_CLOSE ,(void *)&close_args);
      oncrpc_lookup_remove_client(close_args.prog_ver.prog,close_args.prog_ver.ver);
   }
} /* oncrpc_main_os_deinit */

/*===========================================================================
FUNCTION ONCRPC_INIT_MEM_CRIT_SECT

DESCRIPTION
  Alternative method to init a critical section without using
  oncrpc_crit_sect_init which uses "malloc" (at least in the R E X
  implementation.

  oncrpc_crit_sect_init is the preferred method to create a critical section,
  but it cannot be used to create the critical section used in the ONCRPC
  malloc routine.

PARAMETERS
  None

RETURN VALUE
  None

DEPENDENCIES
  None

SIDE EFFECTS
  None
===========================================================================*/
void oncrpc_init_mem_crit_sect(oncrpc_crit_sect_ptr* pcs)
{
   /* "Allocate" static memory for this critical section */
   *pcs  =  &oncrpc_mem_crit_sect_mutex;
}


/*===========================================================================
FUNCTION: oncrpc_proxy_lock_init

DESCRIPTION:
   Initialize the lock (critical section)

DEPENDENCIES:
   None

ARGUMENTS:
   None

RETURN VALUE:
   None

SIDE EFFECTS:
   Will be called by every proxy task, initialize only once.
===========================================================================*/
void oncrpc_proxy_lock_init ( void )
{
   if( proxy_task_count_crit_sect == 0 )
   {
      oncrpc_crit_sect_init( &proxy_task_count_crit_sect );
   }
}

/*===========================================================================
FUNCTION: oncrpc_proxy_lock

DESCRIPTION:
   Lock interrupts or critical section for updating global variables in the
   proxy module. Some functions can be invoked in ISR context in Rex; thus
   the need for INTLOCK instead of a critical section.

DEPENDENCIES:
   None

ARGUMENTS:
   None

RETURN VALUE:
   None

SIDE EFFECTS:
   None
===========================================================================*/
void oncrpc_proxy_lock ( void )
{
   oncrpc_crit_sect_enter( proxy_task_count_crit_sect );
}

/*===========================================================================
FUNCTION: oncrpc_proxy_unlock

DESCRIPTION:
   Unlock interrupts or critical section for updating global variables in the
   proxy module. Some functions can be invoked in ISR context in Rex; thus
   the need for INTLOCK instead of a critical section.

DEPENDENCIES:
   None

ARGUMENTS:
   None

RETURN VALUE:
   None

SIDE EFFECTS:
   None
===========================================================================*/
void oncrpc_proxy_unlock ( void )
{
   oncrpc_crit_sect_leave( proxy_task_count_crit_sect );
}

/*===========================================================================
FUNCTION: oncrpc_library_init

DESCRIPTION:
   Initialize the ONCRPC library.  This function is called when the ONCRPC
   shared library is loaded, before application's main() is started.

DEPENDENCIES:
   None

ARGUMENTS:
   None

RETURN VALUE:
   None

SIDE EFFECTS:
===========================================================================*/
#ifdef __GNUC__
void __attribute__ ((constructor)) oncrpc_library_init(void)
{
}
#endif

/*===========================================================================
FUNCTION: oncrpc_library_exit

DESCRIPTION:
   Cleans up the ONCRPC library.  This function is called after exit() or
   after application's main() completes.

DEPENDENCIES:
   None

ARGUMENTS:
   None

RETURN VALUE:
   None

SIDE EFFECTS:
===========================================================================*/
#ifdef __GNUC__
void __attribute__ ((destructor)) oncrpc_library_exit(void)
{
    oncrpc_tls_deinit();
}
#endif

/*===========================================================================
 * End of module
 *=========================================================================*/
